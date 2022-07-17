#include "_header/CSlotControlManager.hpp"
#include "_header/SSlotGameDataWrapper.hpp"
#include "_header/CRestoreManager.hpp"
#include "_header/keyexport.h"
#include "_header/SStopPosData.hpp"
#include <DxLib.h>

bool CSlotControlManager::Init(const SSlotGameDataWrapper& pData) {
	// �f�[�^�^������(���[�h�����͌��)
	m_flagMax = pData.randManager.GetFlagNum();
	m_reelMax = pData.reelManager.GetReelNum();
	m_comaMax = pData.reelManager.GetCharaNum();
	m_allStopFlag = (unsigned long long)pow(2, pData.reelManager.GetCharaNum()) - 1ull;
	m_isSuspend = false;
	m_refreshFlag = false;
	m_checkLaunchFlag = false;

	/* slipT������ */ {
		const unsigned long long allStopFlag = m_allStopFlag;
		SControlTable st; st.data = 0; st.refNum = 0; st.activePos = allStopFlag & 0xFFFFFFFF;
		tableSlip.resize(SLIP_TABLE_MAX, st);
		tableAvailable.resize(AVAIL_TABLE_MAX, st);
		tableSlip[0].refNum = INT_MAX;
		tableAvailable[0].data = allStopFlag;
		tableAvailable[0].refNum = INT_MAX;
	}
	/* makeData������ */ {
		const int flagMax = m_flagMax;
		const int reelMax = m_reelMax;
		const size_t comaMax = m_comaMax;
		SControlMakeData sm;
		sm.controlStyle = 0x0;
		/* wrapper ������ */ {
			SControlMakeDataWrapper wrapper;
			wrapper.controlData1st = 0;
			const unsigned int allStopFlag = m_allStopFlag & 0xFFFFFFFF;
			SControlAvailableDef sd; sd.tableFlag = 0; sd.availableID = 0;
			SControlDataSA sa; sa.data.resize(AVAIL_ID_MAX * 2, sd);

			wrapper.controlData2nd.activeFlag = allStopFlag;
			wrapper.controlData2nd.controlStyle2nd.resize(comaMax, 1);
			wrapper.controlData2nd.controlData2ndPS.resize(comaMax * 2, 0);
			wrapper.controlData2nd.controlData2ndSS.resize(comaMax * 2, 0);
			wrapper.controlData2nd.controlData2ndSA.resize(comaMax, sa);
			wrapper.controlData2nd.controlDataComSA.resize(comaMax, sa);

			const long long allStop2way = ((long long)allStopFlag << comaMax) | (long long)allStopFlag;
			wrapper.controlData3rd.activeFlag1st = allStopFlag;
			wrapper.controlData3rd.activeFlag2nd.resize(comaMax, allStop2way);
			wrapper.controlData3rd.availableData.resize(comaMax * comaMax, sa);
			sm.controlData.resize(reelMax, wrapper);
		}
		ctrlData.resize(flagMax, sm);
		for (size_t i = 0; i < ctrlData.size(); ++i) ctrlData[i].dataID = i;	// �z��v�f�ԍ��ƈ�v������
	}

	// �e�t���O�ɑ΂���Active�t���O������
	for(posData.currentFlagID = 0; posData.currentFlagID < m_flagMax; ++posData.currentFlagID)
		UpdateActiveFlag();

	/* posData������ */ {
		posData.currentFlagID = 0;
		posData.selectReel = 0;
		posData.stop1st = 0;
		posData.cursorComa.resize(m_reelMax, 0);
		posData.selectAvailID = 0;
		posData.isWatchLeft = true;
		// 20220408�ǉ�
		posData.subFlagList.clear();
		posData.subFlagPos = 0;
	}

	return true;
}

bool CSlotControlManager::Process() {
	m_refreshFlag = false;
	/* ��ʑ��� */ {
		CKeyExport_S& key = CKeyExport_S::GetInstance();
		const bool shiftFlag = key.ExportKeyStateFrame(KEY_INPUT_LSHIFT) >= 1 || key.ExportKeyStateFrame(KEY_INPUT_RSHIFT) >= 1;
		// �ʒu����n
		if		(!shiftFlag && key.ExportKeyState(KEY_INPUT_UP))	SetComaPos(posData.selectReel, false, true);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_DOWN))	SetComaPos(posData.selectReel, false, false);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_LEFT))	--posData.selectAvailID;
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_RIGHT))	++posData.selectAvailID;
		// �ȉ��̈ʒu�����SA�Ŗ����f�[�^������ꍇ�K�p�s��
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_LEFT)) {
			if (posData.currentOrder == 0)	--posData.stop1st;
			else							--posData.selectReel;
		}
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_RIGHT)) {
			if (posData.currentOrder == 0)	++posData.stop1st;
			else							++posData.selectReel;
		}
		else if (!m_isSuspend &&  shiftFlag && key.ExportKeyState(KEY_INPUT_UP))	--posData.currentFlagID;
		else if (!m_isSuspend &&  shiftFlag && key.ExportKeyState(KEY_INPUT_DOWN))	++posData.currentFlagID;
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_PERIOD))++posData.currentOrder;
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_COMMA))	--posData.currentOrder;
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_AT))	posData.isWatchLeft = !posData.isWatchLeft;
		// 20220408add
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_Q)) ++posData.subFlagPos;
		else if (!m_isSuspend && !shiftFlag && key.ExportKeyState(KEY_INPUT_A)) --posData.subFlagPos;


		// ����ҏW�n
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_0)) Action(0);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_1)) Action(1);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_2)) Action(2);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_3)) Action(3);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_4)) Action(4);

		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_0)) SwitchATableType();
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_5)) SetAvailShiftConf(0);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_6)) SetAvailShiftConf(1);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_7)) SetAvailShiftConf(2);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_8)) SetAvailShiftConf(3);


		// �ȉ��̐���ҏW��SA�Ŗ����f�[�^������ꍇ�K�p�s��
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_1)) SetAvailCtrlPattern(0);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_2)) SetAvailCtrlPattern(1);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_3)) SetAvailCtrlPattern(2);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_4)) SetAvailCtrlPattern(3);

		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_Q)) ActionTableID(true);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_A)) ActionTableID(false);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_W)) {
			for (int i = 0; i < 10; ++i) ActionTableID(true);
		}
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_S)) {
			for (int i = 0; i < 10; ++i) ActionTableID(false);
		}
		AdjustPos();

		// �������݃`�F�b�N�\���ݒ�FX�������ςȂ��ŕ\��
		m_checkLaunchFlag = (key.ExportKeyStateFrame(KEY_INPUT_X) >= 1);
	}
	return true;
}

void CSlotControlManager::AdjustPos() {
	// �e�����l��`
	const int comaMax = m_comaMax;
	const int flagMax = m_flagMax;
	const int reelMax = m_reelMax;

	while (posData.currentFlagID < 0) posData.currentFlagID += flagMax;
	while (posData.currentFlagID >= flagMax) posData.currentFlagID -= flagMax;

	for (auto it = posData.cursorComa.begin(); it != posData.cursorComa.end(); ++it) {
		while (*it < 0) *it += comaMax;
		while (*it >= comaMax) *it -= comaMax;
	}

	while (posData.stop1st < 0) posData.stop1st += reelMax;
	while (posData.stop1st >= reelMax) posData.stop1st -= reelMax;

	int orderLim = reelMax;
	// ���ʐ��䎞2nd/3rd��currentOrder = 1�œ��ł�
	if (Get2ndStyle() == 0x03) orderLim -= 1;
	while (posData.currentOrder < 0) posData.currentOrder = 0;
	while (posData.currentOrder >= orderLim) posData.currentOrder = orderLim - 1;

	int selectLim = posData.currentOrder + 1;
	// ���ʐ��䎞2nd/3rd�͑S���[������\�Ƃ��AisWatchLeft���ς�����
	if (posData.currentOrder >= 1 && Get2ndStyle() == 0x03) {
		selectLim = m_reelMax;	posData.isWatchLeft = !(posData.selectReel == 2);
	}
	while (posData.selectReel < 0) posData.selectReel = 0;
	while (posData.selectReel >= selectLim) posData.selectReel = selectLim - 1;

	if (isSlip()) posData.selectAvailID = 0;
	while (posData.selectAvailID < 0) posData.selectAvailID += AVAIL_ID_MAX;
	while (posData.selectAvailID >= AVAIL_ID_MAX) posData.selectAvailID -= AVAIL_ID_MAX;

	// 20220408add subFlagPos�ʒu�C��
	if (posData.subFlagList.empty()) {
		posData.subFlagPos = 0;
	} else {
		while (posData.subFlagPos < 0) posData.subFlagPos = 0;
		while (posData.subFlagPos >= (int)posData.subFlagList.size()) posData.subFlagPos = posData.subFlagList.size() - 1;
	}

	// �ʒu��������
	SetComaPos(posData.selectReel, true, true);
}

bool CSlotControlManager::canChangeTable() {
	// �I�𒆃��[���Ɖ����ʒu����v���Ȃ��ꍇ�f�[�^�X�V���Ȃ�
	// ������ComSA��sel=2, order=1������
	if (posData.selectReel != posData.currentOrder && !(Get2ndStyle() == 0x03 && posData.selectReel == 2 && posData.currentOrder == 1))
		return false;
	return true;
}

// [act]��~������ꏊ��comaPos��������������
// [prm]pMoveOrder	: ����ύX���郊�[���̉�����
// [ret]�������ʒu��ݒ�ł�����(��������L�̏ꍇfalse)
bool CSlotControlManager::SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp) {
	for (int i = pIsReset ? 0 : 1; i < m_comaMax; ++i) {
		int diffPos = posData.cursorComa[pMoveOrder];
		diffPos += pIsUp ? i : -i;
		while (diffPos < 0)				diffPos += m_comaMax;
		while (diffPos >= m_comaMax)	diffPos -= m_comaMax;

		// ��~�۔���
		if(!GetCanStop(pMoveOrder, diffPos, posData.currentFlagID, false)) continue;
		// �ꏊ�m��
		posData.cursorComa[pMoveOrder] = diffPos; return true;
	}
	return false;
}

