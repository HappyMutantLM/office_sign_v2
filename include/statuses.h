#pragma once
#include <Arduino.h>

struct Status {
  const char* id;
  const char* headline;     
  const char* subtext;      
  bool allowMessage;        
};

static const Status STATUS_LIST[] = {
  { "online",     "ONLINE",               "Available - stop by",        false },
  { "compiling",  "COMPILING...",         "Do not interrupt",           true  },
  { "meeting",    "IN THE MATRIX",        "In a meeting",               true  },
  { "dnd",        "DO NOT DISTURB",       "Critical process running",   true  },
  { "away",       "404",                  "Human not found",            true  },
  { "lunch",      "REFUELING",            "Out to lunch",               false },
  { "open",       "PORT OPEN",            "Come on in!",                false },
  { "eod",        "CORE DUMPED",          "Gone for the day",           true  },
};

static const int STATUS_COUNT = sizeof(STATUS_LIST) / sizeof(Status); 
extern RTC_DATA_ATTR int currentStatusIndex;