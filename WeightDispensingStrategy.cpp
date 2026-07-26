#include "WeightDispensingStrategy.h"
#include "RelayManager.h"
#include "SensorManager.h"

namespace
{
    // Weight tolerance - stop relay when within this margin (grams)
    constexpr float WEIGHT_TOLERANCE_G = 2.0f;
    // Safety timeout - max dispensing time per liquid (ms), 0 = disabled
    constexpr unsigned long DISPENSING_SAFETY_TIMEOUT_MS = 300000; // 5 minutes

    // Weight-based mode: grams per ml conversion for each liquid
    constexpr float GRAMS_PER_ML_TEA = 1.0f;
    constexpr float GRAMS_PER_ML_CONDENSED_MILK = 1.0f;
    constexpr float GRAMS_PER_ML_EVAPORATED_MILK = 1.0f;
    constexpr float GRAMS_PER_ML_MILK = 1.0f;

} // anonymous namespace

// =========================================================================
// Public Interface
// =========================================================================

void WeightDispensingStrategy::start(const TeaRecipe &recipe, SensorManager *sensor, RelayManager *relay)
{
    target = recipe;

    // Record the current weight as baseline
    if (sensor)
    {
        weightAtPhaseStart = sensor->getWeight();
    }
    else
    {
        weightAtPhaseStart = 0.0f;
    }

    // Start with the first liquid that has a non-zero target
    currentPhase = DispensingPhase::TEA;
    startPhase(currentPhase, relay);
}

bool WeightDispensingStrategy::update(TeaRecipe &dispensed, SensorManager *sensor, RelayManager *relay)
{
    if (currentPhase == DispensingPhase::DONE)
    {
        return true;
    }

    // Safety timeout check
    if (DISPENSING_SAFETY_TIMEOUT_MS > 0)
    {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed > DISPENSING_SAFETY_TIMEOUT_MS)
        {
            DEBUG_PRINTLN(F("[WeightDispensing] SAFETY TIMEOUT - stopping"));
            stop(relay);
            return true;
        }
    }

    if (!sensor)
        return false;

    float currentWeight = sensor->getWeight();
    float dispensedGrams = currentWeight - weightAtPhaseStart;
    if (dispensedGrams < 0)
        dispensedGrams = 0;

    int targetMl = getTargetMlForPhase(currentPhase);
    float gramsPerMl = getGramsPerMlForPhase(currentPhase);
    float targetGrams = targetMl * gramsPerMl;

    // Update dispensed amount in ml for progress tracking
    float dispensedMl = dispensedGrams / gramsPerMl;
    int dispensedMlInt = constrain((int)dispensedMl, 0, targetMl);

    switch (currentPhase)
    {
    case DispensingPhase::TEA:
        dispensed.tea = dispensedMlInt;
        break;
    case DispensingPhase::CONDENSED_MILK:
        dispensed.condensedMilk = dispensedMlInt;
        break;
    case DispensingPhase::EVAPORATED_MILK:
        dispensed.evaporatedMilk = dispensedMlInt;
        break;
    case DispensingPhase::MILK:
        dispensed.milk = dispensedMlInt;
        break;
    default:
        break;
    }

    // Check if this phase is complete
    if (dispensedGrams >= (targetGrams - WEIGHT_TOLERANCE_G))
    {
        // Phase complete - turn off current relay
        uint8_t relayIdx = getRelayIndexForPhase(currentPhase);
        if (relay)
        {
            relay->setRelay(relayIdx, false);
        }

        // Finalize dispensed amount for this phase
        switch (currentPhase)
        {
        case DispensingPhase::TEA:
            dispensed.tea = targetMl;
            break;
        case DispensingPhase::CONDENSED_MILK:
            dispensed.condensedMilk = targetMl;
            break;
        case DispensingPhase::EVAPORATED_MILK:
            dispensed.evaporatedMilk = targetMl;
            break;
        case DispensingPhase::MILK:
            dispensed.milk = targetMl;
            break;
        default:
            break;
        }

        DEBUG_PRINTF("[WeightDispensing] Phase complete. Dispensed: %.1fg\n", dispensedGrams);

        // Update baseline weight for next phase
        weightAtPhaseStart = currentWeight;

        // Move to next phase
        advancePhase();

        if (currentPhase != DispensingPhase::DONE)
        {
            startPhase(currentPhase, relay);
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}

void WeightDispensingStrategy::stop(RelayManager *relay)
{
    if (relay)
    {
        relay->stopAll();
    }
    currentPhase = DispensingPhase::DONE;
}

// =========================================================================
// Phase Management
// =========================================================================

void WeightDispensingStrategy::startPhase(DispensingPhase phase, RelayManager *relay)
{
    int targetMl = getTargetMlForPhase(phase);

    // Skip phases with zero target
    if (targetMl <= 0)
    {
        advancePhase();
        if (currentPhase != DispensingPhase::DONE)
        {
            startPhase(currentPhase, relay);
        }
        return;
    }

    uint8_t relayIdx = getRelayIndexForPhase(phase);
    if (relay)
    {
        relay->setRelay(relayIdx, true);
    }
    phaseStartTime = millis();

    DEBUG_PRINTF("[WeightDispensing] Phase started: relay %d, target %d ml\n", relayIdx, targetMl);
}

void WeightDispensingStrategy::advancePhase()
{
    switch (currentPhase)
    {
    case DispensingPhase::TEA:
        currentPhase = DispensingPhase::CONDENSED_MILK;
        break;
    case DispensingPhase::CONDENSED_MILK:
        currentPhase = DispensingPhase::EVAPORATED_MILK;
        break;
    case DispensingPhase::EVAPORATED_MILK:
        currentPhase = DispensingPhase::MILK;
        break;
    case DispensingPhase::MILK:
        currentPhase = DispensingPhase::DONE;
        break;
    case DispensingPhase::DONE:
        break;
    }
}

// =========================================================================
// Lookup Helpers
// =========================================================================

int WeightDispensingStrategy::getTargetMlForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return target.tea;
    case DispensingPhase::CONDENSED_MILK:
        return target.condensedMilk;
    case DispensingPhase::EVAPORATED_MILK:
        return target.evaporatedMilk;
    case DispensingPhase::MILK:
        return target.milk;
    default:
        return 0;
    }
}

float WeightDispensingStrategy::getGramsPerMlForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return GRAMS_PER_ML_TEA;
    case DispensingPhase::CONDENSED_MILK:
        return GRAMS_PER_ML_CONDENSED_MILK;
    case DispensingPhase::EVAPORATED_MILK:
        return GRAMS_PER_ML_EVAPORATED_MILK;
    case DispensingPhase::MILK:
        return GRAMS_PER_ML_MILK;
    default:
        return 1.0f;
    }
}

uint8_t WeightDispensingStrategy::getRelayIndexForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return 0;
    case DispensingPhase::CONDENSED_MILK:
        return 1;
    case DispensingPhase::EVAPORATED_MILK:
        return 2;
    case DispensingPhase::MILK:
        return 3;
    default:
        return 0;
    }
}
