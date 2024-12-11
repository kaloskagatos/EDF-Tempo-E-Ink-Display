// HttpClientFactory.cpp

#include "HttpClientFactory.h"

#ifdef ARDUINO
#include "ArduinoHttpClient.h"
#else
#include "CurlHttpClient.h"
#endif

IHttpClient* HttpClientFactory::create()
{
#ifdef ARDUINO
    return new ArduinoHttpClient();
#else
    return new CurlHttpClient();
#endif
}
