#ifndef DISPENSING_STRATEGY_H
#define DISPENSING_STRATEGY_H

#include "Models.h"

// Forward declarations
class RelayManager;
class SensorManager;

// =========================================================================
// Abstract dispensing interface
// =========================================================================
// Concrete implementations handle either weight-based or time-based
// dispensing.  TeaManager delegates to the active strategy.

class DispensingStrategy
{
public:
    virtual ~DispensingStrategy() = default;

    // Begin dispensing the given recipe.
    virtual void start(const TeaRecipe &target, SensorManager *sensor, RelayManager *relay) = 0;

    // Called every tick while dispensing is active.
    // Updates `dispensed` with current progress amounts.
    // Returns true when dispensing is complete.
    virtual bool update(TeaRecipe &dispensed, SensorManager *sensor, RelayManager *relay) = 0;

    // Emergency stop — immediately turn off all relays managed by this strategy.
    virtual void stop(RelayManager *relay) = 0;
};

#endif
