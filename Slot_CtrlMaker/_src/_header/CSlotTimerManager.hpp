#pragma once
#include <vector>
#include <algorithm>
#include <string>
#include <set>
class CEffectVariableManager;
class CRestoreManagerRead;
class CRestoreManagerWrite;

enum class ESystemTimerID{
	eTimerGameStart,
	eTimerBetWaitStart,
	eTimerBetInput,
	eTimerLeverAvailable,
	eTimerWaitStart,
	eTimerWaitEnd,
	// 各リールの押下,停止タイマは別定義 下記定義はいずれかのリールが動いたときに自動適用
	eTimerAnyReelStart,
	eTimerAnyReelStopAvailable,
	eTimerAnyReelPush,
	eTimerAnyReelStop,

	eTimerAllReelStart,
	eTimerAllReelStop,
	eTimerPayout,
	eTimerSystemTimerMax
};

enum class EReelTimerID {
	eTimerReelStart,
	eTimerReelStopAvailable,
	eTimerReelPush,
	eTimerReelStop,
	eTimerReelTimerMax
};

struct SSlotTimerActionData {
	std::string timerName;
	long long startVal;
};

typedef std::string SSlotTimerStopData;

class CSlotTimerManager{
	struct STimerData{
		bool enable;
		long long originVal;
		long long lastGetVal;
	};

	std::vector<STimerData>	m_timerData;	// enableFlag, originVal
	long long				m_lastCount;
	int						m_resetCount;
	int						m_reelNumMax;

	std::vector<std::pair<std::string, int>> mTimerNameList;
	std::set<std::string> mDuplicateFinder;
	std::vector<std::string> mResumeTimerID;

public:
	bool	Init(int pReelNum);
	bool	Process();
	bool	SetTimer(ESystemTimerID pID, int offset = 0);
	bool	SetTimer(EReelTimerID pID, int pReelID, int offset = 0);
	bool	SetTimer(std::string pID, int offset = 0);
	bool	DisableTimer(ESystemTimerID pID);
	bool	DisableTimer(EReelTimerID pID, int pReelID);
	bool	GetTime(long long& pInputFor, ESystemTimerID pID) const;
	bool	GetTime(long long& pInputFor, EReelTimerID pID, int pReelID) const;
	bool	GetTimeDiff(long long& pInputFor, ESystemTimerID pID, bool pRefreshFlag = true);
	bool	GetTimeDiff(long long& pInputFor, EReelTimerID pID, int pReelID, bool pRefreshFlag = true);
};
