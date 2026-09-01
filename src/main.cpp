#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#include "config.h"
#include "statuses.h"
#include "display.h"
#include "webserver.h"
#include "storage.h"
#include "secrets.h" 

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

  // ext1 wakeup is level-triggered at the moment sleep is entered: if the
  // pad is still being held HIGH right now, we'd wake right back up
  // immediately. Wait (briefly, with a cap) for it to release first.
  unsigned long releaseWaitStart = millis();
  while (digitalRead(TOUCH_WAKE_PIN) == HIGH && millis() - releaseWaitStart < 5000) {
    delay(20);
  }

  // Enable TTP223 capacitive touch pad as the wake trigger
  esp_sleep_enable_ext1_wakeup(1ULL << TOUCH_WAKE_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);

  // Safety timer wakeup (24-hour baseline sync)
  esp_sleep_enable_timer_wakeup(24ULL * 3600 * 1000000); 

  Serial.println("Entering deep sleep. Tap touch sensor to wake...");
  Serial.flush();
  esp_deep_sleep_start();
}

void renderCurrentStatus(bool showQR) {
  const Status& s = STATUS_LIST[currentStatusIndex];
  displayShowStatus(s, showQR ? getDeviceIP() : "");
}

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n--- BOOTING SIGN ---");

  pinMode(TOUCH_WAKE_PIN, INPUT);
  
  Serial.print("Initializing Storage... ");
  storageInit(); 
  Serial.println("Done.");

  Serial.print("Initializing Display... ");
  displayInit();
  Serial.println("Done.");

  wakeStart = millis();

  Serial.print("Connecting to WiFi... ");
  connectWiFi();

  // Draw current status with web link if Wi-Fi succeeded
  Serial.print("Drawing status screen... ");
  renderCurrentStatus(wifiActive);
  Serial.println("Done.");

  if (wifiActive) {
    Serial.print("Starting Async Web Server... ");
    startWebServer();
    Serial.println("Web Server Online!");
  } else {
    Serial.println("WiFi connection failed, offline mode.");
  }

  Serial.println("--- SETUP COMPLETE, ENTERING LOOP ---");
}

void loop() {
  // Catch incoming HTTP requests from phone
  if (wifiActive && webserverHasStatusChange()) {
    currentStatusIndex = webserverConsumeStatusChange();
    renderCurrentStatus(true); 
    wakeStart = millis(); // Reset active timer on interaction
  }

  // NOTE: no in-loop "24h maintenance refresh" here anymore. It was dead
  // code in practice - lastRefreshEpoch was reset every setup(), and the
  // device always sleeps within WEB_AWAKE_TIMEOUT_SEC, so 24h of elapsed
  // time could never accumulate inside a single loop() session. The daily
  // refresh is already guaranteed independently by the 24h deep-sleep timer
  // wakeup in goToSleep(), which forces a reboot -> setup() -> unconditional
  // renderCurrentStatus() every 24 hours regardless of touch activity.
  // It also used the same naive "time(nullptr) > 100000" heuristic that the
  // comment in storage.cpp's currentTimestamp() already explains is unsafe
  // (drifts past that threshold after ~27.7h of un-synced free-running time)
  // - getLocalTime() there is the correct way to check for a real NTP sync.

  // Enter deep sleep once active timeout window expires
  if (millis() - wakeStart > ((unsigned long)WEB_AWAKE_TIMEOUT_SEC * 1000)) {
    goToSleep();
  }

  delay(50);
}