// [act]���݂̐�������ɂăt���O: pFlagID��
// [prm]pMoveOrder	: ����ύX���郊�[���̉�����
bool CSlotControlManager::GetCanStop(const int pMoveOrder, const int pLookFor, const int pFlagID, const bool pCheck1st) {
	const int flagID = pFlagID < 0 ? posData.currentFlagID : pFlagID;
	if (flagID >= m_flagMax) return false;
	auto& nowMakeData = ctrlData[flagID];
	auto& srcData = nowMakeData.controlData[posData.stop1st];

	// �g�p�\�ȃ��[���ʒu��return true;(�֐��ŉ���)����d�g��
	if (posData.currentOrder == 0) {
		// 1st�ύX���F����������Ȃ�(�Ώۂ�1st�ł��邩�m�F����)
		if (pMoveOrder != 0) return false;
	} else if (posData.currentOrder == 1) {
		// 2nd�ύX���F�������PS�ȊO && �Q��1st �ł���ΐ����������
		// ���������ComSA�ɂ�3rd�����̃��[�v���Q�ƁA����͂�����Ȃ�
		if(Get2ndStyle(posData.cursorComa[0], pFlagID) != 0 && pMoveOrder == 0 || pCheck1st) {
			const auto activeData = srcData.controlData2nd.activeFlag;
			if ((activeData & (1u << pLookFor)) == 0x0) return false;
		}
		// �����ʉ� or 2nd ��if�𔲂���
	} else if (posData.currentOrder == 2) {
		// 3rd�ύX���F1st,2nd�͕K������
		// ���������ComSA��currentOrder == 1�ɂď��������
		if (pMoveOrder == 0) {
			// 1st�ɐ��䂩��
			const auto activeData = srcData.controlData3rd.activeFlag1st;
			if ((activeData & (1u << pLookFor)) == 0x0) return false;;
		} else if (pMoveOrder == 1){
			// 2nd�ɐ��䂩��
			const auto activeData = srcData.controlData3rd.activeFlag2nd[posData.cursorComa[0]];
			int shiftNum = pLookFor + (posData.isWatchLeft ? 0 : m_comaMax);
			if ((activeData & (1ull << shiftNum)) == 0x0) return false;
		}
		// �����ʉ� or 3rd ��if�𔲂���
	}
	return true;
}

int CSlotControlManager::Get2ndReel(bool pIsLeft, int pStop1stOrder) {
	if (pStop1stOrder < 0 || pStop1stOrder >= m_reelMax)	return -1;

	if (pIsLeft)	return pStop1stOrder == 0 ? 1 : 0;	// 2nd�����̏ꍇ
	else			return pStop1stOrder == 2 ? 1 : 2;	// 2nd�E���̏ꍇ
}
int CSlotControlManager::Get2ndReel(bool pIsLeft) { return Get2ndReel(pIsLeft, posData.stop1st); }

bool CSlotControlManager::Action(int pNewInput) {
	bool setAns = true;
	if (!canChangeTable()) return true;
	m_refreshFlag = true;

	auto refData = GetDef();
	if (refData != nullptr) {	// SA�e�[�u��
		int comaIndex = posData.currentOrder;
		const auto styleData = Get2ndStyle();
		if(posData.currentOrder >= 1 && styleData == 0x3) comaIndex = posData.selectReel;
		*refData = SetAvailT(setAns, refData->availableID, posData.cursorComa[comaIndex], pNewInput, refData->tableFlag);
		CheckSA(); // ��~�s�\�ӏ����݊m�F
	} else {					// SS�e�[�u��
		auto refDataSS = GetSS();
		if (refDataSS == nullptr) return false;
		int comaIndex = posData.currentOrder;
		*refDataSS = SetSlipT(setAns, *refDataSS, posData.cursorComa[comaIndex], pNewInput);
	}
	if (!setAns) return false;
	return UpdateActiveFlag();
}

// [act]�e�[�u���g���܂킵�ړI�Ńe�[�u��ID���蓮�Őݒ肷��
bool CSlotControlManager::ActionTableID(bool pIsUp) {
	auto refData = GetDef();
	if (refData != nullptr) {	// SA�e�[�u��
		// 20220507: spt�f�[�^�Ȃ���͂𖳎�����(return true)
		if ((refData->tableFlag & 0x3) == 0x3) return true;

		int startID = refData->availableID + AVAIL_TABLE_MAX;
		for (int offset = 1; offset <= AVAIL_TABLE_MAX; ++offset) {
			const auto srcID = startID % AVAIL_TABLE_MAX;
			const auto nowID = (startID + (pIsUp ? offset : -offset)) % AVAIL_TABLE_MAX;
			if (tableAvailable[nowID].refNum == 0) continue;
			const unsigned char newID = nowID & (unsigned char)0xFF; // UCHAR�^�ɕϊ�
			refData->availableID = newID;
			--tableAvailable[srcID].refNum;
			++tableAvailable[newID].refNum;
			m_refreshFlag = true;
			refData->tableFlag |= 0x4;	// 20220508: priority�������I�ɗL���ɂ���(�o�O�΍�)
			return UpdateActiveFlag();
		}
		return false;
	} else {					// SS�e�[�u��
		auto refDataSS = GetSS();
		if (refDataSS == nullptr) return false;
		int startID = *refDataSS + SLIP_TABLE_MAX;
		for (int offset = 1; offset <= AVAIL_TABLE_MAX; ++offset) {
			const auto srcID = startID % AVAIL_TABLE_MAX;
			const auto nowID = (startID + (pIsUp ? offset : -offset)) % SLIP_TABLE_MAX;
			if (tableSlip[nowID].refNum == 0) continue;
			const unsigned char newID = nowID & (unsigned char)0xFF; // UCHAR�^�ɕϊ�
			*refDataSS = newID;
			--tableSlip[srcID].refNum;
			++tableSlip[newID].refNum;
			m_refreshFlag = true;
			return UpdateActiveFlag();
		}
	}
	return false;
}

SControlDataSA* CSlotControlManager::GetSA(int pFlagID, int pNowCheckOrder, int pStop1stOrder, int pPushPos1st, int pPushPos2nd) {
	// �p�����[�^�G���[����
	if (pFlagID < 0 || pFlagID >= m_flagMax)				return nullptr;
	if (pNowCheckOrder < 0 || pNowCheckOrder >= m_reelMax)	return nullptr;
	if (pStop1stOrder < 0 || pStop1stOrder >= m_reelMax)	return nullptr;
	if (pPushPos1st < 0 || pPushPos1st >= m_comaMax)		return nullptr;

	auto& nowMakeData = ctrlData[pFlagID];
	const auto styleData = Get2ndStyle(pPushPos1st, pStop1stOrder, pFlagID);
	auto& nowCtrlData = nowMakeData.controlData[pStop1stOrder];

	if (pNowCheckOrder == 0) return nullptr;	// 1st����(=�����Ȃ�)
	if (styleData == 0x3) {							// 2nd�ȍ~ 2nd/3rdCom����
		const int index = pPushPos1st;
		return &(nowCtrlData.controlData2nd.controlDataComSA[index]);
	} else if (pNowCheckOrder == 1) {			// 2nd����
		if (styleData == 0x2) {						// 2ndStopAvailable(pPush1st����~�\�ʒu�̏ꍇ�̂�index�ɗL���f�[�^����)
			const int index = pPushPos1st;
			return &(nowCtrlData.controlData2nd.controlData2ndSA[index]);
		}
	} else if (pNowCheckOrder == 2) {			// 3rd����(��Com)
		const int index = pPushPos1st * m_comaMax + pPushPos2nd;
		return &(nowCtrlData.controlData3rd.availableData[index]);
	}
	return nullptr;
}
SControlDataSA* CSlotControlManager::GetSA() { return GetSA(posData.currentFlagID); }
SControlDataSA* CSlotControlManager::GetSA(int pFlagID) { return GetSA(pFlagID, posData.currentOrder, posData.stop1st, posData.cursorComa[0], posData.cursorComa[1]); }

// [act]��~�s�\�J�����Ȃ������؁E����ꍇ��m_isSuspend��true�ɂ���
void CSlotControlManager::CheckSA() {
	m_isSuspend = false;
	const SControlDataSA* const sa = GetSA();
	if (sa == nullptr) return;
	for (int pos = 0; pos < m_comaMax; ++pos) {
		if (GetPosFromAvailT(*sa, pos, posData.isWatchLeft) == -1) { m_isSuspend = true; break; }
	}
}

SControlAvailableDef* CSlotControlManager::GetDef() {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	const auto styleData = Get2ndStyle();
	auto& nowCtrlData = nowMakeData.controlData[posData.stop1st];
	const int dataID = posData.selectAvailID + (posData.isWatchLeft ? 0 : AVAIL_ID_MAX);

	if (posData.currentOrder == 0) return nullptr;	// 1st����(=�����Ȃ�)
	if (styleData == 0x3) {							// 2nd�ȍ~ 2nd/3rdCom����
		const int index = posData.cursorComa[0];
		return &(nowCtrlData.controlData2nd.controlDataComSA[index].data[dataID]);
	} else if (posData.currentOrder == 1) {			// 2nd����
		if (styleData == 0x2) {						// 2ndStopAvailable(cursorComa[0]�ɐ�������)
			const int index = posData.cursorComa[0];
			return &(nowCtrlData.controlData2nd.controlData2ndSA[index].data[dataID]);
		}
	} else if (posData.currentOrder == 2) {			// 3rd����(��Com)
		const int index = posData.cursorComa[0] * m_comaMax + posData.cursorComa[1];
		return &(nowCtrlData.controlData3rd.availableData[index].data[dataID]);
	}
	return nullptr;
}

// [act]���ݎQ�ƒ��̊Y���X�x��ID�̓������ϐ��|�C���^��Ԃ�
unsigned char* CSlotControlManager::GetSS(int pFlagID, bool pGet1st, int pStop1stOrder, int pPushPos1st, bool pIsWatchLeft) {
	// �p�����[�^�G���[����
	if (pFlagID < 0 || pFlagID >= m_flagMax)				return nullptr;
	if (pStop1stOrder < 0 || pStop1stOrder >= m_reelMax)	return nullptr;
	if (pPushPos1st < 0 || pPushPos1st >= m_comaMax)		return nullptr;

	auto& nowMakeData = ctrlData[pFlagID];
	auto& nowCtrlData = nowMakeData.controlData[pStop1stOrder];

	if (pGet1st) {		// 1st����
		return &nowCtrlData.controlData1st;
	} else {			// 2nd����
		const auto styleData = Get2ndStyle(pPushPos1st, pFlagID);
		int index = pPushPos1st + (pIsWatchLeft ? 0 : m_comaMax);
		if (styleData == 0x0) {						// 2ndPushSlip
			return &nowCtrlData.controlData2nd.controlData2ndPS[index];
		}
		else if (styleData == 0x1) {				// 2ndStopSlip(pPush1st����~�\�ʒu�̏ꍇ�̂�index�ɗL���f�[�^����)
			return &nowCtrlData.controlData2nd.controlData2ndSS[index];
		}
	}
	return nullptr;
}
unsigned char* CSlotControlManager::GetSS(            ) { return GetSS(posData.currentFlagID, posData.currentOrder == 0, posData.stop1st, posData.cursorComa[0], posData.isWatchLeft); }
unsigned char* CSlotControlManager::GetSS(int pFlagID ) { return GetSS(pFlagID              , posData.currentOrder == 0, posData.stop1st, posData.cursorComa[0], posData.isWatchLeft); }
unsigned char* CSlotControlManager::GetSS(bool pGet1st) { return GetSS(posData.currentFlagID, pGet1st                  , posData.stop1st, posData.cursorComa[0], posData.isWatchLeft); }

