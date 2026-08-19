#include "DispensingStrategy.h"

void DispensingStrategy::begin(SensorManager *sensor, RelayManager *relay)
{
    sensorMgr = sensor;
    relayMgr = relay;
}
