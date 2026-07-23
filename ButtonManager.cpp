#include "ButtonManager.h"
#include "Config.h"
#include <Preferences.h>

namespace
{
    // --- Constants ---
    constexpr uint8_t NUM_BUTTONS = 3;

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
    : btnPressCb(nullptr),
      btnSendJsonCb(nullptr)
{
    buttons[0] = OneButton(PIN_BTN_1, true, false);
    buttons[1] = OneButton(PIN_BTN_2, true, false);
    buttons[2] = OneButton(PIN_BTN_3, true, false);
}

void ButtonManager::begin(ButtonPressCallback pressCb, ButtonSendJsonCallback sendCb)
{
    btnPressCb = pressCb;
    btnSendJsonCb = sendCb;

    buttons[0]
        .attachClick(
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

void ButtonManager::handleGetButtonsInfoCommand()
{
    DEBUG_PRINTLN(F("[ButtonManager] Handle: GET_BUTTONS_INFO"));
    sendButtonsInfoJSON();
}

void ButtonManager::handleSetButtonInfoCommand(const JsonDocument &doc)
{
    DEBUG_PRINTLN(F("[ButtonManager] Handle: SET_BUTTON_INFO"));

    JsonObjectConst payload = doc["payload"];
    if (payload.isNull())
    {
        DEBUG_PRINTLN(F("[ButtonManager] Error: Invalid JSON payload"));
        return;
    }

    int id = payload["id"] | -1;
    if (id < 0 || id >= NUM_BUTTONS)
    {
        DEBUG_PRINTF("[ButtonManager] Error: Invalid button ID %d\n", id);
        return;
    }

    ButtonInfo info;
    info.name = payload["name"] | defaultNames[id];
    info.recipe.tea = payload["tea"] | 0;
    info.recipe.condensedMilk = payload["condensedMilk"] | 0;
    info.recipe.evaporatedMilk = payload["evaporatedMilk"] | 0;
    info.recipe.milk = payload["milk"] | 0;

    setButtonInfo(id, info);
    sendButtonsInfoJSON();
}

void ButtonManager::makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field)
{
    snprintf(buffer, bufferSize, "b%d_%s", index, field);
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

    return info;
}

void ButtonManager::setButtonInfo(int index, ButtonInfo info)
{
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

    DEBUG_PRINTF("[ButtonManager] Button %d recipe updated & saved.\n", index + 1);
}

void ButtonManager::handleButtonClick(int btnIndex)
{
    ButtonInfo info = getButtonInfo(btnIndex);

    DEBUG_PRINTF(
        "[ButtonManager] Button %d pressed: name=%s, tea=%d, cond=%d, evap=%d, milk=%d\n",
        btnIndex + 1,
        info.name.c_str(),
        info.recipe.tea,
        info.recipe.condensedMilk,
        info.recipe.evaporatedMilk,
        info.recipe.milk);

    if (btnPressCb)
    {
        btnPressCb(info.recipe);
    }
}

void ButtonManager::sendButtonsInfoJSON()
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

    if (btnSendJsonCb)
    {
        btnSendJsonCb("BUTTONS_INFO", doc);
    }
}