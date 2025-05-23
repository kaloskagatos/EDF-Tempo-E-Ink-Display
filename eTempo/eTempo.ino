// Inclusion relative à app pour Arduino
// Ce fichier est le point d'entrée principal pour l'application lorsqu'elle est compilée avec l'IDE Arduino.
// Contrairement à PlatformIO, l'IDE Arduino nécessite une inclusion relative dans certains cas.
#include "src/TempoApp.h" 

// Création d'une instance unique de l'application
TempoApp app;

void setup()
{
    // Appelé une seule fois au démarrage de l'appareil.
    // La méthode `run` contient toute la logique principale de l'application.
    app.run();
}

void loop()
{
    // La fonction `loop` reste vide car l'appareil entre directement en sommeil profond
    // après avoir exécuté la logique dans `setup()`.
    // Ceci est intentionnel pour les dispositifs à faible consommation d'énergie.
}
