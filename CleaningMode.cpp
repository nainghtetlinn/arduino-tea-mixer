#include "CleaningMode.h"
#include "Config.h"
#include "RelayManager.h"
#include "DisplayManager.h"
#include "BluetoothManager.h"

namespace
{
    constexpr unsigned long CLEANING_DURATION_MS = 20000;
    constexpr unsigned long COMPLETE_DISPLAY_MS = 3000;
} // anonymous namespace

// =========================================================================
// Public Interface
// =========================================================================

void CleaningMode::start(RelayManager *relay, DisplayManager *display, BluetoothManager *ble)
{
    cleaningStartTime = millis();
    lastProgressUpdate = cleaningStartTime;
    isDoneDelay = false;

    if (relay)
    {
        for (uint8_t i = 0; i < RELAY_COUNT; i++)
        {
            relay->setRelay(i, true);
        }
    }

    DEBUG_PRINTLN(F("[CleaningMode] Started."));

    if (ble)
    {
        JsonDocument doc;
        doc["state"] = "CLEANING";
        doc["durationSeconds"] = CLEANING_DURATION_MS / 1000;
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
        if (millis() - cleaningDoneTime >= COMPLETE_DISPLAY_MS)
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

        DEBUG_PRINTLN(F("[CleaningMode] Finished successfully."));

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

    if (ble && (now - lastProgressUpdate >= 1000))
    {
        lastProgressUpdate = now;
        JsonDocument doc;
        doc["remainingSeconds"] = remainingSeconds;
        doc["progress"] = progress;
        ble->sendJSON("CLEANING_PROGRESS", doc);
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
