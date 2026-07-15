#include <OneButton.h>
#include <Preferences.h>
#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "TeaManager.h"
#include "Config.h"

struct KeyPrefixes
{
    String nameKey;
    String teaKey;
    String condKey;
    String evapKey;
    String milkKey;
};

// Default recipes in case none are saved
static const char *defaultNames[3] = {
    "Pon Hman",
    "Cho Saint",
    "Kya Saint"};
static TeaRecipe defaultRecipes[3] = {
    {240, 30, 30, 0}, // Pon Hman
    {240, 45, 30, 0}, // Cho Saint
    {240, 15, 15, 0}  // Kya Saint
};

// TODO: Replace with the actual GPIO pins your buttons are connected to
#define BTN1_PIN 34
#define BTN2_PIN 35
#define BTN3_PIN 32

// Syntax: OneButton(pin, activeLow, pullupActive)
OneButton btn1(BTN1_PIN, false, false);
OneButton btn2(BTN2_PIN, false, false);
OneButton btn3(BTN3_PIN, false, false);

static Preferences preferences;

static void handleButtonClick(int btnIndex);
static KeyPrefixes getPrefixes(int index);
static ButtonInfo getButtonInfo(int index);
static void setButtonInfo(int index, ButtonInfo info);
static void sendButtonsInfoJSON();

// ===========================================================
// PUBLIC API (Exposed in ButtonManager.h)
// ===========================================================

void initButtons()
{
    btn1.attachClick([]()
                     { handleButtonClick(0); });
    btn2.attachClick([]()
                     { handleButtonClick(1); });
    btn3.attachClick([]()
                     { handleButtonClick(2); });
    preferences.begin("tea_settings", false);
}

void updateButtons()
{
    btn1.tick();
    btn2.tick();
    btn3.tick();
}

void handleGetButtonsInfoCommand()
{
    DEBUG_PRINTLN("HANDLE: GET_BUTTONS_INFO");

    sendButtonsInfoJSON();
}

void handleSetButtonInfoCommand(JsonDocument doc)
{
    DEBUG_PRINTLN("HANDLE: SET_BUTTON_INFO");

    JsonObject payload = doc["payload"];

    int id = payload["id"];
    ButtonInfo info;
    info.name = payload["name"].as<String>();
    info.recipe.tea = payload["tea"] | 0;
    info.recipe.condensedMilk = payload["condensedMilk"] | 0;
    info.recipe.evaporatedMilk = payload["evaporatedMilk"] | 0;
    info.recipe.milk = payload["milk"] | 0;

    setButtonInfo(id, info);
    sendButtonsInfoJSON();
}

// ===========================================================
// PRIVATE HELPERS (Internal to this file - marked static)
// ===========================================================

static void handleButtonClick(int btnIndex)
{
    ButtonInfo info = getButtonInfo(btnIndex);
    DEBUG_PRINTF(
        "Button %d recipe: name=%s, tea=%d, condensed=%d, evaporated=%d, milk=%d\n",
        btnIndex + 1,
        info.name.c_str(),
        info.recipe.tea,
        info.recipe.condensedMilk,
        info.recipe.evaporatedMilk,
        info.recipe.milk);
    startMakingTea(info.recipe);
}

static KeyPrefixes getPrefixes(int index)
{
    char keyPrefix[10];
    sprintf(keyPrefix, "btn%d_", index);

    KeyPrefixes prefixes;

    prefixes.nameKey = String(keyPrefix) + "name";
    prefixes.teaKey = String(keyPrefix) + "tea";
    prefixes.condKey = String(keyPrefix) + "cond";
    prefixes.evapKey = String(keyPrefix) + "evap";
    prefixes.milkKey = String(keyPrefix) + "milk";

    return prefixes;
}

static ButtonInfo getButtonInfo(int index)
{
    KeyPrefixes prefixes = getPrefixes(index);

    ButtonInfo info;

    info.name = preferences.getString(prefixes.nameKey.c_str(), defaultNames[index]);
    info.recipe.tea = preferences.getInt(prefixes.teaKey.c_str(), defaultRecipes[index].tea);
    info.recipe.condensedMilk = preferences.getInt(prefixes.condKey.c_str(), defaultRecipes[index].condensedMilk);
    info.recipe.evaporatedMilk = preferences.getInt(prefixes.evapKey.c_str(), defaultRecipes[index].evaporatedMilk);
    info.recipe.milk = preferences.getInt(prefixes.milkKey.c_str(), defaultRecipes[index].milk);

    return info;
}

static void setButtonInfo(int index, ButtonInfo info)
{
    KeyPrefixes prefixes = getPrefixes(index);

    preferences.putString(prefixes.nameKey.c_str(), info.name);
    preferences.putInt(prefixes.teaKey.c_str(), info.recipe.tea);
    preferences.putInt(prefixes.condKey.c_str(), info.recipe.condensedMilk);
    preferences.putInt(prefixes.evapKey.c_str(), info.recipe.evaporatedMilk);
    preferences.putInt(prefixes.milkKey.c_str(), info.recipe.milk);

    DEBUG_PRINTF("Button %d recipe set\n", index);
}

static void sendButtonsInfoJSON()
{
    JsonDocument doc;
    doc["type"] = "BUTTONS_INFO";

    JsonObject payload = doc["payload"].to<JsonObject>();

    for (int i = 0; i < 3; i++)
    {
        JsonObject btnObj = payload[String("btn") + i].to<JsonObject>();

        ButtonInfo info = getButtonInfo(i);
        btnObj["name"] = info.name;
        btnObj["tea"] = info.recipe.tea;
        btnObj["condensedMilk"] = info.recipe.condensedMilk;
        btnObj["evaporatedMilk"] = info.recipe.evaporatedMilk;
        btnObj["milk"] = info.recipe.milk;
    }

    // Serialize to string
    String output;
    serializeJson(doc, output);

    // Notify
    sendJSON("BUTTONS_INFO", output);
}