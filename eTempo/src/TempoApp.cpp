// TempoApp.cpp

#include "TempoApp.h"

#ifdef ARDUINO

#include <time.h>
#include <sys/time.h>
#include <memory>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

//  Mise en mémoire flash
extern const GFXfont FreeSans9pt7b PROGMEM;
extern const GFXfont FreeSansBold12pt7b PROGMEM;

#define MEDIUM_FONT &FreeSans9pt7b
#define LARGE_FONT &FreeSansBold12pt7b

// Définition des variables statiques
RTC_DATA_ATTR int TempoApp::retryAttempts = RetryConfig::INITIAL_RETRY_ATTEMPTS;
RTC_DATA_ATTR TempoColor TempoApp::lastTodayColor = TempoColor::UNKNOWN;
RTC_DATA_ATTR TempoColor TempoApp::lastTomorrowColor = TempoColor::UNKNOWN;
RTC_DATA_ATTR int TempoApp::lastBlueDays = -1;
RTC_DATA_ATTR int TempoApp::lastWhiteDays = -1;
RTC_DATA_ATTR int TempoApp::lastRedDays = -1;
const char *TempoApp::ntpServer = WifiConfig::NTP_SERVER;
const char *TempoApp::accessPointName = WifiConfig::ACCESS_POINT_NAME;

const TempoApp::WakeupTime TempoApp::wakeupTimes[] = {
    {0, 5, false},  // Réveil à 00:05 pas de retry
    {6, 31, false}, // Réveil à 06:31 pour préview RTE pas de retry
    {11, 5, true}   // Réveil à 11:05 avec retry
};

const char DAY_NOT_AVAILABLE[] PROGMEM = "N/A";
const char REMAINING_DAY_COLOR[] PROGMEM = "<?>";

const char MSG_WIFI_ERROR[] PROGMEM = "Erreur de connexion";
const char MSG_NTP_SYNC_ERROR[] PROGMEM = "Erreur de synchro NTP.";
const char MSG_FETCH_ERROR[] PROGMEM = "Pas de données. Programmation d'un réveil.";
const char MSG_WIFI_NOT_CONNECTED[] PROGMEM = "Wi-Fi non connecté !";
const char MSG_SEASON_FETCH_ERROR[] PROGMEM = "Echec de récupération de la saison.";
const char MSG_DATE_ERROR[] PROGMEM = "Echec de récupération de la date.";
const char MSG_EDF_INIT_ERROR[] PROGMEM = "Echec d'initialisation des services EDF: ";
const char MSG_WAKEUP_ERROR[] PROGMEM = "Echec de l'obtention de l'heure pour le réveil";
const char MSG_WAKEUP_BEFORE_NOW[] PROGMEM = "Erreur: L'heure du prochain réveil est antérieure à l'heure actuelle.";

TempoApp::TempoApp()
    : wifiSucceeded(true), currentLinePos(0),
      isTodayColorFound(false), isTomorrowColorFound(false),
      io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16),
      display(io, /*RST=*/16, /*BUSY=*/4)
{
   todayColor = DAY_NOT_AVAILABLE;
   tomorrowColor = DAY_NOT_AVAILABLE;
   remainingBlueDays = REMAINING_DAY_COLOR;
   remainingWhiteDays = REMAINING_DAY_COLOR;
   remainingRedDays = REMAINING_DAY_COLOR;
}

