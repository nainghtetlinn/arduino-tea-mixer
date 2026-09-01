#include "TimeDispensingStrategy.h"
#include "RelayManager.h"
#include "SensorManager.h"

namespace
{
    // Flow rates in ml/s for each pump (relay index 0-3)
    constexpr float FLOW_RATE_TEA_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_CONDENSED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_MILK_ML_PER_SEC = 5.0f;
    constexpr unsigned long RELAY_SWITCH_DELAY_MS = 500; // gap before activating next relay
} // anonymous namespace

// =========================================================================
// Helpers
// =========================================================================

float TimeDispensingStrategy::getFlowRateForIndex(uint8_t index) const
{
    switch (index)
    {
    case 0:
        return FLOW_RATE_TEA_ML_PER_SEC;
    case 1:
        return FLOW_RATE_CONDENSED_MILK_ML_PER_SEC;
    case 2:
        return FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC;
    case 3:
        return FLOW_RATE_MILK_ML_PER_SEC;
    default:
        return 1.0f;
    }
}

void TimeDispensingStrategy::activateRelay(uint8_t index, unsigned long now)
{
    relayStartTime[index] = now;
    relayRunning[index] = true;
    if (relayMgr)
    {
        relayMgr->setRelay(index, true);
    }
    DEBUG_PRINTF("[TimeDispensing] Relay %d ON for %lu ms\n", index, relayDuration[index]);
}

// =========================================================================
// start()
// =========================================================================
// Calculates durations, sorts relays longest-first, and activates the
// first two relays immediately.

void TimeDispensingStrategy::start(const TeaRecipe &recipe)
{
    target = recipe;

    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};

    // --- Compute durations; initialise state ---
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayRunning[i] = false;
        if (targets[i] > 0)
        {
            float flowRate = getFlowRateForIndex(i);
            relayDuration[i] = (unsigned long)((targets[i] / flowRate) * 1000.0f);
            relayStartTime[i] = 0;
        }
        else
        {
            relayDuration[i] = 0;
            relayStartTime[i] = 0;
        }
    }

    // --- Build sorted order (insertion sort, longest duration first) ---
    sortedCount = 0;
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (relayDuration[i] > 0)
        {
            sortedOrder[sortedCount] = i;
            sortedCount++;
        }
    }

    // Sort descending by relayDuration
    for (uint8_t i = 1; i < sortedCount; i++)
    {
        for (uint8_t j = i; j > 0 && relayDuration[sortedOrder[j]] > relayDuration[sortedOrder[j - 1]]; j--)
        {
            uint8_t tmp = sortedOrder[j];
            sortedOrder[j] = sortedOrder[j - 1];
            sortedOrder[j - 1] = tmp;
        }
    }

    // --- Activate first two relays (no delay for the initial pair) ---
    nextToStart = 0;
    pendingCount = 0;
    pendingActivateTime = 0;
    pendingRelayIdx[0] = 0;
    pendingRelayIdx[1] = 0;
    unsigned long now = millis();

    uint8_t launched = 0;
    while (nextToStart < sortedCount && launched < 2)
    {
        activateRelay(sortedOrder[nextToStart], now);
        nextToStart++;
        launched++;
    }
}

// =========================================================================
// update()
// =========================================================================
// Returns true once every relay has finished.

bool TimeDispensingStrategy::update(TeaRecipe &dispensed)
{
    unsigned long now = millis();
    bool anyActive = false;

    // ===== Fire all deferred relays in the batch after 500 ms gap =====
    if (pendingCount > 0 && now >= pendingActivateTime)
    {
        for (uint8_t p = 0; p < pendingCount; p++)
        {
            activateRelay(pendingRelayIdx[p], now);
        }
        pendingCount = 0;
    }

    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (!relayRunning[i])
            continue;

        unsigned long elapsed = now - relayStartTime[i];

        if (elapsed >= relayDuration[i])
        {
            // --- Relay finished ---
            if (relayMgr)
            {
                relayMgr->setRelay(i, false);
            }
            relayRunning[i] = false;

            // Record exact dispensed amount
            switch (i)
            {
            case 0:
                dispensed.tea = targets[0];
                break;
            case 1:
                dispensed.condensedMilk = targets[1];
                break;
            case 2:
                dispensed.evaporatedMilk = targets[2];
                break;
            case 3:
                dispensed.milk = targets[3];
                break;
            }

            DEBUG_PRINTF("[TimeDispensing] Relay %d OFF (completed)\n", i);

            // --- Enqueue next relay into the 500 ms batch (up to 2 slots) ---
            if (nextToStart < sortedCount && pendingCount < 2)
            {
                if (pendingCount == 0)
                {
                    // First entry: set the shared fire timestamp
                    pendingActivateTime = now + RELAY_SWITCH_DELAY_MS;
                }
                // Add to the batch (same fire time regardless of order)
                pendingRelayIdx[pendingCount] = sortedOrder[nextToStart];
                pendingCount++;
                nextToStart++;
                DEBUG_PRINTF("[TimeDispensing] Relay %d queued (batch %d/2), fires in 500 ms\n",
                             pendingRelayIdx[pendingCount - 1], pendingCount);
            }
        }
        else
        {
            anyActive = true;

            // Proportional progress estimate
            float fraction = (float)elapsed / (float)relayDuration[i];
            int dispensedMl = (int)(targets[i] * fraction);
            switch (i)
            {
            case 0:
                dispensed.tea = dispensedMl;
                break;
            case 1:
                dispensed.condensedMilk = dispensedMl;
                break;
            case 2:
                dispensed.evaporatedMilk = dispensedMl;
                break;
            case 3:
                dispensed.milk = dispensedMl;
                break;
            }
        }
    }

    // Also check whether any pending relay will still be activated
    if (nextToStart < sortedCount || pendingCount > 0)
    {
        anyActive = true;
    }

    return !anyActive;
}

// =========================================================================
// stop()
// =========================================================================

void TimeDispensingStrategy::stop()
{
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayRunning[i] = false;
    }
    nextToStart = sortedCount; // drain the queue
    pendingCount = 0;          // cancel any pending activation
}
