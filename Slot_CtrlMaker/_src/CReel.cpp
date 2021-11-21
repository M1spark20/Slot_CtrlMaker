#include "_header/CReel.hpp"
#include "DxLib.h"
#include "_header/CGameDataManage.h"
#include "_header/SReelDrawData.hpp"
#include "_header/CSlotTimerManager.hpp"
#include <cmath>

bool CReel::Init(const SReelChaData& pReelData){
	m_reelData = pReelData;
	if (m_reelData.arrayData.empty()) return false;
	if (m_reelData.reelData.empty()) return false;
	m_rotatePos = 0.f;
	m_pushPos = 0.f;
	m_speed = 0.f;
	m_comaPos = 0;
	m_destination = 0;
	m_lastRotationTime = -1;
	m_lastStatus = EReelStatus::eInitial;
	m_nowStatus = EReelStatus::eStoping;

	m_speedMax = (float)m_reelData.reelData[0].h * GetComaNum() * m_reelData.rpm / 60000.f;
	m_accVal = m_speedMax / m_reelData.accTime;
	return true;
}

bool CReel::Process(CSlotTimerManager& pTimer){

	// タイマー初期化
	if (m_nowStatus == EReelStatus::eAccerating && m_lastStatus == EReelStatus::eStoping){
		pTimer.DisableTimer(EReelTimerID::eTimerReelPush, m_reelData.reelID);
		pTimer.DisableTimer(EReelTimerID::eTimerReelStop, m_reelData.reelID);
		pTimer.SetTimer(EReelTimerID::eTimerReelStart, m_reelData.reelID);
	}
	if (m_nowStatus == EReelStatus::eSliping && m_lastStatus == EReelStatus::eRotating){
		pTimer.DisableTimer(EReelTimerID::eTimerReelStopAvailable, m_reelData.reelID);
		pTimer.SetTimer(EReelTimerID::eTimerReelPush, m_reelData.reelID);
	}
	/* 初回起動時にstopタイマを作動させる */
	if (m_lastStatus == EReelStatus::eInitial) {
		long long temp;
		if (!pTimer.GetTime(temp, EReelTimerID::eTimerReelStop, m_reelData.reelID))
			pTimer.SetTimer(EReelTimerID::eTimerReelStop, m_reelData.reelID);
	}

	const float lastReelPos = m_rotatePos;
	if (m_nowStatus == EReelStatus::eAccerating){
		long long diff;
		if (!pTimer.GetTimeDiff(diff, EReelTimerID::eTimerReelStart, m_reelData.reelID)) return false;
		// 加速をより厳密に再現するために、1msずつ速度と位置を更新する
		// ただし最大速度に達した場合それ以上は掛け算で処理
		long long count;
		for (count = 0; count < diff; ++count){
			m_speed += m_accVal;
			if (m_speed > m_speedMax){
				long long temp;
				if(!pTimer.GetTime(temp, EReelTimerID::eTimerReelStopAvailable, m_reelData.reelID))
					pTimer.SetTimer(EReelTimerID::eTimerReelStopAvailable, m_reelData.reelID);
				m_speed = m_speedMax; m_nowStatus = EReelStatus::eRotating;
			}
			m_rotatePos -= m_speed;
		}
		m_rotatePos -= m_speed * (count-diff);
	} else if (m_nowStatus == EReelStatus::eRotating || m_nowStatus == EReelStatus::eSliping) {
		long long diff;
		if (!pTimer.GetTimeDiff(diff, EReelTimerID::eTimerReelStart, m_reelData.reelID)) return false;
		m_rotatePos -= m_speed * diff;
	}

	// リール位置判断 & 停止判断
	const int newComaPos = (unsigned int)std::floorf(m_rotatePos / m_reelData.reelData[0].h);
	if (m_nowStatus == EReelStatus::eSliping){
		// 値を補正した判定位置
		const float purposPos = (float)m_destination * m_reelData.reelData[0].h;
		float purposFixed = purposPos;
		while ( m_speed > 0 && purposFixed > lastReelPos) {
			purposFixed -= GetComaNum() * m_reelData.reelData[0].h;
		}
		while ( m_speed < 0 && purposFixed < lastReelPos) {
			purposFixed += GetComaNum() * m_reelData.reelData[0].h;
		}

		// 判定
		if ( m_speed > 0 && lastReelPos - m_rotatePos > lastReelPos - purposFixed ||
			 m_speed < 0 && lastReelPos - m_rotatePos < lastReelPos - purposFixed) {
			m_rotatePos = purposPos;
			m_speed = 0.f;	m_nowStatus = EReelStatus::eStoping;
			pTimer.DisableTimer(EReelTimerID::eTimerReelStart, m_reelData.reelID);
			pTimer.SetTimer(EReelTimerID::eTimerReelStop, m_reelData.reelID);
		}
	}
	// 順回転位置補正(補正時はリールを停止させる)
	while (m_rotatePos < 0.f){
		m_rotatePos += GetComaNum() * m_reelData.reelData[0].h;
	}
	// 逆回転位置補正(補正時はリールを停止させる)
	while (m_rotatePos > GetComaNum() * m_reelData.reelData[0].h){
		m_rotatePos -= GetComaNum() * m_reelData.reelData[0].h;
	}
	m_comaPos = (unsigned int)std::floorf(m_rotatePos / m_reelData.reelData[0].h);
	m_lastStatus = m_nowStatus;
	if (!pTimer.GetTime(m_lastRotationTime, EReelTimerID::eTimerReelStart, m_reelData.reelID)) m_lastRotationTime = -1;
	return true;
}

