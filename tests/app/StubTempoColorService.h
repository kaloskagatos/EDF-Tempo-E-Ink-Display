#pragma once
#include "ITempoColorService.h"

class StubTempoColorService : public ITempoColorService {
public:
    StubTempoColorService(TempoColor today, TempoColor tomorrow,
                          int bluePlaced, int whitePlaced, int redPlaced)
        : todayColor(today), tomorrowColor(tomorrow),
          blue(bluePlaced), white(whitePlaced), red(redPlaced) {}

    TempoColor getTodayColor() const override { return todayColor; }
    TempoColor getTomorrowColor() const override { return tomorrowColor; }

    int getBlueDaysPlaced() const override { return blue; }
    int getWhiteDaysPlaced() const override { return white; }
    int getRedDaysPlaced() const override { return red; }

    bool fetchData() override { return true; }

    String getUrl() const override { return String("stub://"); }

private:
    TempoColor todayColor;
    TempoColor tomorrowColor;
    int blue;
    int white;
    int red;
};
