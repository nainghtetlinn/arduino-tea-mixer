#include "TeaManager.h"
#include "BluetoothManager.h"
#include "Config.h"
#include "DisplayManager.h"

#define UPDATE_INTERVAL 1000

static TeaRecipe target = {0, 0, 0, 0};
static TeaRecipe current = {0, 0, 0, 0};

static bool isMaking = false;

static unsigned long lastUpdate = 0;

static void sendTeaStartJSON(TeaRecipe recipe);
static void sendTeaProgressJSON(TeaRecipe target, TeaRecipe current);
static void sendTeaFinishedJSON();
static uint8_t calculateProgress(TeaRecipe target, TeaRecipe current);

// ===========================================================
// PUBLIC API (Exposed in TeaManager.h - NO static)
// ===========================================================

void handleMakeTeaCommand(JsonDocument doc)
{
    DEBUG_PRINTLN("HANDLE: MAKE_TEA");

    if (isMaking)
    {
        sendError("MAKE_TEA_ERROR", "Tea is already being made");
        return;
    }

    JsonObject payload = doc["payload"];
    TeaRecipe recipe;

    recipe.tea = payload["tea"] | 0;
    recipe.condensedMilk = payload["condensedMilk"] | 0;
    recipe.evaporatedMilk = payload["evaporatedMilk"] | 0;
    recipe.milk = payload["milk"] | 0;

    startMakingTea(recipe);
}

void startMakingTea(TeaRecipe recipe)
{
    if (isMaking)
        return;
    target = recipe;
    current = {0, 0, 0, 0};
    isMaking = true;
    lastUpdate = millis();
    displayTeaProgress(0);
    sendTeaStartJSON(recipe);
}

void updateTeaProgress()
{
    if (!isMaking)
        return;

    if (millis() - lastUpdate < UPDATE_INTERVAL)
        return;

    lastUpdate = millis();

    if (current.tea < target.tea)
    {
        current.tea += min(10, target.tea - current.tea);
    }
    else if (current.condensedMilk < target.condensedMilk)
    {
        current.condensedMilk += min(5, target.condensedMilk - current.condensedMilk);
    }
    else if (current.evaporatedMilk < target.evaporatedMilk)
    {
        current.evaporatedMilk += min(5, target.evaporatedMilk - current.evaporatedMilk);
    }
    else if (current.milk < target.milk)
    {
        current.milk += min(5, target.milk - current.milk);
    }
    else
    {
        isMaking = false;
        displayTeaReady();
        sendTeaFinishedJSON();
        return;
    }

    displayTeaProgress(calculateProgress(target, current));
    sendTeaProgressJSON(target, current);
}

// ===========================================================
// PRIVATE HELPERS (Internal to this file - Use static)
// ===========================================================

static void sendTeaStartJSON(TeaRecipe recipe)
{
    JsonDocument doc;
    doc["type"] = "TEA_START";

    JsonObject payload = doc["payload"].to<JsonObject>();

    payload["tea"] = recipe.tea;
    payload["condensedMilk"] = recipe.condensedMilk;
    payload["evaporatedMilk"] = recipe.evaporatedMilk;
    payload["milk"] = recipe.milk;

    // Serialize to string
    String output;
    serializeJson(doc, output);

    // Notify
    sendJSON("TEA_START", output);
}

static void sendTeaProgressJSON(TeaRecipe target, TeaRecipe current)
{
    JsonDocument doc;
    doc["type"] = "TEA_PROGRESS";

    JsonObject payload = doc["payload"].to<JsonObject>();

    payload["tea"] = current.tea;
    payload["condensedMilk"] = current.condensedMilk;
    payload["evaporatedMilk"] = current.evaporatedMilk;
    payload["milk"] = current.milk;

    payload["percentage"] = calculateProgress(target, current);

    // Serialize to string
    String output;
    serializeJson(doc, output);

    // Notify
    sendJSON("TEA_PROGRESS", output);
}

static void sendTeaFinishedJSON()
{
    String output = "{\"type\":\"TEA_FINISH\"}";

    // Notify
    sendJSON("TEA_FINISH", output);
}

static uint8_t calculateProgress(TeaRecipe target, TeaRecipe current)
{
    const int totalTarget = target.tea + target.condensedMilk + target.evaporatedMilk + target.milk;
    const int totalCurrent = current.tea + current.condensedMilk + current.evaporatedMilk + current.milk;

    if (totalTarget <= 0)
        return 0;

    return constrain((totalCurrent * 100) / totalTarget, 0, 100);
}
