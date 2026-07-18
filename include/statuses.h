#pragma once
#include <Arduino.h>

struct Status {
  const char* id;
  const char* headline;     
  const char* subtext;      
  bool allowMessage;        
};

static const Status STATUS_LIST[] = {
  { "online",     "ONLINE",               "Available - stop by",        false }, // No QR
  { "compiling",  "COMPILING...",         "Do not interrupt",           true  }, // QR Active
  { "meeting",    "IN THE MATRIX",        "In a meeting",               true  }, // QR Active
  { "dnd",        "DO NOT DISTURB",       "Critical process running",   true  }, // QR Active
  { "away",       "404",                  "Human not found",            true  }, // QR Active
  { "lunch",      "REFUELING",            "Out to lunch",               false }, // No QR
  { "open",       "PORT OPEN",            "Come on in!",                false }, // No QR
  { "eod",        "CORE DUMPED",   "Gone for the day",           true  }, // QR Active
};

static const int STATUS_COUNT = sizeof(STATUS_LIST) / sizeof(Status); // cite: 13
extern RTC_DATA_ATTR int currentStatusIndex; // cite: 13