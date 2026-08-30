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
    const unsigned char btIcon[] PROGMEM = {
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
}

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN),
      ready(false)
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

    ready = true;
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

// ========== Screens ==========

//  ┌────────────────────────────┐
//  │        ☕ Tea Mixer      📶 │  y=2..10   Icons + Title (size 1)
//  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤  y=14      Dotted divider
//  │  ▸ Pon Hman                │  y=21
//  │  ▸ Cho Saint               │  y=35
//  │  ▸ Kya Saint               │  y=49
//  └────────────────────────────┘
void DisplayManager::showIdleScreen(const String &btn1, const String &btn2, const String &btn3, bool bleConnected, float temperature)
{
    if (!ready)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    constexpr char title[] = "Tea Mixer";
    constexpr int iconWidth = 8;
    constexpr int iconHeight = 8;
    constexpr int gap = 4;

    constexpr int titleWidth = (sizeof(title) - 1) * 6;
    constexpr int totalWidth = iconWidth + gap + titleWidth;
    constexpr int startX = 2;
    constexpr int headerY = 2;

    display.drawBitmap(startX, headerY, smallTeaIcon, iconWidth, iconHeight, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(startX + iconWidth + gap, headerY);
    display.print(title);

    // ===== Temperature — right-aligned in header =====
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%d\xF8"
                                       "C",
             (int)temperature);
    int tempWidth = strlen(tempBuf) * 6;
    int rightCursor = SCREEN_WIDTH - tempWidth - 2;

    if (bleConnected)
    {
        int btX = rightCursor - iconWidth - 2;
        if (btX > 0)
        {
            display.drawBitmap(btX, headerY, btIcon, iconWidth, iconHeight, SSD1306_WHITE);
        }
    }
    display.setCursor(rightCursor, headerY);
    display.print(tempBuf);

    for (int x = 4; x < SCREEN_WIDTH - 4; x += 3)
    {
        display.drawPixel(x, 14, SSD1306_WHITE);
    }

    const String *names[] = {&btn1, &btn2, &btn3};
    constexpr int startY = 21;
    constexpr int rowHeight = 14;

    for (int i = 0; i < 3; i++)
    {
        const int y = startY + i * rowHeight;
        constexpr int triX = 8;
        const int triY = y + 1;

        display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
        display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
        display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

        display.setCursor(16, y);
        display.print(*names[i]);
    }

    display.display();
}

