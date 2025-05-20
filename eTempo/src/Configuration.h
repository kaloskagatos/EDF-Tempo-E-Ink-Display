// Configuration.h
#pragma once

struct DisplayConfig
{
    static const int SCREEN_WIDTH = 250;
    static const int SCREEN_HEIGHT = 122;
    static const int MARGIN = 2;
    static const int RECT_WIDTH = 120;
    static const int RECT_HEIGHT = 100;
    static const int FONT_SIZE_SMALL = 9;
    static const int FONT_SIZE_MEDIUM = 12;
};

struct WifiConfig
{
    static constexpr const char *NTP_SERVER = "pool.ntp.org";
    static constexpr const char *ACCESS_POINT_NAME = "TempoAP";
    static const int WIFI_TIMEOUT = 60; // Seconds
    static const int WIFI_CONNECT_RETRIES = 3;
    static const int WIFI_CONNECT_TIMEOUT = 5; // Seconds per attempt
};

struct BatteryConfig
{
    static const int PIN_BAT = 35;               // ADC pin for battery voltage
    static constexpr float VOLTAGE_FULL = 4.2f;  // Voltage corresponding to 100%
    static constexpr float VOLTAGE_EMPTY = 3.5f; // Voltage corresponding to 0%
};

struct RetryConfig
{
    static const int INITIAL_RETRY_ATTEMPTS = 5;
    static constexpr int RETRY_DELAY_MINUTES = 7; // Minutes before retry
};

struct DisplayOptions
{
    static constexpr bool DEBUG_GRID = false;
    static constexpr bool DEBUG_RTE = false;
    static constexpr bool DISPLAY_RETRY = true;
    enum JoursAffichage
    {
        PASSES,
        RESTANTS
    };
    static const JoursAffichage AFFICHAGE_JOURS = RESTANTS;
};
