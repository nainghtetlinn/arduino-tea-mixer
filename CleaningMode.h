#ifndef CLEANING_MODE_H
#define CLEANING_MODE_H

#include <Arduino.h>
#include "Config.h"
#include "RelayManager.h"
#include "DisplayManager.h"
#include "BluetoothManager.h"

// Forward declarations
class RelayManager;
class DisplayManager;
class BluetoothManager;

class CleaningMode
{
public:
    void begin(RelayManager *relay, DisplayManager *display, BluetoothManager *ble);

    void start();
    bool update();
    void cancel();

private:
    RelayManager *relayMgr;
    DisplayManager *displayMgr;
    BluetoothManager *bleMgr;

    unsigned long lastProgressUpdate;

    unsigned long cleaningStartTime;
    unsigned long cleaningDoneTime;

    uint8_t cleaningPhase; // 0 = relays 0+1, 1 = relays 2+3
    unsigned long phaseStartTime;

    bool isSwitching; // true during the 500 ms gap between phases
    unsigned long switchStartTime;

    bool isDoneDelay;
};

#endif
