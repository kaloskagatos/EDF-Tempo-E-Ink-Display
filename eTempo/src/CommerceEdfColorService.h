// CommerceEdfColorService.h

#pragma once

#include "BaseTempoColorService.h"
#include "Platform.h"

/**
 * Couleur d'aujourd'hui uniquement, inclut les jours placés.
 *
 * Exemple d'URL :
 * https://api-commerce.edf.fr/commerce/activet/v1/saisons/search?option=TEMPO&dateReference=2024-12-11
 */

class CommerceEdfColorService : public BaseTempoColorService
{
   public:
      CommerceEdfColorService(IHttpClient* httpClient);

      String getUrl() const override;

   protected:
      bool parseData(const String &data) override;

   private:
      String dateReference_;
};
