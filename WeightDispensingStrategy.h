#ifndef WEIGHT_DISPENSING_STRATEGY_H
#define WEIGHT_DISPENSING_STRATEGY_H

#include "DispensingStrategy.h"
#include "Config.h"

class WeightDispensingStrategy : public DispensingStrategy
{
public:
    void start(const TeaRecipe &target, SensorManager *sensor, RelayManager *relay) override;
    bool update(TeaRecipe &dispensed, SensorManager *sensor, RelayManager *relay) override;
    void stop(RelayManager *relay) override;

private:
    TeaRecipe target;
    DispensingPhase currentPhase;
    float weightAtPhaseStart;
    unsigned long phaseStartTime;

    void startPhase(DispensingPhase phase, RelayManager *relay);
    void advancePhase();

    int getTargetMlForPhase(DispensingPhase phase) const;
    float getGramsPerMlForPhase(DispensingPhase phase) const;
    uint8_t getRelayIndexForPhase(DispensingPhase phase) const;
};

#endif
