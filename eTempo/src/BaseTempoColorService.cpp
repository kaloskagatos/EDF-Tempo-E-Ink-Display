// BaseTempoColorService.cpp
#include "BaseTempoColorService.h"

BaseTempoColorService::BaseTempoColorService(IHttpClient* httpClient)
    : httpClient_(httpClient)
{
}

bool BaseTempoColorService::fetchData()
{
   String url = getUrl();
   String data = httpClient_->get(url);
   return parseData(data);
}

TempoColor BaseTempoColorService::getTodayColor() const
{
   return todayColor_;
}

TempoColor BaseTempoColorService::getTomorrowColor() const
{
   return tomorrowColor_;
}

int BaseTempoColorService::getBlueDaysPlaced() const
{
   return blueDaysPlaced_;
}

int BaseTempoColorService::getWhiteDaysPlaced() const
{
   return whiteDaysPlaced_;
}

int BaseTempoColorService::getRedDaysPlaced() const
{
   return redDaysPlaced_;
}
