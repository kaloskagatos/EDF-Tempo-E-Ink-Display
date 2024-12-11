// TempoColor.h
#pragma once

#include "Platform.h"

enum class TempoColor
{
    BLUE,
    WHITE,
    RED,
    UNKNOWN
};

inline String toString(TempoColor color)
{
    switch (color)
    {
    case TempoColor::BLUE:
        return F("BLEU");
    case TempoColor::WHITE:
        return F("BLANC");
    case TempoColor::RED:
        return F("ROUGE");
    default:
        return F("<?>");
    }
}
