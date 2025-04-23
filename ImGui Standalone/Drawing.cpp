#include "Drawing.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdint>
#include <cstring>
#include "JsonHandler.h"
#include <regex>
#include <queue>
#include <string>
#include <mutex>
#include <chrono>

LPCSTR Drawing::lpWindowName = "Schedule 1: Save Editor";
ImVec2 Drawing::vSecondaryWindowSize = {
  500,
  300
};  
ImVec2 Drawing::vMainWindowSize = {
  750,
  500
};  
ImGuiWindowFlags Drawing::WindowFlags = 2 | 1;
bool Drawing::bDraw = true;
void OpenURL(const char* url) {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
#endif
}
static int current_window = 0;
#pragma region Steam Shit
static char steamIdBuffer[18] = "";
static bool steamIdValid = false;
static uint64_t steamId64 = 0;

static std::string steamIdErrorMessage = "";
static bool showSteamIdError = false;

struct SteamIDFormatResult {
    bool isValid;
    uint64_t steamId64;
    std::string errorMessage;
};

static SteamIDFormatResult ValidateSteamIDFormat(const char* input) {
    SteamIDFormatResult result = { false, 0, "" };

    if (!input || strlen(input) == 0) {
        result.errorMessage = "Please enter a Steam ID";
        return result;
    }

    std::string inputStr(input);

    if (std::regex_match(inputStr, std::regex("^765[0-9]{14}$"))) {
        uint64_t steamId = strtoull(inputStr.c_str(), nullptr, 10);

        const uint64_t MIN_STEAMID64 = 76561197960265728ULL;
        const uint64_t MAX_STEAMID64 = 76561199999999999ULL;

        if (steamId >= MIN_STEAMID64 && steamId <= MAX_STEAMID64) {
            result.isValid = true;
            result.steamId64 = steamId;
            return result;
        }
        else {
            result.errorMessage = "SteamID64 is out of valid range";
            return result;
        }
    }

    std::regex steamIdRegex("^STEAM_([0-9]):([0-9]):([0-9]+)$");
    std::smatch steamIdMatch;
    if (std::regex_match(inputStr, steamIdMatch, steamIdRegex)) {
        int universe = std::stoi(steamIdMatch[1].str());
        int accountIdLowBit = std::stoi(steamIdMatch[2].str());
        uint64_t accountIdHighBits = std::stoull(steamIdMatch[3].str());

        uint64_t steamId64 = 76561197960265728ULL + accountIdHighBits * 2 + accountIdLowBit;
        result.isValid = true;
        result.steamId64 = steamId64;

        snprintf(const_cast<char*>(input), 18, "%llu", steamId64);

        return result;
    }

    std::regex profileUrlRegex("/profiles/([0-9]+)");
    std::smatch profileUrlMatch;
    if (std::regex_search(inputStr, profileUrlMatch, profileUrlRegex)) {
        std::string steamId64Str = profileUrlMatch[1].str();
        if (std::regex_match(steamId64Str, std::regex("^765[0-9]{14}$"))) {
            uint64_t steamId = strtoull(steamId64Str.c_str(), nullptr, 10);
            const uint64_t MIN_STEAMID64 = 76561197960265728ULL;
            const uint64_t MAX_STEAMID64 = 76561199999999999ULL;

            if (steamId >= MIN_STEAMID64 && steamId <= MAX_STEAMID64) {
                result.isValid = true;
                result.steamId64 = steamId;

                snprintf(const_cast<char*>(input), 18, "%llu", steamId);

                return result;
            }
        }
    }

    std::regex steamId3Regex("\\[U:([0-9]+):([0-9]+)\\]");
    std::smatch steamId3Match;
    if (std::regex_match(inputStr, steamId3Match, steamId3Regex)) {
        int universe = std::stoi(steamId3Match[1].str());
        uint64_t accountId = std::stoull(steamId3Match[2].str());

        uint64_t steamId64 = 76561197960265728ULL + accountId;
        result.isValid = true;
        result.steamId64 = steamId64;

        snprintf(const_cast<char*>(input), 18, "%llu", steamId64);

        return result;
    }

    if (inputStr.find("/id/") != std::string::npos) {
        result.errorMessage = "Custom URL format not supported. Please use your SteamID64 directly.";
        return result;
    }

    result.errorMessage = "Invalid Steam ID format. Please enter a valid SteamID64.";
    return result;
}

