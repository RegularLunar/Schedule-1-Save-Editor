#include "JsonHandler.h"
extern void LogInfo(const std::string& message);
extern void LogSuccess(const std::string& message);
extern void LogError(const std::string& message);
extern void LogWarning(const std::string& message);
void JsonHandler::LogErrorToConsole(const std::string& message) {
    std::cerr << message << std::endl;      
    LogError(message);     
}
void JsonHandler::LogInfoToConsole(const std::string& message) {
    std::cout << message << std::endl;      
    LogInfo(message);     
}
void JsonHandler::LogSuccessToConsole(const std::string& message) {
    std::cout << message << std::endl;      
    LogSuccess(message);     
}
void JsonHandler::LogWarningToConsole(const std::string& message) {
    std::cout << "WARNING: " << message << std::endl;      
    LogWarning(message);     
}
const char* JsonHandler::RANK_NAMES[] = {
    "Street Rat", "Hoodlum", "Peddler", "Hustler", "Bagman",
    "Enforcer", "Shot Caller", "Block Boss", "Underlord", "Baron", "Kingpin"
};
const char* JsonHandler::TIER_NUMERALS[] = {
    "I", "II", "III", "IV", "V", "Custom"
};
const int JsonHandler::RANK_XP_TABLE[11][6] = {
    {200,   400,   600,   800,   1000,  1000},
    {1400,  1800,  2200,  2600,  3000,  3000},
    {3625,  4250,  4875,  5500,  6125,  6125},
    {6750,  7575,  8400,  9225,  10050, 10050},
    {11075, 12100, 13125, 14150, 15175, 15175},
    {16425, 17675, 18925, 20175, 21425, 21425},
    {22875, 24325, 25775, 27225, 28675, 28675},
    {30350, 32025, 33700, 35375, 37050, 37050},
    {38925, 40800, 42675, 44550, 46425, 46425},
    {48500, 50575, 52650, 54725, 56800, 56800},
    {59100, 61400, 63700, 66000, 68300, 0},
};
const int JsonHandler::KINGPIN_XP_PER_TIER = 2300;

#pragma region Reuseable Funcs
std::string JsonHandler::GetSaveBasePath() {
    char* appDataPath = nullptr;
    size_t pathSize = 0;
    _dupenv_s(&appDataPath, &pathSize, "LOCALAPPDATA");
    if (appDataPath == nullptr) {
        return "";
    }
    std::string basePath = std::string(appDataPath) + "Low\\TVGS\\Schedule I\\Saves\\";

    free(appDataPath);

    return basePath;
}

nlohmann::json JsonHandler::ReadJsonFromFile(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        LogErrorToConsole("File not found: " + filePath);
        return nlohmann::json();
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogErrorToConsole("Failed to open file: " + filePath);
        return nlohmann::json();
    }

    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception& e) {
        LogErrorToConsole("JSON parsing error for " + filePath + ": " + e.what());
        return nlohmann::json();
    }

    return jsonData;
}

bool JsonHandler::WriteJsonToFile(const std::string& filePath, const nlohmann::json& jsonData) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        LogErrorToConsole("Failed to open & write to file: " + filePath);
        return false;
    }

    try {
        file << jsonData.dump(4);
        return true;
    }
    catch (const std::exception& e) {
        LogErrorToConsole("Error writing to JSON: " + filePath + ": " + e.what());
        return false;
    }
}
#pragma endregion

#pragma region NPCS REP
std::string JsonHandler::GetNPCRelationshipJsonPath(const std::string& steamId, int saveNumber,
    const std::string& npcName) {
    std::string basePath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) +
        "\\NPCs\\" + npcName + "\\Relationship.json";
    return basePath;
}

