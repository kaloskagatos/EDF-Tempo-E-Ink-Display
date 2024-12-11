// CurlHttpClient.h

#pragma once

#ifndef ARDUINO

#include "IHttpClient.h"

class CurlHttpClient : public IHttpClient
{
public:
    CurlHttpClient();
    ~CurlHttpClient();

    String get(const String &url) override;
};

#endif // ARDUINO