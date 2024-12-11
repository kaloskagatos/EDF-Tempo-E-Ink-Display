// TempoColorServiceManager.cpp
#include "TempoColorServiceManager.h"
#include "Platform.h"

TempoColorServiceManager::~TempoColorServiceManager()
{
   for (const auto &service : services_)
   {
      delete service ;
   }
}

void TempoColorServiceManager::registerService(ITempoColorService* service)
{
    if (service)
    {
        services_.emplace_back(service);
    }
}

bool TempoColorServiceManager::fetchData()
{
    // On initialise des variables temporaires permettant de conserver les meilleures infos trouvées.
    // On ne les écrira dans les membres finaux (finalTodayColor_, etc.) qu’après avoir interrogé tous les services.
    TempoColor bestToday = finalTodayColor_;
    TempoColor bestTomorrow = finalTomorrowColor_;
    int bestBlue = blueDaysPlaced_;
    int bestWhite = whiteDaysPlaced_;
    int bestRed = redDaysPlaced_;

    // On tente d’obtenir d’abord la couleur du jour (priorité haute), puis la couleur de demain (priorité moyenne),
    // et enfin les jours placés (priorité faible).
    // On parcourt tous les services afin de récupérer un maximum d’informations.
    // On ne s’arrête pas au premier service qui donne une info valable :
    // on continue afin d’obtenir éventuellement plus d’infos manquantes des autres services.
    bool atLeastOneSuccess = false;

    for (const auto &service : services_)
    {
        bool fetchSuccess = service->fetchData();
        if (!fetchSuccess)
        {
            DEBUG_PRINTLN(F("Failed to fetch data from a service, trying next service."));
            continue;
        }
        atLeastOneSuccess = true;

        // Priorité 1: Couleur du jour
        if (bestToday == TempoColor::UNKNOWN && service->getTodayColor() != TempoColor::UNKNOWN)
        {
            bestToday = service->getTodayColor();
        }

        // Priorité 2: Couleur de demain
        if (bestTomorrow == TempoColor::UNKNOWN && service->getTomorrowColor() != TempoColor::UNKNOWN)
        {
            bestTomorrow = service->getTomorrowColor();
        }

        // Priorité 3: Jours placés
        // On utilise les jours placés du premier service qui les fournit (non nuls).
        // S’ils sont déjà renseignés (pas 0), on les garde. Sinon, si le service courant les fournit, on les renseigne.
        if (bestBlue == 0 && service->getBlueDaysPlaced() > 0)
        {
            bestBlue = service->getBlueDaysPlaced();
        }
        if (bestWhite == 0 && service->getWhiteDaysPlaced() > 0)
        {
            bestWhite = service->getWhiteDaysPlaced();
        }
        if (bestRed == 0 && service->getRedDaysPlaced() > 0)
        {
            bestRed = service->getRedDaysPlaced();
        }

        // Si on a déjà tout ce dont on a besoin (couleur du jour + couleur de demain + jours placés),
        // on peut quitter la boucle.
        // Toutefois, si on juge moins importants les jours placés, on peut s’arrêter
        // dès qu’on a la couleur du jour et la couleur de demain.
        // Ici, on préfère tout récupérer si possible.
        if (bestToday != TempoColor::UNKNOWN && bestTomorrow != TempoColor::UNKNOWN
            && bestBlue > 0 && bestWhite > 0 && bestRed > 0)
        {
            // On a tout, on peut interrompre la boucle si souhaité.
            break;
        }
    }

    if (!atLeastOneSuccess)
    {
        DEBUG_PRINTLN(F("All services failed to provide valid data."));
        return false;
    }

    // Après avoir interrogé tous les services, on met à jour les valeurs finales.
    finalTodayColor_ = bestToday;
    finalTomorrowColor_ = bestTomorrow;
    blueDaysPlaced_ = bestBlue;
    whiteDaysPlaced_ = bestWhite;
    redDaysPlaced_ = bestRed;

    // Si on a au moins la couleur du jour et du lendemain, on considère cela comme un succès principal.
    // Sinon, on signale un échec global, même si on a d’autres données moins importantes.
    if (finalTodayColor_ == TempoColor::UNKNOWN &&
        finalTomorrowColor_ == TempoColor::UNKNOWN )
    {
        DEBUG_PRINTLN(F("No service provided valid day ."));
        return false;
    }

    return true;
}

TempoColor TempoColorServiceManager::getTodayColor() const
{
    return finalTodayColor_;
}

TempoColor TempoColorServiceManager::getTomorrowColor() const
{
    return finalTomorrowColor_;
}

int TempoColorServiceManager::getBlueDaysPlaced() const
{
    return blueDaysPlaced_;
}

int TempoColorServiceManager::getWhiteDaysPlaced() const
{
    return whiteDaysPlaced_;
}

int TempoColorServiceManager::getRedDaysPlaced() const
{
    return redDaysPlaced_;
}

void TempoColorServiceManager::logServiceResults() const
{
    DEBUG_PRINTLN(F("-------------------------------"));
    DEBUG_PRINTLN(F("Logging results for all registered services:"));
    DEBUG_PRINTLN(F("-------------------------------"));
    for (const auto &service : services_)
    {
        DEBUG_PRINT(F("URL: "));
        DEBUG_PRINTLN(service->getUrl());


        bool success = service->fetchData() ;
        DEBUG_PRINT(F("Success: "));
        DEBUG_PRINTLN(success);

        DEBUG_PRINT(F("Today Color: "));
        DEBUG_PRINTLN(toString(service->getTodayColor()));
        DEBUG_PRINT(F("Tomorrow Color: "));
        DEBUG_PRINTLN(toString(service->getTomorrowColor()));
        DEBUG_PRINT(F("Blue Days Placed: "));
        DEBUG_PRINTLN(service->getBlueDaysPlaced());
        DEBUG_PRINT(F("White Days Placed: "));
        DEBUG_PRINTLN(service->getWhiteDaysPlaced());
        DEBUG_PRINT(F("Red Days Placed: "));
        DEBUG_PRINTLN(service->getRedDaysPlaced());

        DEBUG_PRINTLN(F("-------------------------------"));
    }
}

