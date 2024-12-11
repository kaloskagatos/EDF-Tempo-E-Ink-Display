#!/bin/bash

# Init des dépendances pour compilation sous Windows des tests

# Variables de configuration globales
BASE_DIR="external2"
CURL_DIR="$BASE_DIR/curl"

CURL_URL="https://curl.se/windows/dl-8.11.0_4/curl-8.11.0_4-win64-mingw.zip"
CURL_ZIP="curl-8.11.0_4-win64-mingw.zip"

# Fonctions utilitaires
create_dir() {
    echo "Création du répertoire $1..."
    mkdir -p $1
}

download_file() {
    echo "Téléchargement de $2..."
    curl -L $1 -o $2
    if [ $? -ne 0 ]; then
        echo "Erreur lors du téléchargement de $2."
        exit 1
    fi
}

extract_zip() {
    echo "Extraction de $1..."
    temp_dir=$(mktemp -d)
    unzip -o $1 -d $temp_dir
    if [ $? -ne 0 ]; then
        echo "Erreur lors de l'extraction de $1."
        exit 1
    fi
    mv $temp_dir/*/* $2
    rm -rf $temp_dir
    rm $1
}

# Fonction pour installer curl
install_curl() {
    create_dir $CURL_DIR
    download_file $CURL_URL $CURL_ZIP
    extract_zip $CURL_ZIP $CURL_DIR
    echo "curl a été installé avec succès."
}

# Exécution des installations
install_curl

echo "Toutes les dépendances ont été installées avec succès."
