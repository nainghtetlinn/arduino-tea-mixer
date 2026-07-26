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

    // --- Small 8x8 tea cup icon for header ---
    const unsigned char smallTeaIcon[] PROGMEM = {
        0x24, // .#..#...
        0x48, // .#..#...
        0x24, // ..#..#..
        0xFE, // #######.
        0x83, // #.....##
        0x83, // #.....##
        0xFE, // #######.
        0x7C  // .#####..
    };

    // --- Small 8x8 Bluetooth icon (connected) ---
    const unsigned char btIconConnected[] PROGMEM = {
        0x08, //     #
        0x0C, //     ##
        0x2A, //   # # #
        0x1C, //    ###
        0x1C, //    ###
        0x2A, //   # # #
        0x0C, //     ##
        0x08  //     #
    };

    // --- Small 16x16 Bluetooth icon (connected) ---
    const unsigned char bluetooth_icon[] PROGMEM = {
        0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x01, 0xC0,
        0x09, 0xE0, 0x05, 0xC0, 0x03, 0x80, 0x01, 0x00,
        0x03, 0x80, 0x05, 0xC0, 0x09, 0xE0, 0x01, 0xC0,
        0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
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

    // Show a simple splash until first real update
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 24);
    display.print(F("TEA MIXER"));
    display.display();

    return true;
}

// =========================================================================
// Idle Screen — Beautiful layout with tea icon header + 3 button names
// =========================================================================
//
// Layout (128x64):
//  ┌────────────────────────────┐
//  │        ☕ Tea Mixer      📶 │  y=2..10   Icons + Title (size 1)
//  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤  y=14      Dotted divider
//  │  ▸ Pon Hman                │  y=21
//  │  ▸ Cho Saint               │  y=35
//  │  ▸ Kya Saint               │  y=49
//  └────────────────────────────┘
//
void DisplayManager::showIdle(const String &btn1, const String &btn2, const String &btn3, bool bleConnected)
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Header: Tea Icon + "Tea Mixer" + BT status icon ---
    const char title[] = "Tea Mixer";
    constexpr int iconWidth = 8;
    constexpr int iconHeight = 8;
    constexpr int gap = 4;
    int titleWidth = strlen(title) * 6; // 6px per char at size 1
    int totalWidth = iconWidth + gap + titleWidth;
    int startX = (SCREEN_WIDTH - totalWidth) / 2;
    int headerY = 2;

    // Draw small tea icon on left
    display.drawBitmap(startX, headerY, smallTeaIcon, iconWidth, iconHeight, SSD1306_WHITE);

    // Draw "Tea Mixer" text
    display.setTextSize(1);
    display.setCursor(startX + iconWidth + gap, headerY);
    display.print(title);

    // Draw BT icon at the right edge when connected
    if (bleConnected)
    {
        display.drawBitmap(SCREEN_WIDTH - iconWidth - 4, headerY, btIconConnected, iconWidth, iconHeight, SSD1306_WHITE);
    }

    // Dotted divider under title — elegant dashed line
    for (int x = 4; x < SCREEN_WIDTH - 4; x += 3)
    {
        display.drawPixel(x, 14, SSD1306_WHITE);
    }

    // --- Button List ---
    const String *names[] = {&btn1, &btn2, &btn3};
    const int startY = 21;
    const int rowHeight = 14;

    for (int i = 0; i < 3; i++)
    {
        int y = startY + i * rowHeight;

        // Triangle bullet: ▸
        int triX = 8;
        int triY = y + 1;
        display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
        display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
        display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

        // Button name
        display.setCursor(16, y);
        display.print(*names[i]);
    }

    display.display();
}

