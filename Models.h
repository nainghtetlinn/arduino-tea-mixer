#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum class TeaState : uint8_t
{
    IDLE,
    PREVIEW,
    MAKING,
    CLEANING,
    CALIBRATION,
    FINISHED,
    ERROR
};

enum class DispensingPhase : uint8_t
{
    TEA,
    CONDENSED_MILK,
    EVAPORATED_MILK,
    MILK,
    DONE
};

struct TeaRecipe
{
    int tea{0};
    int condensedMilk{0};
    int evaporatedMilk{0};
    int milk{0};

    // Serialize recipe into a JsonObject reference
    void toJson(JsonObject obj) const
    {
        obj["tea"] = tea;
        obj["condensedMilk"] = condensedMilk;
        obj["evaporatedMilk"] = evaporatedMilk;
        obj["milk"] = milk;
    }

    // Deserialize recipe from a JsonObjectConst reference
    void fromJson(JsonObjectConst obj)
    {
        if (obj.isNull())
            return;
        tea = obj["tea"] | tea;
        condensedMilk = obj["condensedMilk"] | condensedMilk;
        evaporatedMilk = obj["evaporatedMilk"] | evaporatedMilk;
        milk = obj["milk"] | milk;
    }
};

struct ButtonInfo
{
    String name;
    TeaRecipe recipe;

    // Serialize button into a JsonObject reference
    void toJson(JsonObject obj) const
    {
        obj["name"] = name;
        recipe.toJson(obj);
    }

    // Deserialize button from a JsonObjectConst reference
    void fromJson(JsonObjectConst obj)
    {
        if (obj.isNull())
            return;
        name = obj["name"] | name;
        recipe.fromJson(obj);
    }
};

#endif