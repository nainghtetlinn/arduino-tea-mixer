#include "DisplayManager.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
    constexpr int SCREEN_WIDTH = 128;
    constexpr int SCREEN_HEIGHT = 64;
    constexpr int ICON_WIDTH = 16;
    constexpr int ICON_HEIGHT = 16;

    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    bool displayReady = false;

    const unsigned char teaIconFrame1[] PROGMEM = {
        0x0a, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x3f, 0xf0, 0x20, 0x18, 0x3f, 0xd4, 0x3f, 0xd4,
        0x1f, 0xd8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0,
        0x00, 0x00, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00};

    const unsigned char teaIconFrame2[] PROGMEM = {
        0x14, 0x00, 0x0a, 0x00, 0x14, 0x00, 0x00, 0x00,
        0x3f, 0xf0, 0x20, 0x18, 0x3f, 0xd4, 0x3f, 0xd4,
        0x1f, 0xd8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0,
        0x00, 0x00, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00};
} // namespace

bool initDisplay()
{
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        return false;

    displayReady = true;
    display.clearDisplay();
    display.display();
    return true;
}

void displayTeaProgress(uint8_t progress)
{
    if (!displayReady)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    const int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
    const unsigned char *icon = ((progress / 4) % 2 == 0)
                                    ? teaIconFrame1
                                    : teaIconFrame2;
    display.drawBitmap(iconX, 2, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    constexpr int barX = 24;
    constexpr int barY = 24;
    constexpr int barWidth = 80;
    constexpr int barHeight = 10;
    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    const int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
    if (fillWidth > 0)
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 2, SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    const String statusText = "Brewing... " + String(progress) + "%";
    display.setCursor((SCREEN_WIDTH - statusText.length() * 6) / 2, 44);
    display.print(statusText);
    display.display();
}