static bool ValidateSteamID64(const char* input) {
    auto result = ValidateSteamIDFormat(input);

    if (result.isValid) {
        steamId64 = result.steamId64;
        steamIdErrorMessage = "";
        showSteamIdError = false;
        return true;
    }
    else {
        steamIdErrorMessage = result.errorMessage;
        showSteamIdError = true;
        return false;
    }
}
#pragma endregion

#pragma region console shit
static bool showConsole = false;
static bool g_debugConsole = false;
static std::queue<std::pair<std::string, ImVec4>> consoleMessages;
static std::mutex consoleMutex;
static const size_t MAX_CONSOLE_MESSAGES = 100;
static char consoleInputBuffer[256] = "";
static ImVec4 infoColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 successColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 errorColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 warningColor = ImVec4(1.0f, 0.7f, 0.0f, 1.0f);

std::string GetTimestamp() {
    try {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_time);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", &now_tm);
        return std::string(timestamp);
    }
    catch (...) {
        return "[TIME] ";
    }
}
void AddConsoleMessage(const std::string& message, const ImVec4& color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)) {
    try {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::string timestamp = GetTimestamp();
        consoleMessages.push(std::make_pair(timestamp + message, color));

        while (consoleMessages.size() > MAX_CONSOLE_MESSAGES) {
            consoleMessages.pop();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Console error: " << e.what() << " - Failed to add: " << message << std::endl;
    }
}
void LogInfo(const std::string& message) {
    try {
        AddConsoleMessage("[INFO] " + message, infoColor);
    }
    catch (...) {
        std::cout << GetTimestamp() << "[INFO] " << message << std::endl;
    }
}
void LogSuccess(const std::string& message) {
    try {
        AddConsoleMessage("[SUCCESS] " + message, successColor);
    }
    catch (...) {
        std::cout << GetTimestamp() << "[SUCCESS] " << message << std::endl;
    }
}
void LogError(const std::string& message) {
    try {
        AddConsoleMessage("[ERROR] " + message, errorColor);
    }
    catch (...) {
        std::cerr << GetTimestamp() << "[ERROR] " << message << std::endl;
    }
}
void LogWarning(const std::string& message) {
    try {
        AddConsoleMessage("[WARNING] " + message, warningColor);
    }
    catch (...) {
        std::cout << GetTimestamp() << "[WARNING] " << message << std::endl;
    }
}
void SafeClearConsole() {
    try {
        std::lock_guard<std::mutex> lock(consoleMutex);
        std::queue<std::pair<std::string, ImVec4>> empty;
        std::swap(consoleMessages, empty);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to clear console: " << e.what() << std::endl;
        try {
            consoleMessages.push(std::make_pair(GetTimestamp() + "[ERROR] Failed to clear console: " + e.what(), errorColor));
        }
        catch (...) {
        }
    }
}
void DrawConsoleWindow() {
    if (!showConsole) return;

    try {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Developer Console", &showConsole)) {
            const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, -footerHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::queue<std::pair<std::string, ImVec4>> tempQueue;
                {
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    tempQueue = consoleMessages;        
                }

                while (!tempQueue.empty()) {
                    auto& message = tempQueue.front();
                    ImGui::TextColored(message.second, "%s", message.first.c_str());
                    tempQueue.pop();
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            ImGui::Separator();

            bool reclaim_focus = false;
            if (ImGui::InputText("##ConsoleInput", consoleInputBuffer, IM_ARRAYSIZE(consoleInputBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (strlen(consoleInputBuffer) > 0) {
                    std::string command(consoleInputBuffer);
                    LogInfo("> " + command);

                    if (command == "clear") {
                        SafeClearConsole();
                        LogInfo("Console cleared");
                    }
                    else if (command == "help") {
                        SafeClearConsole();
                        LogInfo("Available commands:");
                        LogInfo(" clear - Clear console messages");
                        LogInfo(" help - Show available commands");
                        LogInfo(" exit - Close console window");
                        LogInfo(" discord - Opens my discord link");
                        LogInfo(" github - Opens my Github link");
                        LogInfo(" credits - Me");
                        LogInfo(" ping - pong");
						LogInfo(" donate - Not needed but is very welcomed!");
						LogInfo(" kill - Close the save editor entirely");
						LogInfo(" changeid - Change your Steam ID");
						LogInfo(" changesave - Change your chosen Save");
					}
                    else if (command == "exit") {
                        showConsole = false;
                    }
                    else if (command == "ping") {
                        LogInfo("pong");
                    }
                    else if (command == "discord") {
                        OpenURL("https://discord.gg/mFAxKpT457");
                        LogInfo("pong");
                    }
                    else if (command == "github") {
                        OpenURL("https://github.com/regularlunar");
                    }
                    else if (command == "credits") {
						LogInfo("You really thought ther people helped me?");
                    }
                    else if (command == "changeid") {
                        showConsole = false;
						current_window = 0;
						g_debugConsole = false;
                    }
                    else if (command == "changesave") {
                        showConsole = false;
                        current_window = 1;
                    }
                    else if (command == "kill") {
                        
                    }
                    
                    else if (command == "donate") {
                        LogSuccess("$$$");
						OpenURL("https://www.ko-fi.com/regularlunar");
                    }
                    /*
                    else if (command == "exit") {

                    }
                    else if (command == "exit") {

                    }
                    */
                    else {
                        LogError(":/ Sorry, but thats an unknown command! Try using the command: help");
                    }

                    consoleInputBuffer[0] = '\0';
                    reclaim_focus = true;
                }
            }

            ImGui::SetItemDefaultFocus();
            if (reclaim_focus)
                ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in console window rendering: " << e.what() << std::endl;
        showConsole = false;
    }
}
#pragma endregion


bool g_AppearanceChkBox = false;
bool g_CarAppearanceChkBox = false;
bool g_MoneyChkBox = false;
bool g_RankChkBox = false;
bool g_EditRelationShip = false;
bool g_DrugNameEditor = false;
bool g_DrugColorEditor = false;
bool g_SaveNameEditor = false;

bool g_UnlockAllNPC = false;
bool g_UnlockAllCustomer = false;
bool g_UnlockAllDealer = false;
bool g_UnlockAllSupplier = false;
bool g_UnlockAllBusinessProperties = false;
bool g_UnlockAllMoneyLaunderingProperties = false;

bool anyActionTaken = false;
bool allActionsSuccessful = true;
static bool operationSuccess = false;
static std::string statusMessage = "";
static float statusMessageTimer = 0.0f;

static int selectedSaveGame = 1;

static int selected_tab = 0;

const char* tab_labels[] = {
	"Home", "Player", "NPCs", "Customization", "Properties", "DEV DEBUG"
};

const char* window_names[] = {
    "Auth Window", "Save Game Window", "Main Window"
};

static float relationDeltaValue = 1.0f;
static int selectedTier = 0;
static int selectedRank = 0;
static int customTierValue = 1;
static float g_CashAmount = 0.0f;
static float g_BankAmount = 0.0f;

static const char* regularTiers[] = { "I", "II", "III", "IV", "V" };
static const char* kingpinTiers[] = { "I", "II", "III", "IV", "V", "Custom" };
const bool isKingpin = (selectedRank == 10);
const char** currentTiers = isKingpin ? kingpinTiers : regularTiers;
const int tierCount = isKingpin ? 6 : 5;
const char* currentTierName = (selectedTier < tierCount) ? currentTiers[selectedTier] : currentTiers[0];

void Drawing::Active() {
    bDraw = true;
}
bool Drawing::isActive() {
    return bDraw == true;
}
void DrawSectionHeader(const char* text) {
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(text).x / 2));
    ImGui::Text("%s", text);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();
}
void StatusMessage() {
    if (!statusMessage.empty()) {
        ImVec4 textColor = operationSuccess ?
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        ImGui::TextColored(textColor, "%s", statusMessage.c_str());
        if (ImGui::Button("Launch Schedule 1")) {
            OpenURL("steam://run/3164500//");
        }
        if (!operationSuccess) {
            if (ImGui::Button("Change Steam ID")) {
                current_window = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Change Save")) {
                current_window = 1;
            }
        }
        statusMessageTimer += ImGui::GetIO().DeltaTime;
        if (statusMessageTimer >= 3.0f) {
            statusMessage.clear();
            operationSuccess = false;
            statusMessageTimer = 0.0f;
        }
    }
}
void ApplyChanges() {
    if (anyActionTaken) {
        if (allActionsSuccessful) {
            statusMessage = "Successfully applied changes! Load back into your save! :D";
            operationSuccess = true;
        }
        else {
            statusMessage = "Error: Failed to apply some or all changes.";
            operationSuccess = false;
        }
        statusMessageTimer = 0.0f;
    }
    else {
        statusMessage = "No options selected. Please select at least one action.";
        operationSuccess = false;
        statusMessageTimer = 0.0f;
    }
}
void SaveIDMessage() {
	ImGui::Separator();
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Selected Save: %d").x / 2));
    ImGui::Text("Selected Save: %d", selectedSaveGame);
}

void Drawing::Draw() {
    if (isActive()) {
        DrawConsoleWindow();
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowBgAlpha(1.0f);
        if (current_window == 0) {
            ImGui::SetNextWindowSize(vSecondaryWindowSize, ImGuiCond_Appearing);
            if (ImGui::Begin(window_names[0], &bDraw, WindowFlags)) {
                DrawSectionHeader("Input Steam ID To Continue");
                ImGui::TextWrapped("Enter your SteamID64, SteamID, or SteamID3:");
                ImGui::Spacing();
                bool inputChanged = ImGui::InputText("SteamID", steamIdBuffer, IM_ARRAYSIZE(steamIdBuffer));
                if (showSteamIdError && !steamIdErrorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", steamIdErrorMessage.c_str());
                }
                ImGui::Spacing();
                ImGui::TextWrapped("Valid formats:");
                ImGui::BulletText("SteamID64: 76561198012345678");
                ImGui::BulletText("SteamID: STEAM_0:0:12345678");
                ImGui::BulletText("SteamID3: [U:1:12345678]");
                if (ImGui::Button("How to find Steam ID?")) 
                {
                    OpenURL("https://help.steampowered.com/en/faqs/view/2816-BE67-5B69-0FEC");
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("Continue")) {
                    steamIdValid = ValidateSteamID64(steamIdBuffer);
                    if (steamIdValid) {
                        current_window = 1;
                    }
                }
                ImGui::End();
            }
        }
        else if (current_window == 1) {
            ImGui::SetNextWindowSize(vSecondaryWindowSize, ImGuiCond_Appearing);
            if (ImGui::Begin(window_names[1], &bDraw, WindowFlags)) {
                DrawSectionHeader("Please Choose Your Save");
                static const char* saveGames[] = { "Save 1", "Save 2", "Save 3", "Save 4", "Save 5" };
                int currentIndex = selectedSaveGame - 1;
                if (ImGui::BeginCombo("Select Save", saveGames[currentIndex])) {
                    for (int i = 0; i < IM_ARRAYSIZE(saveGames); i++) {
                        bool isSelected = (currentIndex == i);
                        if (ImGui::Selectable(saveGames[i], isSelected)) {
                            selectedSaveGame = i + 1;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Button("Continue")) {
                    current_window = 2;
                }
                ImGui::End();
            }
        }
        else if (current_window == 2) {
            ImGui::SetNextWindowBgAlpha(1.0f);
            ImGui::SetNextWindowSize(vMainWindowSize, ImGuiCond_Appearing);
            ImGui::Begin(window_names[2], &bDraw, WindowFlags);
            {
                ImGui::BeginChild("TabsColumn", ImVec2(150, 0), true);
                {
                    for (int i = 0; i < IM_ARRAYSIZE(tab_labels); i++) {
                        bool is_selected = (selected_tab == i);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
                        if (ImGui::Button(tab_labels[i], ImVec2(-FLT_MIN, 40)))
                            selected_tab = i;
                        if (is_selected)
                            ImGui::PopStyleColor();
                    }
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::BeginChild("ContentArea", ImVec2(0, 0), true);
                {
                    switch (selected_tab) {
                    case 0:
                        DrawSectionHeader("Welcome Home!");
                        ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("text").x / 2));
                        ImGui::TextWrapped("");
                        break;
                    case 1:
                    #pragma region Player
                        DrawSectionHeader("Player Editor");
                        ImGui::Checkbox("Edit Cash/Bank", &g_MoneyChkBox);
                        ImGui::Checkbox("Edit Player Rank", &g_RankChkBox);
                        if (g_RankChkBox) {
                            ImGui::Separator();
                            ImGui::Text("What rank do you want?");
                            static const char* ranks[] = {
                                "Street Rat", "Hoodlum", "Peddler", "Hustler", "Bagman",
                                "Enforcer", "Shot Caller", "Block Boss", "Underlord", "Baron", "Kingpin"
                            };
                            if (ImGui::BeginCombo("##RankSelector", ranks[selectedRank])) {
                                for (int n = 0; n < IM_ARRAYSIZE(ranks); n++) {
                                    bool isSelected = (selectedRank == n);
                                    if (ImGui::Selectable(ranks[n], isSelected)) {
                                        selectedRank = n;
                                        if (selectedRank != 10 && selectedTier == 5) {
                                            selectedTier = 0;
                                        }
                                    }
                                    if (isSelected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::Separator();
                            ImGui::Text("What tier?");
                            const bool isKingpin = (selectedRank == 10);
                            const char* regularTiers[] = { "I", "II", "III", "IV", "V" };
                            const char* kingpinTiers[] = { "I", "II", "III", "IV", "V", "Custom" };
                            const char** currentTiers = isKingpin ? kingpinTiers : regularTiers;
                            const int tierCount = isKingpin ? 6 : 5;
                            const char* currentTierName = (selectedTier < tierCount) ?
                                currentTiers[selectedTier] : currentTiers[0];
                            if (ImGui::BeginCombo("##TierSelector", currentTierName)) {
                                for (int n = 0; n < tierCount; n++) {
                                    bool isSelected = (selectedTier == n);
                                    if (ImGui::Selectable(currentTiers[n], isSelected)) {
                                        selectedTier = n;
                                    }
                                    if (isSelected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                            if (isKingpin && selectedTier == 5) {
                                ImGui::InputInt("Custom Tier Value", &customTierValue);
                                if (customTierValue < 1) customTierValue = 1;
                            }
                        }
                        if (g_MoneyChkBox) {
                            ImGui::Separator();
                            ImGui::Text("Edit Cash:");
                            ImGui::InputFloat("Cash Amount", &g_CashAmount);
                            ImGui::Text("Edit Bank:");
                            ImGui::InputFloat("Bank Amount", &g_BankAmount);
                        }
                        SaveIDMessage();
                        if (ImGui::Button("Apply Changes")) {
                            std::string steamIdStr(steamIdBuffer);
                            if (g_CashAmount) {
                                bool success = JsonHandler::UpdatePlayerCash(steamIdStr, selectedSaveGame, g_CashAmount);
                                anyActionTaken = true;
                            }
                            if (g_BankAmount) {
                                bool success = JsonHandler::UpdatePlayerBank(steamIdStr, selectedSaveGame, g_BankAmount);
                                anyActionTaken = true;
                            }
                            if (g_RankChkBox) {
                                bool success = JsonHandler::UpdatePlayerRank(
                                    steamIdStr,
                                    selectedSaveGame,
                                    selectedRank,
                                    selectedTier,
                                    (selectedRank == 10 && selectedTier == 5) ? customTierValue : 0
                                );
                                anyActionTaken = true;
                                if (!success) allActionsSuccessful = false;
                            }
                            ApplyChanges();
                        }
                        StatusMessage();
#pragma endregion
                        break;
                    case 2:
                    #pragma region NPCS
                        DrawSectionHeader("NPCs");
                        ImGui::Checkbox("Unlock All Dealers", &g_UnlockAllDealer);
                        ImGui::Checkbox("Unlock All Suppliers", &g_UnlockAllSupplier);
                        ImGui::Checkbox("Unlock All Customers", &g_UnlockAllCustomer);
                        ImGui::Checkbox("Edit Relationships", &g_EditRelationShip);
                        if (g_EditRelationShip) {
                            ImGui::Separator();
                            ImGui::Text("Relationship Value:");
                            ImGui::SliderFloat("##RelationDelta", &relationDeltaValue, 1.0f, 5.0f, "%.1f");
                        }
                        SaveIDMessage();
                        if (ImGui::Button("Apply Changes")) {
                            std::string steamIdStr(steamIdBuffer);
                            if (g_EditRelationShip) {
                                bool success = JsonHandler::UpdateAllNPCRelationDeltas(
                                    steamIdStr, selectedSaveGame, relationDeltaValue);
                                anyActionTaken = true;
                                if (!success) allActionsSuccessful = false;
                            }
                            if (g_UnlockAllDealer) {
                                anyActionTaken = true;
                            }
                            if (g_UnlockAllSupplier) {
                                anyActionTaken = true;
                            }
                            if (g_UnlockAllCustomer) {
                                anyActionTaken = true;
                            }
                            ApplyChanges();
                        }
                        StatusMessage();
                    #pragma endregion
                        break;
                    case 3:
                    #pragma region Customization
                        DrawSectionHeader("Colors, Accessories, etc");
                        ImGui::Checkbox("Rename Drugs", &g_DrugNameEditor);
                        ImGui::Checkbox("Drug Color(s)", &g_DrugColorEditor);
                        ImGui::Checkbox("Save Name Changer", &g_SaveNameEditor);
                        ImGui::Checkbox("Player Appearance", &g_AppearanceChkBox);
                        ImGui::Checkbox("Car Colors", &g_CarAppearanceChkBox);
                        SaveIDMessage();
                        if (ImGui::Button("Apply Changes")) {
                            std::string steamIdStr(steamIdBuffer);
                            if (g_EditRelationShip) {
                                anyActionTaken = true;
                            }
                            ApplyChanges();
                        }
                        StatusMessage();
                    #pragma endregion
                        break;
                    case 4:
                    #pragma region Properties
                        DrawSectionHeader("Properties");
                        ImGui::Checkbox("Unlock All Properties", &g_UnlockAllMoneyLaunderingProperties);
                        ImGui::Checkbox("Unlock All Businesses", &g_UnlockAllBusinessProperties);
                        SaveIDMessage();
                        if (ImGui::Button("Apply Changes")) {
                            std::string steamIdStr(steamIdBuffer);
                            if (g_UnlockAllMoneyLaunderingProperties) {
                                bool success = JsonHandler::UpdateAllPropertyOwnerships(steamIdStr, selectedSaveGame, g_UnlockAllMoneyLaunderingProperties);
                                anyActionTaken = true;
                                if (!success) allActionsSuccessful = false;
                            }
                            if (g_UnlockAllBusinessProperties) {
                                bool success = JsonHandler::UpdateAllBusinessOwnerships(steamIdStr, selectedSaveGame, g_UnlockAllBusinessProperties);
                                anyActionTaken = true;
                                if (!success) allActionsSuccessful = false;
                            }
                            ApplyChanges();
                        }
                        StatusMessage();
                    #pragma endregion
                        break;
                    case 5:
                    #pragma region Dev Tab
                        DrawSectionHeader("DEV DEBUG");
                        if (ImGui::Checkbox("Toggle Debug Console", &g_debugConsole)) {
                            showConsole = !showConsole;
                            if (showConsole) {
                                SafeClearConsole();
                                LogInfo("Hi :D");
                            }
                        }
                        ImGui::Text("Save Path Info:");
                        std::string baseSavePath = JsonHandler::GetSaveBasePath();
                        ImGui::TextWrapped("Base Save Path: %s", baseSavePath.c_str());
                        if (ImGui::Button("Check Save Path Exists")) {
                            bool pathExists = fs::exists(baseSavePath);
                            statusMessage = pathExists ?
                                "Save path exists!" :
                                "Save path does not exist! Check if game is installed.";
                            operationSuccess = pathExists;
                            statusMessageTimer = 0.0f;
                        }
                        ImGui::Separator();
                        ImGui::Text("Current Save Inspection:");
                        static char specificNpcName[64] = "";
                        ImGui::InputText("NPC Name", specificNpcName, IM_ARRAYSIZE(specificNpcName));

                        if (ImGui::Button("Test NPC Relationship Read")) {
                            std::string steamIdStr(steamIdBuffer);
                            std::string npcName(specificNpcName);
                            if (!npcName.empty()) {
                                nlohmann::json npcData = JsonHandler::ReadNPCJson(steamIdStr, selectedSaveGame, npcName);
                                if (!npcData.is_null() && npcData.contains("RelationDelta")) {
                                    float relationValue = npcData["RelationDelta"];
                                    statusMessage = "NPC " + npcName + " relationship value: " + std::to_string(relationValue);
                                    operationSuccess = true;
                                }
                                else {
                                    statusMessage = "Failed to read relationship for NPC: " + npcName;
                                    operationSuccess = false;
                                }
                                statusMessageTimer = 0.0f;
                            }
                        }

                        ImGui::Separator();
                        ImGui::Text("Save Statistics:");

                        if (ImGui::Button("Count Save Files")) {
                            std::string steamIdStr(steamIdBuffer);
                            int npcCount = JsonHandler::GetAllNPCNames(steamIdStr, selectedSaveGame).size();
                            int propertyCount = JsonHandler::GetAllPropertyNames(steamIdStr, selectedSaveGame).size();
                            int businessCount = JsonHandler::GetAllBusinessNames(steamIdStr, selectedSaveGame).size();

                            statusMessage = "NPCs: " + std::to_string(npcCount) +
                                " | Properties: " + std::to_string(propertyCount) +
                                " | Businesses: " + std::to_string(businessCount);
                            operationSuccess = true;
                            statusMessageTimer = 0.0f;
                        }

                        ImGui::Separator();
                        ImGui::Text("Money Debug:");

                        if (ImGui::Button("Show Current Money Values")) {
                            std::string steamIdStr(steamIdBuffer);
                            nlohmann::json bankData = JsonHandler::ReadBankJson(steamIdStr, selectedSaveGame);
                            nlohmann::json cashData = JsonHandler::ReadCashJson(steamIdStr, selectedSaveGame);

                            if (!bankData.is_null() && bankData.contains("OnlineBalance")) {
                                float currentBank = bankData["OnlineBalance"];
                                statusMessage = "Current Bank: $" + std::to_string(currentBank);

                                bool foundCash = false;
                                if (!cashData.is_null() && cashData.contains("Items")) {
                                    for (auto& itemStr : cashData["Items"]) {
                                        if (itemStr.is_string()) {
                                            try {
                                                nlohmann::json item = nlohmann::json::parse(itemStr.get<std::string>());
                                                if (item.contains("ID") && item["ID"] == "cash" && item.contains("CashBalance")) {
                                                    float currentCash = item["CashBalance"];
                                                    statusMessage += " | Current Cash: $" + std::to_string(currentCash);
                                                    foundCash = true;
                                                    break;
                                                }
                                            }
                                            catch (...) {
                                            }
                                        }
                                    }
                                }

                                if (!foundCash) {
                                    statusMessage += " | Cash data not available";
                                }
                                operationSuccess = true;
                            }
                            else {
                                statusMessage = "Failed to read bank data";
                                operationSuccess = false;
                            }
                            statusMessageTimer = 0.0f;
                        }

                        ImGui::Separator();
                        ImGui::Text("Rank Debug:");

                        if (ImGui::Button("Show Current Rank")) {
                            std::string steamIdStr(steamIdBuffer);
                            nlohmann::json rankData = JsonHandler::ReadRankJson(steamIdStr, selectedSaveGame);

                            if (!rankData.is_null() && rankData.contains("Rank") && rankData.contains("Tier") && rankData.contains("TotalXP")) {
                                int currentRank = rankData["Rank"];
                                int currentTier = rankData["Tier"];
                                int currentXP = rankData["TotalXP"];

                                static const char* rankNames[] = {
                                    "Street Rat", "Hoodlum", "Peddler", "Hustler", "Bagman",
                                    "Enforcer", "Shot Caller", "Block Boss", "Underlord", "Baron", "Kingpin"
                                };

                                static const char* tierNumerals[] = {
                                    "I", "II", "III", "IV", "V", "Custom"
                                };

                                std::string rankName = (currentRank >= 0 && currentRank < 11) ?
                                    rankNames[currentRank] : "Unknown";

                                std::string tierName = (currentTier >= 0 && currentTier < 6) ?
                                    tierNumerals[currentTier] : "Unknown";

                                statusMessage = "Current Rank: " + rankName + " " + tierName +
                                    " | XP: " + std::to_string(currentXP);
                                operationSuccess = true;
                            }
                            else {
                                statusMessage = "Failed to read rank data";
                                operationSuccess = false;
                            }
                            statusMessageTimer = 0.0f;
                        }

                        ImGui::Separator();
                        ImGui::Text("File Validation:");

                        static char specificPropertyName[64] = "";
                        ImGui::InputText("Property Name", specificPropertyName, IM_ARRAYSIZE(specificPropertyName));

                        if (ImGui::Button("Check Property JSON")) {
                            std::string steamIdStr(steamIdBuffer);
                            std::string propertyName(specificPropertyName);
                            if (!propertyName.empty()) {
                                std::string propertyPath = JsonHandler::GetPropertyJsonPath(steamIdStr, selectedSaveGame, propertyName);
                                bool fileExists = fs::exists(propertyPath);
                                statusMessage = fileExists ?
                                    "Property file exists: " + propertyPath :
                                    "Property file does not exist: " + propertyPath;
                                operationSuccess = fileExists;
                                statusMessageTimer = 0.0f;
                            }
                        }

                        ImGui::Separator();
                        ImGui::Text("Debug Actions:");

                        if (ImGui::Button("Reset Status Message")) {
                            statusMessage = "";
                            statusMessageTimer = 0.0f;
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Back to Steam ID")) {
                            current_window = 0;
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Open Save Directory")) {
                            std::string steamIdStr(steamIdBuffer);
                            std::string saveDirPath = JsonHandler::GetSaveBasePath() +
                                steamIdStr + "\\SaveGame_" + std::to_string(selectedSaveGame);

                            if (fs::exists(saveDirPath)) {
                                std::string command = "explorer \"" + saveDirPath + "\"";
                                system(command.c_str());
                                statusMessage = "Opening save directory...";
                                operationSuccess = true;
                            }
                            else {
                                statusMessage = "Save directory does not exist: " + saveDirPath;
                                operationSuccess = false;
                            }
                            statusMessageTimer = 0.0f;
                        }
                        StatusMessage();
#pragma endregion
                        break;
                    }
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
    }
}
#ifdef _WINDLL
    if (GetAsyncKeyState(VK_INSERT) & 1)
        bDraw = !bDraw;
#endif