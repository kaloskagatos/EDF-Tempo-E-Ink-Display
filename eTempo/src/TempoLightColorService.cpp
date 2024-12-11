#include "TempoLightColorService.h"

TempoLightColorService::TempoLightColorService(IHttpClient* httpClient)
   : BaseTempoColorService(httpClient) {}

String TempoLightColorService::getUrl() const
{
   return String(F("https://www.services-rte.com/cms/open_data/v1/tempoLight"));
}

bool TempoLightColorService::parseData(const String &data)
{
   blueDaysPlaced_ = 0;
   whiteDaysPlaced_ = 0;
   redDaysPlaced_ = 0;

   time_t now = time(nullptr);
   struct tm *today_tm = localtime(&now);
   char buffer[11];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", today_tm);
   String date_today(buffer);

   time_t tomorrow_time = now + 86400;
   struct tm *tomorrow_tm = localtime(&tomorrow_time);
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", tomorrow_tm);
   String date_tomorrow(buffer);

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
      DEBUG_PRINTLN(F("Invalid JSON data (Tempo Light): 'values' key missing or not object."));
      return false;
   }

   json values = doc["values"];

   auto getColorForDate = [&](const String &d) -> TempoColor
   {
      std::string key = d.c_str();
      if (values.contains(key) && values[key].is_string())
      {
         std::string colorStd = values[key].get<std::string>();
         String colorStr = colorStd.c_str();
         return stringToTempoColor(colorStr);
      }
      return TempoColor::UNKNOWN;
   };

   todayColor_ = getColorForDate(date_today);
   tomorrowColor_ = getColorForDate(date_tomorrow);

   return true;
}

TempoColor TempoLightColorService::stringToTempoColor(const String &colorStr) const
{
   auto toLower = [](const String &s)
   {
      String lower = s;
      for (auto &c : lower)
         c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
      return lower;
   };
   String lowerColor = toLower(colorStr);
   if (lowerColor == "blue" || lowerColor == "bleu")
      return TempoColor::BLUE;
   if (lowerColor == "white" || lowerColor == "blanc")
      return TempoColor::WHITE;
   if (lowerColor == "red" || lowerColor == "rouge")
      return TempoColor::RED;

   return TempoColor::UNKNOWN;
}
