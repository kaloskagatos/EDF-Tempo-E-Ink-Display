#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "time.h"
#include <WiFiManager.h> 
#include <math.h>

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Preferences.h>

Preferences preferences;


#define DEBUG_GRID 0
#define DEBUG_RTE false

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16);
GxEPD_Class display(io, /*RST=*/16, /*BUSY=*/4);

const char *ntpServer = "pool.ntp.org";
const char *accessPointName = "TempoAP";

// Global variables to store TEMPO information
#define DAY_NOT_AVAILABLE "N/A" 
String todayColor = DAY_NOT_AVAILABLE;
String tomorrowColor = DAY_NOT_AVAILABLE;

String remainingBlueDays = "???";
String remainingWhiteDays = "??";
String remainingRedDays = "??";

bool wifiSucceeded = true;
int currentLinePos = 0;


const int   PIN_BAT           = 35; //adc for bat voltage
// Seuils de tension pour la batterie (pour une batterie Li-ion)
const float VOLTAGE_100       = 4.2;     // Tension de batterie pleine
const float VOLTAGE_0         = 3.5;     // Tension de batterie vide

int batteryPercentage = 0;
float batteryVoltage = 0.0;

// Délai avant réessai en cas d'échec de récupération du lendemain
const int RETRY_DELAY_MIN = 1 ;
const int RETRY_COUNT = 3 ;

// Flag  indiquant qu'on a récupéré les infos du jour et de lendemain
bool isTodayColorFound = false ;
bool isTomorrowColorFound = false ;


void setup()
{
    preferences.begin("eTempo", false); 
    setlocale(LC_TIME, "fr_FR.UTF-8");

    Serial.begin(115200);
    Serial.println("Démarrage...\n");

    // récupérer le voltage de la carte 
    updateBatteryPercentage(batteryPercentage, batteryVoltage) ;
  
    // Initialize display
    display.init();
    displayLine("Initialisation...");
    displayLine("Adresse MAC:");
    displayLine(WiFi.macAddress().c_str());
    displayLine("Batterie:");
    char line[24];
    sprintf(line, "%5.3fv (%d%%)",batteryVoltage,batteryPercentage);
    displayLine(line);
    displayLine("SSID de config:");
    displayLine(accessPointName);
    display.update();

    // Connecter au WiFi
    if (!connectToWiFi()) {
      displayLine("Erreur de connexion");  
      display.update();
      Serial.println("Erreur de connexion WiFi, utilisation de l'heure RTC si disponible.");
      // Pas de gestion d'erreur, on tente d'utiliser la date RTC
      wifiSucceeded = false ;
    }

    // Initialiser l'heure
    if( !initializeTime() ) 
    {
      Serial.println("Erreur de synchronisation NTP: passage en deep sleep pendant 6 heures.");
      displayLine("Err de conn ou de synchro: deep sleep.");
      display.update();
      // Deep sleep for 6 hours
      esp_sleep_enable_timer_wakeup(6 * 60 * 60 * 1000000LL);
      esp_deep_sleep_start();
      return ;
    }

    // si affichage précédent
    display.fillScreen(GxEPD_WHITE);

    // Récupération des infos
    fetchTempoInformation();

    // Display info
    displayInfo();
    display.update();

    // Sommeil profond jusqu'à la prochaine heure de réveil
    goToDeepSleepUntilNextWakeup();
}

void loop()
{
    // Nothing to do here, device will go to deep sleep
}


bool connectToWiFi() {
    Serial.println("Lancement de WiFiManager...");
    WiFiManager wm;

    // Vous pouvez utiliser setConfigPortalTimeout pour définir un temps d'attente
    // pour le portail de configuration avant qu'il n'abandonne et redémarre l'ESP.
    wm.setConfigPortalTimeout(240);

    // Définissez ici le nombre de tentatives de reconnexion automatique
    wm.setConnectRetries(3);

    // Vous pouvez également définir un délai de connexion si nécessaire
    wm.setConnectTimeout(10); // Défini à 10 secondes pour chaque tentative

    // Essayez de se connecter avec les identifiants précédemment enregistrés,
    // sinon, ouvre un portail de configuration AP.
    bool res = wm.autoConnect(accessPointName); 

    if(!res) {
        Serial.println("Échec de connexion au WiFi et temps d'attente dépassé.");
        return false;
    }

    Serial.println("Connecté au WiFi.");
    return true;
}