//  ╭────────────────────────────╮
//  │        ▸ Pon Hman          │  y=0..13   Header box (128x14 rounded rect, text y=4)
//  ╰────────────────────────────╯
//  │  Tea ............... 240ml │  y=18      Detail Row 1 (dotted line at y=21)
//  │  Condensed .......... 30ml │  y=27      Detail Row 2 (dotted line at y=30)
//  │  Evaporated ......... 30ml │  y=36      Detail Row 3 (dotted line at y=39)
//  │  Milk ................ 0ml │  y=45      Detail Row 4 (dotted line at y=48)
//  ├────────────────────────────┤  y=54      Footer divider line
//  │    Press again to brew     │  y=56      Footer text (centered)
//  └────────────────────────────┘  y=63      Screen bottom
void DisplayManager::showRecipePreviewScreen(const String &name, const TeaRecipe &recipe)
{
    if (!ready)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    const int nameLen = name.length() + 2;
    int nameX = (SCREEN_WIDTH - (nameLen * 6)) / 2;
    if (nameX < 2)
    {
        nameX = 2;
    }

    const int triX = nameX;
    constexpr int triY = 5;

    display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
    display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
    display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

    display.setCursor(nameX + 6, 4);
    display.print(name);

    display.drawRoundRect(0, 0, SCREEN_WIDTH, 14, 2, SSD1306_WHITE);

    constexpr const char *labels[] = {"Tea", "Condensed", "Evaporated", "Milk"};
    const int values[] = {recipe.tea, recipe.condensedMilk, recipe.evaporatedMilk, recipe.milk};

    constexpr int detailStartY = 18;
    constexpr int rowH = 9;

    for (int i = 0; i < 4; i++)
    {
        const int y = detailStartY + i * rowH;

        display.setCursor(6, y);
        display.print(labels[i]);

        char valBuf[10];
        snprintf(valBuf, sizeof(valBuf), "%dml", values[i]);
        const int valWidth = strlen(valBuf) * 6;

        display.setCursor(SCREEN_WIDTH - valWidth - 6, y);
        display.print(valBuf);

        const int labelEnd = 6 + strlen(labels[i]) * 6 + 4;
        const int valStart = SCREEN_WIDTH - valWidth - 6 - 4;

        for (int dx = labelEnd; dx < valStart; dx += 3)
        {
            display.drawPixel(dx, y + 3, SSD1306_WHITE);
        }
    }

    display.drawFastHLine(0, 54, SCREEN_WIDTH, SSD1306_WHITE);

    constexpr char footerText[] = "Press again to brew";
    constexpr int footerWidth = (sizeof(footerText) - 1) * 6;
    constexpr int footerX = (SCREEN_WIDTH - footerWidth) / 2;

    display.setCursor(footerX, 56);
    display.print(footerText);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..4    Empty space
//  │           [ ☕ ]            │  y=5       Animated Tea Icon (Assumes 24x24)
//  │                            │  y=29..34  Gap (6px)
//  │   ╭────────────────────╮   │  y=35      Progress bar top (100x9 rounded rect)
//  │   │███████████         │   │  y=37      Progress bar fill (1px internal padding)
//  │   ╰────────────────────╯   │  y=43      Progress bar bottom
//  │                            │  y=44..49  Gap (6px)
//  │       Brewing... 45%       │  y=50      Text (dynamic width, centered)
//  │                            │  y=58..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showTeaProgressScreen(uint8_t progress)
{
    if (!ready)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    constexpr int barHeight = 9;
    constexpr int textHeight = 8;
    constexpr int gap = 6;
    constexpr int totalHeight = ICON_HEIGHT + gap + barHeight + gap + textHeight;

    constexpr int iconY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int barY = iconY + ICON_HEIGHT + gap;
    constexpr int textY = barY + barHeight + gap;

    constexpr int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
    const bool useFrame1 = ((millis() / ANIMATION_SPEED_MS) % 2 == 0);
    const unsigned char *icon = useFrame1 ? teaIconFrame1 : teaIconFrame2;

    display.drawBitmap(iconX, iconY, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    constexpr int barX = 14;
    constexpr int barWidth = 100;
    constexpr int fillMax = barWidth - 4;

    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    const int fillWidth = (progress * fillMax) / 100;

    if (fillWidth > 0)
    {
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 1, SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // 12 represents: "Brewing... " (11 chars) + "%" (1 char)
    const uint8_t charCount = 12 + (progress >= 100 ? 3 : (progress >= 10 ? 2 : 1));
    const uint8_t textX = (SCREEN_WIDTH - (charCount * 6)) / 2;

    display.setCursor(textX, textY);
    display.print(F("Brewing... "));
    display.print(progress);
    display.print(F("%"));

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..14   Empty space
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=15      Badge top (112x17 rounded rect)
//  │  █    TEA IS READY!     █  │  y=20      Inverted text (centered)
//  │  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀  │  y=31      Badge bottom
//  │                            │  y=32..39  Gap (8px)
//  │    Remove cup & enjoy!     │  y=40      Footer text
//  │                            │  y=48..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showTeaReadyScreen()
{
    if (!ready)
        return;

    display.clearDisplay();

    constexpr int badgeW = 112;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 8;
    constexpr int totalHeight = badgeH + gap + textHeight;

    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    constexpr char msg[] = "TEA IS READY!";
    constexpr int msgWidth = (sizeof(msg) - 1) * 6;
    constexpr int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    constexpr char footerText[] = "Remove cup & enjoy!";
    constexpr int footerWidth = (sizeof(footerText) - 1) * 6;
    constexpr int footerX = (SCREEN_WIDTH - footerWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(footerX, textY);
    display.print(footerText);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..9    Empty space
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=10      Badge top (112x15 rounded rect)
//  │  █    CLEANING MODE     █  │  y=14      Inverted text (centered)
//  │  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀  │  y=24      Badge bottom
//  │                            │  y=25..30  Gap (6px)
//  │   ╭────────────────────╮   │  y=31      Progress bar top (100x9 rounded rect)
//  │   │███████████         │   │  y=33      Progress bar fill (1px internal padding)
//  │   ╰────────────────────╯   │  y=39      Progress bar bottom
//  │                            │  y=40..45  Gap (6px)
//  │   Flushing lines... 10s    │  y=46      Combined text (dynamic width, centered)
//  │                            │  y=54..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showCleaningProgressScreen(uint8_t remainingSeconds, uint8_t progress)
{
    if (!ready)
        return;

    progress = constrain(progress, 0, 100);
    display.clearDisplay();

    constexpr int badgeW = 112;
    constexpr int badgeH = 15;
    constexpr int barWidth = 100;
    constexpr int barHeight = 9;
    constexpr int textHeight = 8;
    constexpr int gap = 6;

    constexpr int totalHeight = badgeH + gap + barHeight + gap + textHeight;
    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int barY = badgeY + badgeH + gap;
    constexpr int textY = barY + barHeight + gap;

    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;
    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 3, SSD1306_WHITE);

    constexpr char title[] = "CLEANING MODE";
    constexpr int titleWidth = (sizeof(title) - 1) * 6;
    constexpr int titleX = (SCREEN_WIDTH - titleWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(titleX, badgeY + 4);
    display.print(title);

    constexpr int barX = (SCREEN_WIDTH - barWidth) / 2;
    constexpr int fillMax = barWidth - 4;

    display.drawRoundRect(barX, barY, barWidth, barHeight, 3, SSD1306_WHITE);

    const int fillWidth = (progress * fillMax) / 100;

    if (fillWidth > 0)
    {
        display.fillRoundRect(barX + 2, barY + 2, fillWidth, barHeight - 4, 1, SSD1306_WHITE);
    }

    char txtBuf[32];
    snprintf(txtBuf, sizeof(txtBuf), "Flushing lines... %ds", remainingSeconds);

    // Must use strlen() here because the buffer content length changes at runtime
    const int txtWidth = strlen(txtBuf) * 6;
    const int txtX = (SCREEN_WIDTH - txtWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(txtX, textY);
    display.print(txtBuf);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..14   Empty space
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=15      Badge top (112x17 rounded rect)
//  │  █    CLEAN COMPLETE    █  │  y=20      Inverted text (centered)
//  │  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀  │  y=31      Badge bottom
//  │                            │  y=32..39  Gap (8px)
//  │    Lines flushed clear!    │  y=40      Subtext (centered)
//  │                            │  y=48..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showCleaningCompleteScreen()
{
    if (!ready)
        return;

    display.clearDisplay();

    constexpr int badgeW = 112;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 8;
    constexpr int totalHeight = badgeH + gap + textHeight;

    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    constexpr char msg[] = "CLEAN COMPLETE";
    constexpr int msgWidth = (sizeof(msg) - 1) * 6;
    constexpr int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    constexpr char sub[] = "Lines flushed clear!";
    constexpr int subWidth = (sizeof(sub) - 1) * 6;
    constexpr int subX = (SCREEN_WIDTH - subWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(subX, textY);
    display.print(sub);

    display.display();
}

//  ╭────────────────────────────╮
//  │     ▸ CALIBRATION MODE     │  y=0..13   Header box (128x14 rounded rect, text y=4)
//  ╰────────────────────────────╯
//  │  Cup Wt: .......... 50.0 g │  y=16      Cup Weight row
//  │  Mode: ............ WEIGHT │  y=25      Mode row
//  │  Weight: .......... 25.5 g │  y=34      Weight row
//  │  Temp: ............ 95.0 C │  y=43      Temp row
//  ├────────────────────────────┤  y=53      Footer divider line
//  │     Hold B1+2 to Exit      │  y=55      Footer text (centered)
//  └────────────────────────────┘  y=63      Screen bottom
//
//  When calibratingCup == true:
//  ┌────────────────────────────┐
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=0..14   Inverted badge "PLACE CUP"
//  │  Weight: .......... 25.5 g │  y=20      Realtime weight
//  │                            │
//  │   Press B1 again to save   │  y=56      Footer hint
//  └────────────────────────────┘
void DisplayManager::showCalibrationScreen(float temperature, float weight, bool weightMode, float cupWeight, bool calibratingCup)
{
    if (!ready)
        return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (calibratingCup)
    {
        // --- Inverted badge header: "PLACE CUP" ---
        constexpr int badgeW = 112;
        constexpr int badgeH = 15;
        constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;
        constexpr int badgeY = 0;

        display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 3, SSD1306_WHITE);

        constexpr char badge[] = "PLACE CUP";
        constexpr int badgeTxtW = (sizeof(badge) - 1) * 6;
        constexpr int badgeTxtX = (SCREEN_WIDTH - badgeTxtW) / 2;

        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        display.setCursor(badgeTxtX, badgeY + 4);
        display.print(badge);

        // --- Realtime weight display (large, centered) ---
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);

        char wBuf[16];
        snprintf(wBuf, sizeof(wBuf), "%.1f g", weight);
        const int wWidth = strlen(wBuf) * 12; // textSize 2 = 12px per char
        const int wX = (SCREEN_WIDTH - wWidth) / 2;

        display.setCursor(wX, 24);
        display.print(wBuf);

        // --- Footer hints ---
        display.setTextSize(1);

        constexpr char hintSave[] = "Press B1 to save";
        constexpr int saveW = (sizeof(hintSave) - 1) * 6;
        constexpr int saveX = (SCREEN_WIDTH - saveW) / 2;

        display.setCursor(saveX, 47);
        display.print(hintSave);

        constexpr char hintCancel[] = "B2 or B3 to cancel";
        constexpr int cancelW = (sizeof(hintCancel) - 1) * 6;
        constexpr int cancelX = (SCREEN_WIDTH - cancelW) / 2;

        display.setCursor(cancelX, 56);
        display.print(hintCancel);

        display.display();
        return;
    }

    // ===== Normal calibration view =====

    // --- Header ---
    constexpr char title[] = "CALIBRATION MODE";
    constexpr int titleLen = (sizeof(title) - 1) + 2; // +2 for the "▸ " prefix
    constexpr int calcTitleX = (SCREEN_WIDTH - (titleLen * 6)) / 2;
    constexpr int titleX = (calcTitleX < 2) ? 2 : calcTitleX;

    constexpr int triX = titleX;
    constexpr int triY = 5;

    display.drawLine(triX, triY, triX, triY + 4, SSD1306_WHITE);
    display.drawLine(triX + 1, triY + 1, triX + 1, triY + 3, SSD1306_WHITE);
    display.drawPixel(triX + 2, triY + 2, SSD1306_WHITE);

    display.setCursor(titleX + 6, 4);
    display.print(title);

    display.drawRoundRect(0, 0, SCREEN_WIDTH, 14, 2, SSD1306_WHITE);

    // --- Data rows (4 rows, 9px each starting at y=16) ---
    constexpr int detailStartY = 16;
    constexpr int rowH = 9;

    // Helper lambda-like approach using arrays
    const char *labels[] = {"Cup Wt:", "Mode:", "Weight:", "Temp:"};
    char valueBufs[4][16];

    snprintf(valueBufs[0], sizeof(valueBufs[0]), "%.1f g", cupWeight);
    snprintf(valueBufs[1], sizeof(valueBufs[1]), "%s", weightMode ? "WEIGHT" : "TIME");
    snprintf(valueBufs[2], sizeof(valueBufs[2]), "%.1f g", weight);
    snprintf(valueBufs[3], sizeof(valueBufs[3]), "%.1f C", temperature);

    for (int i = 0; i < 4; i++)
    {
        const int y = detailStartY + i * rowH;

        display.setCursor(8, y);
        display.print(labels[i]);

        const int valWidth = strlen(valueBufs[i]) * 6;
        display.setCursor(SCREEN_WIDTH - valWidth - 8, y);
        display.print(valueBufs[i]);

        // Dotted line between label and value
        const int labelEnd = 8 + strlen(labels[i]) * 6 + 4;
        const int valStart = SCREEN_WIDTH - valWidth - 8 - 4;

        for (int x = labelEnd; x < valStart; x += 3)
        {
            display.drawPixel(x, y + 3, SSD1306_WHITE);
        }
    }

    // --- Footer ---
    display.drawFastHLine(0, 53, SCREEN_WIDTH, SSD1306_WHITE);

    constexpr char footer[] = "Hold B1+2 to Exit";
    constexpr int fWidth = (sizeof(footer) - 1) * 6;
    constexpr int fX = (SCREEN_WIDTH - fWidth) / 2;

    display.setCursor(fX, 55);
    display.print(footer);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │
//  │        [ BT icon ]         │  y=10..26  Centered BT icon
//  │                            │
//  │        Connected!          │  y=34      Text size 2, centered
//  │                            │
//  └────────────────────────────┘
void DisplayManager::showBleConnectedScreen()
{
    if (!ready)
        return;

    display.clearDisplay();

    constexpr int iconW = 16;
    constexpr int iconH = 16;
    constexpr int textHeight = 8;
    constexpr int gap = 8;
    constexpr int totalHeight = iconH + gap + textHeight;

    constexpr int iconY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = iconY + iconH + gap;
    constexpr int iconX = (SCREEN_WIDTH - iconW) / 2;

    display.drawBitmap(iconX, iconY, bluetooth_icon, iconW, iconH, SSD1306_WHITE);

    constexpr char msg[] = "Connected!";
    constexpr int msgWidth = (sizeof(msg) - 1) * 6;
    constexpr int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(msgX, textY);
    display.print(msg);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..18   Empty space
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=19      Badge top (96x17 rounded rect)
//  │  █     CUP REMOVED      █  │  y=24      Inverted text (centered)
//  │  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀  │  y=35      Badge bottom
//  │                            │  y=36..43  Gap (8px)
//  │    Dispensing stopped!     │  y=44      Footer text
//  │                            │  y=52..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showCupRemovedWarningScreen()
{
    if (!ready)
        return;

    display.clearDisplay();

    constexpr int badgeW = 96;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 8;
    constexpr int totalHeight = badgeH + gap + textHeight;

    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    constexpr char msg[] = "CUP REMOVED";
    constexpr int msgWidth = (sizeof(msg) - 1) * 6;
    constexpr int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    constexpr char footerText[] = "Dispensing stopped!";
    constexpr int footerWidth = (sizeof(footerText) - 1) * 6;
    constexpr int footerX = (SCREEN_WIDTH - footerWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(footerX, textY);
    display.print(footerText);

    display.display();
}

//  ┌────────────────────────────┐
//  │                            │  y=0..15   Empty space
//  │  ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄  │  y=16      Badge top (120x17 rounded rect)
//  │  █   PLACE CUP TO BREW  █  │  y=21      Inverted text (centered)
//  │  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀  │  y=32      Badge bottom
//  │                            │  y=33..38  Gap (6px)
//  │     Waiting for cup...     │  y=39      Footer text
//  │                            │  y=47..63  Empty space
//  └────────────────────────────┘
void DisplayManager::showPlaceCupScreen()
{
    if (!ready)
        return;

    display.clearDisplay();

    constexpr int badgeW = 120;
    constexpr int badgeH = 17;
    constexpr int textHeight = 8;
    constexpr int gap = 6;
    constexpr int totalHeight = badgeH + gap + textHeight;

    constexpr int badgeY = (SCREEN_HEIGHT - totalHeight) / 2;
    constexpr int textY = badgeY + badgeH + gap;
    constexpr int badgeX = (SCREEN_WIDTH - badgeW) / 2;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    constexpr char msg[] = "PLACE CUP TO BREW";
    constexpr int msgWidth = (sizeof(msg) - 1) * 6;
    constexpr int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    constexpr char footer[] = "Waiting for cup...";
    constexpr int footerW2 = (sizeof(footer) - 1) * 6;
    constexpr int footerX2 = (SCREEN_WIDTH - footerW2) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(footerX2, textY);
    display.print(footer);

    display.display();
}