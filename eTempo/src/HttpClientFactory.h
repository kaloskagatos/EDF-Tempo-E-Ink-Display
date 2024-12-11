// HttpClientFactory.h

#pragma once

#include "IHttpClient.h"

class HttpClientFactory {
public:
    static IHttpClient* create();
};
