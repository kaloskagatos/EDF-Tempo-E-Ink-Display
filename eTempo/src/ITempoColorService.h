// ITempoColorService.h

#pragma once

#include "TempoColor.h"

class ITempoColorService {
   public:
      virtual ~ITempoColorService() = default;

      // Methods to get today's and tomorrow's color
      virtual TempoColor getTodayColor() const = 0;
      virtual TempoColor getTomorrowColor() const = 0;

      // Methods to get the number of placed days for each color
      virtual int getBlueDaysPlaced() const = 0;
      virtual int getWhiteDaysPlaced() const = 0;
      virtual int getRedDaysPlaced() const = 0;

      // Method to fetch data
      virtual bool fetchData() = 0;

      // Method to getUrl
      virtual String getUrl() const = 0;

      // Constants for the total number of available days for each color
      static constexpr int TOTAL_BLUE_DAYS = 300;
      static constexpr int TOTAL_WHITE_DAYS = 43;
      static constexpr int TOTAL_RED_DAYS = 22;
};
