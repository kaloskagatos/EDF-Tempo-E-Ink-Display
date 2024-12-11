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

#ifdef ARDUINO
#include <Arduino.h>
#include "Configuration.h"

class BatteryMonitor : public IBatteryMonitor
{
public:
    int getPercentage() const override
    {
        float voltage = getVoltage();
        if (voltage <= BatteryConfig::VOLTAGE_EMPTY)
            return 0;
        if (voltage >= BatteryConfig::VOLTAGE_FULL)
            return 100;

        int percentage = static_cast<int>(2836.9625 * pow(voltage, 4) -
                                          43987.4889 * pow(voltage, 3) +
                                          255233.8134 * pow(voltage, 2) -
                                          656689.7123 * voltage +
                                          632041.7303);
        if (percentage > 100)
            percentage = 100;
        if (percentage < 0)
            percentage = 0;
        return percentage;
    }

    // Lecture avec moyenne des valeurs
    float getVoltage() const override
    {
        const int samples = 10;
        float sum = 0;
        for (int i = 0; i < samples; ++i)
        {
            sum += analogRead(BatteryConfig::PIN_BAT);
        }
        return (sum / samples) / 4096.0f * 7.05f;
    }
};
#endif