void updateBatteryPercentage( int &percentage, float &voltage ) {
  // Lire la tension de la batterie
  voltage = analogRead(PIN_BAT) / 4096.0 * 7.05;
  percentage = 0;
  if (voltage > 1) { // Afficher uniquement si la lecture est valide
      percentage = static_cast<int>(2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303);
      // Ajuster le pourcentage en fonction des seuils de tension
      if (voltage >= VOLTAGE_100) {
          percentage = 100;
      } else if (voltage <= VOLTAGE_0) {
          percentage = 0;
      }
  }
}

bool initializeTime() {
  // If connected to WiFi, attempt to synchronize time with NTP
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Tentative de synchronisation NTP...");
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer); // Configure time zone to adjust for daylight savings

    const int maxNTPAttempts = 5;
    int ntpAttempts = 0;
    time_t now;
    struct tm timeinfo;
    while (ntpAttempts < maxNTPAttempts) {
      time(&now);
      localtime_r(&now, &timeinfo);

      if (timeinfo.tm_year > (2016 - 1900)) { // Check if the year is plausible
        Serial.println("NTP time synchronized!");
        return true;
      }

      ntpAttempts++;
      Serial.println("Attente de la synchronisation NTP...");
      delay(2000); // Delay between attempts to prevent overloading the server
    }

    Serial.println("Échec de synchronisation NTP, utilisation de l'heure RTC.");
  }

  // Regardless of WiFi or NTP sync, try to use RTC time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) { // Immediately return the RTC time without waiting
    if (timeinfo.tm_year < (2016 - 1900)) { // If year is not plausible, RTC time is not set
      Serial.println("Échec de récupération de l'heure RTC, veuillez vérifier si l'heure a été définie.");
      return false;
    }
  }

  Serial.println("Heure RTC utilisée.");
  return true;
}

void displayLine(String text)
{
  if (currentLinePos > 150) {
      currentLinePos = 0;
      display.fillScreen(GxEPD_WHITE);
  }
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10,currentLinePos);
  display.print(text);
  currentLinePos += 10; 
}

// Helper functions to get French abbreviations
String getDayOfWeekInFrench(int dayOfWeek) {
    const char* daysFrench[] = {"dim", "lun", "mar", "mer", "jeu", "ven", "sam"};
    return daysFrench[dayOfWeek % 7];  // Use modulo just in case
}

String getMonthInFrench(int month) {
    const char* monthsFrench[] = {"jan", "fev", "mar", "avr", "mai", "juin", "juil", "aou", "sep", "oct", "nov", "dec"};
    return monthsFrench[(month - 1) % 12];  // Use modulo and adjust since tm_mon is [0,11]
}

// Function to get current date in French abbreviated format
String getCurrentDateString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Echec de récupération de la date !");
        return "";
    }

    String dayOfWeek = getDayOfWeekInFrench(timeinfo.tm_wday);
    String month = getMonthInFrench(timeinfo.tm_mon + 1); // tm_mon is months since January - [0,11]
    char dayMonthBuffer[10];
    snprintf(dayMonthBuffer, sizeof(dayMonthBuffer), "%02d %s", timeinfo.tm_mday, month.c_str());        
        
    return dayOfWeek + " " + String(dayMonthBuffer);
}

// Function to get next day's date in French abbreviated format
String getNextDayDateString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Echec de récupération de la date !");
        return "";
    }

    // Add one day to the current time
    timeinfo.tm_mday++;
    mktime(&timeinfo); // Normalize the tm structure after manual increment

    String dayOfWeek = getDayOfWeekInFrench(timeinfo.tm_wday);
    String month = getMonthInFrench(timeinfo.tm_mon + 1);
    char dayMonthBuffer[10];
    snprintf(dayMonthBuffer, sizeof(dayMonthBuffer), "%02d %s", timeinfo.tm_mday, month.c_str());
    
    return dayOfWeek + " " + String(dayMonthBuffer);
}

