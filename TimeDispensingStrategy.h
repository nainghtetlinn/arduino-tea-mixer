#ifndef TIME_DISPENSING_STRATEGY_H
#define TIME_DISPENSING_STRATEGY_H

#include "DispensingStrategy.h"
#include "Config.h"

class TimeDispensingStrategy : public DispensingStrategy
{
public:
    void start(const TeaRecipe &target, SensorManager *sensor, RelayManager *relay) override;
    bool update(TeaRecipe &dispensed, SensorManager *sensor, RelayManager *relay) override;
    void stop(RelayManager *relay) override;

private:
    TeaRecipe target;
    unsigned long relayStartTime[RELAY_COUNT];
    unsigned long relayDuration[RELAY_COUNT];
    bool relayRunning[RELAY_COUNT];

    float getFlowRateForIndex(uint8_t index) const;
};

#endif
