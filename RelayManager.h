#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>
#include "Config.h"

class RelayManager
{
public:
    RelayManager();
    void begin();

    // Turn a specific relay on or off by index (0–3)
    void setRelay(uint8_t index, bool on);

    // Emergency stop - turn off all relays immediately
    void stopAll();

    // Query whether a specific relay is currently active
    bool isRelayActive(uint8_t index) const;

private:
    bool relayActive[RELAY_COUNT];

    uint8_t getRelayPin(uint8_t index) const;
};

#endif
