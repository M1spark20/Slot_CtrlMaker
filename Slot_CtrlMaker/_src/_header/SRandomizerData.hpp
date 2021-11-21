#pragma once
#include <vector>
#include <array>
#include <string>

enum EFlagType{
	eFlagID,
	eBonusID,
	eIsReachCheck,
	eFlagTypeMax
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
	std::vector<SRandTable>						randomizerData;		// 
	std::vector<std::array<int, eFlagTypeMax>>	flagType;			// [flagID][EFlagType]
	std::vector<std::pair<int, int>>			betAvailable;		// [gamemode, betNum]
	std::vector<std::pair<int, int>>			rtModeAtBonusHit;	// [bonusID, transferRTMode]
	std::vector<std::string>					flagName;
};
