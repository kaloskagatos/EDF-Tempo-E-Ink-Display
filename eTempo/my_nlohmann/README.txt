
Suppression des commentaires aux defines qui posent problème à l'IDE arduino :

#define NLOHMANN_JSON_VERSION_MAJOR 3   // NOLINT(modernize-macro-to-enum)
#define NLOHMANN_JSON_VERSION_MINOR 11  // NOLINT(modernize-macro-to-enum)
#define NLOHMANN_JSON_VERSION_PATCH 3   // NOLINT(modernize-macro-to-enum)


to 

#define NLOHMANN_JSON_VERSION_MAJOR 3
#define NLOHMANN_JSON_VERSION_MINOR 11
#define NLOHMANN_JSON_VERSION_PATCH 3