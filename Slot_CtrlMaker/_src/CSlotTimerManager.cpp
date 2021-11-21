#include "_header/CSlotTimerManager.hpp"
#include "_header/CRestoreManager.hpp"
#include "DxLib.h"
#include "_header/ErrClass.hpp"
#include <stdexcept>

bool CSlotTimerManager::Init(int pReelNum){
	m_reelNumMax = pReelNum;
	/* initialize timerData */{
		STimerData data;
		data.enable = false; data.originVal = 0; data.lastGetVal = 0;
		m_timerData.resize((int)ESystemTimerID::eTimerSystemTimerMax + m_reelNumMax* (int)EReelTimerID::eTimerReelTimerMax, data);
	}

	/* initialize keyData */ {
		mTimerNameList.push_back(std::pair<std::string, int>("general", (int)ESystemTimerID::eTimerGameStart));
		mTimerNameList.push_back(std::pair<std::string, int>("betWait", (int)ESystemTimerID::eTimerBetWaitStart));
		mTimerNameList.push_back(std::pair<std::string, int>("betInput", (int)ESystemTimerID::eTimerBetInput));
		mTimerNameList.push_back(std::pair<std::string, int>("leverAvailable", (int)ESystemTimerID::eTimerLeverAvailable));
		mTimerNameList.push_back(std::pair<std::string, int>("waitStart", (int)ESystemTimerID::eTimerWaitStart));
		mTimerNameList.push_back(std::pair<std::string, int>("waitEnd", (int)ESystemTimerID::eTimerWaitEnd));
		mTimerNameList.push_back(std::pair<std::string, int>("anyReelStart", (int)ESystemTimerID::eTimerAnyReelStart));
		mTimerNameList.push_back(std::pair<std::string, int>("anyReelStop", (int)ESystemTimerID::eTimerAnyReelStopAvailable));
		mTimerNameList.push_back(std::pair<std::string, int>("anyReelPush", (int)ESystemTimerID::eTimerAnyReelPush));
		mTimerNameList.push_back(std::pair<std::string, int>("anyReelStop", (int)ESystemTimerID::eTimerAnyReelStop));
		mTimerNameList.push_back(std::pair<std::string, int>("allReelStart", (int)ESystemTimerID::eTimerAllReelStart));
		mTimerNameList.push_back(std::pair<std::string, int>("allReelStop", (int)ESystemTimerID::eTimerAllReelStop));
		mTimerNameList.push_back(std::pair<std::string, int>("payout", (int)ESystemTimerID::eTimerPayout));

		for (auto i = 0; i < m_reelNumMax; ++i) {
			const int index = (int)ESystemTimerID::eTimerSystemTimerMax + (int)EReelTimerID::eTimerReelTimerMax * i;
			mTimerNameList.push_back(std::pair<std::string, int>("reelStart[" + std::to_string(i) + "]"			, index + 0));
			mTimerNameList.push_back(std::pair<std::string, int>("reelStopAvailable[" + std::to_string(i) + "]"	, index + 1));
			mTimerNameList.push_back(std::pair<std::string, int>("reelPush[" + std::to_string(i) + "]"			, index + 2));
			mTimerNameList.push_back(std::pair<std::string, int>("reelStop[" + std::to_string(i) + "]"			, index + 3));
		}
	}

	m_lastCount = DxLib::GetNowCount();
	m_resetCount = 0;

	SetTimer(ESystemTimerID::eTimerGameStart);
	return true;
}

bool CSlotTimerManager::Process(){
	int nowCount = DxLib::GetNowCount();
	if (m_lastCount % ((long long)INT_MAX+1) > nowCount) ++m_resetCount;
	m_lastCount = nowCount + m_resetCount * ((long long)INT_MAX+1);

	if (!mResumeTimerID.empty()) {
		for (const auto& data : mResumeTimerID) if (!SetTimer(data)) return false;
		mResumeTimerID.clear();
	}

	return true;
}

bool CSlotTimerManager::SetTimer(ESystemTimerID pID, int offset){
	int ID = (int)pID;
	if (ID >= (int)ESystemTimerID::eTimerSystemTimerMax) return false;
	m_timerData[ID].enable		= true;
	m_timerData[ID].originVal	= m_lastCount - offset;
	m_timerData[ID].lastGetVal = m_lastCount - offset;
	return true;
}

