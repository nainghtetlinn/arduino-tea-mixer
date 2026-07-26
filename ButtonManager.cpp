#include "ButtonManager.h"
#include "Config.h"
#include <Preferences.h>

namespace
{
    // --- Constants ---
    constexpr uint8_t NUM_BUTTONS = 3;

    // --- Pins ---
    constexpr uint8_t PIN_BTN_1 = 27;
    constexpr uint8_t PIN_BTN_2 = 26;
    constexpr uint8_t PIN_BTN_3 = 25;

    // NVS
    constexpr char PREFS_NAMESPACE_TEA[] = "tea_settings";

    // Default recipes in case none are saved in NVS
    const char *defaultNames[NUM_BUTTONS] = {
        "Pon Hman",
        "Cho Saint",
        "Kya Saint"};

    const TeaRecipe defaultRecipes[NUM_BUTTONS] = {
        {240, 30, 30, 0}, // Pon Hman
        {240, 45, 30, 0}, // Cho Saint
        {240, 15, 15, 0}  // Kya Saint
    };
} // anonymous namespace

ButtonManager::ButtonManager()
    : btnPressCb(nullptr)
{
    buttons[0] = OneButton(PIN_BTN_1, true, false);
    buttons[1] = OneButton(PIN_BTN_2, true, false);
    buttons[2] = OneButton(PIN_BTN_3, true, false);
}

void ButtonManager::begin(ButtonPressCallback pressCb)
{
    btnPressCb = pressCb;

    buttons[0].attachClick(
        [](void *ctx)
        {
            static_cast<ButtonManager *>(ctx)->handleButtonClick(0);
        },
        this);
    buttons[1].attachClick(
        [](void *ctx)
        {
            static_cast<ButtonManager *>(ctx)->handleButtonClick(1);
        },
        this);
    buttons[2].attachClick(
        [](void *ctx)
        {
            static_cast<ButtonManager *>(ctx)->handleButtonClick(2);
        },
        this);

    DEBUG_PRINTLN(F("[ButtonManager] Initialized successfully."));
}

void ButtonManager::update()
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        buttons[i].tick();
    }
}

void ButtonManager::setButtonInfo(int index, ButtonInfo info)
{
    if (index < 0)
    {
        DEBUG_PRINTF("[ButtonManager] invalid index %d.\n", index);
        return;
    }
    char key[16];

    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE_TEA, false);

    makeNvsKey(key, sizeof(key), index, "name");
    prefs.putString(key, info.name);

    makeNvsKey(key, sizeof(key), index, "tea");
    prefs.putInt(key, info.recipe.tea);

    makeNvsKey(key, sizeof(key), index, "cond");
    prefs.putInt(key, info.recipe.condensedMilk);

    makeNvsKey(key, sizeof(key), index, "evap");
    prefs.putInt(key, info.recipe.evaporatedMilk);

    makeNvsKey(key, sizeof(key), index, "milk");
    prefs.putInt(key, info.recipe.milk);

    prefs.end();

    DEBUG_PRINTF(
        "[ButtonManager] Button %d recipe updated (name=%s, tea=%d, cond=%d, evap=%d, milk=%d).\n",
        index + 1,
        info.name.c_str(),
        info.recipe.tea,
        info.recipe.condensedMilk,
        info.recipe.evaporatedMilk,
        info.recipe.milk);
}

ButtonInfo ButtonManager::getButtonInfo(int index)
{
    ButtonInfo info;
    char key[16];

    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE_TEA, true);

    makeNvsKey(key, sizeof(key), index, "name");
    info.name = prefs.getString(key, defaultNames[index]);

    makeNvsKey(key, sizeof(key), index, "tea");
    info.recipe.tea = prefs.getInt(key, defaultRecipes[index].tea);

    makeNvsKey(key, sizeof(key), index, "cond");
    info.recipe.condensedMilk = prefs.getInt(key, defaultRecipes[index].condensedMilk);

    makeNvsKey(key, sizeof(key), index, "evap");
    info.recipe.evaporatedMilk = prefs.getInt(key, defaultRecipes[index].evaporatedMilk);

    makeNvsKey(key, sizeof(key), index, "milk");
    info.recipe.milk = prefs.getInt(key, defaultRecipes[index].milk);

    prefs.end();

    DEBUG_PRINTF(
        "[ButtonManager] Button %d recipe (name=%s, tea=%d, cond=%d, evap=%d, milk=%d).\n",
        index + 1,
        info.name.c_str(),
        info.recipe.tea,
        info.recipe.condensedMilk,
        info.recipe.evaporatedMilk,
        info.recipe.milk);

    return info;
}

JsonDocument ButtonManager::getButtonsInfoJSON()
{
    JsonDocument doc;

    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        ButtonInfo info = getButtonInfo(i);

        char btnKey[8];
        snprintf(btnKey, sizeof(btnKey), "btn%d", i);

        JsonObject obj;
        info.toJson(obj);

        doc[btnKey] = obj;
    }

    return doc;
}

void ButtonManager::makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field)
{
    snprintf(buffer, bufferSize, "b%d_%s", index, field);
}

void ButtonManager::handleButtonClick(int index)
{
    DEBUG_PRINTF("[ButtonManager] Button %d clicked.\n", index + 1);

    if (btnPressCb)
    {
        btnPressCb(index);
    }
}

String ButtonManager::getButtonName(int index)
{
    char key[16];
    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE_TEA, true);
    makeNvsKey(key, sizeof(key), index, "name");
    String name = prefs.getString(key, defaultNames[index]);
    prefs.end();
    return name;
}
