// ApiCouleurTempoColorService.h
#pragma once

#include "BaseTempoColorService.h"
#include "Platform.h"

/**
 * Service basé sur l'API couleur tempo EDF, inclut les jours placés.
 *
 * A enregistrer en priorité pour soulager les serveurs RTE
 * voir https://www.api-couleur-tempo.fr/
 *
 * Exemple d'URL :
 * https://www.api-couleur-tempo.fr/api/joursTempo?periode=2024-2025
 */
class ApiCouleurTempoColorService : public BaseTempoColorService
{
public:
    ApiCouleurTempoColorService(const String &periode, IHttpClient* httpClient);

    String getUrl() const override;

protected:
    bool parseData(const String &data) override;

private:
    String periode_;
    TempoColor codeToTempoColor(int code) const;

    static const char baseUrl_PROGMEM[];
    static const char dateJourKey_PROGMEM[];
    static const char codeJourKey_PROGMEM[];
};