void TempoApp::run()
{
   setlocale(LC_TIME, "fr_FR.UTF-8");

   DEBUG_INIT();
#ifdef ARDUINO
   setCpuFrequencyMhz(80);
#endif

   // récupérer le voltage de la carte
   int batteryPercentage = batteryMonitor.getPercentage();
   float batteryVoltage = batteryMonitor.getVoltage();

   // Initialize display
   display.init();
   displayLine("Initialisation...");
   displayLine("Adresse MAC:");
   displayLine(WiFi.macAddress().c_str());
   displayLine("Batterie:");
   {
      char line[24];
      sprintf(line, "%5.3fv (%d%%)", batteryVoltage, batteryPercentage);
      displayLine(line);
   }
   displayLine("SSID de config:");
   displayLine(accessPointName);
   display.update();

   struct tm rtcTime;
   bool haveRtc = getLocalTime(&rtcTime, 0);
   bool earlySlot = haveRtc && rtcTime.tm_hour == 0 && rtcTime.tm_min <= 5 &&
                    lastTomorrowColor != TempoColor::UNKNOWN;
   if (earlySlot)
   {
      todayColorEnum = lastTomorrowColor;
      tomorrowColorEnum = TempoColor::UNKNOWN;
      todayColor = toString(todayColorEnum);
      tomorrowColor = DAY_NOT_AVAILABLE;
      remainingBlueDays = String(lastBlueDays);
      remainingWhiteDays = String(lastWhiteDays);
      remainingRedDays = String(lastRedDays);
      isTodayColorFound = true;
      isTomorrowColorFound = false;
      display.fillScreen(GxEPD_WHITE);
      displayInfo();
      display.update();
      goToDeepSleepUntilNextWakeup();
      return;
   }

// Connecter au WiFi
#if 0
   if (!connectToWiFiDirect())
#else
   if (!connectToWiFi())
#endif
   {
      displayLine(MSG_WIFI_ERROR);
      display.update();
      DEBUG_PRINTLN(MSG_WIFI_ERROR);
      wifiSucceeded = false;
   }

   // Initialiser l'heure
   if (!initializeTime())
   {
      DEBUG_PRINTLN(MSG_NTP_SYNC_ERROR);
      displayLine(MSG_NTP_SYNC_ERROR);
      display.update();
      // Deep sleep for 1 minute
      esp_sleep_enable_timer_wakeup(1 * 60 * 1000000LL);
      esp_deep_sleep_start();
      return;
   }

   // Formatage et affichage de l'heure courante
   char timeBuffer[64];
   strftime(timeBuffer, sizeof(timeBuffer), "%A, %B %d %Y %H:%M:%S", &timeinfo);
   DEBUG_PRINT(F("Date actuelle    : "));
   DEBUG_PRINTLN(timeBuffer);

#ifdef DEBUG_GRID
   drawDebugGrid();
#endif

   // Récupération des infos
   if (fetchTempoInformation())
   {
      bool changed = (todayColorEnum != lastTodayColor) ||
                     (tomorrowColorEnum != lastTomorrowColor) ||
                     (remainingBlueDays.toInt() != lastBlueDays) ||
                     (remainingWhiteDays.toInt() != lastWhiteDays) ||
                     (remainingRedDays.toInt() != lastRedDays);

      if (changed)
      {
         display.fillScreen(GxEPD_WHITE);
         displayInfo();
         display.update();
         lastTodayColor = todayColorEnum;
         lastTomorrowColor = tomorrowColorEnum;
         lastBlueDays = remainingBlueDays.toInt();
         lastWhiteDays = remainingWhiteDays.toInt();
         lastRedDays = remainingRedDays.toInt();
      }
   }
   else
   {
      // currentLinePos = 0;
      displayLine(F("Erreur de fetch."));
      displayLine(F("Wait&Retry..."));
      displayFailureMessage();
      display.update();
   }

   // Sommeil profond jusqu'à la prochaine heure de réveil
   goToDeepSleepUntilNextWakeup();
}

bool TempoApp::connectToWiFi()
{
   DEBUG_PRINTLN(F("Lancement de WiFiManager..."));
   WiFiManager wm;
   wm.setConfigPortalTimeout(WifiConfig::WIFI_TIMEOUT);
   wm.setConnectRetries(WifiConfig::WIFI_CONNECT_RETRIES);
   wm.setConnectTimeout(WifiConfig::WIFI_CONNECT_TIMEOUT);
   bool res = wm.autoConnect(accessPointName);

   if (!res)
   {
      DEBUG_PRINTLN(F("Echec de connexion au WiFi et temps d'attente dépassé."));
      return false;
   }

   DEBUG_PRINTLN(F("Connecté au WiFi."));
   return true;
}

