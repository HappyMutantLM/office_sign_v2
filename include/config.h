#pragma once

// Single source of truth for pin assignments.
#define EPD_CS_PIN     D0
#define EPD_DC_PIN     D1
#define EPD_RST_PIN    D2
#define EPD_BUSY_PIN   D3
#define EPD_SCK_PIN    D8
#define EPD_MISO_PIN   D9   
#define EPD_MOSI_PIN   D10

// 1 = Re-enable actual hardware busy polling now that setup sequence is safe
#define EPD_BUSY_POLLING_ENABLED 0