#include "CleaningMode.h"
#include "Config.h"
#include "RelayManager.h"
#include "DisplayManager.h"
#include "BluetoothManager.h"

namespace
{
    constexpr unsigned long PHASE_DURATION_MS = 30000;                 // 30 s per phase
    constexpr unsigned long TOTAL_DURATION_MS = PHASE_DURATION_MS * 2; // 60 s total
    constexpr unsigned long COMPLETE_DISPLAY_MS = 3000;
    constexpr unsigned long RELAY_SWITCH_DELAY_MS = 500;               // gap between phases
} // anonymous namespace

// ===== Lifecycle =====

void CleaningMode::begin(RelayManager *relay, DisplayManager *display, BluetoothManager *ble)
{
    relayMgr = relay;
    displayMgr = display;
    bleMgr = ble;
}

// ===== Start =====

void CleaningMode::start()
{
    unsigned long now = millis();
    cleaningStartTime = now;
    phaseStartTime = now;
    lastProgressUpdate = now;
    isDoneDelay = false;
    isSwitching = false;
    cleaningPhase = 0;

    // Phase 0: enable relays 0 and 1 only
    if (relayMgr)
    {
        relayMgr->setRelay(0, true);
        relayMgr->setRelay(1, true);
        relayMgr->setRelay(2, false);
        relayMgr->setRelay(3, false);
    }

    DEBUG_PRINTLN(F("[CleaningMode] Started phase 0 (relays 0+1)."));

    if (bleMgr)
    {
        JsonDocument doc;
        doc["state"] = "CLEANING";
        doc["durationSeconds"] = TOTAL_DURATION_MS / 1000;
        bleMgr->sendJSON(BleResponse::CLEANING_START, doc);
    }

    if (displayMgr)
    {
        displayMgr->showCleaningProgressScreen(TOTAL_DURATION_MS / 1000, 0);
    }
}

// ===== Update (call every loop tick) =====

bool CleaningMode::update()
{
    // --- Waiting for the "complete" banner to expire ---
    if (isDoneDelay)
    {
        if (millis() - cleaningDoneTime >= COMPLETE_DISPLAY_MS)
        {
            return true;
        }
        return false;
    }

    unsigned long now = millis();
    unsigned long totalElapsed = now - cleaningStartTime;
    unsigned long phaseElapsed = now - phaseStartTime;

    // ===== Phase transition: 0 → switching gap → 1 =====
    if (cleaningPhase == 0 && !isSwitching && phaseElapsed >= PHASE_DURATION_MS)
    {
        // Turn off phase-0 relays and begin the switch gap
        if (relayMgr)
        {
            relayMgr->setRelay(0, false);
            relayMgr->setRelay(1, false);
        }
        isSwitching = true;
        switchStartTime = now;
        DEBUG_PRINTLN(F("[CleaningMode] Phase 0 done — 500 ms switch gap."));
    }

    if (isSwitching && (now - switchStartTime >= RELAY_SWITCH_DELAY_MS))
    {
        // Gap elapsed — start phase 1
        if (relayMgr)
        {
            relayMgr->setRelay(2, true);
            relayMgr->setRelay(3, true);
        }
        isSwitching = false;
        cleaningPhase = 1;
        phaseStartTime = now;
        DEBUG_PRINTLN(F("[CleaningMode] Phase 1 started (relays 2+3)."));
    }

    // ===== Both phases complete =====
    if (totalElapsed >= TOTAL_DURATION_MS)
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
        return false; // wait for banner delay
    }

    // ===== Progress reporting =====
    uint8_t remainingSeconds = (TOTAL_DURATION_MS - totalElapsed + 999) / 1000;
    uint8_t progress = (uint8_t)((totalElapsed * 100UL) / TOTAL_DURATION_MS);

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

// ===== Cancel =====

void CleaningMode::cancel()
{
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    DEBUG_PRINTLN(F("[CleaningMode] Cancelled by user."));

    if (bleMgr)
    {
        JsonDocument doc;
        doc["status"] = "CANCELLED";
        bleMgr->sendJSON(BleResponse::CLEANING_CANCELLED, doc);
    }
}
