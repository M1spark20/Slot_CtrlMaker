#pragma once
#include "SControlMakeData.hpp"
#include "SControlPositionData.hpp"
struct SSlotGameDataWrapper;
class CGameDataManage;

class CSlotControlManager {
	static const int SLIP_TABLE_MAX  = 256;
	static const int AVAIL_TABLE_MAX = 256;
	static const int AVAIL_ID_MAX = 4;

	static const int BOX_W = 20;
	static const int BOX_H = 26;
	static const int BOX_A = 6;

	int m_flagMax;
	int m_reelMax;
	int m_comaMax;
	unsigned long long m_allStopFlag;
	bool m_isSuspend;					// SAで無効データがある場合trueにして編集対象をロックする

	std::vector<SControlMakeData>	ctrlData;
	std::vector<SControlTable>		tableSlip;
	std::vector<SControlTable>		tableAvailable;
	SControlPositionData			posData;

	// 位置編集系関数
	bool Action(int pNewInput);
	bool ActionTableID(bool pIsUp);
	void AdjustPos();
	SControlDataSA* GetSA(int pFlagID = -1);
	void CheckSA();
	SControlAvailableDef* GetDef();
	unsigned char* GetSS(int pFlagID = -1, bool pGet1st = false);
	unsigned char Get2ndStyle();
	bool SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp);
	void SwitchATableType();
	void SetAvailCtrlPattern(unsigned char pNewFlag);
	void SetAvailShiftConf(unsigned char pNewFlag);
	bool isSilp();
	bool canChangeTable();
	bool UpdateActiveFlag();
	int Get2ndReel(bool pIsLeft);
	unsigned long long GetAvailShiftData(unsigned long long pData, const unsigned char pShiftFlag);
	unsigned long long GetAvailShiftData(const SControlDataSA& pSAData, const int pIndex, bool pIsLeft);
	bool GetCanStop(const int pMoveOrder, const int pLookFor, const int pFlagID = -1);

	int  GetPosFromSlipT(const size_t pTableNo, const int pPushPos);
	int  GetPosFromAvailT(const SControlDataSA& pSAData, const int pPushPos, bool pIsLeft);
	int  GetAvailDistance(const unsigned long long pData, const int pPushPos);
	int  GetActiveFromAvailT(const SControlDataSA& pSAData, bool pIsLeft);

	unsigned char			SetSlipT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal);
	SControlAvailableDef	SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const unsigned char pTableFlag);

	bool DrawComaBox(int x, int y, const unsigned int pStopPos);
	bool DrawSlipTable(int x, int y, int pFlagID);
	bool DrawStopTable(int x, int y, int pFlagID);

public:
	bool Init(const SSlotGameDataWrapper& pData);
	bool Process();
	bool Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor);
};