unsigned char CSlotControlManager::Get2ndStyle(const int pPushPos, int pStop1stOrder, const int pFlagID) {
	// �p�����[�^�G���[����
	if (pFlagID < 0 || pFlagID >= m_flagMax)				return 0;
	if (pPushPos < 0 || pPushPos >= m_comaMax)				return 0;
	if (pStop1stOrder < 0 || pStop1stOrder >= m_reelMax)	return 0;

	const auto& nowMakeData1st = ctrlData[pFlagID];
	if (((nowMakeData1st.controlStyle >> pStop1stOrder) & 0x1) == 0) return 0x0;

	const auto& nowMakeData2nd = nowMakeData1st.controlData[pStop1stOrder].controlData2nd.controlStyle2nd[pPushPos];
	return nowMakeData2nd;
}
unsigned char CSlotControlManager::Get2ndStyle() { return Get2ndStyle(posData.cursorComa[0], posData.currentFlagID); }
unsigned char CSlotControlManager::Get2ndStyle(const int pPushPos, const int pFlagID) { return Get2ndStyle(pPushPos, posData.stop1st, pFlagID); }

bool CSlotControlManager::isSlip(const int pFlagID, int pCurrentOrder, int pStop1stOrder,  int pPushPos1st) {
	// �p�����[�^�G���[����
	if (pFlagID < 0 || pFlagID >= m_flagMax)				return false;
	if (pCurrentOrder < 0 || pCurrentOrder >= m_reelMax)	return false;
	if (pStop1stOrder < 0 || pStop1stOrder >= m_reelMax)	return 0;
	if (pPushPos1st < 0 || pPushPos1st >= m_comaMax)		return false;

	if (pCurrentOrder == 0) return true;
	if (pCurrentOrder == 2) return false;
	return !(Get2ndStyle(pPushPos1st, pStop1stOrder, pFlagID) & 0x2);
}
bool CSlotControlManager::isSlip() {return isSlip(posData.currentFlagID); }
bool CSlotControlManager::isSlip(const int pFlagID) {return isSlip(pFlagID, posData.currentOrder, posData.stop1st, posData.cursorComa[0]); }

// [act]����p�^�[����ʐ؂�ւ�(1:PS, 2:SS, 3:SA, 4:Com)
void CSlotControlManager::SetAvailCtrlPattern(unsigned char pNewFlag) {
	if (posData.currentOrder == 0) {
		auto& nowMakeData = ctrlData[posData.currentFlagID];
		const int offset = (posData.stop1st);
		// ���݂̐ݒ�폜(1st)(pNewFlag == 0�̏ꍇ�͂��̂܂܂ɂ���)
		nowMakeData.controlStyle &= ~(0x1 << offset);

		if (pNewFlag >= 1) {
			// �V�K�ݒ蓱��(1st)
			nowMakeData.controlStyle |= (0x1 << offset);
		}
	}
	else if (posData.currentOrder == 1) {
		if (ctrlData[posData.currentFlagID].controlStyle == 0) return;
		if (pNewFlag == 0) return;
		auto& nowMakeData = ctrlData[posData.currentFlagID].controlData[posData.stop1st].controlData2nd.controlStyle2nd[posData.cursorComa[0]];
		// �V�K�ݒ蓱��(2nd)
		nowMakeData = pNewFlag;
	}
	else return;

	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]SA�e�[�u���^�C�v����(���~T, �D��T)
void CSlotControlManager::SwitchATableType() {
	if (!canChangeTable()) return;
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// �V�K�t���O�ݒ�
	const auto nowFlag = refData->tableFlag & 0x04;
	refData->tableFlag = (refData->tableFlag & 0xFB) | (~nowFlag & 0x04);

	CheckSA(); // ��~�s�\�ӏ����݊m�F
	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]SA�V�t�g�؂�ւ�(0:�V�t�g��, 1:��1�R�}, 2:��1�R�}, 3:����)
void CSlotControlManager::SetAvailShiftConf(unsigned char pNewFlag) {
	if (!canChangeTable()) return;
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// �V�K�t���O�ݒ�
	if(pNewFlag <= 2) refData->tableFlag = (refData->tableFlag & 0xfc) | (pNewFlag & 0x03);
	else {
		if (refData->tableFlag & 0x08) refData->tableFlag &= (~0x08);	// �񔽓]�ݒ�
		else refData->tableFlag |= 0x08;								// ���]�ݒ�
	}

	CheckSA(); // ��~�s�\�ӏ����݊m�F
	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]�X�x��T�X�V�A�e�[�u��No�̕t�^�E�̔Ԃ܂ōs��
// [ret]�V�K�X�x��T �e�[�u���ԍ�(0-255) �������G���[����0+pCHK��false������
unsigned char CSlotControlManager::SetSlipT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal) {
	pCHK = true;
	if (pSrcTableNo >= tableSlip.size()) { pCHK = false; return 0; }
	if (pNewVal < 0 || pNewVal >= 5) { pCHK = false; return 0; }
	unsigned long long newData = 0;
	unsigned int activePos = 0;

	// �f�[�^�����EactivePos�X�V
	for (int pp = 0; pp < m_comaMax; ++pp) {
		int slipVal = (GetPosFromSlipT(pSrcTableNo, pp) + m_comaMax - pp) % m_comaMax;
		if (pp == pPushPos) slipVal = pNewVal;
		newData = newData + (unsigned long long)pow(5, pp) * slipVal;	// �V�t�g+���
		const int nowActive = (pp + slipVal) % m_comaMax;
		activePos |= (1 << nowActive);
	}

	// �̔�
	--tableSlip[pSrcTableNo].refNum;
	size_t firstNonRef = SLIP_TABLE_MAX;
	for (size_t ref = 0; ref < tableSlip.size(); ++ref) {
		// ��v�̂���ꍇ: ref����1�����ăe�[�u���ԍ���Ԃ�
		if (tableSlip[ref].data == newData) { ++tableSlip[ref].refNum; return (unsigned char)ref; }
		// �Q�Ƃ��Ȃ��ŏ��̃f�[�^�̎擾
		if (firstNonRef == SLIP_TABLE_MAX && tableSlip[ref].refNum == 0) { firstNonRef = ref; }
	}
	// ��v�̂Ȃ��ꍇ: �Q�Ƃ̂Ȃ��ŏ��̃f�[�^�ɐV�����f�[�^��������
	tableSlip[firstNonRef].refNum = 1;
	tableSlip[firstNonRef].data = newData;
	tableSlip[firstNonRef].activePos = activePos;
	return firstNonRef;
}

// [act]����T�X�V�A�e�[�u��No�̕t�^�E�t�������w��E�̔Ԃ܂ōs��
// [ret]�V�K����T SControlAvailableDef�C���X�^���X �������G���[���͋�f�[�^+pCHK��false������
SControlAvailableDef CSlotControlManager::SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const unsigned char pTableFlag) {
	pCHK = true;
	const bool isPrior = (pTableFlag & 0x4) != 0;
	const bool isReverse = (pTableFlag & 0x8) != 0;
	SControlAvailableDef ans{ 0,0 };
	if (pSrcTableNo >= tableAvailable.size()) { pCHK = false; return ans; }
	if (pNewVal < 0 || pNewVal >= 5) { pCHK = false; return ans; }

	// �f�[�^�쐬
	unsigned long long srcData = GetAvailDefData(pSrcTableNo, pTableFlag);
	unsigned long long newData = GetAvailShiftData(srcData, pTableFlag);
	const unsigned long long posFlag = 1ull << pPushPos;
	if (pNewVal >= 1)	newData |=  posFlag;	// �ʒu�L����
	else				newData &= ~posFlag;	// �ʒu������
	if((pTableFlag & 0x03) != 0x03) --tableAvailable[pSrcTableNo].refNum;

	/* �r�b�g���J�E���g: 1or20�Ȃ����ӏ��\�L�� */ {
		int bitPos[] = { 0xFF, 0xFF };	// 0��:0xFF, 1��:�Y���ӏ�, 2�`:-1
		int nowBit = newData;
		for(int i=0; i<m_comaMax; ++i){
			const int refB = (nowBit & 0x1) ? 1 : 0;
			if (bitPos[refB] == 0xFF) bitPos[refB] = i; else bitPos[refB] = -1;
			nowBit >>= 1;
		}
		// ����
		for (int i = 0; i < 2; ++i) {
			if (bitPos[i] >= 0 && bitPos[i] != 0xFF) {
				ans.availableID = bitPos[i] + 1;
				ans.tableFlag = 0x3;
				if (isPrior) ans.tableFlag |= 0x4;
				if (i == 0) ans.tableFlag |= 0x8;	// 0:���]T(0��1��), 1:�񔽓]T(1��1��)
				return ans;
			}
		}
	}

	// �̔�
	size_t firstNonRef = AVAIL_TABLE_MAX;
	unsigned char hitStyle = 0xFF;
	for (size_t ref = 0; ref < tableAvailable.size(); ++ref) {
		for (unsigned char style = 0; style < 3*2; ++style) {
			const int applyStyle = style % 3 + 8 * (style / 3);
			if (GetAvailShiftData(tableAvailable[ref].data, applyStyle) == newData) {
				hitStyle = applyStyle; break;
			}
		}
		// ��v�̂���ꍇ: ref����1������Def��Ԃ�
		if (hitStyle != 0xFF) {
			++tableAvailable[ref].refNum;
			ans.availableID = (unsigned char)ref;
			ans.tableFlag = hitStyle;
			if (isPrior) ans.tableFlag |= 0x4;
			return ans;
		}
		// �Q�Ƃ��Ȃ��ŏ��̃f�[�^�̎擾
		if (firstNonRef == AVAIL_TABLE_MAX && tableAvailable[ref].refNum == 0) { firstNonRef = ref; }
	}
	// ��v�̂Ȃ��ꍇ: �Q�Ƃ̂Ȃ��ŏ��̃f�[�^�ɐV�����f�[�^��������
	tableAvailable[firstNonRef].refNum = 1;
	tableAvailable[firstNonRef].data = newData;
	ans.availableID = (unsigned char)firstNonRef;
	ans.tableFlag = isPrior ? 0x4 : 0x0;
	return ans;
}

