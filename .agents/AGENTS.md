# project_tea — Coding Rules

## File Size
- **Max 300 lines per .cpp file**. If a file exceeds this, refactor by extracting
  a new class/module.
- Header files should be lean: declarations only, no inline method bodies
  longer than 3 lines.

## Architecture
- **One class per .h/.cpp pair** — file names must match the class name.
- All files must live in the project root (Arduino IDE flat-directory requirement).
- Use **forward declarations** in headers; `#include` only in .cpp files
  where possible.

## Naming Conventions
- Class names: PascalCase (e.g., `TeaManager`, `WeightDispensingStrategy`)
- Methods/functions: camelCase (e.g., `startMakingTea()`)
- Constants: UPPER_SNAKE_CASE (e.g., `RELAY_COUNT`)
- Private members: camelCase, no prefix (e.g., `currentState`, not `m_currentState`)
- File-scoped constants: use anonymous namespace, not `#define`

## Patterns
- **Manager pattern**: Hardware-facing singletons (`RelayManager`, `SensorManager`, etc.)
  own their pins, I2C bus, BLE stack. One global instance each.
- **Strategy pattern**: Behavioral variation (e.g., dispensing modes) should be
  extracted behind an abstract base class.
- **State machine**: TeaManager owns the top-level state (`TeaState`). Sub-states
  belong to the extracted classes.
- **Callbacks**: Use `attach*Callback()` setters, not constructor/begin() params.

## Dependencies & Includes
- `Config.h` is the single source for debug macros and shared hardware constants.
- `Models.h` is the single source for data structs and enums.
- Managers must not `#include` each other's headers directly — use forward
  declarations and accept pointers via `begin()` or constructors.

## NVS Persistence
- All NVS access uses the ESP32 `Preferences` library.
- Namespace: `"tea_settings"` for all persisted values.
- Load in `begin()`, save immediately on change.

## Comments
- Use `// =====` section banners to delimit major logical blocks within a file.
- Keep debug logs prefixed with `[ClassName]` for grep-ability.
