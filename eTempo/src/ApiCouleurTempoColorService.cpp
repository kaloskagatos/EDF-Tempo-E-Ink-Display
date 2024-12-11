// ApiCouleurTempoColorService.cpp
#include "ApiCouleurTempoColorService.h"
#include <ctime>

const char ApiCouleurTempoColorService::baseUrl_PROGMEM[] PROGMEM = "https://www.api-couleur-tempo.fr/api/joursTempo?periode=";
const char ApiCouleurTempoColorService::dateJourKey_PROGMEM[] PROGMEM = "dateJour";
const char ApiCouleurTempoColorService::codeJourKey_PROGMEM[] PROGMEM = "codeJour";

ApiCouleurTempoColorService::ApiCouleurTempoColorService(const String &periode, IHttpClient* httpClient)
   : BaseTempoColorService(httpClient), periode_(periode)
{
}

String ApiCouleurTempoColorService::getUrl() const
{
   char baseUrl[100];
#ifdef ARDUINO
   strcpy_P(baseUrl, ApiCouleurTempoColorService::baseUrl_PROGMEM);
#else
   // Sous Windows, baseUrl_PROGMEM est normal, pas besoin de strcpy_P
   strcpy(baseUrl, ApiCouleurTempoColorService::baseUrl_PROGMEM);
#endif
   return String(baseUrl) + periode_;
}

bool ApiCouleurTempoColorService::parseData(const String &data)
{
   // Calcul des dates actuelles et de demain
   time_t now = time(nullptr);
   struct tm *today_tm = localtime(&now);
   char buffer[11];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", today_tm);
   String date_today(buffer);

   time_t tomorrow_time = now + 86400;
   struct tm *tomorrow_tm = localtime(&tomorrow_time);
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", tomorrow_tm);
   String date_tomorrow(buffer);

   blueDaysPlaced_ = 0;
   whiteDaysPlaced_ = 0;
   redDaysPlaced_ = 0;

   json doc;
   try
   {
      // En nlohmann::json, parse renvoie des std::string pour les strings
      doc = json::parse(data.c_str());
   }
   catch (json::parse_error &e)
   {
      DEBUG_PRINT(F("JSON parsing error: "));
      DEBUG_PRINTLN(e.what());
      return false;
   }

   if (!doc.is_array())
   {
      DEBUG_PRINTLN(F("Invalid JSON data (EDF): Expected an array."));
      return false;
   }

   for (auto &jour : doc)
   {
      if (jour.contains("dateJour") && jour.contains("codeJour"))
      {
         // Récupérer la date en std::string
         std::string dateStd = jour["dateJour"].get<std::string>();
         // Convertir en String (Arduino ou std::string selon la plateforme)
         String date = dateStd.c_str();

         int code = jour["codeJour"].get<int>();
         TempoColor tempoColor = codeToTempoColor(code);

         // Comparaison compatible dans les deux cas
         if (date == date_today)
         {
            todayColor_ = tempoColor;
         }
         if (date == date_tomorrow)
         {
            tomorrowColor_ = tempoColor;
         }

         if (tempoColor == TempoColor::BLUE)
            blueDaysPlaced_++;
         else if (tempoColor == TempoColor::WHITE)
            whiteDaysPlaced_++;
         else if (tempoColor == TempoColor::RED)
            redDaysPlaced_++;
      }
   }
   return true;
}

TempoColor ApiCouleurTempoColorService::codeToTempoColor(int code) const
{
   switch (code)
   {
      case 1:
      return TempoColor::BLUE;
      case 2:
      return TempoColor::WHITE;
      case 3:
      return TempoColor::RED;
      default:
      return TempoColor::UNKNOWN;
   }
}
