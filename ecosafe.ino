#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Wi-Fi Credentials ---
const char* ssid = "San";
const char* password = "shutup..";

// --- OLED Display Setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Sensor Pin Configuration ---
#define DHTPIN D4
#define DHTTYPE DHT11
#define VOLTAGE_PIN A0

// --- Sensor Objects ---
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

// --- Global Variables ---
float temperature, humidity, voltage;
String zone = "SAFE";

// --- Function to Display Data on OLED ---
void displayToOLED() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    display.println("Solar Env Monitor");
    display.print("Temp: "); display.print(temperature); display.println(" C");
    display.print("Hum : "); display.print(humidity); display.println(" %");
    display.print("Volt: "); display.print(voltage); display.println(" V");
    display.print("Zone: "); display.println(zone);

    if (zone == "UNSAFE") {
        display.setTextColor(BLACK, WHITE);
        display.setCursor(0, 52);
        display.println("!! WARNING !!");
    }

    display.display();
}

// --- API Data Endpoint ---
void handleData() {
    String json = "{";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"voltage\":" + String(voltage) + ",";
    json += "\"zone\":\"" + zone + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

// --- Updated Web Server Function ---
void handleRoot() {
    String html = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
          <meta charset="UTF-8">
          <title>EcoSafe 5G Dashboard</title>
          <meta name="viewport" content="width=device-width, initial-scale=1">
          <style>
            body {font-family: 'Segoe UI', sans-serif; background-color: #eaf6f6; margin: 0; padding: 20px; color: #333;}
            h1 {text-align: center; color: #2c7873; margin-bottom: 30px;}
            .container {max-width: 600px; margin: auto;}
            .card {background: #fff; border-radius: 10px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); text-align: center;}
            .card h2 {margin-top: 0; color: #004445;}
            .icon {font-size: 36px; margin-bottom: 10px;}
            .battery-bar {height: 20px; width: 100%; background: #ddd; border-radius: 10px; overflow: hidden; margin-top: 10px;}
            .battery-level {height: 100%; width: 0%; background: #2c7873; text-align: right; padding-right: 5px; color: white; font-size: 14px; transition: width 0.5s ease;}
            .footer {text-align: center; font-size: 12px; color: #666; margin-top: 40px;}
          </style>
        </head>
        <body>
          <h1>EcoSafe 5G Tower Dashboard</h1>
          <div class="container">
            <div class="card"><div class="icon">🔋</div><h2>Battery Voltage</h2><p id="voltage">-- V</p>
              <div class="battery-bar"><div id="batteryLevel" class="battery-level">--%</div></div></div>
            <div class="card"><div class="icon">🌡</div><h2>Temperature</h2><p id="temperature">-- °C</p></div>
            <div class="card"><div class="icon">💧</div><h2>Humidity</h2><p id="humidity">--%</p></div>
            <div class="card"><div class="icon">📍</div><h2>Zone</h2><p id="zone">--</p></div>
          </div>
          <div class="footer">&copy; 2025 EcoSafe 5G | Powered by Solar & ESP32</div>
          <script>
            function updateDashboard(data) {
              document.getElementById("voltage").innerText = data.voltage.toFixed(2) + " V";
              document.getElementById("temperature").innerText = data.temperature.toFixed(1) + " °C";
              document.getElementById("humidity").innerText = data.humidity.toFixed(1) + "%";
              document.getElementById("zone").innerText = data.zone || "Unknown";

              let batteryPercent = Math.min(Math.max((data.voltage - 3.0) / (4.2 - 3.0) * 100, 0), 100);
              let batteryLevelDiv = document.getElementById("batteryLevel");
              batteryLevelDiv.style.width = batteryPercent + "%";
              batteryLevelDiv.innerText = Math.round(batteryPercent) + "%";

              batteryLevelDiv.style.background = batteryPercent < 25 ? "#d9534f" : batteryPercent < 60 ? "#f0ad4e" : "#2c7873";
            }
            setInterval(() => {
              fetch("/data").then(res => res.json()).then(updateDashboard).catch(console.error);
            }, 2000);
          </script>
        </body>
        </html>
    )rawliteral";

    server.send(200, "text/html", html);
}

// --- Setup Function ---
void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(VOLTAGE_PIN, INPUT);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi. IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
}

// --- Main Loop ---
void loop() {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    voltage = (analogRead(VOLTAGE_PIN) / 1023.0) * 4.2;

    zone = (temperature > 50 || humidity > 50) ? "UNSAFE" : "SAFE";

    displayToOLED();
    server.handleClient();

    delay(3000);
}