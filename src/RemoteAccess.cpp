#include "RemoteAccess.h"
#include <curl/curl.h>
#include <iostream>
namespace
{
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        void* data = *static_cast<void**>(userp);
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    size_t WriteCallbackImage(void* contents, size_t size, size_t nmemb, void* userp)
    {
        std::vector<unsigned char>& data = *static_cast<std::vector<unsigned char>*>(userp);
        size_t oldSize = data.size();
        data.resize(data.size() + size * nmemb);
        memcpy(data.data() + oldSize, contents, size * nmemb);
        return size * nmemb;
    }
}

std::string receiveStringResource(const char* url)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    std::string output;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return output;
}

std::vector<unsigned char>  receiveImageData(const char* url)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    std::vector<unsigned char> output;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackImage);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        res = curl_easy_perform(curl);
        if (res != CURLcode::CURLE_OK)
        {
            std::cerr << "CURL failure " << curl_easy_strerror(res);
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return output;
}
