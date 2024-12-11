#pragma once


// Arduino Only
#ifdef ARDUINO

#include "IHttpClient.h"
#include "Platform.h"

class ArduinoHttpClient : public IHttpClient
{
public:
    ArduinoHttpClient();
    ~ArduinoHttpClient();

    // Implémentation de la méthode GET
    String get(const String &url) override;
};

#endif
