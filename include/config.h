#pragma once

// Single source of truth for pin assignments.
#define EPD_CS_PIN     D0
#define EPD_DC_PIN     D1
#define EPD_RST_PIN    D2
#define EPD_BUSY_PIN   D3
#define EPD_SCK_PIN    D8
#define EPD_MISO_PIN   D9   
#define EPD_MOSI_PIN   D10

// TTP223 Touch Sensor Signal Wire -> Connect to D4 (GPIO4)
#define TOUCH_WAKE_PIN GPIO_NUM_4

// Duration (in seconds) to keep web server active on touch wake before sleeping
#define WEB_AWAKE_TIMEOUT_SEC 180

// 0 = Software safety delays (silences hardware polling timeouts)
#define EPD_BUSY_POLLING_ENABLED 0