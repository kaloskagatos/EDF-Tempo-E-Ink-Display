// IHttpClient.h

#pragma once

#include "Platform.h"

class IHttpClient
{
public:
    virtual ~IHttpClient() = default;
    virtual String get(const String &url) = 0;
};
