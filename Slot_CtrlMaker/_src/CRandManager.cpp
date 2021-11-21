#include "_header/CRandManager.hpp"
#include "_header/CRandomizerReaderFromCSV.hpp"
#include "_header/CGameDataManage.h"
//#include "_header/CSlotInternalDataManager.hpp"

bool CRandManager::Init(const CGameDataManage& pDataManager, int pFileID, long pRandSeed){
	CRandomizerReaderFromCSV reader;
	if (!reader.FileInit(pDataManager.GetDataHandle(pFileID))) return false;
	if (!reader.MakeData(m_flagData)) return false;
	SetRandSeed(pRandSeed);
	return true;
}

int CRandManager::GetFlagData(int pFlagID, EFlagType pType) const{
	if (pFlagID < 0 || pFlagID >= GetFlagNum()) return -1;
	switch (pType)
	{
	case EFlagType::eFlagID:
		return m_flagData.flagType[pFlagID].flagID;
		break;
	case EFlagType::eBonusID:
		return m_flagData.flagType[pFlagID].bonusID;
		break;
	case EFlagType::eIsReachCheck:
		return m_flagData.flagType[pFlagID].reachCheckFlag;
		break;
	default:
		break;
	}
	return -1;
}

std::string	CRandManager::GetFlagName(int pFlagID) const{
	if (pFlagID < 0 || pFlagID >= GetFlagNum()) return "";
	return m_flagData.flagType[pFlagID].flagName;
}

std::string	CRandManager::GetBonusName(int pFlagID) const{
	if (pFlagID < 0 || pFlagID >= GetFlagNum()) return "";
	return m_flagData.flagType[pFlagID].bonusName;
}

void CRandManager::SetRandSeed(long pRandSeed){
	if (pRandSeed == -1){
		std::random_device rd;
		m_randomizer.seed(rd());
	} else {
		m_randomizer.seed(pRandSeed);
	}
}

// [act]フラグを抽選により決定する。結果はpDataの値変更による。
//		なお強制フラグは本クラスを通さずに直接指定が可能。
// [ret]抽選に成功したか
// [prm]pData	: 内部データ管理クラス(データ取得&結果格納)
/*bool CRandManager::Role(CSlotInternalDataManager& pData) {
	const auto data = pData.GetData();
	for (auto it = m_flagData.randomizerData.cbegin(); it != m_flagData.randomizerData.cend(); ++it){
		if (it->set != data.set && it->set != -1) continue;
		if (it->bet != data.betNum && it->bet != -1) continue;
		if (it->gameMode != data.gameMode && it->gameMode != -1) continue;
		if (it->rtMode != data.rtMode.first && it->rtMode != -1) continue;

		// 乱数生成
		int randVal = 0;
		if (it->randMax >= 0){
			std::uniform_int_distribution<int> generator(0, it->randMax - 1);
			randVal = generator(m_randomizer);
		}

		const int temp = randVal;
		// フラグ決定
		for (unsigned int i = 0; i < it->randSeed.size(); ++i){
			randVal -= it->randSeed[i];
			if (randVal >= 0) continue;
			// フラグとRTモード(ボーナス成立Gのみ/定義が存在する場合のみ)をpDataにセット
			pData.SetFlag(m_flagData.flagType[i][eFlagID], m_flagData.flagType[i][eBonusID]);
			if (data.flag.second == 0 && m_flagData.flagType[i][eBonusID] > 0){
				for (auto rtIt = m_flagData.rtModeAtBonusHit.cbegin(); rtIt != m_flagData.rtModeAtBonusHit.cend(); ++rtIt){
					if (rtIt->first != m_flagData.flagType[i][eBonusID]) continue;
					pData.SetRTMode(rtIt->second, false, true);
					break;
				}
			}
			return true;
		}
		return false;
	}
	return false;
}*/

bool CRandManager::GetBetAvailable(int pGameMode, int pBetNum) const {
	if (pGameMode < 0 || pBetNum <= 0) return false;
	for (auto it = m_flagData.betAvailable.cbegin(); it != m_flagData.betAvailable.cend(); ++it){
		if (it->first == pGameMode) return (it->second & 1 << (pBetNum - 1)) != 0;
	}
	return false;
}

int CRandManager::GetMaxBetNum(int pGameMode) const {
	if (pGameMode < 0) return -1;
	for (auto it = m_flagData.betAvailable.cbegin(); it != m_flagData.betAvailable.cend(); ++it){
		if (it->first == pGameMode){
			int buf = it->second & INT_MAX;	// 算術シフトで1をシフトさせない対策
			for (int ans = 0; ; ++ans){
				if (buf == 0) return ans;
				buf >>= 1;
			}
		}
	}
	return -1;
}
