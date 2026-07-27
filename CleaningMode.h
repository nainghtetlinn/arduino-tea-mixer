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
    // Call start() to begin, update() every tick, cancel() to abort.
    void start(RelayManager *relay, DisplayManager *display, BluetoothManager *ble);
    bool update(RelayManager *relay, DisplayManager *display, BluetoothManager *ble);
    void cancel(RelayManager *relay, BluetoothManager *ble);

private:
    unsigned long cleaningStartTime;
    unsigned long lastProgressUpdate;
    bool isDoneDelay;
    unsigned long cleaningDoneTime;
};

#endif