// =========================================================================
// Recipe Preview Screen — Shows recipe details before brewing
// =========================================================================
//
// Layout (128x64):
//  ┌────────────────────────────┐
//  │        ▸ Pon Hman          │  y=0..15   Name (size 1, centered)
//  ├────────────────────────────┤  y=16      Divider
//  │  Tea ............... 240ml │  y=19
//  │  Condensed .......... 30ml │  y=28
//  │  Evaporated ......... 30ml │  y=37
//  │  Milk ................ 0ml │  y=46
//  ├────────────────────────────┤  y=54      Divider
//  │   Press again to brew ☕    │  y=56      Footer prompt
//  └────────────────────────────┘
//
void DisplayManager::showRecipePreview(const String &name, const TeaRecipe &recipe)
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Header: Recipe name centered ---
    display.setTextSize(1);

    // Calculate centered position for "▸ Name"
    int nameLen = name.length() + 2; // "▸ " prefix
    int nameX = (SCREEN_WIDTH - (nameLen * 6)) / 2;
    if (nameX < 2)
        nameX = 2;

    // Triangle bullet
    int triX = nameX;
    int triY = 5;
    display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
    display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
    display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

    display.setCursor(nameX + 6, 4);
    display.print(name);

    // Decorative border around header
    display.drawRoundRect(0, 0, SCREEN_WIDTH, 14, 2, SSD1306_WHITE);

    // --- Recipe Details ---
    const char *labels[] = {"Tea", "Condensed", "Evaporated", "Milk"};
    const int values[] = {recipe.tea, recipe.condensedMilk, recipe.evaporatedMilk, recipe.milk};

    int detailStartY = 18;
    int rowH = 9;

    for (int i = 0; i < 4; i++)
    {
        int y = detailStartY + i * rowH;

        display.setCursor(6, y);
        display.print(labels[i]);

        // Right-aligned value with "ml" suffix
        char valBuf[10];
        snprintf(valBuf, sizeof(valBuf), "%dml", values[i]);
        int valWidth = strlen(valBuf) * 6;
        display.setCursor(SCREEN_WIDTH - valWidth - 6, y);
        display.print(valBuf);

        // Dotted leader line between label and value
        int labelEnd = 6 + strlen(labels[i]) * 6 + 4;
        int valStart = SCREEN_WIDTH - valWidth - 6 - 4;
        for (int dx = labelEnd; dx < valStart; dx += 3)
        {
            display.drawPixel(dx, y + 3, SSD1306_WHITE);
        }
    }

    // --- Footer ---
    display.drawFastHLine(0, 54, SCREEN_WIDTH, SSD1306_WHITE);

    // Static footer: "Press again to brew" (Centered without icon)
    const char footerText[] = "Press again to brew";
    constexpr int footerLen = 19;          // strlen("Press again to brew")
    constexpr int footerW = footerLen * 6; // width of text only
    constexpr int footerX = (SCREEN_WIDTH - footerW) / 2;

    display.setCursor(footerX, 56);
    display.print(F("Press again to brew"));

    display.display();
}

