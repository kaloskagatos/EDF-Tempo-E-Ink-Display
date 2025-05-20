// BatteryMonitor.h
#pragma once

#include <math.h>

// Interface for Battery Monitoring
class IBatteryMonitor
{
public:
    virtual ~IBatteryMonitor() = default;
    virtual int getPercentage() const = 0;
    virtual float getVoltage() const = 0;
};

#include "Configuration.h"
#include "SimpleStorage.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif

class BatteryMonitor : public IBatteryMonitor
{
public:
    BatteryMonitor()
    {
        storage.begin(STORAGE_NS);
        maxVoltage = storage.getFloat(KEY_MAX_VOLTAGE, BatteryConfig::VOLTAGE_FULL);
    }

    int getPercentage() const override
    {
        float voltage = getVoltage();

        if (voltage <= BatteryConfig::VOLTAGE_EMPTY)
        {
            seenLow = true;
            return 0;
        }

        if (seenLow && voltage > maxVoltage * 0.95f)
        {
            maxVoltage = 0.7f * maxVoltage + 0.3f * voltage;
            storage.putFloat(KEY_MAX_VOLTAGE, maxVoltage);
            seenLow = false;
        }

        if (voltage >= maxVoltage)
            return 100;

        float ratio = (voltage - BatteryConfig::VOLTAGE_EMPTY) /
                      (maxVoltage - BatteryConfig::VOLTAGE_EMPTY);
        int percentage = static_cast<int>(ratio * 100);
        if (percentage > 100)
            percentage = 100;
        if (percentage < 0)
            percentage = 0;
        return percentage;
    }

    float getVoltage() const override
    {
#ifdef ARDUINO
        const int samples = 10;
        float sum = 0;
        for (int i = 0; i < samples; ++i)
        {
            sum += analogRead(BatteryConfig::PIN_BAT);
        }
        return (sum / samples) / 4096.0f * 7.05f;
#else
        return simulatedVoltage;
#endif
    }

#ifndef ARDUINO
    void setSimulatedVoltage(float v) const { simulatedVoltage = v; }
#endif

private:
    static constexpr const char *STORAGE_NS = "battery";
    static constexpr const char *KEY_MAX_VOLTAGE = "maxV";

    mutable SimpleStorage storage;
    mutable float maxVoltage = BatteryConfig::VOLTAGE_FULL;
    mutable bool seenLow = false;
#ifndef ARDUINO
    mutable float simulatedVoltage = BatteryConfig::VOLTAGE_FULL;
#endif
};
