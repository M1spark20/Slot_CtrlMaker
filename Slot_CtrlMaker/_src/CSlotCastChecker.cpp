#include "_header/CSlotCastChecker.hpp"
#include "_header/CGameDataManage.h"
#include "_header/SSlotGameDataWrapper.hpp"
#include "_header/CCastReaderFromCSV.hpp"

bool CSlotCastChecker::Init(CGameDataManage& pData, int pFileID){
	CCastReaderFromCSV reader;
	if (!reader.FileInit(pData.GetDataHandle(pFileID))) return false;
	if (!reader.MakeData(m_castData)) return false;
	ResetCastData();
	return true;
}

bool CSlotCastChecker::SetReachData(const SSlotGameDataWrapper& pData, int pBetNum, int pGameMode) {
	for (auto it = m_castData.lineData.begin(); it != m_castData.lineData.end(); ++it){
		if (it->betCount != pBetNum) continue;
		for (unsigned int offsetC = 0; offsetC < it->checkPos.size(); ++offsetC){
			const auto offsetData = pData.reelManager.GetCharaIDList(it->checkPos[offsetC]);
			int verifyData = 0;
			int skipNum = 0;
			for (unsigned int i = 0; i < offsetData.size(); ++i){
				if (offsetData[i] == -1) { ++skipNum; continue; }
				const int shift = (10 * (offsetData.size() - i - 1)) + offsetData[i];
				verifyData |= (1 << shift);
			}

			if (skipNum != 1) continue;		// 残り1リールが回転中以外は音を鳴らさない
			for (auto castIt = m_castData.payData.cbegin(); castIt != m_castData.payData.cend(); ++castIt){
				if (castIt->gamemode != pGameMode && castIt->gamemode != -1) continue;
				if ((castIt->conbination & verifyData) != verifyData) continue;
				m_reachSoundID = m_reachSoundID < castIt->reachSoundID ?
					castIt->reachSoundID : m_reachSoundID;
			}
		}
		return true;
	}
	return false;

}

bool CSlotCastChecker::SetCastData(const SSlotGameDataWrapper& pData, int pBetNum, int pGameMode){
	for (auto it = m_castData.lineData.begin(); it != m_castData.lineData.end(); ++it){
		if (it->betCount != pBetNum) continue;
		m_checkedBet = pBetNum;
		m_castList.clear();
		m_castList.resize(it->checkPos.size(), nullptr);
		for (unsigned int offsetC = 0; offsetC < it->checkPos.size(); ++offsetC){
			const auto offsetData = pData.reelManager.GetCharaIDList(it->checkPos[offsetC]);
			int verifyData = 0;
			for (unsigned int i = 0; i < offsetData.size(); ++i){
				const int shift = (10 * (offsetData.size() - i - 1)) + offsetData[i];
				verifyData |= (1 << shift);
			}

			for (auto castIt = m_castData.payData.cbegin(); castIt != m_castData.payData.cend(); ++castIt){
				if (castIt->gamemode != pGameMode && castIt->gamemode != -1) continue;
				if ((castIt->conbination & verifyData) != verifyData) continue;
				m_castList[offsetC] = &(*castIt);
				break;
			}
		}
		return true;
	}
	return false;
}

void CSlotCastChecker::ResetCastData(){
	m_castList.clear();
	m_checkedBet = 0;
	m_reachSoundID = -1;
}

bool CSlotCastChecker::IsReplay(){
	for (auto it = m_castList.cbegin(); it != m_castList.cend(); ++it){
		if (*it == nullptr) continue;
		const int data = ((*it)->payout >> 8 * (m_checkedBet - 1)) & 0xFF;
		if (data == 0x80) return true;
	}
	return false;
}

int CSlotCastChecker::GetPayout(){
	if (IsReplay()) return 0;
	int ans = 0;
	for (auto it = m_castList.cbegin(); it != m_castList.cend(); ++it){
		if (*it == nullptr) continue;
		const int data = ((*it)->payout >> 8 * (m_checkedBet - 1)) & 0xFF;
		if (data & 0x80) return -(data&0x7F);
		ans += data & 0x7F;
	}
	for (auto it = m_castData.lineData.cbegin(); it != m_castData.lineData.cend(); ++it){
		if (it->betCount != m_checkedBet) continue;
		return ans >= it->maxPayNum ? it->maxPayNum : ans;
	}
	return 0;
}

int CSlotCastChecker::GetReachSoundID() const {
	return m_reachSoundID;
}

int CSlotCastChecker::GetPayoutEffect() const {
	int ans = 0;
	for (auto it = m_castList.cbegin(); it != m_castList.cend(); ++it){
		if (*it == nullptr) continue;
		const int data = (*it)->effectID;
		const int payoutStatus = ((*it)->payout >> 8 * (m_checkedBet - 1)) & 0xFF;
		const int gamemodeChange = (*it)->gamemodeChange[eGameModeDest];
		if (payoutStatus == 0 && gamemodeChange == -1) continue; 
		ans = ans < data ? data : ans;
	}
	return ans;
}

int CSlotCastChecker::GetPayoutLineID() const {
	int ans = 0, addition = 1;
	for (auto it = m_castList.cbegin(); it != m_castList.cend(); ++it){
		addition *= 2;
		if (*it == nullptr) continue; 
		const int data = ((*it)->payout >> 8 * (m_checkedBet - 1)) & 0xFF;
		const int gamemodeChange = (*it)->gamemodeChange[eGameModeDest];
		if (data == 0 && gamemodeChange == -1) continue; 
		ans += (addition / 2);
	}
	return ans;
}
