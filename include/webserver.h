#pragma once
#include <Arduino.h>

void startWebServer();
void stopWebServer();
String getDeviceIP();

bool webserverHasStatusChange();
int  webserverConsumeStatusChange();

// Added this so main.cpp and webserver.cpp share the exact same variable tracking space
extern volatile int pendingStatusChange;