// Pour debug
bool TempoApp::connectToWiFiDirect()
{
   const char *ssid = "Pixel"; // SSID du réseau WiFi
   const char *password = "";  // Pas de mot de passe

   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);

   DEBUG_PRINT("Connexion au WiFi : ");
   DEBUG_PRINT(ssid);
   DEBUG_PRINT("...");

   unsigned long startTime = millis();
   unsigned long timeout = 10000; // Timeout de 10 secondes

   while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout)
   {
      delay(100);
      DEBUG_PRINT(".");
   }

   if (WiFi.status() == WL_CONNECTED)
   {
      DEBUG_PRINTLN(F("\nConnecté au WiFi !"));
      DEBUG_PRINT(F("Adresse IP : "));
      DEBUG_PRINTLN(WiFi.localIP());
      return true;
   }
   else
   {
      DEBUG_PRINTLN(F("\nEchec de connexion au WiFi."));
      return false;
   }
}

bool TempoApp::initializeTime()
{
   // Si le WiFi est connecté, on tente une synchro NTP
   if (WiFi.status() == WL_CONNECTED)
   {
      DEBUG_PRINT(F("Tentative de synchronisation NTP..."));
      configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer);

      const int DELAY_BETWEEN_NTP_ATTEMPS = 1000;
      const int MAX_NTP_ATTEMPT = 10;
      int ntpAttempts = 0;
      time_t now;

      while (ntpAttempts < MAX_NTP_ATTEMPT)
      {
         time(&now);
         localtime_r(&now, &timeinfo);

         // Vérifie si l'année est cohérente (>2016)
         if (timeinfo.tm_year > (2016 - 1900))
         {
            DEBUG_PRINTLN(F("\nHeure NTP synchronisée."));
            return true;
         }

         ntpAttempts++;
         DEBUG_PRINT(F("."));
         delay(DELAY_BETWEEN_NTP_ATTEMPS);
      }

      DEBUG_PRINTLN(F("\nEchec de synchronisation NTP, utilisation de l'heure RTC."));
   }
   DEBUG_PRINTLN("");

   // Vérification de l'heure RTC si la synchronisation NTP a échoué ou si pas de WiFi
   if (!getLocalTime(&timeinfo, 0))
   {
      if (timeinfo.tm_year < (2016 - 1900))
      {
         DEBUG_PRINTLN(F("Echec de récupération de l'heure RTC, heure non définie."));
         return false;
      }
   }
   else
   {
      DEBUG_PRINTLN(F("Heure RTC utilisée."));
   }

   return true;
}

void TempoApp::displayLine(String text)
{
   if (currentLinePos > 150)
   {
      currentLinePos = 0;
      display.fillScreen(GxEPD_WHITE);
   }
   display.setTextColor(GxEPD_BLACK);
   display.setCursor(10, currentLinePos);
   display.print(text);
   currentLinePos += 10;
}

String TempoApp::getDayOfWeekInFrench(int dayOfWeek)
{
   const char *daysFrench[] = {"dim", "lun", "mar", "mer", "jeu", "ven", "sam"};
   return daysFrench[dayOfWeek % 7];
}

String TempoApp::getMonthInFrench(int month)
{
   const char *monthsFrench[] = {"jan", "fev", "mar", "avr", "mai", "juin", "juil", "aou", "sep", "oct", "nov", "dec"};
   return monthsFrench[(month - 1) % 12];
}

String TempoApp::getCurrentDateString()
{
   if (timeinfo.tm_year == 0)
   {
      DEBUG_PRINTLN(MSG_DATE_ERROR);
      return "";
   }

   String dayOfWeek = getDayOfWeekInFrench(timeinfo.tm_wday);
   String month = getMonthInFrench(timeinfo.tm_mon + 1);
   char dayMonthBuffer[10];
   snprintf(dayMonthBuffer, sizeof(dayMonthBuffer), "%02d %s", timeinfo.tm_mday, month.c_str());

   return dayOfWeek + " " + String(dayMonthBuffer);
}

