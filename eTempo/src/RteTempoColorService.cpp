#include "RteTempoColorService.h"

const char RteTempoColorService::baseUrl_PROGMEM[] PROGMEM = "https://www.services-rte.com/cms/open_data/v1/tempo?season=";
const char RteTempoColorService::valuesKey_PROGMEM[] PROGMEM = "values";
const char RteTempoColorService::dateJourKey_PROGMEM[] PROGMEM = "dateJour";
const char RteTempoColorService::codeJourKey_PROGMEM[] PROGMEM = "codeJour";

RteTempoColorService::RteTempoColorService(const String &season, IHttpClient* httpClient)
   : BaseTempoColorService(httpClient), season_(season)
{
}

String RteTempoColorService::getUrl() const
{
   char baseUrl[100];
#ifdef ARDUINO
   strcpy_P(baseUrl, RteTempoColorService::baseUrl_PROGMEM);
#else
   strcpy(baseUrl, RteTempoColorService::baseUrl_PROGMEM);
#endif
   return String(baseUrl) + season_;
}

bool RteTempoColorService::parseData(const String &data)
{
   // Calcul des dates
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
      doc = json::parse(data.c_str());
   }
   catch (json::parse_error &e)
   {
      DEBUG_PRINT(F("JSON parsing error: "));
      DEBUG_PRINTLN(e.what());
      return false;
   }

   if (!doc.contains("values") || !doc["values"].is_object())
   {
      DEBUG_PRINTLN(F("Invalid JSON data (RTE): 'values' key missing or not an object."));
      return false;
   }

   json values = doc["values"];

   auto getColorForDate = [&](const String &d) -> TempoColor
   {
      std::string key = d.c_str();
      if (values.contains(key) && values[key].is_string())
      {
         std::string colorStd = values[key].get<std::string>();
         String color = colorStd.c_str();
         return stringToTempoColor(color);
      }
      return TempoColor::UNKNOWN;
   };

   todayColor_ = getColorForDate(date_today);
   tomorrowColor_ = getColorForDate(date_tomorrow);

   // Parcours de toutes les entrées
   for (auto it = values.begin(); it != values.end(); ++it)
   {
      if (it.value().is_string())
      {
         std::string colorStd = it.value().get<std::string>();
         String colorStr = colorStd.c_str();
         TempoColor tempoColor = stringToTempoColor(colorStr);
         incrementDayCount(tempoColor);
      }
   }
   return true;
}

void RteTempoColorService::incrementDayCount(TempoColor tempoColor)
{
   if (tempoColor == TempoColor::BLUE)
   {
      blueDaysPlaced_++;
   }
   else if (tempoColor == TempoColor::WHITE)
   {
      whiteDaysPlaced_++;
   }
   else if (tempoColor == TempoColor::RED)
   {
      redDaysPlaced_++;
   }
}

TempoColor RteTempoColorService::stringToTempoColor(const String &colorStr) const
{
   if( colorStr == "BLUE" )
   {
      return TempoColor::BLUE ;
   }

   if( colorStr == "WHITE" )
   {
      return TempoColor::WHITE ;
   }

   if( colorStr == "RED" )
   {
      return TempoColor::RED ;
   }

   return TempoColor::UNKNOWN ;
}