void DisplayManager::showReady()
{
    if (!displayReady)
        return;

    display.clearDisplay();

    // -- LAYOUT MATH (Centering the two remaining elements) --
    constexpr int badgeW = 112;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 8; // Slightly larger gap for a balanced two-element layout

    // Total height of the remaining elements combined
    constexpr int totalHeight = badgeH + gap + textHeight;

    // Calculate starting Y to perfectly center the entire block
    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;

    // 1. Highlighted Badge (Now the Top Element)
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    // Text inside badge
    const char msg[] = "TEA IS READY!";
    int msgWidth = strlen(msg) * 6;
    int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    // 2. Footer Text (Bottom Element)
    const char footer[] = "Remove cup & enjoy!";
    int footerWidth = strlen(footer) * 6;
    int footerX = (SCREEN_WIDTH - footerWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(footerX, textY);
    display.print(footer);

    display.display();
}

void DisplayManager::showProgress(uint8_t progress)
{
    if (!displayReady)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    // -- LAYOUT MATH (Centering the whole block) --
    constexpr int barHeight = 9;
    constexpr int textHeight = 8;
    constexpr int gap = 6; // Visually pleasing gap between elements

    // Total height of all elements combined
    constexpr int totalHeight = ICON_HEIGHT + gap + barHeight + gap + textHeight;

    // Calculate starting Y to perfectly center the entire block on the screen
    constexpr int iconY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int barY = iconY + ICON_HEIGHT + gap;
    constexpr int textY = barY + barHeight + gap;

    // 1. Icon (Top Element)
    constexpr int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
    const bool useFrame1 = ((millis() / ANIMATION_SPEED_MS) % 2 == 0);
    const unsigned char *icon = useFrame1 ? teaIconFrame1 : teaIconFrame2;
    display.drawBitmap(iconX, iconY, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    // 2. Sleeker Progress Bar (Middle Element)
    constexpr int barX = 14;
    constexpr int barWidth = 100;

    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    const int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
    if (fillWidth > 0)
    {
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 1, SSD1306_WHITE);
    }

    // 3. Text (Bottom Element)
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    const uint8_t charCount = 12 + (progress >= 100 ? 3 : (progress >= 10 ? 2 : 1));
    const uint8_t textX = (SCREEN_WIDTH - (charCount * 6)) / 2;

    display.setCursor(textX, textY);
    display.print(F("Brewing... "));
    display.print(progress);
    display.print(F("%"));

    display.display();
}

void DisplayManager::showCleaningProgress(uint8_t remainingSeconds, uint8_t progress)
{
    if (!displayReady)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    // -- LAYOUT MATH (Centering the 3-element block) --
    constexpr int badgeW = 112;
    constexpr int badgeH = 15;
    constexpr int barWidth = 100;
    constexpr int barHeight = 9; // Sleek progress bar
    constexpr int textHeight = 8;
    constexpr int gap = 6; // Standard 6px gap

    // Total height: Badge + gap + Bar + gap + Combined Text
    constexpr int totalHeight = badgeH + gap + barHeight + gap + textHeight;

    // Calculate starting Y coordinates to perfectly center the entire block
    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int barY = badgeY + badgeH + gap;
    constexpr int textY = barY + barHeight + gap;

    // 1. Header Badge: "CLEANING MODE"
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;
    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 3, SSD1306_WHITE);

    const char title[] = "CLEANING MODE";
    int titleWidth = strlen(title) * 6;
    int titleX = (SCREEN_WIDTH - titleWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(titleX, badgeY + 4);
    display.print(title);

    // 2. Sleeker Progress Bar
    constexpr int barX = (SCREEN_WIDTH - barWidth) / 2;
    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    const int fillWidth = map(progress, 0, 100, 0, barWidth - 4);
    if (fillWidth > 0)
    {
        // 1px inner radius for the fill
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 1, SSD1306_WHITE);
    }

    // 3. Combined Text (Bottom Element)
    char txtBuf[32];
    // Formats into something like: "Flushing lines... 10s"
    snprintf(txtBuf, sizeof(txtBuf), "Flushing lines... %ds", remainingSeconds);

    int txtWidth = strlen(txtBuf) * 6;
    int txtX = (SCREEN_WIDTH - txtWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(txtX, textY);
    display.print(txtBuf);

    display.display();
}

void DisplayManager::showCleaningComplete()
{
    if (!displayReady)
        return;

    display.clearDisplay();

    // -- LAYOUT MATH (Centering the two remaining elements) --
    constexpr int badgeW = 112;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 8; // Slightly larger gap for a balanced two-element layout

    // Total height of the remaining elements combined
    constexpr int totalHeight = badgeH + gap + textHeight;

    // Calculate starting Y coordinates to perfectly center the entire block
    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;

    // 1. Highlighted badge (Now the Top Element)
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;
    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    const char msg[] = "CLEAN COMPLETE";
    int msgWidth = strlen(msg) * 6;
    int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    // 2. Subtext (Bottom Element)
    const char sub[] = "Lines flushed clear!";
    int subWidth = strlen(sub) * 6;
    int subX = (SCREEN_WIDTH - subWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(subX, textY);
    display.print(sub);

    display.display();
}

void DisplayManager::showCalibration(float temperature, float weight, bool weightMode)
{
    if (!displayReady)
        return;

    display.clearDisplay();

    // 1. Header: "CALIBRATION MODE" centered with bullet and border
    const char title[] = "CALIBRATION MODE";
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Calculate centered position for "▸ CALIBRATION MODE"
    int titleLen = strlen(title) + 2; // "▸ " prefix
    int titleX = (SCREEN_WIDTH - (titleLen * 6)) / 2;
    if (titleX < 2)
        titleX = 2;

    // Triangle bullet
    int triX = titleX;
    int triY = 5;
    display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
    display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
    display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

    display.setCursor(titleX + 6, 4);
    display.print(title);

    // Decorative border around header
    display.drawRoundRect(0, 0, SCREEN_WIDTH, 14, 2, SSD1306_WHITE);

    // 2. Temperature Row
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 16);
    display.print(F("Temp:"));

    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f C", temperature);
    int tempWidth = strlen(tempBuf) * 6;
    display.setCursor(SCREEN_WIDTH - tempWidth - 8, 16);
    display.print(tempBuf);

    for (int x = 44; x < SCREEN_WIDTH - tempWidth - 12; x += 3)
    {
        display.drawPixel(x, 19, SSD1306_WHITE);
    }

    // 3. Weight Row
    display.setCursor(8, 26);
    display.print(F("Weight:"));

    char weightBuf[16];
    snprintf(weightBuf, sizeof(weightBuf), "%.1f g", weight);
    int weightWidth = strlen(weightBuf) * 6;
    display.setCursor(SCREEN_WIDTH - weightWidth - 8, 26);
    display.print(weightBuf);

    for (int x = 56; x < SCREEN_WIDTH - weightWidth - 12; x += 3)
    {
        display.drawPixel(x, 29, SSD1306_WHITE);
    }

    // 4. Dispensing Mode Row (WEIGHT vs TIME)
    display.setCursor(8, 36);
    display.print(F("Mode:"));

    const char *modeStr = weightMode ? "WEIGHT" : "TIME";
    int modeWidth = strlen(modeStr) * 6;
    display.setCursor(SCREEN_WIDTH - modeWidth - 8, 36);
    display.print(modeStr);

    for (int x = 44; x < SCREEN_WIDTH - modeWidth - 12; x += 3)
    {
        display.drawPixel(x, 39, SSD1306_WHITE);
    }

    // 5. Footer Helper Lines
    display.drawFastHLine(0, 46, SCREEN_WIDTH, SSD1306_WHITE);

    const char footer1[] = "B1:Tare  |  B2:Mode";
    int f1Width = strlen(footer1) * 6;
    display.setCursor((SCREEN_WIDTH - f1Width) / 2, 48);
    display.print(footer1);

    const char footer2[] = "Hold B1+2 to Exit";
    int f2Width = strlen(footer2) * 6;
    display.setCursor((SCREEN_WIDTH - f2Width) / 2, 56);
    display.print(footer2);

    display.display();
}

// =========================================================================
// BLE Connected Screen — Minimalist notification
// =========================================================================
//
// Layout (128x64):
//  ┌────────────────────────────┐
//  │                            │
//  │        [ BT icon ]         │  y=10..26  Centered BT icon
//  │                            │
//  │        Connected           │  y=34      Text size 2, centered
//  │                            │
//  └────────────────────────────┘
//
void DisplayManager::showConnected()
{
    if (!displayReady)
        return;

    display.clearDisplay();

    // -- LAYOUT MATH (Centering the 2-element block) --
    constexpr int iconW = 16;
    constexpr int iconH = 16;
    constexpr int textHeight = 8;
    constexpr int gap = 8; // 8px gap looks great with a 16x16 icon

    // Total height of the elements combined (16 + 8 + 8 = 32px)
    constexpr int totalHeight = iconH + gap + textHeight;

    // Calculate starting Y coordinates to perfectly center the entire block
    constexpr int iconY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = iconY + iconH + gap;

    // 1. Bluetooth icon (Top Element)
    constexpr int iconX = (SCREEN_WIDTH - iconW) / 2;
    display.drawBitmap(iconX, iconY, bluetooth_icon, iconW, iconH, SSD1306_WHITE);

    // 2. "Connected!" text (Bottom Element - Size 1)
    const char msg[] = "Connected!";
    int msgWidth = strlen(msg) * 6; // size 1 = 6px per char

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((SCREEN_WIDTH - msgWidth) / 2, textY);
    display.print(msg);

    display.display();
}