String TempoApp::getNextDayDateString()
{
   if (timeinfo.tm_year == 0)
   {
      DEBUG_PRINTLN(MSG_DATE_ERROR);
      return "";
   }

   struct tm tomorrowTimeinfo(timeinfo);

   // Add one day to the current time
   tomorrowTimeinfo.tm_mday++;
   mktime(&tomorrowTimeinfo);

   String dayOfWeek = getDayOfWeekInFrench(tomorrowTimeinfo.tm_wday);
   String month = getMonthInFrench(tomorrowTimeinfo.tm_mon + 1);
   char dayMonthBuffer[10];
   snprintf(dayMonthBuffer, sizeof(dayMonthBuffer), "%02d %s", tomorrowTimeinfo.tm_mday, month.c_str());

   return dayOfWeek + " " + String(dayMonthBuffer);
}

void TempoApp::displayInfo()
{
   // Define layout parameters
   const int rotation = 1;
   const int leftMargin = 2;
   const int topMargin = 6;
   const int rectWidth = 120;
   const int rectHeight = 100;
   const int borderRadius = 8;
   const int topLineY = 30;
   const int separatorY = 50;
   const int colorTextY = 80;
   const int rectSpacing = 5; // Space between rectangles
   const int bottomIndicatorY = 120;
   const int circleRadius = 6;
   const int redRectWidth = 12;
   const int redRectHeight = 13;
   const int redRectRadius = 3;
   const int textOffsetX = 10;
   const int adjustTitleX = -3;
   const int textRemainOffsetX = 10;
   const int textRemainExclamationOffsetX = 15;
   const int textRemainOffsetY = 6;
   const int circleOffsetX = 90;
   const int exclamantionOffsetX = 65;
   // Set the display rotation
   display.setRotation(rotation);

   String todayString = getCurrentDateString();

   String tomorrowString = getNextDayDateString();

   // Calculate positions based on layout parameters
   int secondRectX = leftMargin + rectWidth + rectSpacing;

   // Draw the first rectangle (for today)
   display.drawRoundRect(leftMargin, topMargin, rectWidth, rectHeight, borderRadius, GxEPD_BLACK);
   // Draw date for today
   display.setFont(MEDIUM_FONT);
   display.setCursor(leftMargin + textOffsetX + adjustTitleX, topLineY);
   display.print(todayString);
   // Draw separator
   display.drawLine(leftMargin + textOffsetX, separatorY, rectWidth - textOffsetX, separatorY, GxEPD_BLACK);
   // Draw color for today
   display.setFont(LARGE_FONT);
   display.setCursor(leftMargin + textOffsetX, colorTextY);
   display.print(todayColor);

   // Draw battery Level
   drawBatteryInfo(12, 90);

   // Draw the second rectangle (for tomorrow)
   display.drawRoundRect(secondRectX, topMargin, rectWidth, rectHeight, borderRadius, GxEPD_BLACK);
   // Draw date for tomorrow
   display.setFont(MEDIUM_FONT);
   display.setCursor(secondRectX + textOffsetX + adjustTitleX, topLineY);
   display.print(tomorrowString);
   // Draw separator
   display.drawLine(secondRectX + textOffsetX, separatorY, secondRectX + rectWidth - textOffsetX, separatorY, GxEPD_BLACK);
   // Draw color for tomorrow
   display.setFont(LARGE_FONT);
   display.setCursor(secondRectX + textOffsetX, colorTextY);
   display.print(tomorrowColor);

   // Positioning for the bottom indicators
   int x_bleu = 15;
   int x_blanc = x_bleu + circleOffsetX;
   int x_rouge = x_blanc + exclamantionOffsetX;

   // Draw bottom indicators
   // Blue circle
   display.fillCircle(x_bleu, bottomIndicatorY, circleRadius, GxEPD_BLACK);
   display.setFont(MEDIUM_FONT);
   display.setCursor(x_bleu + textRemainOffsetX, bottomIndicatorY + textRemainOffsetY);

   if (DisplayOptions::AFFICHAGE_JOURS == DisplayOptions::RESTANTS)
   {
      display.print(remainingBlueDays + "/300");
   }
   else
   {
      char nbJours[16];
      sprintf(nbJours, "%ld/300", (300 - remainingBlueDays.toInt()));
      display.print(nbJours);
   }

   // White circle
   display.drawCircle(x_blanc, bottomIndicatorY, circleRadius, GxEPD_BLACK);
   display.setCursor(x_blanc + textRemainOffsetX, bottomIndicatorY + textRemainOffsetY);
   if (DisplayOptions::AFFICHAGE_JOURS == DisplayOptions::RESTANTS)
   {
      display.print(remainingWhiteDays + "/43");
   }
   else
   {
      char nbJours[16];
      sprintf(nbJours, "%ld/43", (43 - remainingWhiteDays.toInt()));
      display.print(nbJours);
   }

   // Red rounded rectangle
   display.drawRoundRect(x_rouge, bottomIndicatorY - redRectHeight / 2, redRectWidth, redRectHeight, redRectRadius, GxEPD_BLACK);
   // Exclamation mark: upper bar (double line for better visibility)
   int exclamationCenterX = x_rouge + redRectWidth / 2;
   display.drawLine(exclamationCenterX - 1, bottomIndicatorY - 4, exclamationCenterX - 1, bottomIndicatorY + 2, GxEPD_BLACK);
   display.drawLine(exclamationCenterX, bottomIndicatorY - 4, exclamationCenterX, bottomIndicatorY + 2, GxEPD_BLACK); // Adjacent line to thicken
   // Exclamation mark: lower dot (double line for better visibility)
   display.drawLine(exclamationCenterX - 1, bottomIndicatorY + 4, exclamationCenterX - 1, bottomIndicatorY + 4, GxEPD_BLACK);
   display.drawLine(exclamationCenterX, bottomIndicatorY + 4, exclamationCenterX, bottomIndicatorY + 4, GxEPD_BLACK); // Adjacent line to thicken

   // ROUGE
   display.setCursor(x_rouge + textRemainExclamationOffsetX, bottomIndicatorY + textRemainOffsetY);
   if (DisplayOptions::AFFICHAGE_JOURS == DisplayOptions::RESTANTS)
   {
      display.print(remainingRedDays + "/22");
   }
   else
   {
      char nbJours[16];
      sprintf(nbJours, "%ld/22", (22 - remainingRedDays.toInt()));
      display.print(nbJours);
   }

   // Ajout d'un symbole indiquant que le wifi ne s'est pas connecté
   if (!wifiSucceeded)
   {
      display.print(" w");
   }
}

