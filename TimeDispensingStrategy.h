#ifndef TIME_DISPENSING_STRATEGY_H
#define TIME_DISPENSING_STRATEGY_H

#include <Arduino.h>
#include "DispensingStrategy.h"
#include "Config.h"
#include "RelayManager.h"
#include "SensorManager.h"

// =========================================================================
// Sequential-pair time-based dispensing strategy
// =========================================================================
// Relays are sorted by dispensing duration (longest first).
// At most two relays run simultaneously.  When the shorter relay of the
// active pair finishes, the next pending relay is started immediately,
// so the longest relay never idles while a shorter one is waiting.
//
// Example (tea=200 ml, condensed=100 ml, evap=50 ml, milk=50 ml):
//   t=0   → relay TEA + relay CONDENSED both ON
//   t=20s → CONDENSED done  → relay EVAPORATED ON  (TEA still running)
//   t=30s → EVAPORATED done → relay MILK ON        (TEA still running)
//   t=40s → TEA + MILK done → all OFF
// =========================================================================

class TimeDispensingStrategy : public DispensingStrategy
{
public:
    void start(const TeaRecipe &target) override;
    bool update(TeaRecipe &dispensed) override;
    void stop() override;

private:
    // Internal phase: prime each relay before starting timed dispensing
    enum class Phase : uint8_t { PRIMING, DISPENSING };

    TeaRecipe target;
    Phase phase;

    // Per-relay state
    unsigned long relayStartTime[RELAY_COUNT];
    unsigned long relayDuration[RELAY_COUNT];
    bool relayRunning[RELAY_COUNT];

    // Sorted indices (longest duration first), used to dequeue next relay
    uint8_t sortedOrder[RELAY_COUNT];
    uint8_t sortedCount; // how many relays are actually needed (duration > 0)
    uint8_t nextToStart; // index into sortedOrder for the next relay to activate

    // 500 ms deferred activation — up to 2 relays can be batched together
    // (e.g. when two equal-duration relays finish in the same tick)
    uint8_t pendingCount;              // 0, 1, or 2
    uint8_t pendingRelayIdx[2];        // relays waiting to start
    unsigned long pendingActivateTime; // shared fire timestamp for the batch

    // Priming state — each relay is tested for flow before timers start
    uint8_t primingIndex;              // index into sortedOrder being primed
    float primingStartWeight;          // weight snapshot when current relay priming began
    unsigned long primingStartTime;    // millis() when current relay priming began

    float getFlowRateForIndex(uint8_t index) const;
    void activateRelay(uint8_t index, unsigned long now);
    void setDispensedForRelay(uint8_t index, int ml, TeaRecipe &dispensed) const;
    void startPrimingRelay();
    void beginDispensing();
};

#endif
