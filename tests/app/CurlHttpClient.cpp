// CurlHttpClient.cpp

#ifndef ARDUINO

#include "CurlHttpClient.h"
#include <curl/curl.h>

CurlHttpClient::CurlHttpClient()
{
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        throw std::runtime_error("Failed to initialize cURL: " + String(curl_easy_strerror(res)));
    }
}

CurlHttpClient::~CurlHttpClient()
{
    curl_global_cleanup();
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((String *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

String CurlHttpClient::get(const String &url)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("Failed to initialize cURL.");
    }

    String readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    // Follow redirections
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // Set the callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    // Set the user data
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // Disable SSL verification (use with caution in production)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        throw std::runtime_error("cURL error: " + String(curl_easy_strerror(res)));
    }

    curl_easy_cleanup(curl);
    return readBuffer;
}
#endif // ARDUINO