void TempoApp::drawBatteryInfo(int batteryTopLeftX, int batteryTopLeftY)
{
   // Constants related to the battery drawing
   const int nbBars = 4;
   const int barWidth = 3;
   const int barHeight = 4;

   // Derived parameters computed from the given arguments and constants
   const int batteryWidth = (barWidth + 1) * nbBars + 2;
   const int batteryHeight = barHeight + 4;

   // Dans cette version, on utilise une interpolation pour la dernière barre,
   // afin d’éviter des transitions trop brutales entre les paliers.
   // Au lieu de dessiner un nombre entier de barres, on remplit partiellement
   // la dernière barre, ce qui offre une visualisation plus progressive.

   // Calcul du pourcentage
   float percentage = batteryMonitor.getPercentage();

   // Calcul du nombre de barres complètes
   int fullBars = static_cast<int>(percentage / 25.0f);

   // Calcul de la fraction de la barre suivante à remplir
   float remainder = (percentage / 25.0f) - fullBars;

   // On s'assure que fullBars reste dans les bornes
   if (fullBars > nbBars)
      fullBars = nbBars;
   if (fullBars < 0)
      fullBars = 0;

   // Dessin du contour de la batterie, inchangé
   display.drawLine(batteryTopLeftX, batteryTopLeftY, batteryTopLeftX + batteryWidth, batteryTopLeftY, GxEPD_BLACK);
   display.drawLine(batteryTopLeftX, batteryTopLeftY + batteryHeight, batteryTopLeftX + batteryWidth, batteryTopLeftY + batteryHeight, GxEPD_BLACK);
   display.drawLine(batteryTopLeftX, batteryTopLeftY, batteryTopLeftX, batteryTopLeftY + batteryHeight, GxEPD_BLACK);
   display.drawLine(batteryTopLeftX + batteryWidth, batteryTopLeftY, batteryTopLeftX + batteryWidth, batteryTopLeftY + batteryHeight, GxEPD_BLACK);

   // Pôle positif, inchangé
   display.drawLine(batteryTopLeftX + batteryWidth + 1, batteryTopLeftY + 1, batteryTopLeftX + batteryWidth + 1, batteryTopLeftY + (batteryHeight - 1), GxEPD_BLACK);
   display.drawLine(batteryTopLeftX + batteryWidth + 2, batteryTopLeftY + 1, batteryTopLeftX + batteryWidth + 2, batteryTopLeftY + (batteryHeight - 1), GxEPD_BLACK);

   // Dessin des barres pleines
   for (int j = 0; j < fullBars; j++)
   {
      for (int i = 0; i < barWidth; i++)
      {
         display.drawLine(batteryTopLeftX + 2 + (j * (barWidth + 1)) + i,
                          batteryTopLeftY + 2,
                          batteryTopLeftX + 2 + (j * (barWidth + 1)) + i,
                          batteryTopLeftY + 2 + barHeight,
                          GxEPD_BLACK);
      }
   }

   // Dessin partiel de la barre suivante si besoin
   if (fullBars < nbBars && remainder > 0.0f)
   {
      // On remplit une fraction du barWidth
      int partialWidth = static_cast<int>(barWidth * remainder);

      // On dessine uniquement la partie correspondant à partialWidth
      for (int i = 0; i < partialWidth; i++)
      {
         display.drawLine(batteryTopLeftX + 2 + (fullBars * (barWidth + 1)) + i,
                          batteryTopLeftY + 2,
                          batteryTopLeftX + 2 + (fullBars * (barWidth + 1)) + i,
                          batteryTopLeftY + 2 + barHeight,
                          GxEPD_BLACK);
      }
   }
}