// [act]���ݑI�𒆃t���O�̒�~�\�ʒu���A�b�v�f�[�g����
bool CSlotControlManager::UpdateActiveFlag() {
	auto& nowData = ctrlData[posData.currentFlagID];
	int orderCnt = 0;
	for (auto ctrlIt = nowData.controlData.begin(); ctrlIt != nowData.controlData.end(); ++ctrlIt, ++orderCnt) {
		const unsigned char IDFirst = ctrlIt->controlData1st;
		const int active1st = tableSlip[IDFirst].activePos;
		ctrlIt->controlData2nd.activeFlag = active1st;
		ctrlIt->controlData3rd.activeFlag1st = active1st;

		// activeFlag2nd������
		for (int i = 0; i < m_comaMax; ++i) ctrlIt->controlData3rd.activeFlag2nd[i] = 0;

		for (int pushPos = 0; pushPos < m_comaMax*2; ++pushPos) {	// 0:LR, 1:LR, ...
			const unsigned char useTable2nd = Get2ndStyle(pushPos % m_comaMax, orderCnt, posData.currentFlagID);
			const bool lrFlag = ((pushPos / m_comaMax) == 0);
			unsigned long long active2nd = 0;
			if (useTable2nd == 0x0) {
				const int ref2nd = (pushPos % m_comaMax) + (m_comaMax * (pushPos / m_comaMax));
				active2nd = tableSlip[ctrlIt->controlData2nd.controlData2ndPS[ref2nd]].activePos;
			} else if (useTable2nd == 0x1) {
				const int ref2nd = (pushPos % m_comaMax) + (m_comaMax * (pushPos / m_comaMax));
				active2nd = tableSlip[ctrlIt->controlData2nd.controlData2ndSS[ref2nd]].activePos;
			} else if (useTable2nd == 0x2) {
				const int ref2nd = pushPos % m_comaMax;
				active2nd = GetActiveFromAvailT(ctrlIt->controlData2nd.controlData2ndSA[ref2nd], lrFlag);
				// �����s�������f�[�^ -> active�f�[�^�N���A���ďI��
				if (active2nd == 0) {
					ctrlIt->controlData3rd.activeFlag2nd[pushPos % m_comaMax] = 0;
					return true;
				}
			} else if (useTable2nd == 0x3) {
				// �������Ȃ� -> 20220507:���Α��̒�~�\�ʒu��activeFlag���R�s�[
				const int ref2nd = pushPos % m_comaMax;
				active2nd = GetActiveFromAvailT(ctrlIt->controlData2nd.controlDataComSA[ref2nd], lrFlag);
				// �����s�������f�[�^ -> active�f�[�^�N���A���ďI��
				if (active2nd == 0) {
					ctrlIt->controlData3rd.activeFlag2nd[pushPos % m_comaMax] = 0;
					return true;
				}
			} else {
				return false;
			}

			int activeInsert = pushPos % m_comaMax;
			if (useTable2nd == 0x0) activeInsert = GetPosFromSlipT(IDFirst, activeInsert);
			if (lrFlag) {
				ctrlIt->controlData3rd.activeFlag2nd[activeInsert] |= active2nd;
			} else {
				ctrlIt->controlData3rd.activeFlag2nd[activeInsert] |= (active2nd << m_comaMax);
			}
		}
	}
	return true;
}

int CSlotControlManager::GetPosFromSlipT(const size_t pTableNo, const int pPushPos) {
	if (pTableNo >= tableSlip.size()) return -1;
	if (pPushPos < 0 || pPushPos >= m_comaMax) return -1;
	const unsigned long long data = tableSlip[pTableNo].data;
	const int slipNum = (data / (unsigned long long)pow(5, pPushPos)) % 5;
	return (pPushPos + slipNum) % m_comaMax;
}

unsigned long long CSlotControlManager::GetAvailShiftData(unsigned long long pData, const unsigned char pShiftFlag) {
	const auto shiftFlag = pShiftFlag & 0x3;
	const unsigned long long allStopFlag = m_allStopFlag;

	if (shiftFlag == 0x1) { // ��1�R�}(LShift) ��bit������O��Ŏ�������
		pData <<= 1;
		if (pData & ~allStopFlag) pData |= 0x1;	// �I�[�o�[�t���[���m
		pData &= allStopFlag;
	}
	if (shiftFlag == 0x2) { // ��1�R�}(RShift)
		const bool addFlag = pData & 0x1;
		pData = (pData >> 1) & allStopFlag;
		if (addFlag) pData = pData | (0x1ull << (m_comaMax - 1));
	}
	if (pShiftFlag & 0x8) {	// ���]
		pData = ~pData & allStopFlag;
	}
	return pData;
}

// [act]�t�B���^�ɂ��V�t�g�E���]�O��availableData��~�ʒu���擾����
//		����ӏ�T�ɑΉ�����
unsigned long long CSlotControlManager::GetAvailDefData(const int pAvailIndex, const unsigned char pTableFlag) {
	unsigned long long data;
	if ((pTableFlag & 0x3) == 0x3) data = 1ull << (pAvailIndex - 1);// ����ӏ�T
	else data = tableAvailable[pAvailIndex].data;					// ��������T
	return data;
}


unsigned long long CSlotControlManager::GetAvailShiftData(const SControlDataSA& pSAData, const int pIndex, bool pIsLeft) {
	if (pIndex < 0 || pIndex >= AVAIL_ID_MAX) return 0;
	const auto	index   = pIndex + (pIsLeft ? 0 : AVAIL_ID_MAX);
	const auto	tableNo = pSAData.data[index].availableID;
	const auto	flag    = pSAData.data[index].tableFlag;
	const auto	data	= GetAvailDefData(tableNo, flag);
	return GetAvailShiftData(data, flag);
}

// [ret]��~�ʒu �������������߂���̂��Ȃ��Ƃ���-1
int CSlotControlManager::GetPosFromAvailT(const SControlDataSA& pSAData, const int pPushPos, bool pIsLeft) {
	const unsigned long long allStopFlag = m_allStopFlag;
	unsigned long long availPos = allStopFlag;
	const int offsetLR = pIsLeft ? 0 : AVAIL_ID_MAX;
	for (int i = 0; i < AVAIL_ID_MAX; ++i) {
		auto it = pSAData.data.cbegin() + i + offsetLR;
		unsigned long long availData = GetAvailDefData(it->availableID, it->tableFlag);
		availData = GetAvailShiftData(availData, it->tableFlag);

		/* �D��x�ɂ�鏈������ */ {
			const bool isPrior = ((it->tableFlag & 0x4) != 0x0);
			if (isPrior) {	// �D��T
				availData &= availPos;	// ���O�ɔ��~�Ƃ��ꂽ��~�ʒu������
				const int slip = GetAvailDistance(availData, pPushPos);
				if (slip >= 0) return (pPushPos + slip) % m_comaMax;
			} else {		// ���~T
				availPos &= availData;
			}
		}
	}

	// ���~T�ɂ��ʒu���茋�ʂ�Ԃ�
	const int slipNum = GetAvailDistance(availPos, pPushPos);
	if (slipNum == -1) return -1;
	return (pPushPos + slipNum) % m_comaMax;
}

// [ret]��~�ʒu�r�b�g�� �����������ȃR�}��1�ł������0
int CSlotControlManager::GetActiveFromAvailT(const SControlDataSA& pSAData, bool pIsLeft) {
	int ans = 0;
	for (int pushPos = 0; pushPos < m_comaMax; ++pushPos) {
		const int availPos = GetPosFromAvailT(pSAData, pushPos, pIsLeft);
		if (availPos == -1) return 0;	// ��~�ʒu�Ȃ��ŋ����I��
		ans |= (1 << availPos);
	}
	return ans;
}

// [act]��~�\�ȍł��߂���~�\�ʒu�܂ł̋������o���B(0-4, �͈͊O��-1�Ƃ���)
int CSlotControlManager::GetAvailDistance(const unsigned long long pData, const int pPushPos) {
	int refPos = pPushPos; // LSB��0�Ԃ̃f�[�^
	for (int pos = 0; pos < 5; ++pos) {
		if ((pData >> refPos) & 0x1) return pos;	// �V�t�g��LSB=1�Ȃ��~�\
		refPos = (refPos + 1) % m_comaMax;			// ���������refPos���Z�o
	}
	return -1;
}


