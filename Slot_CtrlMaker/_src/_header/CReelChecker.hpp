#pragma once
#include "SStopPosData.hpp"
#include "CSlotReachCollection.hpp"
#include <vector>
class CGameDataManage;

class CReelChecker {
	std::vector<SStopPosData> m_posData;
	CSlotReachCollectionData  m_collectionData;
	int m_comaMax;

	bool m_collectionClear;
	int m_collectionErrNo;
	std::vector<int> m_collectionErrPos;

public:
	bool	Init(CGameDataManage& pData, int pFileIDSpot, int pFileIDCollection, const CReelManager& pReelManager);
	bool	Draw();
};