bool CSlotTimerManager::SetTimer(EReelTimerID pID, int pReelID, int offset){
	int ID = (int)pID;
	if (ID == (int)EReelTimerID::eTimerReelTimerMax) return false;
	if (pReelID < 0 || pReelID >= m_reelNumMax) return false;
	/* 各リールの設定 */{
		const int index = (int)ESystemTimerID::eTimerSystemTimerMax + (int)EReelTimerID::eTimerReelTimerMax * pReelID + ID;
		m_timerData[index].enable = true;
		m_timerData[index].originVal = m_lastCount - offset;
		m_timerData[index].lastGetVal = m_lastCount - offset;
	}
	/* 共通リールの設定 */ {
		const int index = (int)ESystemTimerID::eTimerAnyReelStart + ID;
		m_timerData[index].enable = true;
		m_timerData[index].originVal = m_lastCount - offset;
		m_timerData[index].lastGetVal = m_lastCount - offset;
	}
	return true;
}

bool CSlotTimerManager::SetTimer(std::string pID, int offset) {
	for (const auto& data : mTimerNameList) {
		if (pID != data.first) continue;
		m_timerData[data.second].enable = true;
		m_timerData[data.second].originVal = m_lastCount - offset;
		m_timerData[data.second].lastGetVal = m_lastCount - offset;
		return true;
	}
	return false;
}

bool CSlotTimerManager::DisableTimer(ESystemTimerID pID){
	int ID = (int)pID;
	if (ID == (int)ESystemTimerID::eTimerSystemTimerMax) return false;
	m_timerData[ID].enable		= false;
	m_timerData[ID].originVal	= 0;
	m_timerData[ID].lastGetVal	= 0;
	return true;
}

bool CSlotTimerManager::DisableTimer(EReelTimerID pID, int pReelID){
	int ID = (int)pID;
	if (ID == (int)EReelTimerID::eTimerReelTimerMax) return false;
	if (pReelID < 0 || pReelID >= m_reelNumMax) return false;
	const int index = (int)ESystemTimerID::eTimerSystemTimerMax + (int)EReelTimerID::eTimerReelTimerMax * pReelID + ID;
	m_timerData[index].enable		= false;
	m_timerData[index].originVal	= 0;
	m_timerData[index].lastGetVal	= 0;
	return true;
}

bool CSlotTimerManager::GetTime(long long& pInputFor, ESystemTimerID pID) const {
	int ID = (int)pID;
	if (ID == (int)ESystemTimerID::eTimerSystemTimerMax) return false;
	if (!m_timerData[ID].enable) return false;
	pInputFor = m_lastCount - m_timerData[ID].originVal;
	return true;
}

bool CSlotTimerManager::GetTime(long long& pInputFor, EReelTimerID pID, int pReelID) const{
	int ID = (int)pID;
	if (ID == (int)EReelTimerID::eTimerReelTimerMax) return false;
	if (pReelID < 0 || pReelID >= m_reelNumMax) return false;
	const int index = (int)ESystemTimerID::eTimerSystemTimerMax + (int)EReelTimerID::eTimerReelTimerMax * pReelID + ID;
	if (!m_timerData[index].originVal) return false;
	pInputFor = m_lastCount - m_timerData[index].originVal;
	return true;
}

bool CSlotTimerManager::GetTimeDiff(long long& pInputFor, ESystemTimerID pID, bool pRefreshFlag){
	int ID = (int)pID;
	if (ID == (int)ESystemTimerID::eTimerSystemTimerMax) return false;
	if (!m_timerData[ID].enable) return false;
	pInputFor = m_lastCount - m_timerData[ID].lastGetVal;
	if (pRefreshFlag) m_timerData[ID].lastGetVal = m_lastCount;
	return true;
}

bool CSlotTimerManager::GetTimeDiff(long long& pInputFor, EReelTimerID pID, int pReelID, bool pRefreshFlag){
	int ID = (int)pID;
	if (ID == (int)EReelTimerID::eTimerReelTimerMax) return false;
	if (pReelID < 0 || pReelID >= m_reelNumMax) return false;
	const int index = (int)ESystemTimerID::eTimerSystemTimerMax + (int)EReelTimerID::eTimerReelTimerMax * pReelID + ID;
	if (!m_timerData[index].originVal) return false;
	pInputFor = m_lastCount - m_timerData[index].lastGetVal;
	if (pRefreshFlag) m_timerData[index].lastGetVal = m_lastCount;
	return true;
}