bool CSlotControlManager::Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor) {
	DxLib::SetDrawScreen(pDrawFor);
	std::vector<int> drawPos; drawPos.resize(m_reelMax * 2, -1);	// 0PS, 1PS, 2PS
	const int reelPosByOrder[] = { Get2ndReel(posData.isWatchLeft), Get2ndReel(!posData.isWatchLeft) };

	/* ���[���E���ܖ�`�� */ {
		/* �`�惊�[����` */ {
			if (posData.currentOrder == 0) {			// 1st
				const auto ss = GetSS();
				drawPos[posData.stop1st * 2    ] = posData.cursorComa[0];
				drawPos[posData.stop1st * 2 + 1] = GetPosFromSlipT(*ss, posData.cursorComa[0]);
			} else if (Get2ndStyle() == 0x03) {			// 2nd�ȍ~ 2nd/3rd����
				const auto sa = GetSA();
				drawPos[posData.stop1st   * 2    ] = posData.cursorComa[0];
				drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
				drawPos[Get2ndReel(true)  * 2    ] = posData.cursorComa[1];
				drawPos[Get2ndReel(true)  * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[1], true);
				drawPos[Get2ndReel(false) * 2    ] = posData.cursorComa[2];
				drawPos[Get2ndReel(false) * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[2], false);
			} else if (posData.currentOrder == 1) {		// 2ndComSA�ȊO
				drawPos[posData.stop1st   * 2    ] = posData.cursorComa[0];
				drawPos[reelPosByOrder[0] * 2    ] = posData.cursorComa[1];
				if (Get2ndStyle() == 0x00) {				// PS�e�[�u��
					const unsigned char *const ss1 = GetSS(true);	// -1:���ݑI�𒆂̃t���O
					const unsigned char *const ss2 = GetSS(false);
					drawPos[posData.stop1st   * 2 + 1] = GetPosFromSlipT(*ss1, posData.cursorComa[0]);
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromSlipT(*ss2, posData.cursorComa[1]);
				} else if (Get2ndStyle() == 0x01) {			// SS�e�[�u��
					const unsigned char *const ss2 = GetSS(false);
					drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromSlipT(*ss2, posData.cursorComa[1]);
				} else {									// SA�e�[�u��
					const auto sa = GetSA();
					drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[1], posData.isWatchLeft);
				}
			} else {									// 3rdComSA�ȊO
				const auto sa = GetSA();
				drawPos[posData.stop1st   * 2    ] = posData.cursorComa[0];
				drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
				drawPos[reelPosByOrder[0] * 2    ] = posData.cursorComa[1];
				drawPos[reelPosByOrder[0] * 2 + 1] = posData.cursorComa[1];
				drawPos[reelPosByOrder[1] * 2    ] = posData.cursorComa[2];
				drawPos[reelPosByOrder[1] * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[2], posData.isWatchLeft);
			}
		}
		SReelDrawData drawReel;
		/* SReelDrawData�����쐬 */ {
			drawReel.y = 1;
			drawReel.comaW = 77; drawReel.comaH = 35; drawReel.comaNum = 3;
			drawReel.offsetLower = 0; drawReel.offsetUpper = 0;
		}
		// ���[���`��
		for (size_t i = 0; i < drawPos.size(); i+=2) {
			if (drawPos[i] < 0) continue;
			const int posOffset = drawReel.comaW / 2 - 8;
			const int posY = drawReel.y + drawReel.comaH * drawReel.comaNum + 1;
			drawReel.x = 502 + 78 * (i/2); drawReel.reelID = i/2;
			int castPos = (m_comaMax * 2 - 3 - drawPos[i]) % m_comaMax;
			pData.reelManager.DrawReel(pDataManageIns, drawReel, castPos, pDrawFor);
			DxLib::DrawFormatString(drawReel.x + posOffset, posY, 0xFFFF00, "%02d", drawPos[i] + 1);

			drawReel.x = 768 + 78 * (i/2);
			castPos = (m_comaMax * 2 - 3 - drawPos[i + 1]) % m_comaMax;
			pData.reelManager.DrawReel(pDataManageIns, drawReel, castPos, pDrawFor);
			DxLib::DrawFormatString(drawReel.x + posOffset, posY, 0xFFFF00, "%02d", drawPos[i+1] + 1);
		}

		/* ���ܖ�`�� */ {
			bool drawFlag = true;
			std::vector<int> checkPos(m_reelMax);
			for (size_t i = 0; i < checkPos.size(); ++i) {
				if (drawPos[i * 2 + 1] == -1) { drawFlag = false; break; }
				checkPos[i] = (m_comaMax * 2 - drawPos[i * 2 + 1] - 3) % m_comaMax;
			}
			if (drawFlag) {
				const auto stopData = pData.reelChecker.GetPosData(posData.stop1st, checkPos);
				const int color[] = { 0xFFFFFF, 0x009631, 0xFF4040, 0xFF00FF };
				const int width = DxLib::GetDrawStringWidth(stopData.spotFlag.c_str(), stopData.spotFlag.size());
				DxLib::DrawString(752 - width / 2, 65, stopData.spotFlag.c_str(), color[stopData.reachLevel]);
				DxLib::DrawFormatString(744, 80, color[stopData.reachLevel], "L%1d", stopData.reachLevel);
			}
		}
	}

	/* �e����`��(��) */ {
		int tableNo = 0;
		std::string availFlag = "";
		const auto* const sd = GetDef();
		if (sd != nullptr) {
			tableNo = sd->availableID;
			const auto flag = sd->tableFlag;
			const std::string availType[] = {"Def", "1up", "1dn", "Spt"};
			availFlag += availType[flag & 0x3] + ", ";
			availFlag += (flag & 0x4) ? "Pri" : "Neg";
			availFlag += (flag & 0x8) ? ", Inv" : "";
		} else {
			tableNo = *GetSS();
		}
		DxLib::DrawFormatString(225,  0, 0xFFFF00, "selectReel    : %d", posData.selectReel);
		DxLib::DrawFormatString(225, 20, 0xFFFF00, "currentOrder  : %d", posData.currentOrder);
		DxLib::DrawFormatString(225, 40, 0xFFFF00, "stop1st       : %d", posData.stop1st);
		const std::string ctrlType[] = {"PushS", "StopS", "2ndSA", "ComSA"};
		DxLib::DrawFormatString(225, 60, 0xFFFF00, "ctrlType      : %s", ctrlType[Get2ndStyle()].c_str());
		DxLib::DrawFormatString(225, 80, 0xFFFF00, "tableNo       : %d", tableNo);
		if(availFlag != "")
			DxLib::DrawFormatString(225,100, 0xFFFF00, "availFlag     : %s", availFlag.c_str());
	}

	/* �e�[�u���`��E�e�[�u���ԍ��`�� */ {
		// x=275, 298 y = 176
		int xPos = 227;
		const int yPos = 176;
		// ���[���ԍ��`��
		for (int i = 0; i < m_comaMax; ++i) {
			const int strY = yPos + i * BOX_H;
			DxLib::DrawFormatString(xPos-2, strY, 0x808080, "%02d", m_comaMax - i);
		}
		xPos += BOX_W;
		
		DrawSlipTable(xPos, yPos, posData.currentFlagID, true, drawPos, pData);
		xPos += BOX_W;

		if (!isSlip()) {
			DrawStopTable(xPos, yPos, posData.currentFlagID);
			xPos += BOX_W * AVAIL_ID_MAX;
		}
		xPos += BOX_W;

		// �Q�l�����Ƃ��đ��e�[�u����`��
		posData.subFlagList.clear();	// 20220408�ǉ� ���������N���A����
		for (int flagC = 0; flagC < m_flagMax; ++flagC) {
			if (flagC == posData.currentFlagID) continue;
			bool isNotStop = false;
			for (int i = 0; i < posData.currentOrder; ++i) {
				const int drawRefIndex = (i == 0 ? posData.stop1st : reelPosByOrder[i - 1]) * 2 + 1;
				if (!GetCanStop(i, drawPos[drawRefIndex], flagC, true)) { isNotStop = true; break; }
			}
			if (isNotStop) continue;

			// �e�[�u���������Ɠ����ɁA�߂�l(�`�搔)�ɉ������`��ʒu�̈ړ����s��
			xPos += BOX_W * DrawSlipTable(xPos, yPos, flagC, false, drawPos, pData);
			posData.subFlagList.push_back(flagC);	// 20220408�ǉ� ���X�g�ǉ�
		}

		// 20220408�ǉ� ��`�T�u�`��ǉ�
		if (!posData.subFlagList.empty()) {
			if (posData.subFlagPos >= 0 && (size_t)posData.subFlagPos < posData.subFlagList.size()) {
				const int index = posData.subFlagList[posData.subFlagPos];

				// �t���O���`��
				const std::string flagName  = pData.randManager.GetFlagName (index);
				const std::string bonusName = pData.randManager.GetBonusName(index);
				const std::string flagOut   = flagName + " " + bonusName;
				DxLib::DrawFormatString(1010, 65, 0xF0F0FF, "%s", flagOut.c_str());

				// �����`�`��
				if (isSlip(index)) {
					const auto ss = GetSS(index);
					if (ss == nullptr) return false;
					DxLib::DrawFormatString(1010, 80, 0xF0F0FF, "SlipT  : %d", *ss);
				} else {
					SControlDataSA* sa = GetSA(index);
					if (sa == nullptr) return false;
					DxLib::DrawString(1010, 80, "AvailT :", 0xF0F0FF);

					// 20220507: �I�𒆃f�[�^��ComSA����currentOrder=2�Ȃ�watchLeft���]
					const bool watchLeft = (Get2ndStyle(posData.cursorComa[0], index) == 0x3 && posData.currentOrder == 2) ?
						!posData.isWatchLeft : posData.isWatchLeft;

					for (int i = 0; i < AVAIL_ID_MAX; ++i) {
						const int saWatch = i + (watchLeft ? 0 : AVAIL_ID_MAX);
						const std::string shiftType[] = {"D", "u", "d", "s"};
						const std::string invPriorType[] = {"  ", " P", "I ", "IP"};
						const auto& type = sa->data[saWatch].tableFlag;
						const std::string typeStr = shiftType[type & 0x3] + invPriorType[(type >> 2) & 0x3];
						DxLib::DrawFormatString(1090 + i*63, 80, 0xF0F0FF, "%3d%s", sa->data[saWatch].availableID, typeStr.c_str());
					}
				}
			}
		}
	}

	// ����`�F�b�N���ʕ`��
	DrawStopStatus(pData);

	return true;
}

bool CSlotControlManager::DrawComaBox(int x, int y, const unsigned int pStopPos, int pLightPos, int boxColor) {
	for (int pos = 0; pos < m_comaMax; ++pos) {
		const int dx = x + BOX_W - BOX_A;
		const int colorStop = ((pStopPos >> (m_comaMax - 1 - pos)) & 0x1) ? 0xFF0000 : 0x400000;
		const int dy = y + BOX_H * pos;
		DxLib::DrawBox(dx, dy, dx + BOX_A, dy + BOX_H, colorStop,  TRUE);
		const int lineColor = (pLightPos == (m_comaMax - 1 - pos)) ? 0xFFFF00 : boxColor;
		DxLib::DrawBox( x, dy,  x + BOX_W, dy + BOX_H, lineColor, FALSE);
	}
	return true;
}

