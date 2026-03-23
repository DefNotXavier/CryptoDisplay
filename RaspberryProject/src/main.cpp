#include "raylib.h"
#include "cryptoAPI.h"
#include <string>
#include <vector>
#include <cmath> 
#include <thread>
#include "mockData.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

float Normalize(float value, float min, float max) {
    if (max - min == 0) return 0.5f; 
    return (value - min) / (max - min);
}

bool IsWifiConnected() {
    // Pings Google's DNS Really never down
    int result = system("ping -c 1 -W 2 8.8.8.8 > /dev/null 2>&1");
    return result == 0;
}

bool ConnectToWifi(const string& ssid, const string& password, int retries, int delaySeconds) {

    // Read existing config to avoid duplicates
    ifstream checkFile("/etc/wpa_supplicant/wpa_supplicant.conf");
    string fileContents((istreambuf_iterator<char>(checkFile)), istreambuf_iterator<char>());
    checkFile.close();

    // Only write if not already in there
    if (fileContents.find("ssid=\"" + ssid + "\"") == string::npos) {
        ofstream wpaConf("/etc/wpa_supplicant/wpa_supplicant.conf", ios::app);
        if (wpaConf.is_open()) {
            wpaConf << "\nnetwork={\n";
            wpaConf << "    ssid=\"" << ssid << "\"\n";
            wpaConf << "    psk=\"" << password << "\"\n";
            wpaConf << "}\n";
            wpaConf.close();
        } else {
            return false;
        }
    }

    // Reload, disconnect, then reconnect to force a real connection attempt
    system("sudo wpa_cli -i wlan0 reconfigure > /dev/null 2>&1");
    this_thread::sleep_for(chrono::seconds(2));
    system("sudo wpa_cli -i wlan0 disconnect > /dev/null 2>&1");
    this_thread::sleep_for(chrono::seconds(1));
    system("sudo wpa_cli -i wlan0 reconnect > /dev/null 2>&1");
    this_thread::sleep_for(chrono::seconds(5));

    for (int attempt = 1; attempt <= retries; attempt++) {
        this_thread::sleep_for(chrono::seconds(delaySeconds));
        if (IsWifiConnected()) {
            return true;
        }
    }
    return false;
}


