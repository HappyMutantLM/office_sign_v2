#pragma once
#include <Arduino.h>
#include "statuses.h"

void displayInit();
void displayShowStatus(const Status& s, String qrURL);
void displaySleep();