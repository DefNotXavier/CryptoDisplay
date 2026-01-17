#include "cryptoAPI.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>

using namespace std;
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Basically opening my .env file and hardcoding exaclty where to get the api key (CANNOT CHANGE .env structure.)
string LoadApiKey() {
    string key = "";
    // look for .env in the main directory of RASPBERRYPROJECT
    ifstream envFile("../.env");
    if (envFile.is_open()) {
        string line;
        while (getline(envFile, line)) {
            if (line.find("api_key = ") == 0) {
                key = line.substr(10); 
                // Clean up Windows hidden characters (\r) if they exist
                if (!key.empty() && key.back() == '\r') key.pop_back();
            }
        }
        envFile.close();
    }
    return key;
}

vector<Coin> FetchCryptoData(vector<string> coins) {
    CURL* curl;
    CURLcode res;
    string readBuffer;
    vector<Coin> coinList;

    string key = LoadApiKey();
    if (key == "") cout << "WARNING: API Key not found in .env!" << endl;

    curl = curl_easy_init();
    if(curl) {
        string str_coins ="";
        for (int i = 0; i < (int)coins.size(); i++){
            str_coins += coins[i];
            if (i < (int)coins.size() - 1) str_coins += ",";
        }
        
        string url = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=" + str_coins;
        
        struct curl_slist* headers = NULL;
        string fullHeader = "x-cg-demo-api-key: " + key;
        headers = curl_slist_append(headers, fullHeader.c_str()); 
        headers = curl_slist_append(headers, "User-Agent: NerdPi-Dash/1.0"); 

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            try {
                auto j = json::parse(readBuffer);
                for (auto& item : j) {
                    Coin c;
                    c.name = item.value("name", "Unknown");
                    c.symbol = item.value("symbol", "???");
                    c.price = item.value("current_price", 0.0);
                    c.change24h = item.value("price_change_percentage_24h", 0.0);
                    c.image = item.value("image","");
                    string fullTime = item.value("last_updated", "2024-01-01T00:00:00Z"); 
                    if (fullTime.length() >= 19) c.lastUpdated = fullTime.substr(11, 8); 
                    coinList.push_back(c);
                }
                // Should work unless coinGecko changes JSON structure of my endpoint
            } catch(...) { cout << "JSON Parse Error in Markets" << endl; }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return coinList;
}

vector <float> FetchHistoricalData(string coinId) {
    CURL* curl;
    string readBuffer;
    vector<float> prices;

    auto now = chrono::system_clock::now();
    time_t to = chrono::system_clock::to_time_t(now);
    time_t from = to - (24 * 3600); 

    string key = LoadApiKey();

    curl = curl_easy_init();
    if(curl) {
        string url = "https://api.coingecko.com/api/v3/coins/" + coinId + 
                     "/market_chart/range?vs_currency=usd&from=" + to_string(from) + 
                     "&to=" + to_string(to);
        
        struct curl_slist* headers = NULL;
        string fullHeader = "x-cg-demo-api-key: " + key;
        headers = curl_slist_append(headers, fullHeader.c_str()); 
        headers = curl_slist_append(headers, "User-Agent: NerdPi-Dash/1.0"); 

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        if(curl_easy_perform(curl) == CURLE_OK) {
            try {
                auto j = json::parse(readBuffer);
                if (j.contains("prices")) {
                    for (auto& entry : j["prices"]) {
                        prices.push_back(entry[1].get<float>());
                    }
                }
            } catch(...) { cout << "JSON Parse Error in History" << endl; }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return prices;
}