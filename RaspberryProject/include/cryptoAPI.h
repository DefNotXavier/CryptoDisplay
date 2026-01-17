#ifndef CRYPTO_API_H
#define CRYPTO_API_H

#include <string>
#include <vector>
using namespace std;

// Modify structure of "Coin" to display other attributes
struct Coin {
    string name;
    string symbol;
    double price;
    double change24h;
    string image;
    string lastUpdated;
    vector<float> priceHistory;
};

vector <Coin> FetchCryptoData(vector<string> coins); 
vector <float> FetchHistoricalData(string coinId);

#endif