void displayInfo() {
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
    const int rectSpacing = 5;  // Space between rectangles
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
    display.setFont(&FreeSans9pt7b);
    display.setCursor(leftMargin + textOffsetX + adjustTitleX, topLineY);
    display.print(todayString);
    // Draw separator
    display.drawLine(leftMargin + textOffsetX, separatorY, rectWidth - textOffsetX, separatorY, GxEPD_BLACK);
    // Draw color for today
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(leftMargin + textOffsetX, colorTextY);
    display.print(todayColor);

    // Draw battery Level
    const int batteryTopMargin = 10;
    const int nbBars = 4;
    const int barWidth = 3;
    const int batteryWidth = (barWidth + 1) * nbBars + 2;
    const int barHeight = 4;
    const int batteryHeight = barHeight + 4; 
    const int batteryTopLeftX = leftMargin + textOffsetX;
    const int batteryTopLeftY = colorTextY + batteryTopMargin;

    // Horizontal
    display.drawLine(batteryTopLeftX, batteryTopLeftY, batteryTopLeftX + batteryWidth, batteryTopLeftY , GxEPD_BLACK);
    display.drawLine(batteryTopLeftX, batteryTopLeftY + batteryHeight, batteryTopLeftX + batteryWidth, batteryTopLeftY + batteryHeight, GxEPD_BLACK);
    // Vertical
    display.drawLine(batteryTopLeftX, batteryTopLeftY, batteryTopLeftX, batteryTopLeftY + batteryHeight, GxEPD_BLACK);
    display.drawLine(batteryTopLeftX + batteryWidth, batteryTopLeftY, batteryTopLeftX + batteryWidth, batteryTopLeftY + batteryHeight, GxEPD_BLACK);
    // + Pole
    display.drawLine(batteryTopLeftX + batteryWidth + 1, batteryTopLeftY + 1, batteryTopLeftX + batteryWidth + 1, batteryTopLeftY + (batteryHeight - 1), GxEPD_BLACK);
    display.drawLine(batteryTopLeftX + batteryWidth + 2, batteryTopLeftY + 1, batteryTopLeftX + batteryWidth + 2, batteryTopLeftY + (batteryHeight - 1), GxEPD_BLACK);
    
    int i,j;
    int nbBarsToDraw = round(batteryPercentage / 25.0);
    for (j = 0; j < nbBarsToDraw; j++) {
      for(i = 0; i < barWidth; i++) {
        display.drawLine(batteryTopLeftX + 2 + (j * (barWidth + 1)) + i, batteryTopLeftY + 2, batteryTopLeftX + 2 + (j * (barWidth + 1)) + i, batteryTopLeftY + 2 + barHeight, GxEPD_BLACK);
      }
    }
    
    // Draw the second rectangle (for tomorrow)
    display.drawRoundRect(secondRectX, topMargin, rectWidth, rectHeight, borderRadius, GxEPD_BLACK);
    // Draw date for tomorrow
    display.setFont(&FreeSans9pt7b);
    display.setCursor(secondRectX + textOffsetX + adjustTitleX, topLineY);
    display.print(tomorrowString);
    // Draw separator
    display.drawLine(secondRectX + textOffsetX, separatorY, secondRectX + rectWidth - textOffsetX, separatorY, GxEPD_BLACK);
    // Draw color for tomorrow
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(secondRectX + textOffsetX, colorTextY);
    display.print(tomorrowColor);

    // Positioning for the bottom indicators
    int x_bleu = 15;
    int x_blanc = x_bleu + circleOffsetX;
    int x_rouge = x_blanc + exclamantionOffsetX  ;

    // Draw bottom indicators
    // Blue circle
    display.fillCircle(x_bleu, bottomIndicatorY, circleRadius, GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(x_bleu + textRemainOffsetX, bottomIndicatorY + textRemainOffsetY);
    display.print(remainingBlueDays + "/300");

    // White circle
    display.drawCircle(x_blanc, bottomIndicatorY, circleRadius, GxEPD_BLACK);
    display.setCursor(x_blanc + textRemainOffsetX, bottomIndicatorY + textRemainOffsetY);
    display.print(remainingWhiteDays + "/43");

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
    display.setCursor(x_rouge + textRemainExclamationOffsetX , bottomIndicatorY + textRemainOffsetY);
    display.print(remainingRedDays + "/22");

    // Ajout d'un symbole indiquant que le wifi ne s'est pas connecté
    if( !wifiSucceeded ){
      display.print( " w");
    }
}

String mapTempoColor(const String& tempoColor) {
    if (tempoColor == "TEMPO_BLEU") return "BLEU";
    if (tempoColor == "TEMPO_BLANC") return "BLANC";
    if (tempoColor == "TEMPO_ROUGE") return "ROUGE";
    if (tempoColor == "NON_DEFINI") return "???";
    return tempoColor; // If it's an unrecognized value, return as is.
}

String mapRteTempoColor(const String& tempoColor) {
    if (tempoColor == "BLUE") return "BLEU*";
    if (tempoColor == "WHITE") return "BLANC*";
    if (tempoColor == "RED") return "ROUGE*";
    return tempoColor; // If it's an unrecognized value, return as is.
}

void fetchTempoInformation() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            Serial.println("Echec de récupération de la date !");
            return;
        }
        char todayDate[11];
        strftime(todayDate, sizeof(todayDate), "%Y-%m-%d", &timeinfo);
        String todayDateString = String(todayDate);

        // First API to get the colors of the day and the next day
        String tempoStoreUrl = "https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=" + todayDateString;
        http.begin(tempoStoreUrl);
        int httpCode = http.GET();

        if (httpCode > 0) {
            DynamicJsonDocument doc(1024);
            String payload = http.getString();
            deserializeJson(doc, payload);
            if (doc.containsKey("couleurJourJ")) {
              todayColor = mapTempoColor(doc["couleurJourJ"].as<String>());
            }
            if (doc.containsKey("couleurJourJ1")) {
              tomorrowColor = mapTempoColor(doc["couleurJourJ1"].as<String>());
            }
        } else {
            Serial.println("Échec de la récupération des couleurs d'aujourd'hui et de demain, code HTTP : " + String(httpCode));
        }
        http.end();

        if (DEBUG_RTE || (todayColor != "???" && tomorrowColor == "???")) {
          // En théorie on est un appel avant 11h du matin, la couleur de demain est encore inconnue.
          // On tente un appel de l'api RTE
          Serial.println("Preview RTE");
          tempoStoreUrl = "https://www.services-rte.com/cms/open_data/v1/tempoLight";
          http.begin(tempoStoreUrl);
          int httpCode = http.GET();

          if (httpCode > 0) {
              DynamicJsonDocument doc(1024);
              String payload = http.getString();
              deserializeJson(doc, payload);
              // il faut récupérer l'éventuelle valeur de demain
              // formatage de la date de demain
              
              timeinfo.tm_mday++;
              mktime(&timeinfo); // Normalize the tm structure after manual increment

              char tomorrowDate[11];
              strftime(tomorrowDate, sizeof(tomorrowDate), "%Y-%m-%d", &timeinfo);
              String tomorrowDateString = String(tomorrowDate);
              if (doc.containsKey("values") && doc["values"].containsKey(tomorrowDateString)) {
                // WHITE RED BLUE
                tomorrowColor = mapRteTempoColor(doc["values"][tomorrowDateString].as<String>());
                Serial.println("Couleur preview de demain trouvée");
                Serial.println(doc["values"][tomorrowDateString].as<String>());
              } else {
                Serial.println("Couleur preview de demain absente");
              }
          } else {
              Serial.println("Échec de la récupération des couleurs preview RTE, code HTTP : " + String(httpCode));
          }
          http.end();
        }

        // Second API to get the remaining days for each color
        String nbTempoDaysUrl = "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO";
        http.begin(nbTempoDaysUrl);
        httpCode = http.GET();

        if (httpCode > 0) {
            DynamicJsonDocument doc(1024);
            String payload = http.getString();
            deserializeJson(doc, payload);
            if (doc.containsKey("PARAM_NB_J_ROUGE")) {
              remainingRedDays = doc["PARAM_NB_J_ROUGE"].as<String>();
            }
            if (doc.containsKey("PARAM_NB_J_BLANC")) {
              remainingWhiteDays = doc["PARAM_NB_J_BLANC"].as<String>();
            }
            if (doc.containsKey("PARAM_NB_J_BLEU")) {
              remainingBlueDays = doc["PARAM_NB_J_BLEU"].as<String>();
            }
        } else {
            Serial.println("Échec de la récupération des jours restants pour les couleurs, code HTTP : " + String(httpCode));
        }
        http.end();

        // Print the retrieved information
        Serial.println("Couleur d'aujourd'hui: " + todayColor);
        Serial.println("Couleur de demain: " + tomorrowColor);
        Serial.println("Jours Rouges restants: " + remainingRedDays);
        Serial.println("Jours Blancs restants: " + remainingWhiteDays);
        Serial.println("Jours Bleus restants: " + remainingBlueDays);
    } else {
        Serial.println("Wi-Fi non connecté !");
    }


    // Fetch potentiellement nécessaire
    isTodayColorFound = strcmp( todayColor.c_str(), DAY_NOT_AVAILABLE) != 0 ;
    isTomorrowColorFound = strcmp( tomorrowColor.c_str(), DAY_NOT_AVAILABLE) != 0 ;;
}

