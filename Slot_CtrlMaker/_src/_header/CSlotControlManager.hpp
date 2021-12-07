#pragma once
#include "SControlMakeData.hpp"
#include "SControlPositionData.hpp"
struct SSlotGameDataWrapper;
class CGameDataManage;

class CSlotControlManager {
	static const int SLIP_TABLE_MAX  = 256;
	static const int AVAIL_TABLE_MAX = 256;
	static const int AVAIL_ID_MAX = 4;

	static const int BOX_W = 26;
	static const int BOX_H = 26;
	static const int BOX_A = 6;

	int m_flagMax;
	int m_reelMax;
	int m_comaMax;
	unsigned long long m_allStopFlag;

	std::vector<SControlMakeData>	ctrlData;
	std::vector<SControlTable>		tableSlip;
	std::vector<SControlTable>		tableAvailable;
	SControlPositionData			posData;

	// à íuï“èWånä÷êî
	bool Action(int pNewInput);
	bool ActionTableID(bool pIsUp);
	void AdjustPos();
	SControlDataSA* GetSA();
	SControlAvailableDef* GetDef();
	unsigned char* GetSS(bool pGet1st = false);
	unsigned char Get2ndStyle();
	void SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp);
	void SwitchATableType();
	void SetAvailCtrlPattern(unsigned char pNewFlag);
	void SetAvailShiftConf(unsigned char pNewFlag);
	bool isSilp();
	bool UpdateActiveFlag();
	int Get2ndReel(bool pIsLeft);
	unsigned long long GetAvailShiftData(unsigned long long pData, const unsigned char pShiftFlag);

	int  GetPosFromSlipT(const size_t pTableNo, const int pPushPos);
	int  GetPosFromAvailT(const SControlDataSA& pSAData, const int pPushPos, bool pIsLeft);
	int  GetAvailDistance(const unsigned long long pData, const int pPushPos);
	int  GetActiveFromAvailT(const SControlDataSA& pSAData, bool pIsLeft);

	unsigned char			SetSlipT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal);
	SControlAvailableDef	SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const bool pIsPrior);

public:
	bool Init(const SSlotGameDataWrapper& pData);
	bool Process();
	bool Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor);
};