bool CReel::ReelStart(){
	if (m_nowStatus != EReelStatus::eStoping) return false;
	m_nowStatus = EReelStatus::eAccerating;
	return true;
}

bool CReel::ReelStop(unsigned int pDest, bool pForceFlag){
	if (pForceFlag){
		// 即時停止
		m_destination = pDest % m_reelData.arrayData.size();
		m_comaPos = m_destination;
		m_speed = 0.f;
		m_pushPos = m_rotatePos;
		m_rotatePos = (float)m_reelData.reelData[0].h * pDest;
		m_nowStatus = EReelStatus::eStoping;
	} else {
		// 滑って停止する
		if (m_nowStatus != EReelStatus::eRotating) return false;
		m_pushPos = m_rotatePos;
		m_destination = pDest % m_reelData.arrayData.size();
		m_nowStatus = EReelStatus::eSliping;
	}
	return true;
}

int CReel::GetReelComaByReelPos(size_t pOffset) const{
	if (m_nowStatus != EReelStatus::eStoping) return -1;
	const size_t dst = (m_reelData.arrayData.size() + m_comaPos + pOffset) % m_reelData.arrayData.size();
	return m_reelData.arrayData[dst];
}

int CReel::GetReelComaByFixedPos(int pComa) const{
	//if (m_nowStatus != EReelStatus::eStoping) return -1;
	if (pComa < 0 || pComa >= m_reelData.arrayData.size()) return -1;
	return m_reelData.arrayData[pComa];
}

bool CReel::DrawReelMain(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas, unsigned int pStartComa, bool pIsFixed) const{
	//DxLib::FillGraph(
	const size_t comaDivNum = m_reelData.arrayData.size();
	unsigned int destY = 0;
	DxLib::SetDrawScreen(pCanvas);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	for (unsigned int i = 0; i < pData.comaNum+4; ++i){
		const size_t pos = (comaDivNum + pStartComa + i-2) % comaDivNum;
		const SReelChaUnit& nowComa = m_reelData.reelData[m_reelData.arrayData[pos]];
		const int graphH = pDataManager.GetDataHandle(nowComa.imageID);
		DxLib::DrawRectGraph(0, destY, nowComa.x, nowComa.y, nowComa.w, nowComa.h, graphH, TRUE);
		destY += nowComa.h;
	}

	const SReelChaUnit& baseComa = m_reelData.reelData[0];
	DxLib::SetDrawScreen(DX_SCREEN_BACK);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	const float extRate[] = { pData.comaW / (float)baseComa.w, pData.comaH / (float)baseComa.h };
	const float drawOffset = (pIsFixed ? 0.f : m_rotatePos - (float)baseComa.h * pStartComa) +
		((float)baseComa.h * 2.f) - (pData.offsetUpper / extRate[1]);
	const float drawSrcH = pData.comaNum * baseComa.h + (pData.offsetLower + pData.offsetUpper) / extRate[1];
	const float drawDstH = pData.comaH * pData.comaNum + pData.offsetUpper + pData.offsetLower;
	DxLib::DrawRectExtendGraphF(pData.x, pData.y, pData.x + pData.comaW, pData.y + drawDstH,
		0, (int)drawOffset, baseComa.w, (int)drawSrcH, pCanvas, TRUE);
	return true;
}

bool CReel::DrawReel(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas) const{
	return DrawReelMain(pDataManager, pData, pCanvas, m_comaPos, false);
}

bool CReel::DrawReel(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas, unsigned int pComaStart) const{
	if (pComaStart >= m_reelData.arrayData.size()) return false;
	return DrawReelMain(pDataManager, pData, pCanvas, pComaStart, true);
}

// [act]各リールコマを16分割した位置を取得する
int CReel::GetReelDetailPos() const {
	if (m_nowStatus == EReelStatus::eAccerating || m_nowStatus == EReelStatus::eRotating) return -1;
	const int pos = (int)(m_pushPos * 16.f / m_reelData.reelData[0].h);
	return pos;
}

int CReel::GetSlipCount() const {
	const int pos = (int)(m_pushPos / m_reelData.reelData[0].h);
	int ans = pos - m_destination;
	if (ans < 0) ans += (int)GetComaNum();
	return ans;
}
