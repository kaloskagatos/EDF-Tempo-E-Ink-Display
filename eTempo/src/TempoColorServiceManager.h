// TempoColorServiceManager.h
#pragma once

#include "ITempoColorService.h"
#include <vector>

// Autre source pour le jour seulement https://api-commerce.edf.fr/commerce/activet/v1/saisons/search?option=TEMPO&dateReference=2024-12-10


// Preview des jours en avance (fallback ?)
// https://www.services-rte.com/cms/open_data/v1/tempoLight

class TempoColorServiceManager
{
   public:
      TempoColorServiceManager() = default;
      ~TempoColorServiceManager() ;

      void registerService(ITempoColorService* service);

      bool fetchData() ;

      TempoColor getTodayColor() const ;
      TempoColor getTomorrowColor() const ;

      int getBlueDaysPlaced() const ;
      int getWhiteDaysPlaced() const ;
      int getRedDaysPlaced() const ;

      // Logue les résultats de tous les services enregistrés.
      void logServiceResults() const;

   private:
      std::vector<ITempoColorService*> services_;

      TempoColor finalTodayColor_ = TempoColor::UNKNOWN;
      TempoColor finalTomorrowColor_ = TempoColor::UNKNOWN;
      int blueDaysPlaced_ = 0;
      int whiteDaysPlaced_ = 0;
      int redDaysPlaced_ = 0;
};
