// TempoLightColorService.h

#pragma once

#include "BaseTempoColorService.h"
#include "Platform.h"


/**
 * Couleur du jour et du lendemain uniquement, pas de jours placés.
 *
 * Exemple d'URL :
 * https://www.services-rte.com/cms/open_data/v1/tempoLight
 */

class TempoLightColorService : public BaseTempoColorService
{
public:
    TempoLightColorService(IHttpClient* httpClient);

    String getUrl() const override;

protected:
    bool parseData(const String &data) override;

private:
    TempoColor stringToTempoColor(const String &colorStr) const;
};
