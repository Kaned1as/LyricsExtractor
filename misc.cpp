using namespace std;

int writer(char *data, size_t size, size_t nmemb, string *buffer)
{
  int result = 0;
  if (buffer != NULL)
  {
      buffer->append(data, size * nmemb);
      result = size * nmemb;
  }
  return result;
}

string curl_httpget(char * url)
{
    string buffer;

    CURL *curl;
    CURLcode result;

    curl = curl_easy_init();
    if (curl)
    {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

      result = curl_easy_perform(curl);//http get performed

      curl_easy_cleanup(curl);//must cleanup

      if (result == CURLE_OK)
        return buffer;

      cerr << "error: " << result << " " << curl_easy_strerror(result) << endl;
      return "";
    }

    cerr << "error: could not initalize curl" << endl;
    return "";
}

void strrep(char *str, char old, char newc)
{
    char *pos = strchr(str, old);
    while (pos != NULL)
    {
        *pos = newc;
        pos = strchr(pos + 1, old);
    }
}
