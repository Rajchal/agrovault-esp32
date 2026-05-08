#include <WiFi.h>
#include <WebServer.h>

// Access point credentials
const char *kApSsid = "ColdRoomMonitor";
const char *kApPassword = "12345678";

// API server on port 80
WebServer server(80);

// Simulated temperature state
float currentTemperatureC = 5.0f;
float currentHumidityPercent = 75.0f;
unsigned long lastTemperatureUpdateMs = 0;
String lastUpdatedText = "--:--:--";

// Temperature simulation timing
const unsigned long kTemperatureUpdateIntervalMs = 1000;

// Forward declarations
void handleApi();
void handleNotFound();
void updateSimulatedTemperature();
String buildTimestamp();

void startAccessPoint()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(kApSsid, kApPassword);

  Serial.println();
  Serial.println("Access Point started");
  Serial.print("SSID: ");
  Serial.println(kApSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupRoutes()
{
  server.on("/api", HTTP_GET, handleApi);
  server.onNotFound(handleNotFound);
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  randomSeed((uint32_t)esp_random());
  currentTemperatureC = random(200, 801) / 100.0f;
  currentHumidityPercent = random(6000, 9101) / 100.0f;
  lastTemperatureUpdateMs = millis();
  lastUpdatedText = buildTimestamp();

  startAccessPoint();
  setupRoutes();
  server.begin();

  Serial.println("HTTP server started on port 80");
}

void loop()
{
  server.handleClient();
  updateSimulatedTemperature();
}

void updateSimulatedTemperature()
{
  const unsigned long now = millis();

  if (now - lastTemperatureUpdateMs < kTemperatureUpdateIntervalMs)
  {
    return;
  }

  lastTemperatureUpdateMs = now;

  // Small random walk, clamped to 2.0C..8.0C.
  const int deltaStep = random(-25, 26); // -0.25C..+0.25C
  currentTemperatureC += deltaStep / 100.0f;

  // Small random walk, clamped to 60.0%..91.0%.
  const int humidityDeltaStep = random(-40, 41); // -0.40%..+0.40%
  currentHumidityPercent += humidityDeltaStep / 100.0f;

  if (currentTemperatureC < 2.0f)
  {
    currentTemperatureC = 2.0f;
  }
  else if (currentTemperatureC > 8.0f)
  {
    currentTemperatureC = 8.0f;
  }

  if (currentHumidityPercent < 60.0f)
  {
    currentHumidityPercent = 60.0f;
  }
  else if (currentHumidityPercent > 91.0f)
  {
    currentHumidityPercent = 91.0f;
  }

  lastUpdatedText = buildTimestamp();
}

String buildTimestamp()
{
  const unsigned long totalSeconds = millis() / 1000UL;
  const unsigned int hours = (totalSeconds / 3600UL) % 24U;
  const unsigned int minutes = (totalSeconds / 60UL) % 60U;
  const unsigned int seconds = totalSeconds % 60U;

  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
  return String(buffer);
}
void handleApi()
{
  char json[256];
  snprintf(json, sizeof(json),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"lastUpdated\":\"%s\"}",
           currentTemperatureC, currentHumidityPercent, lastUpdatedText.c_str());
  server.send(200, "application/json", json);
}

void handleNotFound()
{
  server.send(404, "text/plain", "Not Found");
}
