#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "json.hpp"


#include <switch.h>

using json = nlohmann::json;

#define VERSION "1.1"
#define ENTRIESPERPAGE 30


int hexToRGB(std::string hex){
    std::string R = hex.substr(0, 2);
    std::string G = hex.substr(2, 2);
    std::string B = hex.substr(4, 2);
    return std::stoi(B + G + R, 0, 16);
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

int main(int argc, char* argv[])
{

    consoleInit(NULL);
    hiddbgInitialize();
    hidsysInitialize();

    int currentScreen = 0;
    int currentItem = 0;
    int currentPage = 0;

    std::ifstream profilesFile("sdmc:/switch/JC-color-swapper/profiles.json");
    json profilesJson;

    if (!profilesFile.fail()) {
        try {
            profilesFile >> profilesJson;
        } catch(json::parse_error& e) {}
    }

    std::vector<std::string> colorProfiles;
    std::vector<std::vector<int>> colorValues;

    for (const auto& x : profilesJson.items()){
        colorProfiles.push_back(x.value()["name"]);
        colorValues.push_back({
            hexToRGB(std::string(x.value()["L_JC"])),
            hexToRGB(std::string(x.value()["L_BTN"])),
            hexToRGB(std::string(x.value()["R_JC"])),
            hexToRGB(std::string(x.value()["R_BTN"]))
        });
    }

    int nbItems = colorProfiles.size();
    //if (nbItems > 33) nbItems = 33;
    int nbPages = (nbItems/ENTRIESPERPAGE) + 1;

    int res = 0;

    while (appletMainLoop())
    {
        consoleClear();
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);


        if(currentScreen == 0){

            std::cout << "*** Joy-Con color swapper v" << VERSION << " - By HamletDuFromage" << std::endl;
            std::cout << "* Select your desired profile with [UP]/[DOWN]|[LEFT]/[RIGHT]. Confirm with [A]. \n* Press [B] to quit\n\n" << std::endl;


            res = 0; 

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

            if (kDown & KEY_B)
                break;
        }
        else if (currentScreen == 1){
            std::cout << "Applying the " << colorProfiles[currentItem] << " color profile" << std::endl;
            res = setColor(colorValues[currentItem]);
            std::cout << "\n\n" << std::endl;
            
            if(res == 0){
                std::cout << "Successfully changed the Joy-Cons color!" << std::endl;
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
            std::cout << "Press [B] to return to the main menu" << std::endl;
            if (kDown & KEY_B) currentScreen = 0;
        }
        else if(currentScreen == 2){
            std::cout << "Press [B] to return to the main menu" << std::endl;
        }



        consoleUpdate(NULL);
    }

    hiddbgExit();
    hidsysExit();
    consoleExit(NULL);
    return 0;
}
