# Tea Mixer block diagram

`project_tea` is an ESP32-based, four-liquid dispenser. `TeaManager` is the
application controller: it processes local/BLE requests, enforces cup-safety,
selects a dispensing strategy, and coordinates the user interfaces.

```mermaid
flowchart LR
    subgraph Inputs[User and sensor inputs]
        BTN["3 push-buttons\nGPIO 27 / 26 / 25"]
        APP["BLE mobile client\nJSON commands / notifications"]
        HX["HX711 load cell\nDT 14, CLK 13"]
        TEMP["DS18B20 temperature sensor\n1-Wire GPIO 4"]
    end

    subgraph ESP32[ESP32 firmware]
        BM["ButtonManager\nclicks; 1+2 calibration; 1+3 cleaning"]
        BLEM["BluetoothManager\nBLE GATT server"]
        SM["SensorManager\n0.5 s temperature + weight sampling"]
        TM["TeaManager\nstate machine and safety controller"]
        NVS[("NVS: tea_settings\nrecipes, scale factor, mode,\ncup threshold, target volume")]
        DS{"Dispensing mode"}
        WS["WeightDispensingStrategy\nsequential, load-cell feedback\n2 g tolerance; 5 min safety timeout"]
        TS["TimeDispensingStrategy\nflow-rate timing; max two\nchannels active; 500 ms changeover"]
        CM["CleaningMode\n0+1 for 30 s, then 2+3 for 30 s"]
        RM["RelayManager\nactive-low relay control"]
        DM["DisplayManager"]
    end

    subgraph Outputs[Physical / user-facing outputs]
        OLED["128x64 SSD1306 OLED\nI²C: SDA 21, SCL 22, 0x3C"]
        RELAYS["4 relay channels\nTea: GPIO 18\nCondensed milk: GPIO 19\nEvaporated milk: GPIO 17\nMilk: GPIO 16"]
        PUMPS["Pumps / valves\nand liquid reservoirs"]
    end

    BTN --> BM --> TM
    APP <--> BLEM
    BLEM <--> TM
    HX --> SM
    TEMP --> SM
    SM --> TM
    SM <--> NVS
    BM <--> NVS
    TM <--> NVS

    TM -->|recipe scaled to target total mL| DS
    DS -->|weight mode| WS
    DS -->|time mode| TS
    WS --> RM
    TS --> RM
    TM --> CM --> RM
    RM --> RELAYS --> PUMPS
    TM --> DM --> OLED
    TM -->|progress, status, errors| BLEM
```

## Control loop

`loop()` runs every ~20 ms: `ButtonManager.update()`, `SensorManager.update()`,
then `TeaManager.update()`. Sensor readings refresh every 500 ms; dispensing
state and progress are evaluated every 100 ms while making tea.

## TeaManager states

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> PREVIEW: first recipe-button press
    PREVIEW --> MAKING: same button again + cup present
    PREVIEW --> WAITING_FOR_CUP: confirm without cup
    WAITING_FOR_CUP --> MAKING: cup >= configured threshold for 1 s
    PREVIEW --> IDLE: timeout / cancel
    WAITING_FOR_CUP --> IDLE: cancel
    MAKING --> FINISHED: strategy completes
    MAKING --> CUP_WARNING: cup removed
    CUP_WARNING --> IDLE: dismiss / 5 s timeout
    FINISHED --> IDLE: cup removed
    IDLE --> CLEANING: buttons 1+3 held 3 s
    CLEANING --> IDLE: complete / cancel
    IDLE --> CALIBRATION: buttons 1+2 held 3 s
    CALIBRATION --> IDLE: buttons 1+2 held 3 s
```

The BLE `MAKE_TEA` command bypasses the preview but still uses the cup check,
recipe scaling, active dispensing strategy, display updates, and BLE status
notifications.
