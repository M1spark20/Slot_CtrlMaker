#pragma once
#include "SRandomizerData.hpp"
#include <random>
class CGameDataManage;
class CSlotInternalDataManager;

class CRandManager {
	SRandomizerData	m_flagData;
	std::mt19937	m_randomizer;

public:
	bool		Init(const CGameDataManage& pDataManager, int pFileID, long pRandSeed=-1);
	size_t		GetFlagNum() const { return m_flagData.flagType.size(); }
	int			GetFlagData(int pFlagID, EFlagType pType) const;
	std::string	GetFlagName(int pFlagID) const;
	std::string	GetBonusName(int pFlagID) const;
	void		SetRandSeed(long pRandSeed);
	//bool		Role(CSlotInternalDataManager& pData);
	bool		GetBetAvailable(int pGameMode, int pBetNum) const;
	int			GetMaxBetNum(int pGameMode) const;
};
