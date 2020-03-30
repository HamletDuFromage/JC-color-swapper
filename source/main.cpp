#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "json.hpp"


#include <switch.h>

using json = nlohmann::json;

#define VERSION "1.2"
#define JSON_PATH "./profiles.json"
#define ENTRIESPERPAGE 30
#define TERM_WIDTH 80



int hexToBGR(std::string hex){
    std::string R = hex.substr(0, 2);
    std::string G = hex.substr(2, 2);
    std::string B = hex.substr(4, 2);
    return std::stoi(B + G + R, 0, 16);
}

std::string BGRToHex(int v){
    std::stringstream ss;
    v = ((v & 0xFF) << 16) + (v & 0xFF00) + (v >> 16) + 256;
    ss << std::setfill('0') << std::setw(6) << std::right << std::hex << v;
    return ss.str();
}

bool isHexaAnd3Bytes(std::string str){
    if(str.size()!=6) return false;
    for(char const &c : str){
        if(!isxdigit(c)) return false;
    }
    return true;

}

int setColor(std::vector<int> colors){

    Result pads, ljc, rjc;
    int res = 0;
    s32 nbEntries;
    u64 UniquePadIds[2] = {};

    pads = hidsysGetUniquePadsFromNpad(CONTROLLER_HANDHELD, UniquePadIds, 2,&nbEntries);
    if(R_SUCCEEDED(pads)){
        ljc = hiddbgUpdateControllerColor(colors[0], colors[1], UniquePadIds[0]);
        if (R_FAILED(ljc)) res +=1;
        rjc = hiddbgUpdateControllerColor(colors[2], colors[3], UniquePadIds[1]);
        if (R_FAILED(rjc)) res +=2;
    }
    else{
        res +=4;
    }
    return res;
}

void displayResult(int res, int &done){
    if(res == 0){
        std::cout << "Successfully changed the Joy-Cons color!" << std::endl;
        std::cout << "You may need to dock/undock them once or twice for it to take effect" << std::endl;
        done = 1;
    }
    if(res == 1 || res == 3){
        std::cout << "Couldn't change the left JC color" << std::endl;
    }
    if(res == 2 || res == 3){
        std::cout << "Couldn't change the right JC color" << std::endl;
    }
    if(res == 4){
        std::cout << "Couldn't ID the joycons, make sure they're connected to the Switch" << std::endl;
    }
}

void scanBackup(json &profiles, std::vector<int> &backupValues, bool &backupExists){
    bool properData;
    for (const auto& x : profiles.items()){
        std::string name = x.value()["name"];
        std::vector<std::string> values = {
            std::string(x.value()["L_JC"]),
            std::string(x.value()["L_BTN"]),
            std::string(x.value()["R_JC"]),
            std::string(x.value()["R_BTN"])
        };
        properData = true;
        for(auto& str : values){
            if(!isHexaAnd3Bytes(str)){
                properData = false;
            }
        }

        if(properData && name == "_backup"){
            backupValues = {
                hexToBGR(values[0]),
                hexToBGR(values[1]),
                hexToBGR(values[2]),
                hexToBGR(values[3])
            };
            backupExists = true;
        }
    }
}

void backupToJSON(json &profiles){
    HidControllerColors colors;
    hidGetControllerColors(CONTROLLER_P1_AUTO, &colors);
    std::vector<int> oldBackups;
    if (colors.splitSet) {
        int i = 0;
        for (const auto& x : profiles.items()){
            if(x.value()["name"] == "_backup") {
                oldBackups.push_back(i);
            }
            i++;
        }
        for (auto &k : oldBackups){
            profiles.erase(profiles.begin() + k);
        }
        json newBackup = json::object({
            {"name", "_backup"},
            {"L_JC", BGRToHex(colors.leftColorBody)},
            {"L_BTN", BGRToHex(colors.leftColorButtons)},
            {"R_JC", BGRToHex(colors.rightColorBody)},
            {"R_BTN", BGRToHex(colors.rightColorButtons)}
        });
        profiles.push_back(newBackup);
        std::fstream newProfiles;
        newProfiles.open(JSON_PATH, std::fstream::out | std::fstream::trunc);
        newProfiles << std::setw(4) << profiles << std::endl;
        newProfiles.close();
    }
    else{
        std::cout << "Couldn't back up the profile, are your Joy-Con docked?" << std::endl; 
    }
}