// [act]�e�e�[�u���ɂ����邷�ׂ�R�}�e�[�u����`�悷��
//		�������e�[�u����������̏ꍇ�͕`������{���Ȃ�
// [prm]drawPos	: 1PS, 2PS, 3PS
// [ret]�`�搔�A�������G���[���Ɛ��䖢���莞�͕`��ʒu���V�t�g����ړI��1��Ԃ�
int CSlotControlManager::DrawSlipTable(int x, int y, int pFlagID, bool isDrawSelf, std::vector<int> pDrawPos, SSlotGameDataWrapper& pData) {
	const int color = (pFlagID == posData.currentFlagID) ? 0xFFFF00 : 0x808080;
	const std::string flagName  = pData.randManager.GetFlagName (pFlagID);
	const std::string bonusName = pData.randManager.GetBonusName(pFlagID);
	int drawCount = 0;

	DxLib::DrawString(x - 1, y - BOX_H*2 + 14, flagName .c_str(), 0xFFFF00);
	DxLib::DrawString(x - 1, y - BOX_H   +  4, bonusName.c_str(), 0xFFFF00);

	int highLightPos = posData.currentOrder;
	if (Get2ndStyle() == 0x3 && posData.selectReel == 2) highLightPos = 2;

	if (Get2ndStyle(0, pFlagID) == 0x0 && posData.currentOrder == 1) {
		// PushS 2nd���͑S�����ʒu���X�L�������āA��~�ʒu�Ɠ����Ȃ炷�ׂĕ`�悷��
		const auto ss1st = GetSS(pFlagID, true, posData.stop1st, pDrawPos[posData.stop1st * 2 + 1], posData.isWatchLeft);
		if (ss1st == nullptr)			return 1;
		if (*ss1st == 0 && !isDrawSelf)	return 1;
		const int pos1stLoopMax = isDrawSelf ? 1 : m_comaMax;

		for (int pos1st = 0; pos1st < pos1stLoopMax; ++pos1st) {
			const int checkPos = (pos1st + pDrawPos[posData.stop1st * 2]) % m_comaMax;
			if (GetPosFromSlipT(*ss1st, checkPos) != pDrawPos[posData.stop1st * 2 + 1]) continue;

			const auto ss = GetSS(pFlagID, posData.currentOrder == 0, posData.stop1st, checkPos, posData.isWatchLeft);
			if (ss == nullptr)				return 1;
			if (*ss == 0 && !isDrawSelf)	continue;
			DrawComaBox(x-3, y-6, tableSlip[*ss].activePos, posData.cursorComa[highLightPos]);

			// ��~�ʒu����v�����f�[�^�̕`��
			for (int i = 0; i < m_comaMax; ++i) {
				const int posY = y + 26 * (m_comaMax - i - 1);
				int stopPos = GetPosFromSlipT(*ss, i);
				int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
				DxLib::DrawFormatString(x, posY, color, "%d", showVal);
			}
			++drawCount;
			x += BOX_W;
		}
	} else if (isSlip(pFlagID, posData.currentOrder, posData.stop1st, pDrawPos[posData.stop1st * 2 + 1])) {
		// PushS 1st�� �܂��� StopS 2nd��
		const auto ss = GetSS(pFlagID, posData.currentOrder == 0, posData.stop1st,
			pDrawPos[posData.stop1st * 2 + 1], posData.isWatchLeft);
		if (ss == nullptr)				return 1;
		if (*ss == 0 && !isDrawSelf)	return 1;
		DrawComaBox(x-3, y-6, tableSlip[*ss].activePos, posData.cursorComa[highLightPos]);
		for (int i = 0; i < m_comaMax; ++i) {
			const int posY = y + 26 * (m_comaMax - i - 1);
			int stopPos = GetPosFromSlipT(*ss, i);
			int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
			DxLib::DrawFormatString(x, posY, color, "%d", showVal);
		}
		++drawCount;
		x += BOX_W;
	} else {
		// ComSA��currentOrder=2�̎��͕\���f�[�^�𔽓]������
		const bool watchLeft = (Get2ndStyle(pDrawPos[posData.stop1st * 2 + 1], pFlagID)==0x3 && posData.currentOrder == 2) ?
			!posData.isWatchLeft : posData.isWatchLeft;

		SControlDataSA* sa = GetSA(pFlagID, posData.currentOrder, posData.stop1st,
			pDrawPos[posData.stop1st * 2 + 1], pDrawPos[Get2ndReel(watchLeft) * 2 + 1]);
		if (sa == nullptr)										return 1;	// �f�[�^�Ȃ���
		const int checkSA = watchLeft ? 0 : AVAIL_ID_MAX;
		if (sa->data[checkSA].availableID == 0 && !isDrawSelf)	return 1;	// �f�[�^�����͎�

		const unsigned int stopFlag = m_isSuspend ? 0 : GetActiveFromAvailT(*sa, watchLeft);
		DrawComaBox(x-3, y-6, stopFlag, posData.cursorComa[highLightPos]);
		for (int i = 0; i < m_comaMax; ++i) {
			const int posY = y + BOX_H * (m_comaMax - i - 1);
			if (!m_isSuspend) {
				int stopPos = GetPosFromAvailT(*sa, i, watchLeft);
				int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
				DxLib::DrawFormatString(x, posY, color, "%d", showVal);
			} else {
				DxLib::DrawString(x, posY, "X", 0xFF0000);
			}
		}
		++drawCount;
		x += BOX_W;
	}

	return drawCount <= 0 ? 1 : drawCount;
}

bool CSlotControlManager::DrawStopTable(int x, int y, int pFlagID) {
	const int color = (pFlagID == posData.currentFlagID) ? 0xFFFF00 : 0x808080;
	SControlDataSA* sa = GetSA(pFlagID);
	if(sa == nullptr) return false;

	int highLightPos = posData.currentOrder;
	if (Get2ndStyle() == 0x3 && posData.selectReel == 2) highLightPos = 2;

	for (int j = 0; j < AVAIL_ID_MAX; ++j) {
		const int posX = x + BOX_W * j;
		unsigned long long stopFlag = 0;
		/* ��~�ʒu�Ăяo�� */ {
			const int index = j + (posData.isWatchLeft ? 0 : AVAIL_ID_MAX);
			const auto nowTableID = sa->data[index].availableID;
			const int boxColor = (j == posData.selectAvailID) ? 0x8080FF : 0x404040;
			stopFlag = GetAvailShiftData(*sa, j, posData.isWatchLeft);
			DrawComaBox(posX-3, y-6, stopFlag & 0xFFFFFFFF, posData.cursorComa[highLightPos], boxColor);
		}
		for (int pos = 0; pos < m_comaMax; ++pos) {
			const int posY = y + BOX_H * (m_comaMax - pos - 1);
			const std::string drawStr = (stopFlag & 0x1) ? "@" : "-";
			DxLib::DrawFormatString(posX, posY, color, "%s", drawStr.c_str());
			stopFlag >>= 1;
		}
	}
	return true;
}

// 20220411�ǉ�
// [act]�ő�̒�~�ʒu���x���Ɠ��܃t���O�ꗗ��\������
void CSlotControlManager::DrawStopStatus(SSlotGameDataWrapper& pData) {
	int maxStopLevel = 0;				// �ő�̃��[�`�ڃ��x��
	std::vector<int> maxPos(m_reelMax), maxTemp(m_reelMax);
	std::set<std::string> flagList;	// ���܂�����t���O�ꗗ
	std::string pullFail = "";		// ��������NG�ӏ�(""�ň�������)

	for (int push1st = 0; push1st < m_comaMax; ++push1st) {
		const int stop1st = GetPosFromSlipT(*GetSS(posData.currentFlagID, true, posData.stop1st, push1st, true), push1st);
		// 2nd�͍����A�E����2�񂸂��[�v������
		for (int push2ndCNT = 0; push2ndCNT < m_comaMax * 2; ++push2ndCNT) {
			bool watchLeft = push2ndCNT >= m_comaMax;
			const int push2nd = push2ndCNT % m_comaMax;
			int stop2nd = -1;

			const auto style = Get2ndStyle(stop1st, posData.stop1st, posData.currentFlagID);
			// SlipT: �n��push1st
			if (style == 0x0) stop2nd = GetPosFromSlipT(*GetSS(posData.currentFlagID, false, posData.stop1st, push1st, watchLeft), push2nd);
			// SlipT: �n��stop1st
			else if (isSlip(posData.currentFlagID, 1, posData.stop1st, stop1st)) stop2nd = GetPosFromSlipT(*GetSS(posData.currentFlagID, false, posData.stop1st, stop1st, watchLeft), push2nd);
			// AvailT: �n��stop1st
			else stop2nd = GetPosFromAvailT(*GetSA(posData.currentFlagID, 1, posData.stop1st, stop1st, push2nd), push2nd, watchLeft);

			// �G���[����(�b��)
			if (stop2nd == -1) return;

			for (int push3rd = 0; push3rd < m_comaMax; ++push3rd) {
				int stop3rd = -1;
				// ComSA��p
				if (style == 0x3) stop3rd = GetPosFromAvailT(*GetSA(posData.currentFlagID, 1, posData.stop1st, stop1st, stop2nd), push3rd, !watchLeft);
				// �ėp
				else stop3rd = GetPosFromAvailT(*GetSA(posData.currentFlagID, 2, posData.stop1st, stop1st, stop2nd), push3rd, watchLeft);

				// ����p�f�[�^�쐬
				std::vector<int> checkPos(m_reelMax);
				checkPos[posData.stop1st] = stop1st;
				checkPos[Get2ndReel(watchLeft, posData.stop1st)] = stop2nd % m_comaMax;
				checkPos[Get2ndReel(!watchLeft, posData.stop1st)] = stop3rd;
				maxTemp = checkPos;
				// �ʒu�␳
				for (size_t i = 0; i < checkPos.size(); ++i)
					checkPos[i] = (m_comaMax * 2 - checkPos[i] - 3) % m_comaMax;

				// �����o���f�[�^�擾
				const auto stopData = pData.reelChecker.GetPosData(posData.stop1st, checkPos);
				if (stopData.reachLevel > maxStopLevel) {
					maxStopLevel = stopData.reachLevel;
					maxPos = maxTemp;
				}
				flagList.insert(stopData.spotFlag);
				// 3rd�������݃`�F�b�N
				if (pullFail=="") pullFail = CheckPull(pData, posData.stop1st, watchLeft, push3rd, stop3rd, stop1st, stop2nd);
				
			}
			// 2nd�������݃`�F�b�N
			if (pullFail == "") pullFail = CheckPull(pData, posData.stop1st, watchLeft, push2nd, stop2nd, stop1st);
		}
		// 1st�������݃`�F�b�N
		if (pullFail == "") pullFail = CheckPull(pData, posData.stop1st, true, push1st, stop1st);
	}

	// �`��(+1����͉̂�ʏ�̕`�悪+1������)
	DxLib::DrawFormatString(1010, 100, 0xA0A0FF, "maxReachLv.: %d (%02d-%02d-%02d)", maxStopLevel, maxPos[0] + 1, maxPos[1] + 1, maxPos[2] + 1);
	int drawC = 0;
	for (auto& val : flagList) {
		DxLib::DrawString(1010 + 30 * drawC, 115, (val == "" ? "-" : val.c_str()), 0xA0A0FF);
		++drawC;
	}

	if(m_checkLaunchFlag)
		DxLib::DrawString(1010 + 30 * drawC, 115, (pullFail == "" ? "OK!" : pullFail.c_str()), 0xA0A0FF);

	return;
}

