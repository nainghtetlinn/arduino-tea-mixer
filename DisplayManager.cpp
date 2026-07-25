#include "DisplayManager.h"
#include "Config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
    // --- Pins ---
    constexpr uint8_t PIN_I2C_SDA = 21;
    constexpr uint8_t PIN_I2C_SCL = 22;

    // --- Display Configuration ---
    constexpr uint8_t SCREEN_WIDTH = 128;
    constexpr uint8_t SCREEN_HEIGHT = 64;
    constexpr int8_t OLED_RESET_PIN = -1; // -1 if sharing Arduino reset pin
    constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

    // --- Layout Constants ---
    constexpr uint8_t ICON_WIDTH = 24;
    constexpr uint8_t ICON_HEIGHT = 24;

    // --- Animation ---
    constexpr unsigned long ANIMATION_SPEED_MS = 500;

    // --- Animated Steam Bitmaps ---
    const unsigned char teaIconFrame1[] PROGMEM = {
        0x08, 0x20, 0x80, //    #     #     #
        0x10, 0x41, 0x00, //   #     #     #
        0x10, 0x41, 0x00, //   #     #     #
        0x08, 0x20, 0x80, //    #     #     #
        0x04, 0x10, 0x40, //     #     #     #
        0x04, 0x10, 0x40, //     #     #     #
        0x08, 0x20, 0x80, //    #     #     #
        0x10, 0x41, 0x00, //   #     #     #
        0x00, 0x00, 0x00,
        0x3f, 0xff, 0xc0, // ##############
        0x20, 0x00, 0x60, // #            ##
        0x20, 0x00, 0x30, // #             #
        0x20, 0x00, 0x38, // #            ##
        0x20, 0x00, 0x38, // #            ##
        0x20, 0x00, 0x30, // #             #
        0x20, 0x00, 0x60, // #            ##
        0x3f, 0xff, 0xc0, // ##############
        0x1f, 0xff, 0x80, //  ############
        0x0f, 0xff, 0x00, //   ##########
        0x07, 0xfe, 0x00, //    ########
        0x00, 0x00, 0x00,
        0x1f, 0xff, 0x80, //  ############ (Saucer)
        0x0f, 0xff, 0x00, //   ##########
        0x00, 0x00, 0x00};

    const unsigned char teaIconFrame2[] PROGMEM = {
        0x04, 0x10, 0x40, //     #     #     #
        0x08, 0x20, 0x80, //     #     #     #
        0x08, 0x20, 0x80, //    #     #     #
        0x10, 0x41, 0x00, //   #     #     #
        0x20, 0x82, 0x00, //  #     #     #
        0x20, 0x82, 0x00, //  #     #     #
        0x10, 0x41, 0x00, //   #     #     #
        0x08, 0x20, 0x80, //    #     #     #
        0x00, 0x00, 0x00,
        0x3f, 0xff, 0xc0, // ##############
        0x20, 0x00, 0x60, // #            ##
        0x20, 0x00, 0x30, // #             #
        0x20, 0x00, 0x38, // #            ##
        0x20, 0x00, 0x38, // #            ##
        0x20, 0x00, 0x30, // #             #
        0x20, 0x00, 0x60, // #            ##
        0x3f, 0xff, 0xc0, // ##############
        0x1f, 0xff, 0x80, //  ############
        0x0f, 0xff, 0x00, //   ##########
        0x07, 0xfe, 0x00, //    ########
        0x00, 0x00, 0x00,
        0x1f, 0xff, 0x80, //  ############ (Saucer)
        0x0f, 0xff, 0x00, //   ##########
        0x00, 0x00, 0x00};
} // anonymous namespace

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN),
      displayReady(false)
{
}

bool DisplayManager::begin()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS))
    {
        DEBUG_PRINTLN(F("[DisplayManager] Error: SSD1306 initialization failed!"));
        return false;
    }

    displayReady = true;
    DEBUG_PRINTLN(F("[DisplayManager] Initialized successfully."));

    showIdle();
    return true;
}

void DisplayManager::showIdle(float temperature, float weight)
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // 1. Header Bar
    display.setTextSize(1);
    display.setCursor(37, 2);
    display.print(F("TEA MIXER"));
    display.drawFastHLine(0, 13, SCREEN_WIDTH, SSD1306_WHITE);

    // 2. Vertical Column Divider (Center line)
    display.drawFastVLine(64, 14, 38, SSD1306_WHITE);

    // --- LEFT COLUMN: TEMPERATURE ---
    display.setTextSize(1);
    display.setCursor(20, 18);
    display.print(F("TEMP"));

    display.setTextSize(2);
    display.setCursor(4, 32);
    // Print 1 decimal place if < 100, otherwise print whole number to fit
    if (temperature < 100.0f)
    {
        display.print(temperature, 1);
    }
    else
    {
        display.print((int)temperature);
    }
    display.setTextSize(1);
    display.print((char)247); // Degree symbol (°) in Adafruit GFX font
    display.print(F("C"));

    // --- RIGHT COLUMN: WEIGHT ---
    display.setTextSize(1);
    display.setCursor(76, 18);
    display.print(F("WEIGHT"));

    display.setTextSize(2);
    display.setCursor(70, 32);
    display.print((int)weight); // Display weight as whole grams
    display.setTextSize(1);
    display.print(F("g"));

    // 3. Footer Bar
    display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(49, 55);
    display.print(F("READY"));

    display.display();
}

void DisplayManager::showReady()
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor((SCREEN_WIDTH - 72) / 2, 14);
    display.print(F("Tea is"));

    display.setCursor((SCREEN_WIDTH - 60) / 2, 39);
    display.print(F("Ready"));

    display.display();
}

void DisplayManager::showProgress(uint8_t progress)
{
    if (!displayReady)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    // 1. Draw animated steam icon (Time-based animation via millis())
    constexpr int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
    constexpr int iconY = 0; // Positioned right at top center

    // Toggle frames every [x]ms regardless of whether progress changes
    const bool useFrame1 = ((millis() / ANIMATION_SPEED_MS) % 2 == 0);
    const unsigned char *icon = useFrame1 ? teaIconFrame1 : teaIconFrame2;

    display.drawBitmap(iconX, iconY, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    // 2. Draw progress bar container (Wider 100px bar placed lower)
    constexpr int barX = 14;
    constexpr int barY = 27;
    constexpr int barWidth = 100;
    constexpr int barHeight = 11;
    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    // 3. Draw progress bar fill
    const int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
    if (fillWidth > 0)
    {
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 2, SSD1306_WHITE);
    }

    // 4. Draw percentage text centered below progress bar
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Dynamic width calculation: "Brewing... " (11) + progress digits (1-3) + "%" (1)
    const uint8_t charCount = 12 + (progress >= 100 ? 3 : (progress >= 10 ? 2 : 1));
    const uint8_t textX = (SCREEN_WIDTH - (charCount * 6)) / 2;

    display.setCursor(textX, 50);
    display.print(F("Brewing... "));
    display.print(progress);
    display.print(F("%"));

    display.display();
}
