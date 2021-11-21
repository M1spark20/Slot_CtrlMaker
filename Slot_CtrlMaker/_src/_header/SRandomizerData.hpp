#pragma once
#include <vector>
#include <array>
#include <string>

enum class EFlagType{
	eFlagID,
	eBonusID,
	eIsReachCheck,
	eFlagTypeMax
};

// [act]フラグデータ格納構造体
struct SFlagTable {
	int flagID, bonusID;
	int dataID;
	int reachCheckFlag;
	std::string flagName, bonusName;
};

struct SRandTable{
	int set;
	int randMax;
	int bet;
	int gameMode;
	int rtMode;
	std::vector<int> randSeed;
};

struct SRandomizerData{
	std::vector<SRandTable>				randomizerData;		// 
	std::vector<SFlagTable>				flagType;			//
	std::vector<std::pair<int, int>>	betAvailable;		// [gamemode, betNum]
	std::vector<std::pair<int, int>>	rtModeAtBonusHit;	// [bonusID, transferRTMode]
};
