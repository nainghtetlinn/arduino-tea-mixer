#include "TimeDispensingStrategy.h"

namespace
{
    // Flow rates in ml/s for each pump (relay index 0-3)
    constexpr float FLOW_RATE_TEA_ML_PER_SEC = 1.7f;
    constexpr float FLOW_RATE_CONDENSED_MILK_ML_PER_SEC = 1.3f;
    constexpr float FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC = 1.6f;
    constexpr float FLOW_RATE_MILK_ML_PER_SEC = 1.6f;
    constexpr unsigned long RELAY_SWITCH_DELAY_MS = 500; // gap before activating next relay

    // Priming: confirm liquid flow before starting timers
    constexpr float PRIMING_WEIGHT_THRESHOLD_G = 1.0f;
    constexpr unsigned long PRIMING_TIMEOUT_MS = 30000;
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

void TimeDispensingStrategy::setDispensedForRelay(uint8_t index, int ml,
                                                  TeaRecipe &dispensed) const
{
    switch (index)
    {
    case 0:
        dispensed.tea = ml;
        break;
    case 1:
        dispensed.condensedMilk = ml;
        break;
    case 2:
        dispensed.evaporatedMilk = ml;
        break;
    case 3:
        dispensed.milk = ml;
        break;
    }
}

// =========================================================================
// start()
// =========================================================================
// Calculates durations, sorts relays longest-first, then enters the
// PRIMING phase — each relay is tested for flow (weight increase)
// before any timed countdown begins.

void TimeDispensingStrategy::start(const TeaRecipe &recipe)
{
    target = recipe;

    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};

    // --- Compute durations; initialise state ---
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayRunning[i] = false;
        primedWeight[i] = 0.0f;
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

    // --- Enter priming phase (or skip if nothing to dispense) ---
    if (sortedCount == 0)
    {
        phase = Phase::DISPENSING;
        return;
    }

    primingIndex = 0;
    phase = Phase::PRIMING;
    startPrimingRelay();
}

// Activates the relay at sortedOrder[primingIndex] and records
// the weight baseline to detect when liquid starts flowing.
void TimeDispensingStrategy::startPrimingRelay()
{
    uint8_t idx = sortedOrder[primingIndex];
    primingStartWeight = sensorMgr ? sensorMgr->getWeight() : 0.0f;
    primingStartTime = millis();

    if (relayMgr)
    {
        relayMgr->setRelay(idx, true);
    }
    DEBUG_PRINTF("[TimeDispensing] Priming relay %d (%d/%d)\n",
                 idx, primingIndex + 1, sortedCount);
}

// Transitions from PRIMING to DISPENSING and launches the first
// two relays with fresh timer timestamps.
void TimeDispensingStrategy::beginDispensing()
{
    phase = Phase::DISPENSING;
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
    DEBUG_PRINTLN(F("[TimeDispensing] All relays primed — dispensing started"));
}

// =========================================================================
// update()
// =========================================================================
// Returns true once every relay has finished.

bool TimeDispensingStrategy::update(TeaRecipe &dispensed)
{
    unsigned long now = millis();

    // ===== Priming phase: wait for weight confirmation on each relay =====
    if (phase == Phase::PRIMING)
    {
        if (now - primingStartTime >= PRIMING_TIMEOUT_MS)
        {
            DEBUG_PRINTF("[TimeDispensing] Priming TIMEOUT on relay %d\n",
                         sortedOrder[primingIndex]);
            stop();
            return true;
        }

        if (sensorMgr)
        {
            float weight = sensorMgr->getWeight();
            if (weight - primingStartWeight >= PRIMING_WEIGHT_THRESHOLD_G)
            {
                uint8_t idx = sortedOrder[primingIndex];
                float primedGrams = weight - primingStartWeight;
                primedWeight[idx] = primedGrams;

                // Priming has already dispensed this amount. Reduce the
                // timed run so it is not dispensed a second time.
                unsigned long primingDuration = (unsigned long)(
                    (primedGrams / getFlowRateForIndex(idx)) * 1000.0f);
                if (primingDuration >= relayDuration[idx])
                {
                    relayDuration[idx] = 0;
                }
                else
                {
                    relayDuration[idx] -= primingDuration;
                }

                if (relayMgr)
                {
                    relayMgr->setRelay(idx, false);
                }
                DEBUG_PRINTF("[TimeDispensing] Priming relay %d confirmed (%.2fg, %lu ms deducted)\n",
                             idx, primedWeight[idx], primingDuration);

                primingIndex++;
                if (primingIndex < sortedCount)
                {
                    startPrimingRelay();
                }
                else
                {
                    beginDispensing();
                }
            }
        }
        return false;
    }

    // ===== Dispensing phase: timed relay countdown =====
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

            setDispensedForRelay(i, targets[i], dispensed);
            DEBUG_PRINTF("[TimeDispensing] Relay %d OFF (completed)\n", i);

            // --- Enqueue next relay into the 500 ms batch (up to 2 slots) ---
            if (nextToStart < sortedCount && pendingCount < 2)
            {
                if (pendingCount == 0)
                {
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

            float fraction = (float)elapsed / (float)relayDuration[i];
            int dispensedMl = (int)(targets[i] * fraction);
            setDispensedForRelay(i, dispensedMl, dispensed);
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
    phase = Phase::DISPENSING; // exit priming if active
}