// Structure pour stocker les heures de réveil
struct WakeupTime {
  int hour;
  int minute;
  bool retry;
};

// Tableau des heures de réveil
const WakeupTime wakeupTimes[] = {
  {0, 5, false},  // Réveil à 00:05
  {7, 5, false},  // Réveil à 07:05 pour préview RTE
  {11, 5, true}  // Réveil à 11:05
}; 

// Fonction pour obtenir le temps actuel sous forme de structure tm
bool getCurrentTime(struct tm *timeinfo) {
  if (!getLocalTime(timeinfo)) {
    Serial.println("Échec de l'obtention de l'heure");
    return false;
  }
  return true;
}

// Fonction pour calculer la prochaine heure de réveil
time_t getNextWakeupTime() {
 struct tm timeinfo;
  if (!getCurrentTime(&timeinfo)) {
    return 0; // Retourner 0 si l'heure n'a pas pu être obtenue
  }

  // Les 2 couleurs ont été trouvée, on réinitialise le nombre d'essais
  if( isTodayColorFound && isTomorrowColorFound)
  {
    preferences.putInt("retryCount", RETRY_COUNT); // Décrémenter le nombre de tentatives
  }

  time_t now = mktime(&timeinfo);
  time_t nextWakeup = 0;
  bool found = false;
  int retryAttempts = preferences.getInt("retryCount", RETRY_COUNT); // Nombre de tentatives restantes

  for (WakeupTime wakeup : wakeupTimes) {
    if ( (!isTodayColorFound || ( !isTomorrowColorFound && wakeup.retry))  && retryAttempts > 0 ) {
      // Si réessai est nécessaire et il reste des tentatives
      nextWakeup = now + RETRY_DELAY_MIN * 60; // Programmer le réveil pour 5 minutes plus tard
      preferences.putInt("retryCount", retryAttempts - 1); // Décrémenter le nombre de tentatives
      found = true;
      break;
    } else {
      struct tm futureTime = timeinfo;
      futureTime.tm_hour = wakeup.hour;
      futureTime.tm_min = wakeup.minute;
      futureTime.tm_sec = 0;
      time_t futureTimestamp = mktime(&futureTime);

      if (futureTimestamp > now) {
        // Si l'heure de réveil est dans le futur, c'est le prochain réveil
        nextWakeup = futureTimestamp;
        found = true;
        break;
      }
    }
  }

  // Si aucun réveil futur n'a été trouvé, prendre le premier réveil du lendemain
  if (!found) {
    struct tm nextDayTime = timeinfo;
    nextDayTime.tm_mday += 1; // Ajouter un jour
    nextDayTime.tm_hour = wakeupTimes[0].hour;
    nextDayTime.tm_min = wakeupTimes[0].minute;
    nextDayTime.tm_sec = 0;
    mktime(&nextDayTime); // Normaliser la structure tm après modification manuelle
    nextWakeup = mktime(&nextDayTime); // mktime will handle the end of month/year
  }

  return nextWakeup;
}

