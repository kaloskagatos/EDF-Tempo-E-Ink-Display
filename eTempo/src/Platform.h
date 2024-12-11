// Platform.h

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include <pgmspace.h>

#ifdef FULL_NLOHMANN_JSON_PATH
#include <nlohmann/json.hpp>
#else
#include "my_nlohmann/json.hpp"
#endif
using nlohmann::json;

// TODO: define pour supprimer les debug

#define DEBUG_INIT() Serial.begin(9600)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)

#else

#include <iostream>
#include <string>

#define DEBUG_INIT() // Pas besoin d'initialisation spécifique
#define DEBUG_PRINT(x) std::cout << x
#define DEBUG_PRINTLN(x) std::cout << x << std::endl
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)

#define F(x) (x)
using String = std::string;

#define PROGMEM

#include <nlohmann/json.hpp>
using nlohmann::json;

#endif