// [act]�������݃`�F�b�N�����{����
// [ret]"":����
//		����ȊO�F�G���[�����ʒu
std::string CSlotControlManager::CheckPull(SSlotGameDataWrapper& pData, int order1st, bool watchLeft, int pushPos, int stopPos, int stop1st, int stop2nd) {
	// �����X�L�b�v�H
	if (!m_checkLaunchFlag) return "";

	// ���،��ʊi�[�f�[�^
	std::vector<std::set<std::string>> flagList(5);

	// ���ׂ�R�}���m�F
	int slipCount = stopPos - pushPos;
	while (slipCount < 0) slipCount += m_comaMax;

	// 1st,2nd�̏ꍇ�͉�]�����[����S�擾����(�ȉ��Ń��[�v���ƊJ�n�ʒu��ݒ肷��)
	int loopMax[] = { 1, 1, 5 };
	int startPos[] = { stop1st, stop2nd, pushPos };
	int targetOrder = m_reelMax - 1;
	if (stop1st == -1) {
		targetOrder = 0;
		loopMax[0] = 5; loopMax[1] = m_comaMax; loopMax[2] = m_comaMax;
		startPos[0] = pushPos; startPos[1] = 0; startPos[2] = 0;
	} else if (stop2nd == -1) {
		targetOrder = 1;
		loopMax[1] = 5; loopMax[2] = m_comaMax;
		startPos[1] = pushPos; startPos[2] = 0;
	}

	// ���[�v�J�E���^��`
	std::vector<int> lp(m_reelMax);
	// ���ׂ�R�}�����������ܖ����擾����
	for (lp[0] = 0; lp[0] < loopMax[0]; ++lp[0])
	for (lp[1] = 0; lp[1] < loopMax[1]; ++lp[1])
	for (lp[2] = 0; lp[2] < loopMax[2]; ++lp[2]) {
		std::vector<int> checkPos(m_reelMax);
		checkPos[order1st] = startPos[0] + lp[0];
		checkPos[Get2ndReel(watchLeft, order1st)] = startPos[1] + lp[1];
		checkPos[Get2ndReel(!watchLeft, order1st)] = startPos[2] + lp[2];
		// �ʒu�␳
		for (size_t i = 0; i < checkPos.size(); ++i)
			checkPos[i] = (m_comaMax * 2 - checkPos[i] - 3) % m_comaMax;
		// �����o���f�[�^�擾
		const auto stopData = pData.reelChecker.GetPosData(order1st, checkPos);
		flagList[lp[targetOrder]].insert(stopData.spotFlag);
	}

	std::vector<unsigned char> prior(flagList.size(), 0);
	for (size_t i = 0; i < flagList.size(); ++i) {
		const auto& setList = flagList[i];
		for (const auto& val : setList) {
			const int level = pData.randManager.GetFlagPriority(posData.currentFlagID, val, loopMax[2] == 5);
			if (level < 0) continue;
			if (pData.randManager.GetonlyCheckFirst(posData.currentFlagID)) {
				prior[i] = (std::max)((int)prior[i], 1 << (7 - level));
			} else {
				prior[i] |= (1 << (7 - level));
			}
		}
	}

	// OK����
	if (prior[slipCount] == *std::max_element(prior.begin(), prior.end())) return "";
	
	// NG������
	std::string ans("---L");
	ans[order1st]							= loopMax[0] >= m_comaMax ? '-' : '0' + startPos[0];
	ans[Get2ndReel(watchLeft, order1st)]	= loopMax[1] >= m_comaMax ? '-' : '0' + startPos[1];
	ans[Get2ndReel(!watchLeft, order1st)]	= loopMax[2] >= m_comaMax ? '-' : '0' + startPos[2];
	for (int i = 0; i < 3; ++i) ans[i] = ans[i] > '9' ? (ans[i] + 7) : ans[i];
	ans[3] = watchLeft ? 'L' : 'R';
	return ans;
}

