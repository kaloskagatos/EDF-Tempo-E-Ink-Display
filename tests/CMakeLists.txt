# Minimum requis de CMake
cmake_minimum_required(VERSION 3.10)

# Nom du projet
project(TempoApp)

# Définir le standard C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Optionnel : Activer les messages de compilation détaillés
# set(CMAKE_VERBOSE_MAKEFILE ON)

# Définir les chemins vers les dépendances externes
# Supposons que les bibliothèques sont dans le répertoire 'external'
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external")

# Chemins spécifiques pour cURL
set(CURL_INCLUDE_DIR "${EXTERNAL_DIR}/curl/include")
set(CURL_LIB_DIR "${EXTERNAL_DIR}/curl/lib")

# Chemins spécifiques pour nlohmann/json
set(NLOHMANN_JSON_INCLUDE_DIR "${EXTERNAL_DIR}")

# Inclure les répertoires d'en-tête
include_directories(
    ${CURL_INCLUDE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/../eTempo/src
    ${CMAKE_SOURCE_DIR}/../eTempo/
)

# Ajouter le répertoire des bibliothèques pour le linker
link_directories(
    ${CURL_LIB_DIR}
)

# Vérifier que la bibliothèque cURL existe
find_library(CURL_LIB
    NAMES curl
    PATHS ${CURL_LIB_DIR}
)

if(NOT CURL_LIB)
    message(FATAL_ERROR "La bibliothèque cURL n'a pas été trouvée dans ${CURL_LIB_DIR}")
endif()

# Ajouter les fichiers sources
file(GLOB SOURCES
     "app/*.cpp"
     "app/*.h"
     "../eTempo/src/*.h"
     "../eTempo/src/*.cpp"
)


# Créer l'exécutable
add_executable(TempoApp ${SOURCES})

# Lier l'exécutable avec cURL
target_link_libraries(TempoApp PRIVATE ${CURL_LIB})