// Fonction pour afficher un message d'échec
void TempoApp::displayFailureMessage()
{
   // Mode paysage
   display.setRotation(1);

   display.setTextColor(GxEPD_BLACK);
   display.setFont(LARGE_FONT);

   // Texte à afficher
   String message = ":-(";

   int16_t tbx, tby;
   uint16_t tbw, tbh;
   display.getTextBounds(message, 0, 0, &tbx, &tby, &tbw, &tbh);

   // Calcul pour centrer le message dans le tiers droit de l'écran
   int x = (2 * DisplayConfig::SCREEN_WIDTH / 3) + ((DisplayConfig::SCREEN_WIDTH / 3 - tbw) / 2);
   int y = (DisplayConfig::SCREEN_HEIGHT / 2) + (tbh / 2);

   display.setCursor(x, y);
   display.print(message);
}

bool TempoApp::getCurrentSeason(String &season)
{
   if (timeinfo.tm_year == 0)
   {
      DEBUG_PRINTLN(F("Echec de récupération de l'heure pour déterminer la saison."));
      return false;
   }

   int year = timeinfo.tm_year + 1900; // tm_year est l'année depuis 1900
   int month = timeinfo.tm_mon + 1;    // tm_mon est de [0,11]

   if (month >= 9)
   { // Septembre à Décembre
      season = String(year) + "-" + String(year + 1);
   }
   else
   { // Janvier à Août
      season = String(year - 1) + "-" + String(year);
   }

   return true;
}