void goToDeepSleepUntilNextWakeup() {
  time_t nextWakeupTime = getNextWakeupTime();
  if (nextWakeupTime == 0) {
    Serial.println("Aucune heure de réveil valide trouvée. Passage en mode veille indéfiniment.");
    esp_deep_sleep_start();
    return;
  }

  // Convertir le temps de réveil en structure tm pour l'affichage
  struct tm* nextWakeupTm = localtime(&nextWakeupTime);
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S", nextWakeupTm);

  // Afficher la prochaine date de réveil
  Serial.print("L'heure du prochain réveil est : ");
  Serial.println(buffer);

  // Calculer la durée de sommeil en secondes
  time_t now;
  time(&now);
  time_t sleepDuration = nextWakeupTime - now;


    // Convertir l'heure courante en structure tm pour l'affichage
  struct tm* currentTimeTm = localtime(&now);
  char currentTimeBuffer[64];
  strftime(currentTimeBuffer, sizeof(currentTimeBuffer), "%A, %B %d %Y %H:%M:%S", currentTimeTm);

  // Afficher l'heure courante
  Serial.print("La date courante est: ");
  Serial.println(currentTimeBuffer);

  // Afficher la durée de sommeil en secondes
  Serial.print("Durée de sommeil en secondes: ");
  Serial.println(sleepDuration);

  // Configurer l'alarme pour se réveiller à la prochaine heure configurée
  esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
  Serial.println("Passage en mode sommeil profond jusqu'au prochain réveil.");
  esp_deep_sleep_start();
}


#if DEBUG_GRID
void drawDebugGrid()
{
    int gridSpacing = 10; // Espacement entre les lignes de la grille
    int screenWidth = 122;
    int screenHeight = 250;

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

