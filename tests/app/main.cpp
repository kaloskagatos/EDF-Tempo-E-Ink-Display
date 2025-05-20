// main.cpp

#include "RteTempoColorService.h"
#include "ApiCouleurTempoColorService.h"
#include "TempoLightColorService.h"
#include "CommerceEdfColorService.h"
#include "TempoColor.h"
#include "HttpClientFactory.h"
#include "TempoColorServiceManager.h"
#include "BatteryMonitor.h"
#include <cassert>
#include <iostream>
#include <ctime>
#include "Platform.h"

int main()
{
   // Battery calibration basic test
   {
      BatteryMonitor monitor;
      monitor.setSimulatedVoltage(3.4f);
      monitor.getPercentage(); // mark low
      monitor.setSimulatedVoltage(4.25f);
      monitor.getPercentage(); // update max
      monitor.setSimulatedVoltage(4.25f);
      int pct = monitor.getPercentage();
      assert(pct == 100);
   }

   // Define the season and period
   const String season = "2024-2025";


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

   // Log DEBUG
   manager.logServiceResults() ;

   // Fetch data using the manager
   bool dataFetched = manager.fetchData();
   if (!dataFetched)
   {
      // Gérer le cas où aucune donnée valide n'a été récupérée
      DEBUG_PRINTLN(F("Erreur : Aucune donnée valide n'a été récupérée."));
      return -1 ;
   }

   // Get the data from the manager
   TempoColor finalTodayColor = manager.getTodayColor();
   TempoColor finalTomorrowColor = manager.getTomorrowColor();

   int blueDaysPlaced = manager.getBlueDaysPlaced();
   int whiteDaysPlaced = manager.getWhiteDaysPlaced();
   int redDaysPlaced = manager.getRedDaysPlaced();

   // Total days available (defined in the interface)
   const int totalBlueDays = ITempoColorService::TOTAL_BLUE_DAYS;
   const int totalWhiteDays = ITempoColorService::TOTAL_WHITE_DAYS;
   const int totalRedDays = ITempoColorService::TOTAL_RED_DAYS;

   // Get today's and tomorrow's dates
   time_t now = time(0);
   struct tm *today_tm = localtime(&now);
   char buffer[11];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", today_tm);
   String date_today(buffer);

   time_t tomorrow_time = now + 86400;
   struct tm *tomorrow_tm = localtime(&tomorrow_time);
   strftime(buffer, sizeof(buffer), "%Y-%m-%d", tomorrow_tm);
   String date_tomorrow(buffer);

   std::cout << "aujourdhui         : " << toString(finalTodayColor) << std::endl ;
   std::cout << "demain             : " << toString(finalTomorrowColor) << std::endl;
   std::cout << "date_aujourdhui    : " << date_today << std::endl;
   std::cout << "date_demain        : " << date_tomorrow << std::endl;
   std::cout << "jours_bleus_place  : " << blueDaysPlaced << " / " << totalBlueDays << std::endl;
   std::cout << "jours_blancs_place : " << whiteDaysPlaced << " / " << totalWhiteDays << std::endl;
   std::cout << "jours_rouges_place : " << redDaysPlaced << " / " << totalRedDays << std::endl;

   return 0;
}
