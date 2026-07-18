#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#include "config.h"
#include "statuses.h"
#include "display.h"
#include "webserver.h"
#include "storage.h"
#include "secrets.h" 

#define AWAKE_TIMEOUT_MS 120000  
#define FORCE_REFRESH_HOURS 24   

RTC_DATA_ATTR static time_t lastRefreshEpoch = 0;
RTC_DATA_ATTR int currentStatusIndex = 0;

unsigned long wakeStart = 0;
bool wifiActive = false;
static bool mdnsStarted = false;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(200);
  }
  wifiActive = (WiFi.status() == WL_CONNECTED);

  if (wifiActive) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    if (!mdnsStarted) {
      if (MDNS.begin("doorsign")) {
        Serial.println("mDNS ready: http://doorsign.local");
        mdnsStarted = true;
      } else {
        Serial.println("mDNS setup failed");
      }
    }
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  }
}

void goToSleep() {
  if (wifiActive) {
    stopWebServer();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  displaySleep(); 
  esp_sleep_enable_timer_wakeup(30ULL * 60 * 1000000); 
  esp_deep_sleep_start();
}

void renderCurrentStatus(bool showQR) {
  const Status& s = STATUS_LIST[currentStatusIndex];
  displayShowStatus(s, showQR ? getDeviceIP() : "");
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give Serial monitor a moment to catch up
  Serial.println("\n--- BOOTING SIGN ---");
  
  Serial.print("Initializing Storage... ");
  storageInit(); 
  Serial.println("Done.");

  Serial.print("Initializing Display... ");
  displayInit();
  Serial.println("Done.");

  wakeStart = millis();

  Serial.print("Drawing initial status screen... ");
  renderCurrentStatus(false);
  Serial.println("Done.");

  Serial.print("Connecting to WiFi... ");
  connectWiFi();
  // loop won't proceed if connectWiFi hung, but we printed inside it[span_6](start_span)[span_6](end_span)
  
  if (wifiActive) {
    Serial.print("Starting mDNS... ");
    if (!mdnsStarted) {
      if (MDNS.begin("doorsign")) {
        Serial.println("mDNS success (http://doorsign.local)");
        mdnsStarted = true;
      } else {
        Serial.println("mDNS failed");
      }
    }

    Serial.print("Configuring NTP Time Sync... ");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Done.");

    Serial.print("Starting Async Web Server... ");
    startWebServer();
    Serial.println("Web Server Online!");
  } else {
    Serial.println("WiFi connection failed, skipping network services.");
  }

  lastRefreshEpoch = time(nullptr);
  Serial.println("--- SETUP COMPLETE, ENTERING LOOP ---");
}

void loop() {
  if (wifiActive && webserverHasStatusChange()) {
    currentStatusIndex = webserverConsumeStatusChange();
    renderCurrentStatus(true); 
    wakeStart = millis(); 
  }

  // Only run the 24h refresh calculation if the time system has actually synced
  if (time(nullptr) > 100000) { 
    if (time(nullptr) - lastRefreshEpoch > FORCE_REFRESH_HOURS * 3600) {
      renderCurrentStatus(wifiActive);
      lastRefreshEpoch = time(nullptr);
    }
  }

  if (millis() - wakeStart > AWAKE_TIMEOUT_MS) {
    goToSleep();
  }

  delay(50);
}