#include <string.h>
#include <curl/curl.h>

#include "misc.cpp"

using namespace std;

int writer(char *data, size_t size, size_t nmemb, string *buffer);

string curl_httpget(const string &url);

void strrep(char *str, char old, char newc);
