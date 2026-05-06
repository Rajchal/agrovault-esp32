#include <WiFi.h>
#include <WebServer.h>

// Access point credentials
const char* kApSsid = "ColdRoomMonitor";
const char* kApPassword = "12345678";

// Web server on port 80
WebServer server(80);

// Simulated temperature state
float currentTemperatureC = 5.0f;
unsigned long lastTemperatureUpdateMs = 0;
String lastUpdatedText = "--:--:--";

// Temperature simulation timing
const unsigned long kTemperatureUpdateIntervalMs = 1000;

// Forward declarations
void handleRoot();
void handleApiData();
void handleNotFound();
void updateSimulatedTemperature();
String buildDashboardHtml();
String buildTimestamp();

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kApSsid, kApPassword);

  Serial.println();
  Serial.println("Access Point started");
  Serial.print("SSID: ");
  Serial.println(kApSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.onNotFound(handleNotFound);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  randomSeed((uint32_t)esp_random());
  currentTemperatureC = random(200, 801) / 100.0f;
  lastTemperatureUpdateMs = millis();
  lastUpdatedText = buildTimestamp();

  startAccessPoint();
  setupRoutes();
  server.begin();

  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
  updateSimulatedTemperature();
}

void updateSimulatedTemperature() {
  const unsigned long now = millis();

  if (now - lastTemperatureUpdateMs < kTemperatureUpdateIntervalMs) {
    return;
  }

  lastTemperatureUpdateMs = now;

  // Small random walk, clamped to 2.0C..8.0C.
  const int deltaStep = random(-25, 26); // -0.25C..+0.25C
  currentTemperatureC += deltaStep / 100.0f;

  if (currentTemperatureC < 2.0f) {
    currentTemperatureC = 2.0f;
  } else if (currentTemperatureC > 8.0f) {
    currentTemperatureC = 8.0f;
  }

  lastUpdatedText = buildTimestamp();
}

String buildTimestamp() {
  const unsigned long totalSeconds = millis() / 1000UL;
  const unsigned int hours = (totalSeconds / 3600UL) % 24U;
  const unsigned int minutes = (totalSeconds / 60UL) % 60U;
  const unsigned int seconds = totalSeconds % 60U;

  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
  return String(buffer);
}

String buildDashboardHtml() {
  String html;
  html.reserve(5000);

  html += F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<title>Cold Room Monitor</title>");
  html += F("<style>");
  html += F(":root{--bg:#07111f;--panel:#0f1b2d;--panel-2:#13213a;--text:#e8f1ff;--muted:#9db0cc;--accent:#55d6be;--accent-2:#4aa3ff;--border:rgba(255,255,255,.08);}");
  html += F("*{box-sizing:border-box;}body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at top,#12304f 0,#07111f 45%,#04070d 100%);color:var(--text);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px;}");
  html += F(".shell{width:min(720px,100%);}.hero{padding:28px;border:1px solid var(--border);border-radius:24px;background:linear-gradient(180deg,rgba(19,33,58,.96),rgba(10,18,31,.98));box-shadow:0 20px 60px rgba(0,0,0,.35);backdrop-filter:blur(12px);} ");
  html += F(".top{display:flex;justify-content:space-between;gap:16px;align-items:flex-start;flex-wrap:wrap;}.brand{font-size:14px;letter-spacing:.18em;text-transform:uppercase;color:var(--muted);}h1{margin:10px 0 0;font-size:clamp(28px,4vw,42px);line-height:1.05;} .sub{margin:10px 0 0;color:var(--muted);max-width:50ch;}");
  html += F(".grid{display:grid;grid-template-columns:1fr;gap:16px;margin-top:24px;}@media(min-width:640px){.grid{grid-template-columns:1.3fr .7fr;}}.card{border:1px solid var(--border);background:linear-gradient(180deg,rgba(255,255,255,.04),rgba(255,255,255,.02));border-radius:20px;padding:22px;}");
  html += F(".temp{font-size:clamp(52px,10vw,84px);font-weight:700;line-height:1;color:var(--accent);}.unit{font-size:.42em;color:var(--muted);margin-left:8px;}.label{margin-top:10px;color:var(--muted);font-size:14px;letter-spacing:.08em;text-transform:uppercase;}");
  html += F(".stat{display:flex;justify-content:space-between;align-items:center;gap:12px;padding:14px 0;border-bottom:1px solid var(--border);}.stat:last-child{border-bottom:0;}.stat span:first-child{color:var(--muted);} .pill{display:inline-flex;align-items:center;gap:8px;padding:8px 12px;border-radius:999px;background:rgba(85,214,190,.12);color:var(--accent);border:1px solid rgba(85,214,190,.2);font-size:13px;}");
  html += F(".dot{width:10px;height:10px;border-radius:50%;background:var(--accent);box-shadow:0 0 18px var(--accent);} .footer{margin-top:16px;color:var(--muted);font-size:13px;display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;}");
  html += F("</style></head><body><main class='shell'><section class='hero'><div class='top'><div><div class='brand'>Cold Room Monitor</div><h1>Temperature Dashboard</h1><p class='sub'>Live temperature snapshot from ESP32 access point. Data refreshes every 2 seconds through REST API.</p></div><div class='pill'><span class='dot'></span><span>AP Ready</span></div></div>");
  html += F("<div class='grid'><div class='card'><div id='temperature' class='temp'>--<span class='unit'>&deg;C</span></div><div class='label'>Current Temperature</div></div><div class='card'><div class='stat'><span>Last updated</span><strong id='lastUpdated'>--:--:--</strong></div><div class='stat'><span>API endpoint</span><strong>/api/data</strong></div><div class='stat'><span>SSID</span><strong>ColdRoomMonitor</strong></div></div></div>");
  html += F("<div class='footer'><span>ESP32 IP: 192.168.4.1</span><span>Open from any device connected to hotspot</span></div></section></main><script>");
  html += F("async function refreshData(){try{const response=await fetch('/api/data',{cache:'no-store'});const data=await response.json();document.getElementById('temperature').innerHTML=data.temperature.toFixed(1)+'<span class=\"unit\">&deg;C</span>';document.getElementById('lastUpdated').textContent=data.lastUpdated;}catch(error){console.error('Failed to load data:',error);}}refreshData();setInterval(refreshData,2000);");
  html += F("</script></body></html>");

  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", buildDashboardHtml());
}

void handleApiData() {
  char json[128];
  snprintf(json, sizeof(json), "{\"temperature\":%.1f,\"lastUpdated\":\"%s\"}", currentTemperatureC, lastUpdatedText.c_str());
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}
