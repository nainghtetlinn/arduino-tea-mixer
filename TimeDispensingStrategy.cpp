#include "TimeDispensingStrategy.h"
#include "RelayManager.h"
#include "SensorManager.h"

namespace
{
    // Time-based mode: flow rate in ml/s for each pump
    constexpr float FLOW_RATE_TEA_ML_PER_SEC = 5.0f; // ~300ml/min
    constexpr float FLOW_RATE_CONDENSED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_MILK_ML_PER_SEC = 5.0f;

} // anonymous namespace

void TimeDispensingStrategy::start(const TeaRecipe &recipe)
{
    target = recipe;

    // Calculate duration for each relay based on flow rate
    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};
    unsigned long now = millis();

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (targets[i] > 0)
        {
            float flowRate = getFlowRateForIndex(i);
            relayDuration[i] = (unsigned long)((targets[i] / flowRate) * 1000.0f);
            relayStartTime[i] = now;
            relayRunning[i] = true;
            if (relayMgr)
            {
                relayMgr->setRelay(i, true);
            }

            DEBUG_PRINTF("[TimeDispensing] Relay %d ON for %lu ms (%d ml)\n",
                         i, relayDuration[i], targets[i]);
        }
        else
        {
            relayDuration[i] = 0;
            relayStartTime[i] = 0;
            relayRunning[i] = false;
        }
    }
}

bool TimeDispensingStrategy::update(TeaRecipe &dispensed)
{
    unsigned long now = millis();
    bool anyActive = false;

    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (!relayRunning[i])
            continue;

        unsigned long elapsed = now - relayStartTime[i];

        if (elapsed >= relayDuration[i])
        {
            // This relay is done
            if (relayMgr)
            {
                relayMgr->setRelay(i, false);
            }
            relayRunning[i] = false;

            // Set dispensed to exact target
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
        }
        else
        {
            anyActive = true;

            // Calculate proportional dispensed amount for progress
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

    return !anyActive;
}

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
}

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
