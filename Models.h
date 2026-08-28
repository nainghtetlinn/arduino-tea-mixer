#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum class TeaState : uint8_t
{
    IDLE,
    PREVIEW,
    MAKING,
    FINISHED,
    CLEANING,
    CALIBRATION,
    CUP_WARNING,
    WAITING_FOR_CUP,
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

// ===== Ratio-based scaling =====

// Scale a recipe so its ingredients add up to exactly targetMl.
// The raw ingredient values are treated as ratios; each is scaled
// proportionally. Integer rounding is corrected on the largest ingredient
// so the result always sums to exactly targetMl.
inline TeaRecipe scaleRecipeToTarget(const TeaRecipe &src, int targetMl)
{
    const int total = src.tea + src.condensedMilk + src.evaporatedMilk + src.milk;
    if (total <= 0 || targetMl <= 0)
        return src;

    TeaRecipe scaled;
    scaled.tea = (src.tea * targetMl) / total;
    scaled.condensedMilk = (src.condensedMilk * targetMl) / total;
    scaled.evaporatedMilk = (src.evaporatedMilk * targetMl) / total;
    scaled.milk = (src.milk * targetMl) / total;

    // Distribute rounding remainder to the largest ingredient
    int remainder = targetMl - (scaled.tea + scaled.condensedMilk +
                                scaled.evaporatedMilk + scaled.milk);
    if (remainder != 0)
    {
        // Find index of largest scaled ingredient and add remainder there
        int vals[4] = {scaled.tea, scaled.condensedMilk,
                       scaled.evaporatedMilk, scaled.milk};
        int maxIdx = 0;
        for (int i = 1; i < 4; i++)
        {
            if (vals[i] > vals[maxIdx])
                maxIdx = i;
        }
        if (maxIdx == 0)
            scaled.tea += remainder;
        else if (maxIdx == 1)
            scaled.condensedMilk += remainder;
        else if (maxIdx == 2)
            scaled.evaporatedMilk += remainder;
        else
            scaled.milk += remainder;
    }

    return scaled;
}

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