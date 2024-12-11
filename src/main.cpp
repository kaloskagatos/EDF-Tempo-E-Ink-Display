// Inclusion pour PlatformIO
// Ce fichier est le point d'entrée principal pour l'application lorsque celle-ci est compilée avec PlatformIO.
// PlatformIO utilise un environnement de build spécifique et s'attend à ce que le fichier d'entrée soit défini sous "src/main.cpp".
#include "TempoApp.h"

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