int main(int argc, char* argv[])
{

    consoleInit(NULL);
    hiddbgInitialize();
    hidsysInitialize();

    int currentScreen = 0;
    int currentItem = 0;
    int currentPage = 0;

    std::fstream profilesFile;
    json profilesJson;
    if(std::filesystem::exists(JSON_PATH)){
        profilesFile.open(JSON_PATH, std::fstream::in);
        profilesFile >> profilesJson;
        profilesFile.close();
    }
    else{
        profilesJson = {{
            {"L_BTN", "0A1E0A"},
            {"L_JC", "82FF96"},
            {"R_BTN", "0A1E28"},
            {"R_JC", "96F5F5"},
            {"name", "Animal Crossing: New Horizons"}
        }};
    }



    std::vector<std::string> colorProfiles;
    std::vector<std::vector<int>> colorValues;
    std::string backupProfile;
    std::vector<int> backupValues;
    bool backupExists = false;
    bool properData;

    for (const auto& x : profilesJson.items()){
        std::string name = x.value()["name"];
        name = name.substr(0, TERM_WIDTH - 4);
        std::vector<std::string> values = {
            std::string(x.value()["L_JC"]),
            std::string(x.value()["L_BTN"]),
            std::string(x.value()["R_JC"]),
            std::string(x.value()["R_BTN"])
        };
        properData = true;
        for(auto& str : values){
            if(!isHexaAnd3Bytes(str)){
                properData = false;
            }
        }
        if(properData && name !="_backup"){
            if(name == "") name = "Unamed";
            colorProfiles.push_back(name);
            colorValues.push_back({
                hexToBGR(values[0]),
                hexToBGR(values[1]),
                hexToBGR(values[2]),
                hexToBGR(values[3])
            });
        }
        if(properData && name == "_backup"){
            backupProfile = name;
            backupValues = {
                hexToBGR(values[0]),
                hexToBGR(values[1]),
                hexToBGR(values[2]),
                hexToBGR(values[3])
            };
            backupExists = true;
        }
    }

    int nbItems = colorProfiles.size();
    int nbPages = (nbItems/ENTRIESPERPAGE) + 1;

    int res = 0;
    int done = 0;

    while (appletMainLoop())
    {
        consoleClear();
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);


        if(currentScreen == 0){

            std::cout << "*** Joy-Con color swapper v" << VERSION << " - By HamletDuFromage" << std::endl;
            std::cout << "* Select your desired profile with [UP]/[DOWN]|[LEFT]/[RIGHT] \n* Confirm with [A]" << std::endl;
            std::cout << "* Press [L] to backup a profile and [R] to restore it" << std::endl;
            std::cout << "* Press [B] to quit\n" << std::endl;

            std::cout << "* Visit >bit.ly/colorswapper< to generate your own color profiles\n\n" << std::endl;

            res = 0; 
            done = 0;

            for (size_t i = currentPage * ENTRIESPERPAGE; i < (size_t) std::min(nbItems, ENTRIESPERPAGE*(currentPage+1)); i++){
                if ((int) i == currentItem) std::cout << "> " << colorProfiles[i] << std::endl;
                else std::cout << colorProfiles[i] << std::endl;
            }

            std::cout << "\n\t\t\t\t\t\t\t(" << currentPage + 1 << "/" << nbPages << ")" <<std::endl;

            if ((kDown & KEY_DOWN) && currentItem < nbItems - 1) currentItem += 1;
            if ((kDown & KEY_UP) && currentItem > 0) currentItem -= 1;
            if ((kDown & KEY_RIGHT) && currentPage < nbPages - 1) {
                currentPage += 1;
                currentItem = std::min(currentItem + ENTRIESPERPAGE, nbItems - 1);
            }
            if ((kDown & KEY_LEFT) && currentPage > 0) {
                currentPage -= 1;
                currentItem -= ENTRIESPERPAGE;
            }

            if (kDown & KEY_A){
                // SET COLOR
                currentScreen = 1;
            }

            if (kDown & KEY_L){
                //BACKUP PROFILE
                currentScreen = 2;
            }

            if (kDown & KEY_R){
                //LOAD BACKUP
                currentScreen = 3;
            }

            if (kDown & KEY_B)
                break;
        }
        else if (currentScreen == 1){
            std::cout << "Applying the " << colorProfiles[currentItem] << " color profile\n\n\n" << std::endl;
            if(done == 0) res = setColor(colorValues[currentItem]);
            
            displayResult(res, done);
            std::cout << "\nPress [B] to return to the main menu" << std::endl;
            if (kDown & KEY_B) currentScreen = 0;
        }

        else if(currentScreen == 2){
            if(done == 0) backupToJSON(profilesJson);
            std::cout << "Backed up the current profile to profiles.json" << std::endl;
            std::cout << "\nPress [B] to return to the main menu" << std::endl;
            done = 1;
            if (kDown & KEY_B) currentScreen = 0;
        }

        else if(currentScreen == 3){
            if(backupExists){
                std::cout << "Restoring backed up profile\n\n\n" << std::endl;
                if(done == 0) res = setColor(backupValues);
                displayResult(res, done);

            }
            else{
                if(done == 0){
                    scanBackup(profilesJson, backupValues, backupExists);
                    done = 1;
                }
                else{
                    std::cout << "Couldn't find a backup" << std::endl; 
                }
            }
            std::cout << "\nPress [B] to return to the main menu" << std::endl;
            if (kDown & KEY_B) currentScreen = 0;
        }


        consoleUpdate(NULL);
    }

    hiddbgExit();
    hidsysExit();
    consoleExit(NULL);
    return 0;
}
