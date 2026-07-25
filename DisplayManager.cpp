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
//  │    ☕ Tea Mixer            │  y=2..10   Icon + Title (size 1)
//  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤  y=14      Dotted divider
//  │  ▸ Pon Hman               │  y=21
//  │  ▸ Cho Saint              │  y=35
//  │  ▸ Kya Saint              │  y=49
//  └────────────────────────────┘
//
void DisplayManager::showIdle(const String &btn1, const String &btn2, const String &btn3)
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Header: Tea Icon + "Tea Mixer" centered ---
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
//  │     ▸ Pon Hman             │  y=0..15   Name (size 1, centered)
//  ├────────────────────────────┤  y=16      Divider
//  │  Tea           240ml      │  y=19
//  │  Condensed      30ml      │  y=28
//  │  Evaporated     30ml      │  y=37
//  │  Milk            0ml      │  y=46
//  ├────────────────────────────┤  y=54      Divider
//  │   Press again to brew ☕     │  y=56      Footer prompt
//  └────────────────────────────┘
//
void DisplayManager::showRecipePreview(const String &name, const TeaRecipe &recipe)
{
    if (!displayReady)
        return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Header: Recipe name centered ---
    // Draw a small triangle bullet before the name
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

    // "Press to brew" centered with tea icon
    const char *footerText = "Press again to brew";
    int footerLen = strlen(footerText);
    int totalFooterW = footerLen * 6 + 10; // text + space + icon
    int footerX = (SCREEN_WIDTH - totalFooterW) / 2;

    display.setCursor(footerX, 56);
    display.print(footerText);

    // Small tea cup icon next to text
    display.drawBitmap(footerX + footerLen * 6 + 2, 56, smallTeaIcon, 8, 8, SSD1306_WHITE);

    display.display();
}

void DisplayManager::showReady()
{
    if (!displayReady)
        return;

    display.clearDisplay();

    // 1. Animated steaming tea cup icon at top center
    constexpr int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
    constexpr int iconY = 2;

    const bool useFrame1 = ((millis() / ANIMATION_SPEED_MS) % 2 == 0);
    const unsigned char *icon = useFrame1 ? teaIconFrame1 : teaIconFrame2;

    display.drawBitmap(iconX, iconY, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    // 2. Decorative sparkle accents on sides
    // Left sparkle
    display.drawFastHLine(18, 13, 5, SSD1306_WHITE);
    display.drawFastVLine(20, 11, 5, SSD1306_WHITE);

    // Right sparkle
    display.drawFastHLine(102, 13, 5, SSD1306_WHITE);
    display.drawFastVLine(104, 11, 5, SSD1306_WHITE);

    // 3. Highlighted badge: "TEA IS READY!"
    constexpr int badgeX = 8;
    constexpr int badgeY = 28;
    constexpr int badgeW = 112;
    constexpr int badgeH = 17;

    display.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, SSD1306_WHITE);

    // Inverted black text inside white badge
    const char msg[] = "TEA IS READY!";
    int msgWidth = strlen(msg) * 6;
    int msgX = (SCREEN_WIDTH - msgWidth) / 2;

    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(msgX, badgeY + 5);
    display.print(msg);

    // 4. Footer subtext
    const char footer[] = "Enjoy your drink!";
    int footerWidth = strlen(footer) * 6;
    int footerX = (SCREEN_WIDTH - footerWidth) / 2;

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(footerX, 51);
    display.print(footer);

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
