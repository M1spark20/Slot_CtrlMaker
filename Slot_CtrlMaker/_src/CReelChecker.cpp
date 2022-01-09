#include "_header/CReelChecker.hpp"
#include "_header/CGameDataManage.h"
#include "_header/CStopPosDataReaderFromCSV.hpp"
#include "_header/CReadReachCollectionFromCSV.hpp"
#include "_header/CReelManager.hpp"

bool CReelChecker::Init(CGameDataManage& pData, int pFileIDSpot, int pFileIDCollection, const CReelManager& pReelManager) {
	m_comaMax = pReelManager.GetCharaNum();
	const int reelNum = pReelManager.GetReelNum();

	CStopPosDataReaderFromCSV   readerSpot;
	if (!readerSpot.FileInit(pData.GetDataHandle(pFileIDSpot))) return false;
	if (!readerSpot.MakeData(m_posData, m_comaMax)) return false;
	if (!m_collectionData.Init(pData, pFileIDCollection, reelNum)) return false;

	/* ���[�`�ڃR���N�V�����s������ */ {
		m_collectionClear = true;
		const int loopMax = pow(m_comaMax, 3);
		std::vector<int> checkPos(reelNum);
		for (int checkC = 0; checkC < loopMax; ++checkC) {
			for (size_t i = 0; i < checkPos.size(); ++i) {
				checkPos[i] = checkC / (int)pow(m_comaMax, reelNum - 1 - i) % m_comaMax;
			}
			if (!m_collectionData.JudgeCollection(pReelManager, 0, checkPos)) return false;
			if (m_collectionData.GetLatchNum() > 0) {
				if (m_posData[checkC].spotFlag == "" && m_posData[checkC].reachLevel == 0) { m_collectionClear = false; break; }
			}
			m_collectionData.Latch(false);
		}
		if (!m_collectionClear) {
			m_collectionErrNo = m_collectionData.GetLatchReachID(0);
			m_collectionErrPos.resize(checkPos.size());
			// Excel�t�@�C���͏�i��E�t���̂��߁A�ȉ��ŉ��i��ɕϊ�����
			for (size_t i = 0; i < m_collectionErrPos.size(); ++i) {
				m_collectionErrPos[i] = (m_comaMax * 2 - 2 - checkPos[i]) % m_comaMax;
			}
		}
	}

	return true;
}