// RteTempoColorService.h
#pragma once

#include "BaseTempoColorService.h"
#include "Platform.h"


/**
 * Service basé sur les données RTE Tempo, inclut les jours placés.
 *
 * Exemple d'URL :
 * https://www.services-rte.com/cms/open_data/v1/tempo?season=2024-2025
 */

class RteTempoColorService : public BaseTempoColorService
{
public:
    RteTempoColorService(const String &season, IHttpClient* httpClient);

    String getUrl() const override;

protected:
    bool parseData(const String &data) override;

private:
    String season_;
    TempoColor stringToTempoColor(const String &colorStr) const;
    void incrementDayCount(TempoColor tempoColor);

    static const char baseUrl_PROGMEM[];
    static const char valuesKey_PROGMEM[];
    static const char dateJourKey_PROGMEM[];
    static const char codeJourKey_PROGMEM[];
};