bool CSlotControlManager::ReadRestore(CRestoreManagerRead& pReader) {
	size_t sizeTemp = 0;
	size_t defSize[4] = { 0,0,0,0 };
	size_t loopMax[4] = { 0,0,0,0 };
	bool   canCountRef[4] = { true, true, true, true };	// ���[�v�J�E���g��defSize�ȏ�ƂȂ�ꍇ�j�������̂�refNum�ɒǉ����Ȃ��悤�ɂ��邽�߂̕ϐ�

	// tableSlip
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > tableSlip.size()) { defSize[0] = tableSlip.size(); tableSlip.resize(sizeTemp, tableSlip[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		if (!pReader.ReadNum(tableSlip[i].data     )) return false;
		if (!pReader.ReadNum(tableSlip[i].activePos)) return false;
		// Version:3�ڍs��ctrlData�ǂݍ��ݎ��Ɍv�Z���� �� �ǂݍ��ނ����ǂݍ����0������
		if (pReader.GetReadVer()<=2)
			if (!pReader.ReadNum(tableSlip[i].refNum   )) return false;
		tableSlip[i].refNum = 0;
	}
	if (defSize[0] > 0) { tableSlip.resize(defSize[0]); defSize[0] = 0; }
	
	// tableAvailable(activePos�͎g�p���Ȃ�)
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > tableAvailable.size()) { defSize[0] = tableAvailable.size(); tableAvailable.resize(sizeTemp, tableAvailable[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		if (!pReader.ReadNum(tableAvailable[i].data     )) return false;
		// Version:3�ڍs��ctrlData�ǂݍ��ݎ��Ɍv�Z���� �� �ǂݍ��ނ����ǂݍ����0������
		if (pReader.GetReadVer()<=2)
			if (!pReader.ReadNum(tableAvailable[i].refNum   )) return false;
		tableAvailable[i].refNum = 0;
	}
	if (defSize[0] > 0) { tableAvailable.resize(defSize[0]); defSize[0] = 0; }

	// ctrlData
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > ctrlData.size()) { defSize[0] = ctrlData.size(); ctrlData.resize(sizeTemp, ctrlData[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		// Version<=3�� ctrlData[i].controlStyle�X�V�p
		unsigned char cont1stRef = 0;

		canCountRef[0] = (defSize[0] == 0 || i < defSize[0]);
		if (!pReader.ReadNum(ctrlData[i].dataID      )) return false;
		if (!pReader.ReadNum(ctrlData[i].controlStyle)) return false;

		if (!pReader.ReadNum(sizeTemp)) return false; loopMax[1] = sizeTemp;
		if (sizeTemp > ctrlData[i].controlData.size()) { defSize[1] = ctrlData[i].controlData.size(); ctrlData[i].controlData.resize(sizeTemp, ctrlData[i].controlData[0]); }
		for (size_t dataC = 0; dataC < loopMax[1]; ++dataC) {
			canCountRef[1] = canCountRef[0] && (defSize[1] == 0 || dataC < defSize[1]);
			auto& nowData = ctrlData[i].controlData[dataC];
			if (!pReader.ReadNum(nowData.controlData1st)) return false;
			if (canCountRef[1]) tableSlip[nowData.controlData1st].refNum++;	// refNum�g�p���ъm�F
			if (!pReader.ReadNum(nowData.controlData2nd.activeFlag)) return false;

			// controlStyle2nd�̍X�V
			if (pReader.GetReadVer() <= 3) {
				const unsigned char styleVal = (ctrlData[i].controlStyle >> 2 * dataC) & 0x3;
				if (styleVal > 0) {
					for (size_t styleC = 0; styleC < nowData.controlData2nd.controlStyle2nd.size(); ++styleC) {
						nowData.controlData2nd.controlStyle2nd[styleC] = styleVal;
					}
					cont1stRef |= (0x1 << dataC);
				}
				if (dataC + 1 >= loopMax[1]) ctrlData[i].controlStyle = cont1stRef;
			} else {
				if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
				if (sizeTemp > nowData.controlData2nd.controlStyle2nd.size()) {
					defSize[2] = nowData.controlData2nd.controlStyle2nd.size();
					nowData.controlData2nd.controlStyle2nd.resize(sizeTemp, nowData.controlData2nd.controlStyle2nd[0]);
				}
				for (size_t j = 0; j < loopMax[2]; ++j) {
					canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
					auto& style = nowData.controlData2nd.controlStyle2nd[j];
					if (!pReader.ReadNum(style)) return false;
				}
				if (defSize[2] > 0) { nowData.controlData2nd.controlStyle2nd.resize(defSize[2]); defSize[2] = 0; }
			}

			// PS
			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData2nd.controlData2ndPS.size()) {
				defSize[2] = nowData.controlData2nd.controlData2ndPS.size();
				nowData.controlData2nd.controlData2ndPS.resize(sizeTemp, nowData.controlData2nd.controlData2ndPS[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
				auto& ps = nowData.controlData2nd.controlData2ndPS[j];
				if (!pReader.ReadNum(ps)) return false;
				if (canCountRef[2]) tableSlip[ps].refNum++;		// refNum�g�p���ъm�F
			}
			if (defSize[2] > 0) { nowData.controlData2nd.controlData2ndPS.resize(defSize[2]); defSize[2] = 0; }

			// SS
			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData2nd.controlData2ndSS.size()) {
				defSize[2] = nowData.controlData2nd.controlData2ndSS.size();
				nowData.controlData2nd.controlData2ndSS.resize(sizeTemp, nowData.controlData2nd.controlData2ndSS[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
				auto& ss = nowData.controlData2nd.controlData2ndSS[j];
				if (!pReader.ReadNum(ss)) return false;
				if (canCountRef[2]) tableSlip[ss].refNum++;		// refNum�g�p���ъm�F
			}
			if (defSize[2] > 0) { nowData.controlData2nd.controlData2ndSS.resize(defSize[2]); defSize[2] = 0; }

			// 2ndSA
			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData2nd.controlData2ndSA.size()) {
				defSize[2] = nowData.controlData2nd.controlData2ndSA.size();
				nowData.controlData2nd.controlData2ndSA.resize(sizeTemp, nowData.controlData2nd.controlData2ndSA[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
				auto& sa = nowData.controlData2nd.controlData2ndSA[j];
				if (!pReader.ReadNum(sizeTemp)) return false; loopMax[3] = sizeTemp;
				if (sizeTemp > sa.data.size()) { defSize[3] = sa.data.size(); sa.data.resize(sizeTemp, sa.data[0]); }
				for (size_t k = 0; k < loopMax[3]; ++k) {
					canCountRef[3] = canCountRef[2] && (defSize[3] == 0 || k < defSize[3]);
					if (!pReader.ReadNum(sa.data[k].tableFlag  )) return false;
					if (!pReader.ReadNum(sa.data[k].availableID)) return false;
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum�g�p���ъm�F
				}
				if (defSize[3] > 0) { sa.data.resize(defSize[3]); defSize[3] = 0; }
			}
			if (defSize[2] > 0) { nowData.controlData2nd.controlData2ndSA.resize(defSize[2]); defSize[2] = 0; }

			// ComSA
			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData2nd.controlDataComSA.size()) {
				defSize[2] = nowData.controlData2nd.controlDataComSA.size();
				nowData.controlData2nd.controlDataComSA.resize(sizeTemp, nowData.controlData2nd.controlDataComSA[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
				auto& sa = nowData.controlData2nd.controlDataComSA[j];
				if (!pReader.ReadNum(sizeTemp)) return false; loopMax[3] = sizeTemp;
				if (sizeTemp > sa.data.size()) { defSize[3] = sa.data.size(); sa.data.resize(sizeTemp, sa.data[0]); }
				for (size_t k = 0; k < loopMax[3]; ++k) {
					canCountRef[3] = canCountRef[2] && (defSize[3] == 0 || k < defSize[3]);
					if (!pReader.ReadNum(sa.data[k].tableFlag  )) return false;
					if (!pReader.ReadNum(sa.data[k].availableID)) return false;
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum�g�p���ъm�F
				}
				if (defSize[3] > 0) { sa.data.resize(defSize[3]); defSize[3] = 0; }
			}
			if (defSize[2] > 0) { nowData.controlData2nd.controlDataComSA.resize(defSize[2]); defSize[2] = 0; }

			// 3rd
			if (!pReader.ReadNum(nowData.controlData3rd.activeFlag1st)) return false;
			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData3rd.activeFlag2nd.size()) {
				defSize[2] = nowData.controlData3rd.activeFlag2nd.size();
				nowData.controlData3rd.activeFlag2nd.resize(sizeTemp, nowData.controlData3rd.activeFlag2nd[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				if (!pReader.ReadNum(nowData.controlData3rd.activeFlag2nd[j])) return false;
			}
			if (defSize[2] > 0) { nowData.controlData3rd.activeFlag2nd.resize(defSize[2]); defSize[2] = 0; }

			if (!pReader.ReadNum(sizeTemp)) return false; loopMax[2] = sizeTemp;
			if (sizeTemp > nowData.controlData3rd.availableData.size()) {
				defSize[2] = nowData.controlData3rd.availableData.size();
				nowData.controlData3rd.availableData.resize(sizeTemp, nowData.controlData3rd.availableData[0]);
			}
			for (size_t j = 0; j < loopMax[2]; ++j) {
				canCountRef[2] = canCountRef[1] && (defSize[2] == 0 || j < defSize[2]);
				auto& sa = nowData.controlData3rd.availableData[j];
				if (!pReader.ReadNum(sizeTemp)) return false; loopMax[3] = sizeTemp;
				if (sizeTemp > sa.data.size()) { defSize[3] = sa.data.size(); sa.data.resize(sizeTemp, sa.data[0]); }
				for (size_t k = 0; k < loopMax[3]; ++k) {
					canCountRef[3] = canCountRef[2] && (defSize[3] == 0 || k < defSize[3]);
					if (!pReader.ReadNum(sa.data[k].tableFlag)) return false;
					if (!pReader.ReadNum(sa.data[k].availableID)) return false;
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum�g�p���ъm�F
				}
				if (defSize[3] > 0) { sa.data.resize(defSize[3]); defSize[3] = 0; }
			}
			if (defSize[2] > 0) { nowData.controlData3rd.availableData.resize(defSize[2]); defSize[2] = 0; }
		}
		if (defSize[1] > 0) { ctrlData[i].controlData.resize(defSize[1]); defSize[1] = 0; }
	}
	if (defSize[0] > 0) { ctrlData.resize(defSize[0]); defSize[0] = 0; }

	// refPos[0](No Data)�̎Q�Ɛ��t���Ȃ���
	tableSlip[0].refNum = INT_MAX;	tableAvailable[0].refNum = INT_MAX;
	for (size_t i = 1; i < tableSlip.size(); ++i) tableSlip[0].refNum -= tableSlip[i].refNum;
	for (size_t i = 1; i < tableAvailable.size(); ++i) tableAvailable[0].refNum -= tableAvailable[i].refNum;

	// posData
	int currentFlagIDMemo;
	if (!pReader.ReadNum(currentFlagIDMemo)) return false;
	if (!pReader.ReadNum(posData.selectReel)) return false;
	if (!pReader.ReadNum(posData.currentOrder)) return false;
	if (!pReader.ReadNum(posData.stop1st)) return false;
	if (!pReader.ReadNum(posData.selectAvailID)) return false;
	if (!pReader.ReadNum(posData.isWatchLeft)) return false;

	if (!pReader.ReadNum(sizeTemp)) return false;
	if (sizeTemp > posData.cursorComa.size()) { defSize[0] = posData.cursorComa.size(); posData.cursorComa.resize(sizeTemp, posData.cursorComa[0]); }
	for (size_t i = 0; i < posData.cursorComa.size(); ++i) {
		if (!pReader.ReadNum(posData.cursorComa[i])) return false;
	}
	if (defSize[0] > 0) { posData.cursorComa.resize(defSize[0]); defSize[0] = 0; }

	// �e�t���O�ɑ΂���Active�t���O������
	for (posData.currentFlagID = 0; posData.currentFlagID < m_flagMax; ++posData.currentFlagID)
		UpdateActiveFlag();
	posData.currentFlagID = currentFlagIDMemo;

	AdjustPos();
	return true;
}

bool CSlotControlManager::WriteRestore(CRestoreManagerWrite& pWriter) const {
	// tableSlip
	if (!pWriter.WriteNum(tableSlip.size())) return false;
	for (size_t i = 0; i < tableSlip.size(); ++i) {
		if (!pWriter.WriteNum(tableSlip[i].data     )) return false;
		if (!pWriter.WriteNum(tableSlip[i].activePos)) return false;
		// Version=2�őł��؂�(�ǂݍ��ݎ��Ɍv�Z)
		//if (!pWriter.WriteNum(tableSlip[i].refNum   )) return false;
	}

	// tableAvailable(activePos�͎g�p���Ȃ�)
	if (!pWriter.WriteNum(tableAvailable.size())) return false;
	for (size_t i = 0; i < tableAvailable.size(); ++i) {
		if (!pWriter.WriteNum(tableAvailable[i].data  )) return false;
		// Version=2�őł��؂�(�ǂݍ��ݎ��Ɍv�Z)
		//if (!pWriter.WriteNum(tableAvailable[i].refNum)) return false;
	}

	// ctrlData
	if (!pWriter.WriteNum(ctrlData.size())) return false;
	for (size_t i = 0; i < ctrlData.size(); ++i) {
		if (!pWriter.WriteNum(ctrlData[i].dataID      )) return false;
		if (!pWriter.WriteNum(ctrlData[i].controlStyle)) return false;

		if (!pWriter.WriteNum(ctrlData[i].controlData.size())) return false;
		for (size_t dataC = 0; dataC < ctrlData[i].controlData.size(); ++dataC) {
			const auto& nowData = ctrlData[i].controlData[dataC];
			if (!pWriter.WriteNum(nowData.controlData1st)) return false;
			if (!pWriter.WriteNum(nowData.controlData2nd.activeFlag)) return false;

			// controlStyle2nd
			if (!pWriter.WriteNum(nowData.controlData2nd.controlStyle2nd.size())) return false;
			for (size_t j = 0; j < nowData.controlData2nd.controlStyle2nd.size(); ++j) {
				const auto& style = nowData.controlData2nd.controlStyle2nd[j];
				if (!pWriter.WriteNum(style)) return false;
			}

			// PS
			if (!pWriter.WriteNum(nowData.controlData2nd.controlData2ndPS.size())) return false;
			for (size_t j = 0; j < nowData.controlData2nd.controlData2ndPS.size(); ++j) {
				const auto& ps = nowData.controlData2nd.controlData2ndPS[j];
				if (!pWriter.WriteNum(ps)) return false;
			}

			// SS
			if (!pWriter.WriteNum(nowData.controlData2nd.controlData2ndSS.size())) return false;
			for (size_t j = 0; j < nowData.controlData2nd.controlData2ndSS.size(); ++j) {
				const auto& ss = nowData.controlData2nd.controlData2ndSS[j];
				if (!pWriter.WriteNum(ss)) return false;
			}

			// 2ndSA
			if (!pWriter.WriteNum(nowData.controlData2nd.controlData2ndSA.size())) return false;
			for (size_t j = 0; j < nowData.controlData2nd.controlData2ndSA.size(); ++j) {
				const auto& sa = nowData.controlData2nd.controlData2ndSA[j];
				if (!pWriter.WriteNum(sa.data.size())) return false;
				for (size_t k = 0; k < sa.data.size(); ++k) {
					if (!pWriter.WriteNum(sa.data[k].tableFlag  )) return false;
					if (!pWriter.WriteNum(sa.data[k].availableID)) return false;
				}
			}

			// ComSA
			if (!pWriter.WriteNum(nowData.controlData2nd.controlDataComSA.size())) return false;
			for (size_t j = 0; j < nowData.controlData2nd.controlDataComSA.size(); ++j) {
				const auto& sa = nowData.controlData2nd.controlDataComSA[j];
				if (!pWriter.WriteNum(sa.data.size())) return false;
				for (size_t k = 0; k < sa.data.size(); ++k) {
					if (!pWriter.WriteNum(sa.data[k].tableFlag  )) return false;
					if (!pWriter.WriteNum(sa.data[k].availableID)) return false;
				}
			}

			// 3rd
			if (!pWriter.WriteNum(nowData.controlData3rd.activeFlag1st)) return false;
			if (!pWriter.WriteNum(nowData.controlData3rd.activeFlag2nd.size())) return false;
			for (size_t j = 0; j < nowData.controlData3rd.activeFlag2nd.size(); ++j) {
				if (!pWriter.WriteNum(nowData.controlData3rd.activeFlag2nd[j])) return false;
			}
			if (!pWriter.WriteNum(nowData.controlData3rd.availableData.size())) return false;
			for (size_t j = 0; j < nowData.controlData3rd.availableData.size(); ++j) {
				const auto& sa = nowData.controlData3rd.availableData[j];
				if (!pWriter.WriteNum(sa.data.size())) return false;
				for (size_t k = 0; k < sa.data.size(); ++k) {
					if (!pWriter.WriteNum(sa.data[k].tableFlag  )) return false;
					if (!pWriter.WriteNum(sa.data[k].availableID)) return false;
				}
			}
		}
	}

	// posData
	if (!pWriter.WriteNum(posData.currentFlagID)) return false;
	if (!pWriter.WriteNum(posData.selectReel   )) return false;
	if (!pWriter.WriteNum(posData.currentOrder )) return false;
	if (!pWriter.WriteNum(posData.stop1st      )) return false;
	if (!pWriter.WriteNum(posData.selectAvailID)) return false;
	if (!pWriter.WriteNum(posData.isWatchLeft  )) return false;

	if (!pWriter.WriteNum(posData.cursorComa.size())) return false;
	for (size_t i = 0; i < posData.cursorComa.size(); ++i) {
		if (!pWriter.WriteNum(posData.cursorComa[i])) return false;
	}

	return true;
}

bool CSlotControlManager::RefreshFlag() const {
	return m_refreshFlag && !m_isSuspend;
}