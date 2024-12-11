#pragma once

#include "ITempoColorService.h"
#include "IHttpClient.h"
#include "Platform.h"

class BaseTempoColorService : public ITempoColorService
{
public:
    explicit BaseTempoColorService(IHttpClient* httpClient);
    virtual ~BaseTempoColorService() = default;

    // Implémentation des méthodes de ITempoColorService
    TempoColor getTodayColor() const override;
    TempoColor getTomorrowColor() const override;

    int getBlueDaysPlaced() const override;
    int getWhiteDaysPlaced() const override;
    int getRedDaysPlaced() const override;

    // Implémentation de fetchData()
    bool fetchData() override;

   protected:
    virtual bool parseData(const String &data) = 0;

    IHttpClient* httpClient_;
    TempoColor todayColor_ = TempoColor::UNKNOWN;
    TempoColor tomorrowColor_ = TempoColor::UNKNOWN;
    int blueDaysPlaced_ = 0;
    int whiteDaysPlaced_ = 0;
    int redDaysPlaced_ = 0;
};