Texture2D LoadTextureFromURL(string url, string name) {
    string fileName = name + ".png";
    // Building a shell command using curl to dowload image from url and write it to a file
    string command = "curl -s -L \"" + url + "\" -o " + fileName;
    int result = system(command.c_str());
    Texture2D tex = { 0 };
    if (result == 0) {
        Image img = LoadImage(fileName.c_str());
        if (img.data != NULL) {
            ImageResize(&img, 50, 50); 
            tex = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        remove(fileName.c_str()); 
    }
    return tex;
}

int main() {

    // 2 paths for cyprot info, USB or default
    const string usbConfig = "/media/pi/CRYPTOS/config.json";
    const string defaultConfig = "config.json";

    // Assume usb is not plugged in to begin with
    string activePath = defaultConfig;
    bool isUsingUsb = false;

    // Check if USB is plugged in through hard coded path
    if (fs::exists(usbConfig)) {
        activePath = usbConfig;
        isUsingUsb = true;

    } 

    // Load in JSON Config file (either usb or defualt)
    ifstream f(activePath);
    json config = json::parse(f);

    // Grab all jSON Config file values
    bool useMockData = config["api_settings"]["use_mock_data"];
    vector<string> coin_ids_list = config["api_settings"]["coin_ids"];
    float priceInterval = config["api_settings"]["update_interval_current_seconds"];
    float historyInterval = config["api_settings"]["update_interval_historic_seconds"];
    int apiDelay = config["api_settings"]["api_delay_ms"];

    // --- WiFi Check & Connect ---
    string ssid     = config["wifi_settings"]["ssid"];
    string password = config["wifi_settings"]["password"];
    int retryAttempts   = config["wifi_settings"]["retry_attempts"];
    int retryDelaySecs  = config["wifi_settings"]["retry_delay_seconds"];

    // WINDOW SETTINGS (Match the JSON keys)
    int screenW = config["window_settings"]["screenW"];
    int screenH = config["window_settings"]["screenH"];
    string title = config["window_settings"]["title"];

    if (!IsWifiConnected()) {
        bool connected = ConnectToWifi(ssid, password, retryAttempts, retryDelaySecs);
        if (!connected) {
            // Launch a simple error window
            SetConfigFlags(FLAG_WINDOW_UNDECORATED);
            InitWindow(screenW, screenH, "WiFi Error");
            ToggleFullscreen();
            HideCursor();
            SetWindowPosition(0, 0);
            SetTargetFPS(30);

            while (!WindowShouldClose()) {
                // check if usb is un plugged in error window as well
                if (isUsingUsb) {
                    static int errorFrameCounter = 0;
                    if (++errorFrameCounter % 120 == 0) {
                        if (!fs::exists(usbConfig)) {
                            CloseWindow();
                            return 0;
                        }
                    }
                }

                BeginDrawing();
                    ClearBackground(BLACK);

                    const char* errTitle = "NO WIFI CONNECTION";
                    const char* errSub   = "Could not connect to network:";
                    const char* errSSID  = ssid.c_str();
                    const char* errHint  = "Check your config.json and restart.";

                    int titleSize = screenH * 0.07f;
                    int subSize   = screenH * 0.04f;

                    // Centered error title in red
                    DrawText(errTitle,
                        screenW/2 - MeasureText(errTitle, titleSize)/2,
                        screenH * 0.30f, titleSize, RED);

                    // Subtitle
                    DrawText(errSub,
                        screenW/2 - MeasureText(errSub, subSize)/2,
                        screenH * 0.50f, subSize, GRAY);

                    // The SSID it tried
                    DrawText(errSSID,
                        screenW/2 - MeasureText(errSSID, subSize)/2,
                        screenH * 0.58f, subSize, YELLOW);

                    // Hint at the bottom
                    DrawText(errHint,
                        screenW/2 - MeasureText(errHint, subSize)/2,
                        screenH * 0.75f, subSize, DARKGRAY);

                EndDrawing();
            }

            CloseWindow();
            return 0; 
        }
    }


    // COLORS (Match the JSON keys)
    Color color1 = { config["ui_colors"]["row_bg_color"][0], config["ui_colors"]["row_bg_color"][1], config["ui_colors"]["row_bg_color"][2], config["ui_colors"]["row_bg_color"][3] };
    Color color2 = { config["ui_colors"]["header_color"][0], config["ui_colors"]["header_color"][1], config["ui_colors"]["header_color"][2], config["ui_colors"]["header_color"][3] };
    Color color3 = { config["ui_colors"]["text_color_gold"][0], config["ui_colors"]["text_color_gold"][1], config["ui_colors"]["text_color_gold"][2], config["ui_colors"]["text_color_gold"][3] };

    // Initialize the window & ignore title bar
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(screenW,screenH, title.c_str());
    ToggleFullscreen();
    HideCursor();
    SetWindowPosition(0, 0);
    SetTargetFPS(30);

    vector<Coin> data;
    if (useMockData) {
        data = GetMockData();
    } else {
        data = FetchCryptoData(coin_ids_list);
        for (int i = 0; i < (int)data.size(); i++) {
            data[i].priceHistory = FetchHistoricalData(coin_ids_list[i]);
            // Wait time between api calls, make this longer to not get blocked
            this_thread::sleep_for(chrono::milliseconds(apiDelay));
        }
    }

    bool isStable = (data.size() > 0);
    const char* statusText = isStable ? "SYSTEM: STABLE" : "SYSTEM: UNSTABLE";

    // 4. Force first update by initializing timers to negative values
    float lastPriceUpdate = -500.0f;       
    float lastHistoryUpdate = -90000.0f;

    vector<Texture2D> coinImages;
    for (int i = 0; i < (int)data.size(); i++) {
        coinImages.push_back(LoadTextureFromURL(data[i].image, coin_ids_list[i]));
    }

    while (!WindowShouldClose()) {

        //  Check USB status every 120 frames (~4 seconds at 30fps)
        if (isUsingUsb) {
            static int frameCounter = 0;
            if (++frameCounter % 120 == 0) {
                if (!fs::exists(usbConfig)) {
                    break; 
                }
            }
        }

        double currentTime = GetTime();
        // Code to check for api refresh
        if (currentTime - lastPriceUpdate >= priceInterval) {
            lastPriceUpdate = currentTime; 
            if (useMockData) {
                data = GetMockData();
                statusText = "SYSTEM: MOCK MODE";
            } else {
                vector<Coin> newData = FetchCryptoData(coin_ids_list);
                if (!newData.empty()) {
                    // Safety: Match by index but check bounds
                    for (int i = 0; i < (int)newData.size(); i++) {
                        if (i < (int)data.size()) {
                            newData[i].priceHistory = data[i].priceHistory; 
                        }
                    }
                    data = newData;
                    isStable = true;
                    statusText = "SYSTEM: STABLE";
                }
            }
        }

        // CALCULATE TIMER BEFORE DRAWING 
        int timeLeft = (int)(priceInterval - (currentTime - lastPriceUpdate));
        if (timeLeft < 0) timeLeft = 0;

        BeginDrawing();
            ClearBackground(BLACK);
            
            // Cache screen dimensions for easier math
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            
            // 1. Dynamic Header (height is 8% of screen)
            float headerH = sh * 0.08f;
            DrawRectangle(0, 0, (int)sw, (int)headerH, color2);
            DrawText("Crypto DashBoard v1.0 powered by CoinGecko", 20, (int)(headerH * 0.25f), (int)(headerH * 0.5f), color3);

            // 2. Dynamic Row Calculation
            float footerH = sh * 0.08f;
            float usableH = sh - headerH - footerH;
            float rowSlotH = usableH / (int)data.size(); 
            float rowH = rowSlotH * 0.85f;              

            // Looping through each row
            for (int i = 0; i < (int)data.size(); i++) {
                float yPos = headerH + (i * rowSlotH) + (rowSlotH * 0.075f);
                // 2% margin from left
                float xPos = sw * 0.02f;   
                // 96% total width     
                float rowWidth = sw * 0.96f;  
                  
                Color chartColor = (data[i].change24h >= 0) ? LIME : RED;

                // Row background
                DrawRectangleRec({xPos, yPos, rowWidth, rowH}, color1);
                
                // Icon (Positioned at 2% of row width)
                if (i < (int)coinImages.size()) {
                    float iconSize = rowH * 0.6f;
                    DrawTexturePro(coinImages[i], 
                        {0,0, (float)coinImages[i].width, (float)coinImages[i].height},
                        {xPos + (rowWidth * 0.02f), yPos + (rowH * 0.2f), iconSize, iconSize}, 
                        {0,0}, 0, WHITE);
                }

                // Name (At 10% of row width)
                DrawText(data[i].name.c_str(), (int)(xPos + (rowWidth * 0.10f)), (int)(yPos + (rowH * 0.3f)), (int)(rowH * 0.3f), WHITE);

                // Price (At 30% of row width)
                DrawText(TextFormat("$%.2f", data[i].price), (int)(xPos + (rowWidth * 0.30f)), (int)(yPos + (rowH * 0.3f)), (int)(rowH * 0.4f), WHITE);

                // 3. Dynamic Graph Logic (Taking up middle-right space)
                float gX = xPos + (rowWidth * 0.55f);
                float gY = yPos + (rowH * 0.25f);
                float gW = rowWidth * 0.28f;
                float gH = rowH * 0.5f;

                if (data[i].priceHistory.size() > 1) {
                    float minP = data[i].priceHistory[0], maxP = data[i].priceHistory[0];
                    for(float p : data[i].priceHistory) {
                        if (p < minP) minP = p; if (p > maxP) maxP = p;
                    }
                    float xStep = gW / (data[i].priceHistory.size() - 1);
                    for (int j = 0; j < (int)data[i].priceHistory.size() - 1; j++) {
                        float v1 = Normalize(data[i].priceHistory[j], minP, maxP);
                        float v2 = Normalize(data[i].priceHistory[j+1], minP, maxP);
                        Vector2 p1 = { gX + (j * xStep), gY + gH - (v1 * gH) };
                        Vector2 p2 = { gX + ((j + 1) * xStep), gY + gH - (v2 * gH) };
                        DrawLineEx(p1, p2, (rowH * 0.03f), chartColor);
                    }
                }
                
                // Percentage (Pinned to right side)
                string pct = TextFormat("%s%.2f%%", (data[i].change24h > 0 ? "+" : ""), data[i].change24h);
                float pctX = (xPos + rowWidth) - (rowWidth * 0.12f);
                DrawText(pct.c_str(), (int)pctX, (int)(yPos + (rowH * 0.35f)), (int)(rowH * 0.3f), chartColor);
            }

            // 4. Dynamic Footer (Y based on Screen Height)
            float footerY = sh - (footerH * 0.7f);
            int footerFontSize = (int)(sh * 0.025f);

            string syncText = "LAST SYNC: " + (data.empty() ? "N/A" : data[0].lastUpdated);
            DrawText(syncText.c_str(), 20, (int)footerY, footerFontSize, MAROON);

            const char* timerT = TextFormat("NEXT UPDATE IN: %02d:%02d", timeLeft/60, timeLeft%60);
            DrawText(timerT, (int)(sw/2 - MeasureText(timerT, footerFontSize)/2), (int)footerY, footerFontSize, GRAY);
            
            // Calculate the width of the status text so we can align it perfectly to the right
            int statusWidth = MeasureText(statusText, footerFontSize);

            // Draw it with a 20-pixel margin from the right edge
            DrawText(statusText, (int)(sw - statusWidth - 20), (int)footerY, footerFontSize, (isStable ? GREEN : RED));

        EndDrawing();
    }
    
    for (Texture2D tex : coinImages) UnloadTexture(tex);
    CloseWindow();
    return 0;
}
