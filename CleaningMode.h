#ifndef CLEANING_MODE_H
#define CLEANING_MODE_H

#include <Arduino.h>

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

    unsigned long cleaningStartTime;
    unsigned long lastProgressUpdate;
    bool isDoneDelay;
    unsigned long cleaningDoneTime;
};

#endif
