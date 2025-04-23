#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "json.hpp"     
namespace fs = std::filesystem;

class JsonHandler {
public:
    //==========================================================================================//
    // Functions
    //==========================================================================================//
    static std::string GetSaveBasePath();
    static bool WriteJsonToFile(const std::string& filePath, const nlohmann::json& jsonData);
    static nlohmann::json ReadJsonFromFile(const std::string& filePath);

    //==========================================================================================//
    // NPCs
    //==========================================================================================//
    static std::string GetNPCRelationshipJsonPath(const std::string& steamId, int saveNumber, const std::string& npcName);
    static std::vector<std::string> GetAllNPCNames(const std::string& steamId, int saveNumber);
    static nlohmann::json ReadNPCJson(const std::string& steamId, int saveNumber, const std::string& npcName);
    static bool UpdateRelationDelta(const std::string& steamId, int saveNumber, const std::string& npcName, float newDeltaValue);
    static bool UpdateAllNPCRelationDeltas(const std::string& steamId, int saveNumber, float newDeltaValue);

    //==========================================================================================//
    // Properties
    //==========================================================================================//
    static std::string GetPropertyJsonPath(const std::string& steamId, int saveNumber, const std::string& propertyName);
    static std::vector<std::string> GetAllPropertyNames(const std::string& steamId, int saveNumber);
    static nlohmann::json ReadPropertyJson(const std::string& steamId, int saveNumber, const std::string& propertyName);
    static bool UpdatePropertyOwnership(const std::string& steamId, int saveNumber, const std::string& propertyName, bool isOwned);
    static bool UpdateAllPropertyOwnerships(const std::string& steamId, int saveNumber, bool isOwned);

    //==========================================================================================//
    // Businesses
    //==========================================================================================//
    static std::string GetBusinessJsonPath(const std::string& steamId, int saveNumber, const std::string& businessName);
    static std::vector<std::string> GetAllBusinessNames(const std::string& steamId, int saveNumber);
    static nlohmann::json ReadBusinessJson(const std::string& steamId, int saveNumber, const std::string& businessName);
    static bool UpdateBusinessOwnership(const std::string& steamId, int saveNumber, const std::string& businessName, bool isOwned);
    static bool UpdateAllBusinessOwnerships(const std::string& steamId, int saveNumber, bool isOwned);

    //==========================================================================================//
    // Ranks
    //==========================================================================================//
    static std::string GetRankJsonPath(const std::string& steamId, int saveNumber);
    static nlohmann::json ReadRankJson(const std::string& steamId, int saveNumber);
    static bool UpdatePlayerRank(const std::string& steamId, int saveNumber, int rankIndex, int tierIndex, int customTierValue = 0);
    static int CalculateXPForRank(int rankIndex, int tierIndex, int customTierValue = 0);

    //==========================================================================================//
    // Bank
    //==========================================================================================//
    static std::string GetBankJsonPath(const std::string& steamId, int saveNumber);
	static nlohmann::json ReadBankJson(const std::string& steamId, int saveNumber);
    static bool UpdatePlayerBank(const std::string& steamId, int saveNumber, float bankAmount);

    //==========================================================================================//
    // Cash
    //==========================================================================================//
    static std::string GetCashJsonPath(const std::string& steamId, int saveNumber);
    static nlohmann::json ReadCashJson(const std::string& steamId, int saveNumber);
    static bool UpdatePlayerCash(const std::string& steamId, int saveNumber, float cashAmount);

    //==========================================================================================//
    // Console Logging
    //==========================================================================================//
    static void LogErrorToConsole(const std::string& message);
    static void LogInfoToConsole(const std::string& message);
    static void LogSuccessToConsole(const std::string& message);
    static void LogWarningToConsole(const std::string& message);

private:
    static const char* RANK_NAMES[];
    static const char* TIER_NUMERALS[];
    static const int RANK_XP_TABLE[11][6];
    static const int KINGPIN_XP_PER_TIER;
};