std::vector<std::string> JsonHandler::GetAllNPCNames(const std::string& steamId, int saveNumber) {
    std::vector<std::string> npcNames;

    std::string npcDirPath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\NPCs\\";

    if (!fs::exists(npcDirPath)) {
		LogErrorToConsole("NPCs directory not found: " + npcDirPath);
        return npcNames;
    }

    try {
        for (const auto& entry : fs::directory_iterator(npcDirPath)) {
            if (entry.is_directory()) {
                npcNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
		LogErrorToConsole("Error accessing NPC directories: " + std::string(e.what()));
    }

    return npcNames;
}

nlohmann::json JsonHandler::ReadNPCJson(const std::string& steamId, int saveNumber,
    const std::string& npcName) {
    std::string fullPath = GetNPCRelationshipJsonPath(steamId, saveNumber, npcName);
    return ReadJsonFromFile(fullPath);
}

bool JsonHandler::UpdateRelationDelta(const std::string& steamId, int saveNumber,
    const std::string& npcName, float newDeltaValue) {

    std::string fullPath = GetNPCRelationshipJsonPath(steamId, saveNumber, npcName);

    if (!fs::exists(fullPath)) {
		LogErrorToConsole("Relationship file not found: " + fullPath);
        return false;
    }

    nlohmann::json relationshipData = ReadNPCJson(steamId, saveNumber, npcName);
    if (relationshipData.is_null()) {
		LogErrorToConsole("Failed to read relationship data for " + npcName);
        return false;
    }

    if (!relationshipData.contains("RelationDelta")) {
		LogErrorToConsole("RelationDelta field not found in " + fullPath);
        return false;
    }

    relationshipData["RelationDelta"] = newDeltaValue;

    if (WriteJsonToFile(fullPath, relationshipData)) {
		LogInfoToConsole("Updated relationship delta for " + npcName);
        return true;
    }

    return false;
}

bool JsonHandler::UpdateAllNPCRelationDeltas(const std::string& steamId, int saveNumber, float newDeltaValue) {
    std::vector<std::string> npcNames = GetAllNPCNames(steamId, saveNumber);

    if (npcNames.empty()) {
		LogErrorToConsole("No NPCs found in save " + std::to_string(saveNumber) +
			" for Steam ID " + steamId);
        return false;
    }

    bool allSuccessful = true;
    int successCount = 0;

    for (const auto& npc : npcNames) {
        if (UpdateRelationDelta(steamId, saveNumber, npc, newDeltaValue)) {
            successCount++;
        }
        else {
            allSuccessful = false;
        }
    }

	LogSuccessToConsole("Updated " + std::to_string(successCount) +
		" out of " + std::to_string(npcNames.size()) + " NPCs");

    return allSuccessful;
}
#pragma endregion

#pragma region PROPERTIES
std::string JsonHandler::GetPropertyJsonPath(const std::string& steamId, int saveNumber,
    const std::string& propertyName) {
    std::string basePath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) +
        "\\Properties\\" + propertyName + "\\Property.json";

    return basePath;
}

std::vector<std::string> JsonHandler::GetAllPropertyNames(const std::string& steamId, int saveNumber) {
    std::vector<std::string> propertyNames;
    std::string propertyDirPath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\Properties\\";

    if (!fs::exists(propertyDirPath)) {
		LogErrorToConsole("Properties directory not found: " + propertyDirPath);
        return propertyNames;
    }

    try {
        for (const auto& entry : fs::directory_iterator(propertyDirPath)) {
            if (entry.is_directory()) {
                propertyNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
		LogErrorToConsole("Error accessing Property directories: " + std::string(e.what()));
		return propertyNames;
    }

    return propertyNames;
}

nlohmann::json JsonHandler::ReadPropertyJson(const std::string& steamId, int saveNumber,
    const std::string& propertyName) {
    std::string fullPath = GetPropertyJsonPath(steamId, saveNumber, propertyName);
    return ReadJsonFromFile(fullPath);
}

bool JsonHandler::UpdatePropertyOwnership(const std::string& steamId, int saveNumber,
    const std::string& propertyName, bool isOwned) {

    std::string fullPath = GetPropertyJsonPath(steamId, saveNumber, propertyName);

    if (!fs::exists(fullPath)) {
        LogErrorToConsole("Property file not found: " + fullPath);
        return false;
    }

    nlohmann::json propertyData = ReadPropertyJson(steamId, saveNumber, propertyName);
    if (propertyData.is_null()) {
		LogErrorToConsole("Failed to read property data for " + propertyName);
        return false;
    }

    if (!propertyData.contains("IsOwned")) {
		LogErrorToConsole("IsOwned field not found in " + fullPath);
        return false;
    }

    propertyData["IsOwned"] = isOwned;

    if (WriteJsonToFile(fullPath, propertyData)) {
        LogInfoToConsole("updated ownership status for " + propertyName);
        return true;
    }

    return false;
}

bool JsonHandler::UpdateAllPropertyOwnerships(const std::string& steamId, int saveNumber, bool isOwned) {
    std::vector<std::string> propertyNames = GetAllPropertyNames(steamId, saveNumber);

    if (propertyNames.empty()) {
		LogErrorToConsole("No Properties found in save " + std::to_string(saveNumber) +
			" for Steam ID " + steamId);
        return false;
    }

    bool allSuccessful = true;
    int successCount = 0;

    for (const auto& property : propertyNames) {
        if (UpdatePropertyOwnership(steamId, saveNumber, property, isOwned)) {
            successCount++;
        }
        else {
            allSuccessful = false;
        }
    }

	LogSuccessToConsole("Updated " + std::to_string(successCount) + " out of " + std::to_string(propertyNames.size()) + " Properties");

    return allSuccessful;
}
#pragma endregion

#pragma region BUSINESSES
std::string JsonHandler::GetBusinessJsonPath(const std::string& steamId, int saveNumber,
    const std::string& businessName) {
    std::string basePath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) +
        "\\Businesses\\" + businessName + "\\Business.json";

    return basePath;
}

std::vector<std::string> JsonHandler::GetAllBusinessNames(const std::string& steamId, int saveNumber) {
    std::vector<std::string> businessNames;
    std::string businessDirPath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\Businesses\\";

    if (!fs::exists(businessDirPath)) {
		LogErrorToConsole("Businesses directory not found: " + businessDirPath);
        return businessNames;
    }

    try {
        for (const auto& entry : fs::directory_iterator(businessDirPath)) {
            if (entry.is_directory()) {
                businessNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
		LogErrorToConsole("Error accessing Business directories: " + std::string(e.what()));
    }

    return businessNames;
}

nlohmann::json JsonHandler::ReadBusinessJson(const std::string& steamId, int saveNumber,
    const std::string& businessName) {
    std::string fullPath = GetBusinessJsonPath(steamId, saveNumber, businessName);
    return ReadJsonFromFile(fullPath);
}

bool JsonHandler::UpdateBusinessOwnership(const std::string& steamId, int saveNumber,
    const std::string& businessName, bool isOwned) {

    std::string fullPath = GetBusinessJsonPath(steamId, saveNumber, businessName);

    if (!fs::exists(fullPath)) {
		LogErrorToConsole("Business file not found: " + fullPath);
        return false;
    }

    nlohmann::json businessData = ReadBusinessJson(steamId, saveNumber, businessName);
    if (businessData.is_null()) {
		LogErrorToConsole("Failed to read business data for " + businessName);
        return false;
    }

    if (!businessData.contains("IsOwned")) {
		LogErrorToConsole("IsOwned field not found in " + fullPath);
        return false;
    }

    businessData["IsOwned"] = isOwned;

    if (WriteJsonToFile(fullPath, businessData)) {
        LogInfoToConsole("updated ownership status for " + businessName);
        return true;
    }

    return false;
}

bool JsonHandler::UpdateAllBusinessOwnerships(const std::string& steamId, int saveNumber, bool isOwned) {
    std::vector<std::string> businessNames = GetAllBusinessNames(steamId, saveNumber);

    if (businessNames.empty()) {
		LogErrorToConsole("No Businesses found in save " + std::to_string(saveNumber) +
			" for Steam ID " + steamId);
        return false;
    }

    bool allSuccessful = true;
    int successCount = 0;

    for (const auto& business : businessNames) {
        if (UpdateBusinessOwnership(steamId, saveNumber, business, isOwned)) {
            successCount++;
        }
        else {
            allSuccessful = false;
        }
    }

	LogSuccessToConsole("Updated " + std::to_string(successCount) +
		" out of " + std::to_string(businessNames.size()) + " Businesses");

    return allSuccessful;
}
#pragma endregion

#pragma region RANKS
std::string JsonHandler::GetRankJsonPath(const std::string& steamId, int saveNumber) {
    std::string basePath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\Rank.json";
    return basePath;
}

nlohmann::json JsonHandler::ReadRankJson(const std::string& steamId, int saveNumber) {
    std::string fullPath = GetRankJsonPath(steamId, saveNumber);
    return ReadJsonFromFile(fullPath);
}

int JsonHandler::CalculateXPForRank(int rankIndex, int tierIndex, int customTierValue) {
    if (rankIndex < 0 || rankIndex > 10 || tierIndex < 0 || tierIndex > 5) {
		LogErrorToConsole("Invalid rank or tier index: " +
			std::to_string(rankIndex) + ", " + std::to_string(tierIndex));
        return 0;
    }

    if (rankIndex == 10 && tierIndex == 5) {
        if (customTierValue <= 5) {
            return RANK_XP_TABLE[10][customTierValue - 1];
        }
        else {
            return 68300 + ((customTierValue - 5) * KINGPIN_XP_PER_TIER);
        }
    }

    return RANK_XP_TABLE[rankIndex][tierIndex];
}

bool JsonHandler::UpdatePlayerRank(const std::string& steamId, int saveNumber,
    int rankIndex, int tierIndex, int customTierValue) {
    std::string fullPath = GetRankJsonPath(steamId, saveNumber);

    std::string dirPath = GetSaveBasePath() + steamId + "\\SaveGame_" + std::to_string(saveNumber);
    if (!fs::exists(dirPath)) {
		LogErrorToConsole("Save directory not found: " + dirPath);
        return false;
    }

    nlohmann::json rankData;
    if (fs::exists(fullPath)) {
        rankData = ReadRankJson(steamId, saveNumber);
        if (rankData.is_null()) {
            rankData = nlohmann::json::object();
        }
    }

    int requiredXP = CalculateXPForRank(rankIndex, tierIndex, customTierValue);

    std::string rankTitle;
    if (rankIndex == 10 && tierIndex == 5) {
        rankTitle = std::string(RANK_NAMES[rankIndex]) + " " + std::to_string(customTierValue);
    }
    else {
        rankTitle = std::string(RANK_NAMES[rankIndex]) + " " + TIER_NUMERALS[tierIndex];
    }

    rankData["Rank"] = rankIndex;
    rankData["Tier"] = tierIndex;
    rankData["XP"] = 0;
    rankData["TotalXP"] = requiredXP;

    if (WriteJsonToFile(fullPath, rankData)) {
		LogSuccessToConsole("Successfully updated rank to " + rankTitle);
        return true;
    }

    return false;
}
#pragma endregion

#pragma region BANK
std::string JsonHandler::GetBankJsonPath(const std::string& steamId, int saveNumber) {
	std::string basePath = GetSaveBasePath() +
		steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\Money.json";
	return basePath;
}
nlohmann::json JsonHandler::ReadBankJson(const std::string& steamId, int saveNumber) {
	std::string fullPath = GetBankJsonPath(steamId, saveNumber);
	return ReadJsonFromFile(fullPath);
}
bool JsonHandler::UpdatePlayerBank(const std::string& steamId, int saveNumber, float bankAmount) {
	std::string fullPath = GetBankJsonPath(steamId, saveNumber);
	if (!fs::exists(fullPath)) {
		LogErrorToConsole("Bank file not found: " + fullPath);
		return false;
	}
	nlohmann::json bankData = ReadBankJson(steamId, saveNumber);
	if (bankData.is_null()) {
		LogErrorToConsole("Failed to read bank data for Steam ID " + steamId);
		return false;
	}
	bankData["OnlineBalance"] = bankAmount;
	if (WriteJsonToFile(fullPath, bankData)) {
		LogErrorToConsole("Successfully updated bank amount to " + std::to_string(bankAmount));
		return true;
	}
	return false;
}
#pragma endregion

#pragma region CASH
std::string JsonHandler::GetCashJsonPath(const std::string& steamId, int saveNumber) {
    std::string basePath = GetSaveBasePath() +
        steamId + "\\SaveGame_" + std::to_string(saveNumber) + "\\Players" + "\\Player_0" + "\\Inventory.json";
    return basePath;
}

nlohmann::json JsonHandler::ReadCashJson(const std::string& steamId, int saveNumber) {
    std::string fullPath = GetCashJsonPath(steamId, saveNumber);
    return ReadJsonFromFile(fullPath);
}

bool JsonHandler::UpdatePlayerCash(const std::string& steamId, int saveNumber, float cashAmount) {
    std::string fullPath = GetCashJsonPath(steamId, saveNumber);

    if (!fs::exists(fullPath)) {
		LogErrorToConsole("Cash file not found: " + fullPath);
        return false;
    }

    nlohmann::json inventoryData = ReadCashJson(steamId, saveNumber);
    if (inventoryData.is_null()) {
		LogErrorToConsole("Failed to read inventory data for Steam ID " + steamId);
        return false;
    }

    if (!inventoryData.contains("Items") || !inventoryData["Items"].is_array()) {
		LogErrorToConsole("Items field not found or not an array in " + fullPath);
        return false;
    }

    bool found = false;
    for (auto& itemStr : inventoryData["Items"]) {
        if (!itemStr.is_string()) {
            continue;
        }

        try {
            nlohmann::json item = nlohmann::json::parse(itemStr.get<std::string>());

            if (item.contains("ID") && item["ID"] == "cash") {
                item["CashBalance"] = cashAmount;

                itemStr = item.dump();
                found = true;
                break;
            }
        }
        catch (const std::exception& e) {
			LogErrorToConsole("Error parsing cash item: " + std::string(e.what()));
            continue;
        }
    }

    if (!found) {
		LogErrorToConsole("Cash item not found in inventory for Steam ID " + steamId);
        return false;
    }

    if (WriteJsonToFile(fullPath, inventoryData)) {
		LogSuccessToConsole("Successfully updated cash amount to " + std::to_string(cashAmount));
        return true;
    }

    return false;
}
#pragma endregion