// TempoApp

#pragma once

#ifdef ARDUINO
#include "Platform.h"
#include "Configuration.h"
#include "BatteryMonitor.h"
#include "TempoColorServiceManager.h"
#include "HttpClientFactory.h"
#include "ApiCouleurTempoColorService.h"
#include "RteTempoColorService.h"
#include "TempoLightColorService.h"
#include "CommerceEdfColorService.h"

// Ajout des librairies d'affichage et WiFi, etc.
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <time.h>
#include <math.h>

class TempoApp
{
public:
    TempoApp();
    void run();

private:
    bool connectToWiFi();
    bool connectToWiFiDirect();
    bool initializeTime();
    void displayLine(String text);
    String getDayOfWeekInFrench(int dayOfWeek);
    String getMonthInFrench(int month);
    String getCurrentDateString();
    String getNextDayDateString();

    void displayInfo();
    void drawBatteryInfo(int batteryTopLeftX, int batteryTopLeftY);

    void displayFailureMessage();

    bool getCurrentSeason(String &season);
    bool fetchTempoInformation();
    time_t getNextWakeupTime();
    void goToDeepSleepUntilNextWakeup();
#ifdef DEBUG_GRID
    void drawDebugGrid();
#endif

    static const char *ntpServer;
    static const char *accessPointName;

    struct tm timeinfo;

    String todayColor;
    String tomorrowColor;
    TempoColor todayColorEnum;
    TempoColor tomorrowColorEnum;
    String remainingBlueDays;
    String remainingWhiteDays;
    String remainingRedDays;

    bool wifiSucceeded;
    int currentLinePos;

    BatteryMonitor batteryMonitor;

    bool isTodayColorFound;
    bool isTomorrowColorFound;

    // Structure de réveil
    struct WakeupTime
    {
        int hour;
        int minute;
        bool retry;
    };

    static const WakeupTime wakeupTimes[];

    // Ecran
    GxIO_Class io;
    GxEPD_Class display;

    // RTC_data
    // arret de l'utilisation de la librairie préférences, passage en mémoire RTC
    // Nombre de tentatives pour les retries
    static RTC_DATA_ATTR int retryAttempts;
    static RTC_DATA_ATTR TempoColor lastTodayColor;
    static RTC_DATA_ATTR TempoColor lastTomorrowColor;
    static RTC_DATA_ATTR int lastBlueDays;
    static RTC_DATA_ATTR int lastWhiteDays;
    static RTC_DATA_ATTR int lastRedDays;
};
#endif
