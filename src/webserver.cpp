#include "webserver.h"
#include "statuses.h"
#include "storage.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
volatile int pendingStatusChange = -1; //changed to match h file

static const size_t MAX_NAME_LEN = 60;
static const size_t MAX_MSG_LEN  = 300;

String getDeviceIP() {
  return WiFi.localIP().toString();
}

static String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '&':  out += "&amp;";  break;
      case '<':  out += "&lt;";   break;
      case '>':  out += "&gt;";   break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;";  break;
      default:   out += c;        break;
    }
  }
  return out;
}

void startWebServer() {
  // --- Status control page ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<html><body style='font-family:sans-serif'><h2>Door Sign</h2>";
    for (int i = 0; i < STATUS_COUNT; i++) {
      html += "<form style='display:inline' method='POST' action='/set-status'>";
      html += "<input type='hidden' name='idx' value='" + String(i) + "'>";
      html += "<button style='margin:4px;padding:10px'>" + String(STATUS_LIST[i].headline) + "</button>";
      html += "</form>";
    }
    html += "<p><a href='/messages'>View messages</a></p></body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/set-status", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("idx", true)) {
      int idx = request->getParam("idx", true)->value().toInt();
      if (idx >= 0 && idx < STATUS_COUNT) pendingStatusChange = idx;
    } else if (request->hasParam("idx")) {
      int idx = request->getParam("idx")->value().toInt();
      if (idx >= 0 && idx < STATUS_COUNT) pendingStatusChange = idx;
    }
    request->redirect("/");
  });

  // --- Visitor-facing message form ---
  server.on("/leave-message", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<html><body style='font-family:sans-serif;max-width:400px;margin:auto'>"
                   "<h3>Leave a message</h3>"
                   "<form method='POST' action='/leave-message'>"
                   "Name: <input name='name' style='width:100%'><br><br>"
                   "Message: <textarea name='msg' style='width:100%' rows=4></textarea><br><br>"
                   "<button style='padding:10px 20px'>Send</button>"
                   "</form></body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/leave-message", HTTP_POST, [](AsyncWebServerRequest* request) {
    String name = "Anonymous";
    String msg  = "";

    if (request->hasParam("name", true)) name = request->getParam("name", true)->value();
    else if (request->hasParam("name"))  name = request->getParam("name")->value();

    if (request->hasParam("msg", true))  msg = request->getParam("msg", true)->value();
    else if (request->hasParam("msg"))   msg = request->getParam("msg")->value();

    if (name.length() > MAX_NAME_LEN) name = name.substring(0, MAX_NAME_LEN);
    if (msg.length() > MAX_MSG_LEN)   msg  = msg.substring(0, MAX_MSG_LEN);

    storageSaveMessage(name, msg);
    request->send(200, "text/html", "<html><body style='font-family:sans-serif'>"
                                     "<h3>Thanks! Your message was left.</h3></body></html>");
  });

  // --- View stored messages ---
  server.on("/messages", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<html><body style='font-family:sans-serif'><h3>Messages</h3><ul>";
    for (auto& m : storageGetMessages()) {
      html += "<li><b>" + htmlEscape(m.name) + "</b> (" + htmlEscape(m.timestamp) + "): " + htmlEscape(m.text) + "</li>";
    }
    html += "</ul></body></html>";
    request->send(200, "text/html", html);
  });

  server.begin();
}

void stopWebServer() {
  server.end();
}

bool webserverHasStatusChange() {
  return pendingStatusChange != -1;
}

int webserverConsumeStatusChange() {
  int v = pendingStatusChange;
  pendingStatusChange = -1;
  return v;
}