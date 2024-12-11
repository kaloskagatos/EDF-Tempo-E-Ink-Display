#include "CommerceEdfColorService.h"


CommerceEdfColorService::CommerceEdfColorService(IHttpClient* httpClient)
   : BaseTempoColorService(httpClient)
{
   // Calcul de la date d'aujourd'hui
   time_t now = time(nullptr);
   struct tm *today_tm = localtime(&now);
   char buffer[11];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", today_tm);
   dateReference_= String(buffer);
}

String CommerceEdfColorService::getUrl() const
{
   return String(F("https://api-commerce.edf.fr/commerce/activet/v1/saisons/search?option=TEMPO&dateReference=")) + dateReference_;
}


bool CommerceEdfColorService::parseData(const String &data)
{
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

   if (!doc.contains("content") || !doc["content"].is_array() || doc["content"].empty())
   {
      DEBUG_PRINTLN(F("Invalid JSON data (Commerce EDF): 'content' key missing or empty."));
      return false;
   }

   // On itère sur tous les objets du tableau "content"
   bool firstElement = true;
   for (auto& item : doc["content"])
   {
      // On suppose que les clés existent toujours comme spécifié par l'API
      std::string typeJourEffStd = item["typeJourEff"].get<std::string>();
      int nombreJoursTires = item["nombreJoursTires"].get<int>();

      // Conversion du type de jour en TempoColor
      TempoColor c = TempoColor::UNKNOWN;
      {
         String typeJourEff = typeJourEffStd.c_str();
         if (typeJourEff == F("TEMPO_ROUGE"))
         {
            c = TempoColor::RED;
         }
         else if (typeJourEff == F("TEMPO_BLANC"))
         {
            c = TempoColor::WHITE;
         }
         else if (typeJourEff == F("TEMPO_BLEU"))
         {
            c = TempoColor::BLUE;
         }
      }

      // Si c'est le premier élément, on considère que c'est la couleur du jour
      if (firstElement)
      {
         todayColor_ = c;
         tomorrowColor_ = TempoColor::UNKNOWN;
         firstElement = false;
      }

      // Mise à jour du compteur pour la couleur correspondante
      switch (c)
      {
         case TempoColor::BLUE:
            blueDaysPlaced_ = nombreJoursTires;
            break;
         case TempoColor::WHITE:
            whiteDaysPlaced_ = nombreJoursTires;
            break;
         case TempoColor::RED:
            redDaysPlaced_ = nombreJoursTires;
            break;
         default:
            // UNKNOWN, on ne fait rien
            break;
      }
   }

   return true;
}
