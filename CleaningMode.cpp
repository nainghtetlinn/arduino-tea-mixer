#include "CleaningMode.h"
#include "Config.h"
#include "RelayManager.h"
#include "DisplayManager.h"
#include "BluetoothManager.h"

namespace
{
    constexpr unsigned long CLEANING_DURATION_MS = 20000;
} // anonymous namespace

// =========================================================================
// Public Interface
// =========================================================================

void CleaningMode::start(RelayManager *relay, DisplayManager *display, BluetoothManager *ble)
{
    cleaningStartTime = millis();
    isDoneDelay = false;

    if (relay)
    {
        for (uint8_t i = 0; i < RELAY_COUNT; i++)
        {
            relay->setRelay(i, true);
        }
    }

    DEBUG_PRINTLN(F("[CleaningMode] Started — 4 pumps ON for 20s"));

    if (ble)
    {
        JsonDocument doc;
        doc["state"] = "CLEANING";
        doc["durationSeconds"] = 20;
        ble->sendJSON("CLEANING_START", doc);
    }

    if (display)
    {
        display->showCleaningProgress(20, 0);
    }
}

bool CleaningMode::update(RelayManager *relay, DisplayManager *display, BluetoothManager *ble)
{
    if (isDoneDelay)
    {
        if (millis() - cleaningDoneTime >= 3000)
        {
            return true;
        }
        return false;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - cleaningStartTime;

    if (elapsed >= CLEANING_DURATION_MS)
    {
        if (relay)
        {
            relay->stopAll();
        }

        DEBUG_PRINTLN(F("[CleaningMode] Finished successfully, waiting 3s"));

        if (ble)
        {
            JsonDocument doc;
            doc["status"] = "COMPLETE";
            ble->sendJSON("CLEANING_FINISHED", doc);
        }

        if (display)
        {
            display->showCleaningComplete();
        }
        
        isDoneDelay = true;
        cleaningDoneTime = millis();
        return false; // wait for delay
    }

    uint8_t remainingSeconds = (CLEANING_DURATION_MS - elapsed + 999) / 1000;
    uint8_t progress = (elapsed * 100) / CLEANING_DURATION_MS;

    if (display)
    {
        display->showCleaningProgress(remainingSeconds, progress);
    }

    return false; // still running
}

void CleaningMode::cancel(RelayManager *relay, BluetoothManager *ble)
{
    if (relay)
    {
        relay->stopAll();
    }

    DEBUG_PRINTLN(F("[CleaningMode] Cancelled by user"));

    if (ble)
    {
        JsonDocument doc;
        doc["status"] = "CANCELLED";
        ble->sendJSON("CLEANING_CANCELLED", doc);
    }
}
