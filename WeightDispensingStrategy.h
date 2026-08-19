#ifndef WEIGHT_DISPENSING_STRATEGY_H
#define WEIGHT_DISPENSING_STRATEGY_H

#include "DispensingStrategy.h"
#include "Config.h"

class WeightDispensingStrategy : public DispensingStrategy
{
public:
    void start(const TeaRecipe &target) override;
    bool update(TeaRecipe &dispensed) override;
    void stop() override;

private:
    TeaRecipe target;
    DispensingPhase currentPhase;
    float weightAtPhaseStart;
    unsigned long phaseStartTime;

    void startPhase(DispensingPhase phase);
    void advancePhase();

    int getTargetMlForPhase(DispensingPhase phase) const;
    float getGramsPerMlForPhase(DispensingPhase phase) const;
    uint8_t getRelayIndexForPhase(DispensingPhase phase) const;
};

#endif