bool TempoApp::fetchTempoInformation()
{
   // Valeurs par défaut au cas où aucun service ne fournisse d'informations
   TempoColor today = TempoColor::UNKNOWN;
   TempoColor tomorrow = TempoColor::UNKNOWN;

   if (WiFi.status() == WL_CONNECTED)
   {

      //"""

      // Define the season and period
      String season;
      bool success = getCurrentSeason(season);
      if (!success)
      {
         DEBUG_PRINTLN(MSG_SEASON_FETCH_ERROR);
         return false;
      }
      DEBUG_PRINT(F("Saison: "));
      DEBUG_PRINTLN(season);

      // Create the service manager
      TempoColorServiceManager manager;

      // Création d'une factory HTTP partagée
      auto httpClient = std::unique_ptr<IHttpClient>(HttpClientFactory::create());

      // Register API Couleur service
      auto couleurService = new ApiCouleurTempoColorService(season, httpClient.get());
      manager.registerService(couleurService);

      // Register RTE service
      auto rteService = new RteTempoColorService(season, httpClient.get());
      manager.registerService(rteService);

      // Register Tempo Light service
      auto tempoLightService = new TempoLightColorService(httpClient.get());
      manager.registerService(tempoLightService);

      // Register Commerce EDF service
      auto commerceEdfService = new CommerceEdfColorService(httpClient.get());
      manager.registerService(commerceEdfService);

      // Fetch data using the manager
      bool dataFetched = manager.fetchData();
      if (!dataFetched)
      {
         // Fetch invalide
         DEBUG_PRINTLN(MSG_FETCH_ERROR);

         return false;
      }

      today = manager.getTodayColor();
      tomorrow = manager.getTomorrowColor();
      todayColorEnum = today;
      tomorrowColorEnum = tomorrow;

      // Conversion en String - si la couleur n'est pas connue, on conserve
      // la valeur indiquant l'absence d'information.
      if (today == TempoColor::UNKNOWN)
      {
         todayColor = DAY_NOT_AVAILABLE;
      }
      else
      {
         todayColor = toString(today);
      }

      if (tomorrow == TempoColor::UNKNOWN)
      {
         tomorrowColor = DAY_NOT_AVAILABLE;
      }
      else
      {
         tomorrowColor = toString(tomorrow);
      }

      remainingRedDays = manager.getRedDaysPlaced();
      remainingWhiteDays = manager.getWhiteDaysPlaced();
      remainingBlueDays = manager.getBlueDaysPlaced();
   }
   else
   {
      DEBUG_PRINTLN(MSG_WIFI_NOT_CONNECTED);
   }

   // Mise à jour des indicateurs de disponibilité des couleurs
   isTodayColorFound = (today != TempoColor::UNKNOWN);
   isTomorrowColorFound = (tomorrow != TempoColor::UNKNOWN);
   return true;
}

// Dans getNextWakeupTime(), on clarifie la logique.
// On considère d'abord si on a besoin d'un retry, puis sinon on passe au planning normal.

time_t TempoApp::getNextWakeupTime()
{
   struct tm wakeUpTimeInfo;
   if (!getLocalTime(&wakeUpTimeInfo))
   {
      DEBUG_PRINTLN(MSG_WAKEUP_ERROR);
      return 0;
   }

   time_t now = mktime(&wakeUpTimeInfo);
   time_t nextWakeup = 0;
   bool found = false;

   // Si on a déjà toutes les infos, pas besoin de retry à 11h05
   // On réinitialise les tentatives
   if (isTodayColorFound && isTomorrowColorFound)
   {
      retryAttempts = RetryConfig::INITIAL_RETRY_ATTEMPTS;
   }

   // Parcours des horaires de réveil
   for (WakeupTime wakeup : wakeupTimes)
   {
      // Si nous n’avons pas encore toutes les infos ou qu’un retry est nécessaire
      // et que le créneau est marqué comme retry
      if ((!isTodayColorFound || !isTomorrowColorFound) && wakeup.retry && retryAttempts > 0)
      {
         // Dans ce cas, on ne se réveille pas à l’heure fixée mais on fait un retry rapide
         nextWakeup = now + RetryConfig::RETRY_DELAY_MINUTES * 60;
         retryAttempts -= 1;
         found = true;
         break;
      }
      else
      {
         // Si on a toutes les infos (pas de retry nécessaire) et que l’heure est un créneau retry (ex: 11h05)
         // on skip ce créneau
         if ((isTodayColorFound && isTomorrowColorFound) && wakeup.retry)
         {
            continue;
         }

         // Sinon, on prend le prochain créneau horaire si plus tard que now
         struct tm futureTime = wakeUpTimeInfo;
         futureTime.tm_hour = wakeup.hour;
         futureTime.tm_min = wakeup.minute;
         futureTime.tm_sec = 0;
         time_t futureTimestamp = mktime(&futureTime);

         if (futureTimestamp > now)
         {
            nextWakeup = futureTimestamp;
            found = true;
            break;
         }
      }
   }

   // Si aucun créneau trouvé, on passe au lendemain, premier horaire
   if (!found)
   {
      struct tm nextDayTime = wakeUpTimeInfo;
      nextDayTime.tm_mday += 1;
      nextDayTime.tm_hour = wakeupTimes[0].hour;
      nextDayTime.tm_min = wakeupTimes[0].minute;
      nextDayTime.tm_sec = 0;
      mktime(&nextDayTime);
      nextWakeup = mktime(&nextDayTime);
   }

   return nextWakeup;
}

