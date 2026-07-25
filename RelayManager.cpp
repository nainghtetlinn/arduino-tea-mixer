#include "RelayManager.h"
#include "Config.h"

namespace
{
    // --- Pins ---
    constexpr uint8_t PIN_RELAY_TEA = 16;
    constexpr uint8_t PIN_RELAY_CONDENSED_MILK = 17;
    constexpr uint8_t PIN_RELAY_EVAPORATED_MILK = 18;
    constexpr uint8_t PIN_RELAY_MILK = 19;

    // Relay activation mode
    constexpr bool RELAY_ACTIVE_LOW = true;
} // anonymous namespace

RelayManager::RelayManager()
{
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayActive[i] = false;
    }
}

void RelayManager::begin()
{
    setupRelayPins();
    DEBUG_PRINTLN(F("[RelayManager] Initialized"));
}

// =========================================================================
// Public API
// =========================================================================

void RelayManager::setRelay(uint8_t index, bool on)
{
    if (index >= RELAY_COUNT)
        return;

    uint8_t pin = getRelayPin(index);
    // For active-LOW relays: on=true → write LOW, on=false → write HIGH
    // For active-HIGH relays: on=true → write HIGH, on=false → write LOW
    if (RELAY_ACTIVE_LOW)
    {
        digitalWrite(pin, on ? LOW : HIGH);
    }
    else
    {
        digitalWrite(pin, on ? HIGH : LOW);
    }

    relayActive[index] = on;
}

void RelayManager::stopAll()
{
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        setRelay(i, false);
    }
    DEBUG_PRINTLN(F("[RelayManager] Emergency stop - all relays OFF"));
}

bool RelayManager::isRelayActive(uint8_t index) const
{
    if (index >= RELAY_COUNT)
        return false;
    return relayActive[index];
}

// =========================================================================
// Private Helpers
// =========================================================================

void RelayManager::setupRelayPins()
{
    const uint8_t pins[] = {PIN_RELAY_TEA, PIN_RELAY_CONDENSED_MILK,
                            PIN_RELAY_EVAPORATED_MILK, PIN_RELAY_MILK};

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        pinMode(pins[i], OUTPUT);
        // Ensure all relays start OFF
        setRelay(i, false);
    }
}

uint8_t RelayManager::getRelayPin(uint8_t index) const
{
    switch (index)
    {
    case 0:
        return PIN_RELAY_TEA;
    case 1:
        return PIN_RELAY_CONDENSED_MILK;
    case 2:
        return PIN_RELAY_EVAPORATED_MILK;
    case 3:
        return PIN_RELAY_MILK;
    default:
        return PIN_RELAY_TEA;
    }
}
