// Platform.h

#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include <pgmspace.h>


#include "my_nlohmann/json.hpp"
using nlohmann::json;

#ifdef ENABLE_DEBUG
#define DEBUG_INIT() Serial.begin(9600)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#else

#include <iostream>
#include <string>

#ifdef ENABLE_DEBUG
#define DEBUG_INIT() // Pas besoin d'initialisation spécifique
#define DEBUG_PRINT(x) std::cout << x
#define DEBUG_PRINTLN(x) std::cout << x << std::endl
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_INIT()
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#define F(x) (x)
using String = std::string;

#define PROGMEM

#include "my_nlohmann/json.hpp"
using nlohmann::json;
#ifndef ARDUINO
#define RTC_DATA_ATTR
#endif

#endif
