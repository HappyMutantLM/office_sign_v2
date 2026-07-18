#include "storage.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

static const char* MESSAGES_FILE = "/messages.json";

void storageInit() {
  if (!LittleFS.begin(true)) { // true = format on first-run failure
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Create an empty messages file if none exists yet
  if (!LittleFS.exists(MESSAGES_FILE)) {
    File f = LittleFS.open(MESSAGES_FILE, "w");
    if (f) {
      f.print("[]");
      f.close();
    }
  }
}

static String currentTimestamp() {
  // getLocalTime() checks time(nullptr) against a fixed absolute sentinel
  // (a 2017-ish Unix timestamp) rather than a boot-relative threshold, so
  // it correctly reports "not synced" even after long uptimes - a plain
  // "now < 100000" check would eventually drift past that and start
  // stamping messages with bogus 1970-ish dates once free-running time()
  // (no NTP) exceeded ~27.7 hours of accumulated value.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    // Time not synced yet (NTP hasn't completed) — fall back to millis-based marker
    return "uptime:" + String(millis() / 1000) + "s";
  }
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

void storageSaveMessage(const String& name, const String& text) {
  // Load existing messages
  File f = LittleFS.open(MESSAGES_FILE, "r");
  
  // ArduinoJson 7: Elastic allocation, no size required
  JsonDocument doc; 

  if (f) {
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      Serial.println("Failed to parse existing messages, starting fresh.");
      doc.clear();
      doc.to<JsonArray>();
    }
  } else {
    doc.to<JsonArray>();
  }

  JsonArray arr = doc.is<JsonArray>() ? doc.as<JsonArray>() : doc.to<JsonArray>();

  // ArduinoJson 7 syntax: use add<JsonObject>() instead of createNestedObject()
  JsonObject entry = arr.add<JsonObject>(); 
  entry["name"] = name.length() > 0 ? name : "Anonymous";
  entry["text"] = text;
  entry["timestamp"] = currentTimestamp();

  // Cap history so flash doesn't fill up — keep the most recent 50
  const int MAX_MESSAGES = 50;
  while (arr.size() > MAX_MESSAGES) {
    arr.remove(0);
  }

  File out = LittleFS.open(MESSAGES_FILE, "w");
  if (out) {
    serializeJson(doc, out);
    out.close();
  } else {
    Serial.println("Failed to write messages file!");
  }
}

std::vector<Message> storageGetMessages() {
  std::vector<Message> result;

  File f = LittleFS.open(MESSAGES_FILE, "r");
  if (!f) return result;

  // ArduinoJson 7: Elastic allocation, no size required
  JsonDocument doc; 
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return result;

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject entry : arr) {
    Message m;
    m.name = entry["name"] | "Anonymous";
    m.text = entry["text"] | "";
    m.timestamp = entry["timestamp"] | "";
    result.push_back(m);
  }

  // Most recent first for display
  std::reverse(result.begin(), result.end());
  return result;
}

void storageClearMessages() {
  File f = LittleFS.open(MESSAGES_FILE, "w");
  if (f) {
    f.print("[]");
    f.close();
  }
}