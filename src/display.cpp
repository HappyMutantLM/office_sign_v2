#include "display.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <qrcode.h> 
#include "config.h"

// Declare a pointer instead of an uninitialized global object
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>* display = nullptr;

static const char* HEADER_NAME_TITLE = "Leila Mureebe, MD — CHIO"; 

void displayInit() {
  SPI.end(); 

  pinMode(EPD_CS_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  if (EPD_BUSY_POLLING_ENABLED) {
    pinMode(EPD_BUSY_PIN, INPUT);
  }

  SPI.begin(EPD_SCK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN, -1);

  display = new GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>(
    GxEPD2_420_GDEY042T81(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_POLLING_ENABLED ? EPD_BUSY_PIN : -1)
  );

  display->init(0, /*initial=*/true, /*reset_duration=*/20, /*pulldown_rst_mode=*/false);
  
  // Explicitly ensure invertDisplay is completely off
  display->invertDisplay(false); 
  display->setRotation(0); 
}

static void drawQRCode(const String& url, int originX, int originY, int moduleSize) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(4)]; 

  if (qrcode_initText(&qrcode, qrcodeData, 4, ECC_LOW, url.c_str()) != 0) {
    Serial.println("QR code generation failed - URL too long for version 4");
    return;
  }

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // SWAPPED: Use GxEPD_WHITE to draw the black QR pixels on this inverted panel layout
        display->fillRect(
          originX + x * moduleSize,
          originY + y * moduleSize,
          moduleSize,
          moduleSize,
          GxEPD_WHITE
        );
      }
    }
  }
}

void displayShowStatus(const Status& s, String qrURL) {
  if (display == nullptr) return;

  display->init(0, /*initial=*/false, /*reset_duration=*/40, /*pulldown_rst_mode=*/false);

  display->setFullWindow();
  display->firstPage();
  do {
    // 1. SWAPPED: Force fill the background with GxEPD_BLACK to get a beautiful white physical background
    display->fillScreen(GxEPD_BLACK); 

    // 2. SWAPPED: Set foreground text color to WHITE (renders as black ink) and background to BLACK (renders as white canvas)
    display->setTextColor(GxEPD_WHITE, GxEPD_BLACK); 
    
    // Name/title header
    display->setFont(&FreeSansBold9pt7b);
    display->setCursor(10, 20);
    display->print(HEADER_NAME_TITLE);
    display->drawLine(0, 28, 400, 28, GxEPD_WHITE); // SWAPPED: Draw line with GxEPD_WHITE for black ink

    // Big status headline
    display->setFont(&FreeSansBold18pt7b);
    display->setCursor(10, 100);
    display->print(s.headline);

    // Subtext
    display->setFont(&FreeSans9pt7b);
    display->setCursor(10, 130);
    display->print(s.subtext);

    // QR code + instructions conditional block
    if (s.allowMessage && qrURL.length() > 0) {
      String fullURL = "http://" + qrURL + "/leave-message";
      drawQRCode(fullURL, 270, 150, 3);

      display->setTextColor(GxEPD_WHITE, GxEPD_BLACK); // SWAPPED
      display->setFont(&FreeSans9pt7b);
      display->setCursor(255, 280);
      display->print("Scan to leave");
      display->setCursor(255, 295);
      display->print("a message");
    }
  } while (display->nextPage());
}

void displaySleep() {
  if (display != nullptr) display->hibernate();
}