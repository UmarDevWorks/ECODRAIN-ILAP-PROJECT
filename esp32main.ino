#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "DHT.h"

const int analogPin = 34;       // Water level sensor input
const int soilPin = 33;         // Soil moisture sensor pin
const int dhtPin = 25;          // DHT sensor pin
const int relay_bioswale = 26;   // Bioswale channel relay

// DHT Setup 
#define DHTTYPE DHT11         
DHT dht(dhtPin, DHTTYPE);

// Web Server & Wifi Preferences Storage
WebServer server(80);
Preferences preferences;

// Wi-Fi credentials storage variables
String ssid = "";
String password = "";

// Timing 
unsigned long previousMillis = 0;
const long interval = 2000;    

// Global sensor variables
int raw_data = 0;
int soil_raw = 0;
float humidity = 0.0;
float tempC = 0.0;
bool relayState = HIGH;

void setup() {
  Serial.begin(115200);

  // Initialize Pins
  pinMode(relay_bioswale, OUTPUT);
  digitalWrite(relay_bioswale, HIGH); // Default OFF for our active-low relay

  // Initialize DHT
  dht.begin();

  // Load saved Wi-Fi credentials from ESP32 Flash (Preferences)
  preferences.begin("wifi-config", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("pass", "");
  preferences.end();

  // Attempt to connect to Wi-Fi if credentials exist
  if (ssid.length() > 0) {
    Serial.printf("Connecting to saved Wi-Fi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttemptTime = millis();
   
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
      delay(500);
      Serial.print(".");
    }
  }

  // Fallback to AP Mode if connection failed or no credentials found
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to Wi-Fi. Starting Access Point (AP Mode)...");
    WiFi.softAP("ESP32-Bioswale-Setup", "12345678"); // AP Name and Password
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  } else {
    Serial.println("\nConnected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleJSONData); // Endpoint for live background updates
  server.on("/save", HTTP_POST, handleSaveWiFi);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

 
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read sensors
    raw_data = analogRead(analogPin);
    soil_raw = analogRead(soilPin);
    humidity = dht.readHumidity();
    tempC = dht.readTemperature();

    
    if (isnan(humidity)) humidity = 0.0;
    if (isnan(tempC)) tempC = 0.0;

    // Relay control logic for Bioswale channel 
    if (raw_data < 1000) {
      digitalWrite(relay_bioswale, LOW);  // Turn ON relay 
      relayState = LOW;
    } else {
      digitalWrite(relay_bioswale, HIGH); // Turn OFF relay
      relayState = HIGH;
    }
  }
}

// --- JSON API Endpoint for Live Data ---
void handleJSONData() {
  // Inverted mapping: 4095 = 0%, 0 = 100%
  int soil_pct = constrain(map(soil_raw, 4095, 0, 0, 100), 0, 100);
  int water_pct = constrain(map(raw_data, 4095, 0, 0, 100), 0, 100);

  String json = "{";
  json += "\"temp\":\"" + String(tempC, 1) + "\",";
  json += "\"humidity\":\"" + String(humidity, 1) + "\",";
  json += "\"soil\":\"" + String(soil_pct) + "\",";
  json += "\"water\":\"" + String(water_pct) + "\",";
  json += "\"relayState\":" + String(relayState == LOW ? 1 : 0);
  json += "}";

  server.send(200, "application/json", json);
}

// HTML DASHBOARD
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 Bioswale Dashboard</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: #121212; color: #e0e0e0; text-align: center; margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: auto; background: #1e1e1e; padding: 20px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }";
  html += "h1 { color: #4caf50; font-size: 24px; margin-bottom: 5px; }";
  html += "p.subtitle { color: #888; font-size: 14px; margin-top: 0; }";
  html += ".grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-top: 20px; }";
  html += ".card { background: #2c2c2c; padding: 15px; border-radius: 8px; border-left: 5px solid #4caf50; text-align: left; }";
  html += ".card h3 { margin: 0 0 10px 0; font-size: 14px; color: #aaa; }";
  html += ".card .val { font-size: 22px; font-weight: bold; color: #fff; }";
  html += ".status-badge { display: inline-block; padding: 5px 12px; border-radius: 15px; font-weight: bold; font-size: 14px; }";
  html += ".on { background: #2e7d32; color: #fff; }";
  html += ".off { background: #c62828; color: #fff; }";
  html += ".wifi-box { margin-top: 30px; background: #252525; padding: 15px; border-radius: 8px; text-align: left; }";
  html += "input { width: 100%; padding: 8px; margin: 5px 0 15px 0; background: #333; border: 1px solid #444; color: #fff; border-radius: 4px; box-sizing: border-box;}";
  html += "button { background: #4caf50; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; font-weight: bold; width: 100%; }";
  html += "button:hover { background: #45a049; }";
  html += "</style>";
  
 
  html += "<script>";
  html += "setInterval(() => {";
  html += "  fetch('/data').then(response => response.json()).then(data => {";
  html += "    document.getElementById('temp').innerText = data.temp + ' °C';";
  html += "    document.getElementById('humidity').innerText = data.humidity + ' %';";
  html += "    document.getElementById('soil').innerText = data.soil + ' %';";
  html += "    document.getElementById('water').innerText = data.water + ' %';";
  html += "    let badge = document.getElementById('relay-badge');";
  html += "    let card = document.getElementById('relay-card');";
  html += "    if(data.relayState == 1) {";
  html += "      badge.className = 'status-badge on';";
  html += "      badge.innerText = 'ACTIVE (LOW / WATER DIVERTED)';";
  html += "      card.style.borderLeftColor = '#2e7d32';";
  html += "    } else {";
  html += "      badge.className = 'status-badge off';";
  html += "      badge.innerText = 'STANDBY (HIGH / NORMAL)';";
  html += "      card.style.borderLeftColor = '#c62828';";
  html += "    }";
  html += "  }).catch(err => console.log('Error fetching data:', err));";
  html += "}, 2000);";
  html += "</script>";
  
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 Environmental Node</h1>";
  html += "<p class='subtitle'>Live Bioswale & Weather Telemetry</p>";

  int initial_soil_pct = constrain(map(soil_raw, 4095, 0, 0, 100), 0, 100);
  int initial_water_pct = constrain(map(raw_data, 4095, 0, 0, 100), 0, 100);

  html += "<div class='grid'>";
  
  html += "<div class='card'>";
  html += "<h3>AIR TEMPERATURE</h3>";
  html += "<div class='val' id='temp'>" + String(tempC, 1) + " &deg;C</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>AIR HUMIDITY</h3>";
  html += "<div class='val' id='humidity'>" + String(humidity, 1) + " %</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>SOIL MOISTURE</h3>";
  html += "<div class='val' id='soil'>" + String(initial_soil_pct) + " %</div>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>WATER LEVEL</h3>";
  html += "<div class='val' id='water'>" + String(initial_water_pct) + " %</div>";
  html += "</div>";

  html += "</div>"; // End grid

  // Relay status block
  html += "<div class='card' id='relay-card' style='margin-top: 15px; border-left-color: " + String(relayState == LOW ? "#2e7d32" : "#c62828") + ";'>";
  html += "<h3>BIOSWALE CHANNEL RELAY STATE</h3>";
  if (relayState == LOW) {
    html += "<span class='status-badge on' id='relay-badge'>ACTIVE (LOW / WATER DIVERTED)</span>";
  } else {
    html += "<span class='status-badge off' id='relay-badge'>STANDBY (HIGH / NORMAL)</span>";
  }
  html += "</div>";

  // Wi-Fi Config Form Section
  html += "<div class='wifi-box'>";
  html += "<h3 style='color:#fff; margin-top:0;'>Configure Wi-Fi Router</h3>";
  html += "<form action='/save' method='POST'>";
  html += "<label>SSID (Network Name):</label>";
  html += "<input type='text' name='ssid' placeholder='Enter Wi-Fi SSID' required>";
  html += "<label>Password:</label>";
  html += "<input type='password' name='pass' placeholder='Enter Wi-Fi Password'>";
  html += "<button type='submit'>Save & Restart</button>";
  html += "</form>";
  html += "</div>";

  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// Handle Wi-Fi Form Submission
void handleSaveWiFi() {
  if (server.hasArg("ssid")) {
    ssid = server.arg("ssid");
    password = server.arg("pass");

    // Save credentials into Flash memory
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();

    String html = "<html><body style='background:#121212;color:#fff;text-align:center;font-family:sans-serif;padding-top:50px;'>";
    html += "<h2>Credentials Saved Successfully!</h2>";
    html += "<p>The ESP32 is restarting to connect to <b>" + ssid + "</b>...</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart(); // Reboot into station mode with new credentials
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