void TempoApp::goToDeepSleepUntilNextWakeup()
{
   // Récupération de l'heure du prochain réveil
   time_t nextWakeupTime = getNextWakeupTime();
   if (nextWakeupTime == 0)
   {
      // Aucune heure de réveil valide trouvée. Mode veille infini.
      esp_deep_sleep_start();
      return;
   }

   // Récupération de l'heure actuelle
   time_t now;
   time(&now);
   if (now > nextWakeupTime)
   {
      // Erreur: L'heure du prochain réveil est antérieure à l'heure actuelle.
      return;
   }

   // Calcul de la durée du sommeil en secondes
   time_t sleepDuration = nextWakeupTime - now;

   // Réutilisation du buffer pour afficher les dates
   char timeBuffer[64];
   struct tm *localTimeInfo;

   // Formatage et affichage de l'heure courante
   localTimeInfo = localtime(&now);
   strftime(timeBuffer, sizeof(timeBuffer), "%A, %B %d %Y %H:%M:%S", localTimeInfo);
   DEBUG_PRINT(F("Date actuelle    : "));
   DEBUG_PRINTLN(timeBuffer);

   // Formatage et affichage de l'heure du prochain réveil
   localTimeInfo = localtime(&nextWakeupTime);
   strftime(timeBuffer, sizeof(timeBuffer), "%A, %B %d %Y %H:%M:%S", localTimeInfo);
   DEBUG_PRINT(F("Prochain réveil  : "));
   DEBUG_PRINTLN(timeBuffer);

   // Affichage de la durée de sommeil
   int hours = sleepDuration / 3600;
   int minutes = (sleepDuration % 3600) / 60;
   int seconds = sleepDuration % 60;
   sprintf(timeBuffer, "%d heures %d minutes %d secondes", hours, minutes, seconds);
   DEBUG_PRINT(F("Durée de sommeil : "));
   DEBUG_PRINTLN(timeBuffer);

   // Configuration du réveil par le timer et mise en sommeil profond
   esp_sleep_enable_timer_wakeup((uint64_t)sleepDuration * 1000000ULL);
   DEBUG_PRINTLN(F("Hibernate sequence initiated. I’ll be back."));
#ifdef ARDUINO
   WiFi.disconnect(true);
   WiFi.mode(WIFI_OFF);
#endif
   esp_deep_sleep_start();
}

#ifdef DEBUG_GRID
void TempoApp::drawDebugGrid()
{
   int gridSpacing = 10;                           // Espacement entre les lignes de la grille
   int screenWidth = DisplayConfig::SCREEN_HEIGHT; // mode portrait
   int screenHeight = DisplayConfig::SCREEN_WIDTH;

   // Dessiner des lignes verticales
   for (int x = 0; x <= screenWidth; x += gridSpacing)
   {
      display.drawLine(x, 0, x, screenHeight, GxEPD_BLACK);
   }

   // Dessiner des lignes horizontales
   for (int y = 0; y <= screenHeight; y += gridSpacing)
   {
      display.drawLine(0, y, screenWidth, y, GxEPD_BLACK);
   }
}
#endif
#endif
