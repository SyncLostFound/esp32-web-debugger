#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <driver/ledc.h>

const char* ssid = "YOUR-SSID";
const char* password = "PASSWORD";

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");

HardwareSerial uart1(1);
#define UART_TX 17
#define UART_RX 16

#define I2C_SDA 21
#define I2C_SCL 22

const int usablePins[] = {
  0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19,
  21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39
};
const int numUsablePins = sizeof(usablePins) / sizeof(usablePins[0]);

bool pinModes[numUsablePins];
bool pinStates[numUsablePins];
int pwmFrequencies[numUsablePins];
unsigned int bounceCounters[numUsablePins];
unsigned long lastChangeTimes[numUsablePins];

unsigned long lastPollTime = 0;
unsigned long lastOscilloTime = 0;
const unsigned long pollInterval = 50;
const unsigned long oscilloInterval = 200;
int oscilloPin = 34;
float lastVoltage = 0;
const float smoothingFactor = 0.3;

// --- PWM Control ---
void setPWMFrequency(int pin, int freq) {
  int channel = pin % 8;
  int resolution = 8;

  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .duty_resolution = (ledc_timer_bit_t)resolution,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = freq,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel = {
    .gpio_num = (gpio_num_t)pin,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = (ledc_channel_t)channel,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 127,
    .hpoint = 0
  };
  ledc_channel_config(&ledc_channel);
}

// --- JSON Generator ---
String getPinStatusJSON() {
  String json = "[";
  for (int i = 0; i < numUsablePins; ++i) {
    int pin = usablePins[i];
    String mode = pinModes[i] ? "OUTPUT" : "INPUT";
    bool state = digitalRead(pin);
    json += "{\"pin\":" + String(pin) +
            ",\"mode\":\"" + mode + "\"" +
            ",\"state\":" + (state ? "true" : "false") +
            ",\"bounce\":" + String(bounceCounters[i]) + "},";
  }
  if (json.length() > 1) json.remove(json.length() - 1);
  json += "]";
  return json;
}

// --- Voltage Reader ---
String getVIN() {
  int raw = analogRead(34);
  float vin = raw * (3.3 / 4095.0) * 2.0;
  return String(vin, 2);
}

// --- Pin Mode Set ---
void setPinMode(int pin, String mode) {
  for (int i = 0; i < numUsablePins; ++i) {
    if (usablePins[i] == pin) {
      pinMode(pin, (mode == "OUTPUT") ? OUTPUT : INPUT);
      pinModes[i] = (mode == "OUTPUT");
    }
  }
}

// --- Pin Write ---
void writePinState(int pin, bool value) {
  for (int i = 0; i < numUsablePins; ++i) {
    if (usablePins[i] == pin && pinModes[i]) {
      digitalWrite(pin, value ? HIGH : LOW);
      pinStates[i] = value;
    }
  }
}

// --- Notify All Clients ---
void notifyClients() {
  if (webSocket.availableForWriteAll()) {
    String message = getPinStatusJSON();
    webSocket.textAll(message);
  }
}

// --- I2C Scanner ---
void scanI2C() {
  String json = "{\"i2c\":[";
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      json += String(address) + ",";
    }
  }
  if (json.endsWith(",")) json.remove(json.length() - 1);
  json += "]}";
  webSocket.textAll(json);
}

// --- WebSocket Events ---
void onWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(getPinStatusJSON());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String msg = String((char*)data);
      if (msg.startsWith("MODE:")) {
        int pin = msg.substring(5, msg.indexOf(",")).toInt();
        String mode = msg.substring(msg.indexOf(",") + 1);
        setPinMode(pin, mode);
      } else if (msg.startsWith("WRITE:")) {
        int pin = msg.substring(6, msg.indexOf(",")).toInt();
        int val = msg.substring(msg.indexOf(",") + 1).toInt();
        writePinState(pin, val);
      } else if (msg.startsWith("PWM:")) {
        int pin = msg.substring(4, msg.indexOf(",")).toInt();
        int freq = msg.substring(msg.indexOf(",") + 1).toInt();
        setPWMFrequency(pin, freq);
      } else if (msg.startsWith("OSCILLO:")) {
        oscilloPin = msg.substring(8).toInt();
      } else if (msg.startsWith("UART_SEND:")) {
        String content = msg.substring(10);
        uart1.print(content);
      } else if (msg == "UART_READ") {
        String received = "";
        while (uart1.available()) {
          received += (char)uart1.read();
        }
        if (received.length()) {
          String json = "{\"uart\":\"" + received + "\"}";
          webSocket.textAll(json);
        }
      } else if (msg == "I2C_SCAN") {
        scanI2C();
      }
      notifyClients();
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  uart1.begin(9600, SERIAL_8N1, UART_RX, UART_TX);
  Wire.begin(I2C_SDA, I2C_SCL);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  for (int i = 0; i < numUsablePins; ++i) {
    pinMode(usablePins[i], INPUT);
    pinModes[i] = false;
    pinStates[i] = digitalRead(usablePins[i]);
    bounceCounters[i] = 0;
    lastChangeTimes[i] = millis();
  }

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "application/json", getPinStatusJSON());
  });

  server.on("/api/vin", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "text/plain", getVIN());
  });

  webSocket.onEvent(onWebSocketMessage);
  server.addHandler(&webSocket);
  server.begin();
  Serial.println("HTTP server started");
}

// --- Main Loop ---
void loop() {
  unsigned long now = millis();

  // GPIO polling with debounce
  if (now - lastPollTime >= pollInterval) {
    bool changed = false;
    for (int i = 0; i < numUsablePins; ++i) {
      if (!pinModes[i]) {
        bool current = digitalRead(usablePins[i]);
        if (current != pinStates[i] && (now - lastChangeTimes[i]) > 20) {
          pinStates[i] = current;
          bounceCounters[i]++;
          lastChangeTimes[i] = now;
          changed = true;
        }
      }
    }
    if (changed) notifyClients();
    lastPollTime = now;
  }

  // Oscilloscope smoothed sample
  if (now - lastOscilloTime > oscilloInterval) {
    int raw = analogRead(oscilloPin);
    float voltage = raw * (3.3 / 4095.0);
    lastVoltage = lastVoltage * (1.0 - smoothingFactor) + voltage * smoothingFactor;

    if (webSocket.availableForWriteAll()) {
      String json = "{\"oscilloscope\":{\"pin\":" + String(oscilloPin) + ",\"voltage\":" + String(lastVoltage, 2) + "}}";
      webSocket.textAll(json);
    }

    lastOscilloTime = now;
  }
}
