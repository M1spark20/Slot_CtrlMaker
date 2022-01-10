#include "_header/CReelChecker.hpp"
#include "_header/CGameDataManage.h"
#include "_header/CStopPosDataReaderFromCSV.hpp"
#include "_header/CReadReachCollectionFromCSV.hpp"
#include "_header/CReelManager.hpp"
#include "DxLib.h"

bool CReelChecker::Init(CGameDataManage& pData, int pFileIDSpot, int pFileIDCollection, const CReelManager& pReelManager) {
	m_comaMax = pReelManager.GetCharaNum();
	const int reelNum = pReelManager.GetReelNum();

	CStopPosDataReaderFromCSV   readerSpot;
	if (!readerSpot.FileInit(pData.GetDataHandle(pFileIDSpot))) return false;
	if (!readerSpot.MakeData(m_posData, m_comaMax)) return false;
	if (!m_collectionData.Init(pData, pFileIDCollection, reelNum)) return false;

	/* リーチ目コレクション不足判定 */ {
		m_collectionClear = true;
		const int loopMax = pow(m_comaMax, 3);
		std::vector<int> checkPos(reelNum);
		for (int checkC = 0; checkC < loopMax; ++checkC) {
			for (size_t i = 0; i < checkPos.size(); ++i) {
				checkPos[i] = checkC / (int)pow(m_comaMax, reelNum - 1 - i) % m_comaMax;
			}
			if (!m_collectionData.JudgeCollection(pReelManager, 0, checkPos)) return false;
			if (m_collectionData.GetLatchNum() > 0) {
				const auto spot = m_posData[checkC].spotFlag;
				const bool isCheck = spot == "";
				if (isCheck && m_posData[checkC].reachLevel == 0) { m_collectionClear = false; break; }
			}
			m_collectionData.Latch(false);
		}
		if (!m_collectionClear) {
			m_collectionErrNo = m_collectionData.GetLatchReachID(0);
			m_collectionErrPos.resize(checkPos.size());
			// Excelファイルは上段基準・逆順のため、以下で下段基準に変換する
			for (size_t i = 0; i < m_collectionErrPos.size(); ++i) {
				m_collectionErrPos[i] = (m_comaMax * 2 - 2 - checkPos[i]) % m_comaMax;
			}
		}
	}

	return true;
}

bool CReelChecker::Draw() {
	if (m_collectionClear) {
		DxLib::DrawString(1010, 0, "Collection Status: Clear", 0xFFFF00);
	} else {
		DxLib::DrawString(1010, 0, "Collection Status: Not Clear", 0xFF0000);
		DxLib::DrawFormatString(1010, 20, 0xFF0000, "No: %d / Pos: %d-%d-%d",
			m_collectionErrNo, m_collectionErrPos[0], m_collectionErrPos[1], m_collectionErrPos[2]);
	}
	return true;
}

SStopPosData CReelChecker::GetPosData(int pFirstStop, std::vector<int> pStopPos) {
	if (pStopPos.size() < 3) return SStopPosData();
	int index = pFirstStop * m_comaMax * m_comaMax * m_comaMax;
	index += pStopPos[0] * m_comaMax * m_comaMax;
	index += pStopPos[1] * m_comaMax;
	index += pStopPos[2];
	if (index >= m_posData.size()) return SStopPosData();
	return m_posData[index];
}