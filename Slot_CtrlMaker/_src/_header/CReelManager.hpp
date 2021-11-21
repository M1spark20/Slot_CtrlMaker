#pragma once
#include "SReelChaData.hpp"
#include "CReel.hpp"
class CGameDataManage;
class CGetSysDataFromCSV;
class CSlotTimerManager;
class IImageSourceManager;
class CImageColorManager;
class CRestoreManagerRead;
class CRestoreManagerWrite;

class CReelManager {
	std::vector<CReel>	m_reelChaData;	// 各リールデータ
	int					m_drawingCanvas;
	int					m_flagID;
	int					m_bonusID;

	std::vector<CReel>::const_iterator FindReelData(int pIndex) const;

public:
	bool Init(const CGameDataManage& pDataManager, CGetSysDataFromCSV& pSysData);
	bool Process(CSlotTimerManager& pTimer);
	bool DrawReel(const CGameDataManage& pDataManager, const SReelDrawData pDrawData);
	bool DrawReel(const CGameDataManage& pDataManager, const SReelDrawData pDrawData, unsigned int comaBegin);

	size_t	GetReelNum() const { return m_reelChaData.size(); }
	int		GetCharaNum(int pReelID = 0) const;
	int		GetRotatingReelNum();

	int					GetReelPos(int pCheckReelID) const;
	std::vector<int>	GetReelPos() const;
	int					GetCharaIDByReelPos(int pReelID, int pComaOffset) const;
	int					GetCharaIDByFixedPos(int pReelID, int pComaID) const;
	int					GetComaDetailPos(int pReelID) const;
	std::vector<int>	GetCharaIDList(std::vector<int> pOffsetList) const;

	~CReelManager();
};
