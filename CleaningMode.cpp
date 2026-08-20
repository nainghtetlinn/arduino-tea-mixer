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

void CleaningMode::begin(RelayManager *relay, DisplayManager *display, BluetoothManager *ble)
{
    relayMgr = relay;
    displayMgr = display;
    bleMgr = ble;
}

void CleaningMode::start()
{
    cleaningStartTime = millis();
    lastProgressUpdate = cleaningStartTime;
    isDoneDelay = false;

    if (relayMgr)
    {
        for (uint8_t i = 0; i < RELAY_COUNT; i++)
        {
            relayMgr->setRelay(i, true);
        }
    }

    DEBUG_PRINTLN(F("[CleaningMode] Started."));

    if (bleMgr)
    {
        JsonDocument doc;
        doc["state"] = "CLEANING";
        doc["durationSeconds"] = CLEANING_DURATION_MS / 1000;
        bleMgr->sendJSON(BleResponse::CLEANING_START, doc);
    }

    if (displayMgr)
    {
        displayMgr->showCleaningProgressScreen(20, 0);
    }
}

bool CleaningMode::update()
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
        if (relayMgr)
        {
            relayMgr->stopAll();
        }

        DEBUG_PRINTLN(F("[CleaningMode] Finished successfully."));

        if (bleMgr)
        {
            JsonDocument doc;
            doc["status"] = "COMPLETE";
            bleMgr->sendJSON(BleResponse::CLEANING_FINISHED, doc);
        }

        if (displayMgr)
        {
            displayMgr->showCleaningCompleteScreen();
        }

        isDoneDelay = true;
        cleaningDoneTime = millis();
        return false; // wait for delay
    }

    uint8_t remainingSeconds = (CLEANING_DURATION_MS - elapsed + 999) / 1000;
    uint8_t progress = (elapsed * 100) / CLEANING_DURATION_MS;

    if (displayMgr)
    {
        displayMgr->showCleaningProgressScreen(remainingSeconds, progress);
    }

    if (bleMgr && (now - lastProgressUpdate >= 1000))
    {
        lastProgressUpdate = now;
        JsonDocument doc;
        doc["remainingSeconds"] = remainingSeconds;
        doc["progress"] = progress;
        bleMgr->sendJSON(BleResponse::CLEANING_PROGRESS, doc);
    }

    return false; // still running
}

void CleaningMode::cancel()
{
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    DEBUG_PRINTLN(F("[CleaningMode] Cancelled by user"));

    if (bleMgr)
    {
        JsonDocument doc;
        doc["status"] = "CANCELLED";
        bleMgr->sendJSON(BleResponse::CLEANING_CANCELLED, doc);
    }
}
