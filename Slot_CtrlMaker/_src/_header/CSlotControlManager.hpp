#pragma once
#include "SControlMakeData.hpp"
#include "SControlPositionData.hpp"
struct SSlotGameDataWrapper;
class CGameDataManage;
class CRestoreManagerRead;
class CRestoreManagerWrite;

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
	bool m_refreshFlag;

	std::vector<SControlMakeData>	ctrlData;
	std::vector<SControlTable>		tableSlip;
	std::vector<SControlTable>		tableAvailable;
	SControlPositionData			posData;

	// 位置編集系関数
	bool Action(int pNewInput);
	bool ActionTableID(bool pIsUp);
	void AdjustPos();

	void CheckSA();
	SControlAvailableDef* GetDef();

	unsigned char Get2ndStyle(const int pPushPos, int pStop1stOrder, const int pFlagID);
	unsigned char Get2ndStyle();
	unsigned char Get2ndStyle(const int pPushPos, const int pFlagID);

	unsigned char* GetSS(int pFlagID, bool pGet1st, int pStop1stOrder, int pPushPos1st, bool pIsWatchLeft);
	unsigned char* GetSS();
	unsigned char* GetSS(int pFlagID);
	unsigned char* GetSS(bool pGet1st);

	SControlDataSA* GetSA(int pFlagID, int pNowCheckOrder, int pStop1stOrder, int pPushPos1st, int pPushPos2nd);
	SControlDataSA* GetSA();
	SControlDataSA* GetSA(int pFlagID);

	bool isSlip(const int pFlagID, int pCurrentOrder, int pStop1stOrder, int pPushPos1st);
	bool isSlip();
	bool isSlip(const int pFlagID);

	int Get2ndReel(bool pIsLeft, int pStop1stOrder);
	int Get2ndReel(bool pIsLeft);

	bool SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp);
	void SwitchATableType();
	void SetAvailCtrlPattern(unsigned char pNewFlag);
	void SetAvailShiftConf(unsigned char pNewFlag);
	bool canChangeTable();
	bool UpdateActiveFlag();
	unsigned long long GetAvailDefData(const int pAvailIndex, const unsigned char pTableFlag);
	unsigned long long GetAvailShiftData(unsigned long long pData, const unsigned char pShiftFlag);
	unsigned long long GetAvailShiftData(const SControlDataSA& pSAData, const int pIndex, bool pIsLeft);
	bool GetCanStop(const int pMoveOrder, const int pLookFor, const int pFlagID, const bool pCheck1st);

	int  GetPosFromSlipT(const size_t pTableNo, const int pPushPos);
	int  GetPosFromAvailT(const SControlDataSA& pSAData, const int pPushPos, bool pIsLeft);
	int  GetAvailDistance(const unsigned long long pData, const int pPushPos);
	int  GetActiveFromAvailT(const SControlDataSA& pSAData, bool pIsLeft);

	unsigned char			SetSlipT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal);
	SControlAvailableDef	SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const unsigned char pTableFlag);

	bool DrawComaBox(int x, int y, const unsigned int pStopPos, int pLightPos, int boxColor = 0x404040);
	bool DrawSlipTable(int x, int y, int pFlagID, SSlotGameDataWrapper& pData);
	bool DrawStopTable(int x, int y, int pFlagID);

	void DrawStopStatus(SSlotGameDataWrapper& pData);	// 20220411add 現在フラグの停止位置情報を描画する

public:
	bool Init(const SSlotGameDataWrapper& pData);
	bool Process();
	bool Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor);

	bool RefreshFlag() const;
	bool ReadRestore(CRestoreManagerRead& pReader);
	bool WriteRestore(CRestoreManagerWrite& pWriter) const;
};