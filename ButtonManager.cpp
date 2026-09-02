#include "ButtonManager.h"

namespace
{
    // --- Pins ---
    constexpr uint8_t PIN_BTN_1 = 27;
    constexpr uint8_t PIN_BTN_2 = 26;
    constexpr uint8_t PIN_BTN_3 = 25;

    // Default recipes in case none are saved in NVS
    const char *defaultNames[BUTTON_COUNT] = {
        "Pon Hman",
        "Cho Saint",
        "Kya Saint"};

    const TeaRecipe defaultRecipes[BUTTON_COUNT] = {
        {240, 30, 30, 0}, // Pon Hman
        {240, 45, 30, 0}, // Cho Saint
        {240, 15, 15, 0}  // Kya Saint
    };
} // anonymous namespace

ButtonManager::ButtonManager()
    : bleMgr(nullptr),
      btnPressCb(nullptr),
      combo13Cb(nullptr),
      combo12Cb(nullptr),
      combo23Cb(nullptr),
      combo13HoldStartTime(0),
      combo13Triggered(false),
      combo12HoldStartTime(0),
      combo12Triggered(false),
      combo23HoldStartTime(0),
      combo23Triggered(false)
{
    suppressClick[0] = false;
    suppressClick[1] = false;
    suppressClick[2] = false;
    buttons[0] = OneButton(PIN_BTN_1, true, false);
    buttons[1] = OneButton(PIN_BTN_2, true, false);
    buttons[2] = OneButton(PIN_BTN_3, true, false);
}

void ButtonManager::begin(BluetoothManager *ble)
{
    bleMgr = ble;
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

void ButtonManager::attachPressCallback(ButtonPressCallback pressCb)
{
    btnPressCb = pressCb;
}

void ButtonManager::attachCleaningComboCallback(ButtonComboCallback comboCallback)
{
    combo13Cb = comboCallback;
}

void ButtonManager::attachCalibrationComboCallback(ButtonComboCallback comboCallback)
{
    combo12Cb = comboCallback;
}

void ButtonManager::attachTargetMlComboCallback(ButtonComboCallback comboCallback)
{
    combo23Cb = comboCallback;
}

void ButtonManager::update()
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        buttons[i].tick();
    }

    bool b1Pressed = (digitalRead(PIN_BTN_1) == LOW);
    bool b2Pressed = (digitalRead(PIN_BTN_2) == LOW);
    bool b3Pressed = (digitalRead(PIN_BTN_3) == LOW);

    // Combo: Btn 1 + Btn 2  (Calibration, 3 s hold)
    updateCombo(b1Pressed, b2Pressed, b3Pressed, 0, 1,
                combo12HoldStartTime, combo12Triggered, combo12Cb,
                !b3Pressed, true);

    // Combo: Btn 1 + Btn 3  (Cleaning, 3 s hold)
    updateCombo(b1Pressed, b3Pressed, b2Pressed, 0, 2,
                combo13HoldStartTime, combo13Triggered, combo13Cb,
                !b2Pressed, true);

    // Combo: Btn 2 + Btn 3  (Target ML, 3 s hold)
    updateCombo(b2Pressed, b3Pressed, b1Pressed, 1, 2,
                combo23HoldStartTime, combo23Triggered, combo23Cb,
                !b1Pressed, !b1Pressed);
}

// Generic combo helper.
// aPinHeld / bPinHeld  — the two buttons that must both be held.
// thirdPinHeld         — must be false for the combo to arm.
// suppressA/B          — suppressClick[] indices to set on trigger.
// holdStart / triggered— per-combo state variables (passed by ref).
// cb                   — callback fired once per hold event.
// canClearA/B          — extra conditions under which suppress is released.
void ButtonManager::updateCombo(bool aPinHeld, bool bPinHeld, bool thirdPinHeld,
                                int suppressA, int suppressB,
                                unsigned long &holdStart, bool &triggered,
                                ButtonComboCallback cb,
                                bool canClearA, bool canClearB)
{
    if (aPinHeld && bPinHeld && !thirdPinHeld)
    {
        if (holdStart == 0)
        {
            holdStart = millis();
        }
        else if (millis() - holdStart >= 3000 && !triggered)
        {
            triggered = true;
            suppressClick[suppressA] = true;
            suppressClick[suppressB] = true;
            if (cb)
            {
                cb();
            }
        }
    }
    else
    {
        holdStart = 0;
        triggered = false;
        if (canClearA)
            suppressClick[suppressA] = false;
        if (canClearB)
            suppressClick[suppressB] = false;
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
    prefs.begin("tea_settings", false);

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
    prefs.begin("tea_settings", true);

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

void ButtonManager::makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field)
{
    snprintf(buffer, bufferSize, "b%d_%s", index, field);
}

void ButtonManager::handleButtonClick(int index)
{
    if (index >= 0 && index < BUTTON_COUNT && suppressClick[index])
    {
        suppressClick[index] = false;
        DEBUG_PRINTF("[ButtonManager] Button %d click suppressed due to combo hold release.\n", index + 1);
        return;
    }

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
    prefs.begin("tea_settings", true);
    makeNvsKey(key, sizeof(key), index, "name");
    String name = prefs.getString(key, defaultNames[index]);
    prefs.end();
    return name;
}

// ===== JSON Reporting =====

void ButtonManager::sendButtonsInfoJSON()
{
    if (bleMgr)
    {
        JsonDocument doc;

        for (int i = 0; i < BUTTON_COUNT; i++)
        {
            ButtonInfo info = getButtonInfo(i);

            char btnKey[8];
            snprintf(btnKey, sizeof(btnKey), "btn%d", i);

            JsonObject obj = doc[btnKey].to<JsonObject>();
            info.toJson(obj);
        }

        bleMgr->sendJSON(BleResponse::BUTTONS_INFO, doc);
    }
}