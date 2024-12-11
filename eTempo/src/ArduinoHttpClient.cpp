// ArduinoHttpClient.cpp

// Arduino Only
#ifdef ARDUINO

#include "ArduinoHttpClient.h"

#include <HTTPClient.h> // Arduino Only

ArduinoHttpClient::ArduinoHttpClient()
{
}

ArduinoHttpClient::~ArduinoHttpClient()
{
}

String ArduinoHttpClient::get(const String &url)
{
    DEBUG_PRINT(F("[ArduinoHttpClient] HTTP GET called with URL: "));
    DEBUG_PRINTLN(url);

    HTTPClient http;
    String payload;

    DEBUG_PRINTLN(F("[ArduinoHttpClient] Initializing HTTP request"));
    if (http.begin(url.c_str()))
    {
        DEBUG_PRINTLN(F("[ArduinoHttpClient] HTTP begin success, attempting GET..."));
        int httpCode = http.GET();

        DEBUG_PRINT(F("[ArduinoHttpClient] HTTP Code received: "));
        DEBUG_PRINTLN(httpCode);

        if (httpCode > 0)
        {
            // Vérification du code 200 ou HTTP_CODE_OK
            if (httpCode == HTTP_CODE_OK || httpCode == 200)
            {
                DEBUG_PRINTLN(F("[ArduinoHttpClient] HTTP 200 OK, reading payload..."));
                String response = http.getString();
                payload = String(response.c_str());
            }
            else
            {
                DEBUG_PRINT(F("[ArduinoHttpClient] Non-OK HTTP code: "));
                DEBUG_PRINTLN(httpCode);
            }
        }
        else
        {
            DEBUG_PRINT(F("[ArduinoHttpClient] HTTP GET failed, error: "));
            DEBUG_PRINTLN(http.errorToString(httpCode));
        }

        http.end();
        DEBUG_PRINTLN(F("[ArduinoHttpClient] HTTP connection closed"));
    }
    else
    {
        DEBUG_PRINTLN(F("[ArduinoHttpClient] HTTP begin failed, unable to start HTTP request"));
    }

    DEBUG_PRINTLN(F("[ArduinoHttpClient] HTTP GET finished"));
    return payload;
}

#endif
