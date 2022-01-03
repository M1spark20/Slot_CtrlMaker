#include "_header/CSlotControlManager.hpp"
#include "_header/SSlotGameDataWrapper.hpp"
#include "_header/CRestoreManager.hpp"
#include "_header/keyexport.h"
#include <DxLib.h>

bool CSlotControlManager::Init(const SSlotGameDataWrapper& pData) {
	// データ型初期化(ロード処理は後で)
	m_flagMax = pData.randManager.GetFlagNum();
	m_reelMax = pData.reelManager.GetReelNum();
	m_comaMax = pData.reelManager.GetCharaNum();
	m_allStopFlag = (unsigned long long)pow(2, pData.reelManager.GetCharaNum()) - 1ull;
	m_isSuspend = false;
	m_refreshFlag = false;

	/* slipT初期化 */ {
		const unsigned long long allStopFlag = m_allStopFlag;
		SControlTable st; st.data = 0; st.refNum = 0; st.activePos = allStopFlag & 0xFFFFFFFF;
		tableSlip.resize(SLIP_TABLE_MAX, st);
		tableAvailable.resize(AVAIL_TABLE_MAX, st);
		tableSlip[0].refNum = INT_MAX;
		tableAvailable[0].data = allStopFlag;
		tableAvailable[0].refNum = INT_MAX;
	}
	/* makeData初期化 */ {
		const int flagMax = m_flagMax;
		const int reelMax = m_reelMax;
		const size_t comaMax = m_comaMax;
		SControlMakeData sm;
		sm.controlStyle = 0x0;
		/* wrapper 初期化 */ {
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
		for (size_t i = 0; i < ctrlData.size(); ++i) ctrlData[i].dataID = i;	// 配列要素番号と一致させる
	}

	// 各フラグに対してActiveフラグ初期化
	for(posData.currentFlagID = 0; posData.currentFlagID < m_flagMax; ++posData.currentFlagID)
		UpdateActiveFlag();

	/* posData初期化 */ {
		posData.currentFlagID = 0;
		posData.selectReel = 0;
		posData.stop1st = 0;
		posData.cursorComa.resize(m_reelMax, 0);
		posData.selectAvailID = 0;
		posData.isWatchLeft = true;
	}

	return true;
}

bool CSlotControlManager::Process() {
	m_refreshFlag = false;
	/* 画面操作 */ {
		CKeyExport_S& key = CKeyExport_S::GetInstance();
		const bool shiftFlag = key.ExportKeyStateFrame(KEY_INPUT_LSHIFT) >= 1 || key.ExportKeyStateFrame(KEY_INPUT_RSHIFT) >= 1;
		// 位置操作系
		if		(!shiftFlag && key.ExportKeyState(KEY_INPUT_UP))	SetComaPos(posData.selectReel, false, true);
		else if (!shiftFlag && key.ExportKeyState(KEY_INPUT_DOWN))	SetComaPos(posData.selectReel, false, false);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_LEFT))	--posData.selectAvailID;
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_RIGHT))	++posData.selectAvailID;
		// 以下の位置操作はSAで無効データがある場合適用不可
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

		// 制御編集系
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


		// 以下の制御編集はSAで無効データがある場合適用不可
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_1)) SetAvailCtrlPattern(0);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_2)) SetAvailCtrlPattern(1);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_3)) SetAvailCtrlPattern(2);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_4)) SetAvailCtrlPattern(3);

		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_Q)) ActionTableID(true);
		else if (!m_isSuspend && shiftFlag && key.ExportKeyState(KEY_INPUT_A)) ActionTableID(false);
		AdjustPos();
	}
	return true;
}

void CSlotControlManager::AdjustPos() {
	// 各種上限値定義
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
	// 共通制御時2nd/3rdはcurrentOrder = 1で頭打ち
	if (Get2ndStyle() == 0x03) orderLim -= 1;
	while (posData.currentOrder < 0) posData.currentOrder = 0;
	while (posData.currentOrder >= orderLim) posData.currentOrder = orderLim - 1;

	int selectLim = posData.currentOrder + 1;
	// 共通制御時2nd/3rdは全リール操作可能とし、isWatchLeftを可変させる
	if (posData.currentOrder >= 1 && Get2ndStyle() == 0x03) {
		selectLim = m_reelMax;	posData.isWatchLeft = !(posData.selectReel == 2);
	}
	while (posData.selectReel < 0) posData.selectReel = 0;
	while (posData.selectReel >= selectLim) posData.selectReel = selectLim - 1;

	if (isSilp()) posData.selectAvailID = 0;
	while (posData.selectAvailID < 0) posData.selectAvailID += AVAIL_ID_MAX;
	while (posData.selectAvailID >= AVAIL_ID_MAX) posData.selectAvailID -= AVAIL_ID_MAX;

	// 位置自動調整
	SetComaPos(posData.selectReel, true, true);
}

bool CSlotControlManager::canChangeTable() {
	// 選択中リールと押し位置が一致しない場合データ更新しない
	// ただしComSAのsel=2, order=1を除く
	if (posData.selectReel != posData.currentOrder && !(Get2ndStyle() == 0x03 && posData.selectReel == 2 && posData.currentOrder == 1))
		return false;
	return true;
}

// [act]停止し得る場所へcomaPosを自動調整する
// [prm]pMoveOrder	: 今回変更するリールの押し順
// [ret]正しく位置を設定できたか(無効制御有の場合false)
bool CSlotControlManager::SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp) {
	for (int i = pIsReset ? 0 : 1; i < m_comaMax; ++i) {
		int diffPos = posData.cursorComa[pMoveOrder];
		diffPos += pIsUp ? i : -i;
		while (diffPos < 0)				diffPos += m_comaMax;
		while (diffPos >= m_comaMax)	diffPos -= m_comaMax;

		// 停止可否判定
		if(!GetCanStop(pMoveOrder, diffPos)) continue;
		// 場所確定
		posData.cursorComa[pMoveOrder] = diffPos; return true;
	}
	return false;
}

// [act]現在の制御条件にてフラグ: pFlagIDの
// [prm]pMoveOrder	: 今回変更するリールの押し順
bool CSlotControlManager::GetCanStop(const int pMoveOrder, const int pLookFor, const int pFlagID) {
	const int flagID = pFlagID < 0 ? posData.currentFlagID : pFlagID;
	if (flagID >= m_flagMax) return false;
	auto& nowMakeData = ctrlData[flagID];
	auto& srcData = nowMakeData.controlData[posData.stop1st];

	// 使用可能なリール位置でreturn true;(関数最下部)する仕組み
	if (posData.currentOrder == 0) {
		// 1st変更中：制御をかけない(対象が1stであるか確認する)
		if (pMoveOrder != 0) return false;
	} else if (posData.currentOrder == 1) {
		// 2nd変更中：制御方式PS以外 && 参照1st であれば制御をかける
		// ※制御方式ComSAにて3rdもこのループを参照、制御はかからない
		if(Get2ndStyle() != 0 && pMoveOrder == 0) {
			const auto activeData = srcData.controlData2nd.activeFlag;
			if ((activeData & (1u << pLookFor)) == 0x0) return false;
		}
		// 条件通過 or 2nd でifを抜ける
	} else if (posData.currentOrder == 2) {
		// 3rd変更中：1st,2ndは必ず制御
		// ※制御方式ComSAはcurrentOrder == 1にて処理される
		if (pMoveOrder == 0) {
			// 1stに制御かけ
			const auto activeData = srcData.controlData3rd.activeFlag1st;
			if ((activeData & (1u << pLookFor)) == 0x0) return false;;
		} else if (pMoveOrder == 1){
			// 2ndに制御かけ
			const auto activeData = srcData.controlData3rd.activeFlag2nd[posData.cursorComa[0]];
			int shiftNum = pLookFor + (posData.isWatchLeft ? 0 : m_comaMax);
			if ((activeData & (1ull << shiftNum)) == 0x0) return false;
		}
		// 条件通過 or 3rd でifを抜ける
	}
	return true;
}

int CSlotControlManager::Get2ndReel(bool pIsLeft) {
	if (pIsLeft)	return posData.stop1st == 0 ? 1 : 0;	// 2nd左側の場合
	else			return posData.stop1st == 2 ? 1 : 2;	// 2nd右側の場合
}

bool CSlotControlManager::Action(int pNewInput) {
	bool setAns = true;
	if (!canChangeTable()) return true;
	m_refreshFlag = true;

	auto refData = GetDef();
	if (refData != nullptr) {	// SAテーブル
		int comaIndex = posData.currentOrder;
		const auto styleData = Get2ndStyle();
		if(posData.currentOrder >= 1 && styleData == 0x3) comaIndex = posData.selectReel;
		*refData = SetAvailT(setAns, refData->availableID, posData.cursorComa[comaIndex], pNewInput, refData->tableFlag);
		CheckSA(); // 停止不能箇所存在確認
	} else {					// SSテーブル
		auto refDataSS = GetSS();
		if (refDataSS == nullptr) return false;
		int comaIndex = posData.currentOrder;
		*refDataSS = SetSlipT(setAns, *refDataSS, posData.cursorComa[comaIndex], pNewInput);
	}
	if (!setAns) return false;
	return UpdateActiveFlag();
}

// [act]テーブル使いまわし目的でテーブルIDを手動で設定する
bool CSlotControlManager::ActionTableID(bool pIsUp) {
	auto refData = GetDef();
	if (refData != nullptr) {	// SAテーブル
		int startID = refData->availableID + AVAIL_TABLE_MAX;
		for (int offset = 1; offset <= AVAIL_TABLE_MAX; ++offset) {
			const auto srcID = startID % AVAIL_TABLE_MAX;
			const auto nowID = (startID + (pIsUp ? offset : -offset)) % AVAIL_TABLE_MAX;
			if (tableAvailable[nowID].refNum == 0) continue;
			const unsigned char newID = nowID & (unsigned char)0xFF; // UCHAR型に変換
			refData->availableID = newID;
			--tableAvailable[srcID].refNum;
			++tableAvailable[newID].refNum;
			m_refreshFlag = true;
			return UpdateActiveFlag();
		}
		return false;
	} else {					// SSテーブル
		auto refDataSS = GetSS();
		if (refDataSS == nullptr) return false;
		int startID = *refDataSS + SLIP_TABLE_MAX;
		for (int offset = 1; offset <= AVAIL_TABLE_MAX; ++offset) {
			const auto srcID = startID % AVAIL_TABLE_MAX;
			const auto nowID = (startID + (pIsUp ? offset : -offset)) % SLIP_TABLE_MAX;
			if (tableSlip[nowID].refNum == 0) continue;
			const unsigned char newID = nowID & (unsigned char)0xFF; // UCHAR型に変換
			*refDataSS = newID;
			--tableSlip[srcID].refNum;
			++tableSlip[newID].refNum;
			m_refreshFlag = true;
			return UpdateActiveFlag();
		}
	}
	return false;
}

// [act]停止不能カ所がないか検証・ある場合はm_isSuspendをtrueにする
SControlDataSA* CSlotControlManager::GetSA(int pFlagID) {
	const int flagID = pFlagID < 0 ? posData.currentFlagID : pFlagID;
	if (flagID >= m_flagMax) return nullptr;
	auto& nowMakeData = ctrlData[flagID];
	const auto styleData = Get2ndStyle();
	auto& nowCtrlData = nowMakeData.controlData[posData.stop1st];

	if (posData.currentOrder == 0) return nullptr;	// 1st制御(=処理なし)
	if (styleData == 0x3) {							// 2nd以降 2nd/3rdCom制御
		const int index = posData.cursorComa[0];
		return &(nowCtrlData.controlData2nd.controlDataComSA[index]);
	} else if (posData.currentOrder == 1) {			// 2nd制御
		if (styleData == 0x2) {						// 2ndStopAvailable(cursorComa[0]に制限あり)
			const int index = posData.cursorComa[0];
			return &(nowCtrlData.controlData2nd.controlData2ndSA[index]);
		}
	} else if (posData.currentOrder == 2) {			// 3rd制御(非Com)
		const int index = posData.cursorComa[0] * m_comaMax + posData.cursorComa[1];
		return &(nowCtrlData.controlData3rd.availableData[index]);
	}
	return nullptr;

}

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

	if (posData.currentOrder == 0) return nullptr;	// 1st制御(=処理なし)
	if (styleData == 0x3) {							// 2nd以降 2nd/3rdCom制御
		const int index = posData.cursorComa[0];
		return &(nowCtrlData.controlData2nd.controlDataComSA[index].data[dataID]);
	} else if (posData.currentOrder == 1) {			// 2nd制御
		if (styleData == 0x2) {						// 2ndStopAvailable(cursorComa[0]に制限あり)
			const int index = posData.cursorComa[0];
			return &(nowCtrlData.controlData2nd.controlData2ndSA[index].data[dataID]);
		}
	} else if (posData.currentOrder == 2) {			// 3rd制御(非Com)
		const int index = posData.cursorComa[0] * m_comaMax + posData.cursorComa[1];
		return &(nowCtrlData.controlData3rd.availableData[index].data[dataID]);
	}
	return nullptr;
}

// [act]現在参照中の該当スベリIDの入った変数ポインタを返す
unsigned char* CSlotControlManager::GetSS(int pFlagID, bool pGet1st) {
	const int flagID = pFlagID < 0 ? posData.currentFlagID : pFlagID;
	if (flagID >= m_flagMax) return nullptr;
	auto& nowMakeData = ctrlData[flagID];
	const auto styleData = Get2ndStyle();
	auto& nowCtrlData = nowMakeData.controlData[posData.stop1st];
	if (posData.currentOrder == 0 || pGet1st) {		// 1st制御
		return &nowCtrlData.controlData1st;
	} else if (posData.currentOrder == 1) {			// 2nd制御
		if (styleData == 0x0) {						// 2ndPushSlip
			int index = posData.cursorComa[0] + (posData.isWatchLeft ? 0 : m_comaMax);
			return &nowCtrlData.controlData2nd.controlData2ndPS[index];
		}
		else if (styleData == 0x1) {				// 2ndStopSlip(cursorComa[0]に制限あり)
			int index = posData.cursorComa[0] + (posData.isWatchLeft ? 0 : m_comaMax);
			return &nowCtrlData.controlData2nd.controlData2ndSS[index];
		}
	}
	return nullptr;
}

unsigned char CSlotControlManager::Get2ndStyle(const int pPushPos) {
	const auto& nowMakeData1st = ctrlData[posData.currentFlagID];
	if (((nowMakeData1st.controlStyle >> posData.stop1st) & 0x1) == 0) return 0x0;
	const int refPos = pPushPos == -1 ? posData.cursorComa[0] : pPushPos;
	const auto& nowMakeData2nd = nowMakeData1st.controlData[posData.stop1st].controlData2nd.controlStyle2nd[refPos];
	return nowMakeData2nd;
}

// [act]制御パターン種別切り替え(1:PS, 2:SS, 3:SA, 4:Com)
void CSlotControlManager::SetAvailCtrlPattern(unsigned char pNewFlag) {
	if (posData.currentOrder == 0) {
		auto& nowMakeData = ctrlData[posData.currentFlagID];
		const int offset = (posData.stop1st);
		// 現在の設定削除(1st)(pNewFlag == 0の場合はこのままにする)
		nowMakeData.controlStyle &= ~(0x1 << offset);

		if (pNewFlag >= 1) {
			// 新規設定導入(1st)
			nowMakeData.controlStyle |= (0x1 << offset);
		}
	}
	else if (posData.currentOrder == 1) {
		if (ctrlData[posData.currentFlagID].controlStyle == 0) return;
		if (pNewFlag == 0) return;
		auto& nowMakeData = ctrlData[posData.currentFlagID].controlData[posData.stop1st].controlData2nd.controlStyle2nd[posData.cursorComa[0]];
		// 新規設定導入(2nd)
		nowMakeData = pNewFlag;
	}
	else return;

	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]SAテーブルタイプ決定(非停止T, 優先T)
void CSlotControlManager::SwitchATableType() {
	if (!canChangeTable()) return;
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// 新規フラグ設定
	const auto nowFlag = refData->tableFlag & 0x04;
	refData->tableFlag = (refData->tableFlag & 0xFB) | (~nowFlag & 0x04);

	CheckSA(); // 停止不能箇所存在確認
	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]SAシフト切り替え(0:シフト無, 1:下1コマ, 2:上1コマ, 3:反転)
void CSlotControlManager::SetAvailShiftConf(unsigned char pNewFlag) {
	if (!canChangeTable()) return;
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// 新規フラグ設定
	refData->tableFlag = (refData->tableFlag & 0xfc) | (pNewFlag & 0x03);

	CheckSA(); // 停止不能箇所存在確認
	UpdateActiveFlag();
	m_refreshFlag = true;
}

// [act]スベリT更新、テーブルNoの付与・採番まで行う
// [ret]新規スベリT テーブル番号(0-255) ただしエラー時は0+pCHKにfalseが入る
unsigned char CSlotControlManager::SetSlipT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal) {
	pCHK = true;
	if (pSrcTableNo >= tableSlip.size()) { pCHK = false; return 0; }
	if (pNewVal < 0 || pNewVal >= 5) { pCHK = false; return 0; }
	unsigned long long newData = 0;
	unsigned int activePos = 0;

	// データ生成・activePos更新
	for (int pp = 0; pp < m_comaMax; ++pp) {
		int slipVal = (GetPosFromSlipT(pSrcTableNo, pp) + m_comaMax - pp) % m_comaMax;
		if (pp == pPushPos) slipVal = pNewVal;
		newData = newData + (unsigned long long)pow(5, pp) * slipVal;	// シフト+代入
		const int nowActive = (pp + slipVal) % m_comaMax;
		activePos |= (1 << nowActive);
	}

	// 採番
	--tableSlip[pSrcTableNo].refNum;
	size_t firstNonRef = SLIP_TABLE_MAX;
	for (size_t ref = 0; ref < tableSlip.size(); ++ref) {
		// 一致のある場合: ref数を1足してテーブル番号を返す
		if (tableSlip[ref].data == newData) { ++tableSlip[ref].refNum; return (unsigned char)ref; }
		// 参照がない最初のデータの取得
		if (firstNonRef == SLIP_TABLE_MAX && tableSlip[ref].refNum == 0) { firstNonRef = ref; }
	}
	// 一致のない場合: 参照のない最初のデータに新しいデータを代入する
	tableSlip[firstNonRef].refNum = 1;
	tableSlip[firstNonRef].data = newData;
	tableSlip[firstNonRef].activePos = activePos;
	return firstNonRef;
}

// [act]引込T更新、テーブルNoの付与・付加条件指定・採番まで行う
// [ret]新規引込T SControlAvailableDefインスタンス ただしエラー時は空データ+pCHKにfalseが入る
SControlAvailableDef CSlotControlManager::SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const unsigned char pTableFlag) {
	pCHK = true;
	const bool isPrior = (pTableFlag & 0x4) != 0;
	SControlAvailableDef ans{ 0,0 };
	if (pSrcTableNo >= tableAvailable.size()) { pCHK = false; return ans; }
	if (pNewVal < 0 || pNewVal >= 5) { pCHK = false; return ans; }

	// データ作成
	unsigned long long newData = GetAvailShiftData(tableAvailable[pSrcTableNo].data, pTableFlag);
	const unsigned long long posFlag = 1ull << pPushPos;
	if (pNewVal >= 1)	newData |=  posFlag;	// 位置有効化
	else				newData &= ~posFlag;	// 位置無効化

	// 採番
	--tableAvailable[pSrcTableNo].refNum;
	size_t firstNonRef = AVAIL_TABLE_MAX;
	unsigned char hitStyle = 0xFF;
	for (size_t ref = 0; ref < tableAvailable.size(); ++ref) {
		for (unsigned char style = 0; style < 4; ++style) {
			if (GetAvailShiftData(tableAvailable[ref].data, style) == newData) {
				hitStyle = style; break;
			}
		}
		// 一致のある場合: ref数を1足してDefを返す
		if (hitStyle != 0xFF) {
			++tableAvailable[ref].refNum;
			ans.availableID = (unsigned char)ref;
			ans.tableFlag = hitStyle;
			if (isPrior) ans.tableFlag |= 0x4;
			return ans;
		}
		// 参照がない最初のデータの取得
		if (firstNonRef == AVAIL_TABLE_MAX && tableAvailable[ref].refNum == 0) { firstNonRef = ref; }
	}
	// 一致のない場合: 参照のない最初のデータに新しいデータを代入する
	tableAvailable[firstNonRef].refNum = 1;
	tableAvailable[firstNonRef].data = newData;
	ans.availableID = (unsigned char)firstNonRef;
	ans.tableFlag = isPrior ? 0x4 : 0x0;
	return ans;
}

bool CSlotControlManager::isSilp() {
	if (posData.currentOrder == 0) return true;
	if (posData.currentOrder == 2) return false;
	return !(Get2ndStyle() & 0x2);
}

// [act]現在選択中フラグの停止可能位置をアップデートする
bool CSlotControlManager::UpdateActiveFlag() {
	auto& nowData = ctrlData[posData.currentFlagID];
	int orderCnt = 0;
	for (auto ctrlIt = nowData.controlData.begin(); ctrlIt != nowData.controlData.end(); ++ctrlIt, ++orderCnt) {
		const unsigned char IDFirst = ctrlIt->controlData1st;
		const int active1st = tableSlip[IDFirst].activePos;
		ctrlIt->controlData2nd.activeFlag = active1st;
		ctrlIt->controlData3rd.activeFlag1st = active1st;

		for (int pushPos = 0; pushPos < m_comaMax*2; ++pushPos) {	// 0:LR, 1:LR, ...
			const unsigned char useTable2nd = Get2ndStyle(pushPos);
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
				// 引込不可時無効データ -> activeデータクリアして終了
				if (active2nd == 0) {
					ctrlIt->controlData3rd.activeFlag2nd[pushPos % m_comaMax] = 0;
					return true;
				}
			} else if (useTable2nd == 0x3) {
				// 何もしない
			} else {
				return false;
			}

			if (lrFlag) {
				ctrlIt->controlData3rd.activeFlag2nd[pushPos % m_comaMax] = active2nd;
			} else {
				ctrlIt->controlData3rd.activeFlag2nd[pushPos % m_comaMax] |= (active2nd << m_comaMax);
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

	if (shiftFlag == 0x1) { // 下1コマ(LShift) 空きbitがある前提で実装する
		pData <<= 1;
		if (pData & ~allStopFlag) pData |= 0x1;	// オーバーフロー検知
		pData &= allStopFlag;
	}
	if (shiftFlag == 0x2) { // 上1コマ(RShift)
		const bool addFlag = pData & 0x1;
		pData = (pData >> 1) & allStopFlag;
		if (addFlag) pData = pData | (0x1ull << (m_comaMax - 1));
	}
	if (shiftFlag == 0x3) {	// 反転
		pData = ~pData & allStopFlag;
	}
	return pData;
}

unsigned long long CSlotControlManager::GetAvailShiftData(const SControlDataSA& pSAData, const int pIndex, bool pIsLeft) {
	if (pIndex < 0 || pIndex >= AVAIL_ID_MAX) return 0;
	const auto index   = pIndex + (pIsLeft ? 0 : AVAIL_ID_MAX);
	const auto tableNo = pSAData.data[index].availableID;
	const auto data    = tableAvailable[tableNo].data;
	const auto flag    = pSAData.data[index].tableFlag;
	return GetAvailShiftData(data, flag);
}

// [ret]停止位置 ただし引き込めるものがないときは-1
int CSlotControlManager::GetPosFromAvailT(const SControlDataSA& pSAData, const int pPushPos, bool pIsLeft) {
	const unsigned long long allStopFlag = m_allStopFlag;
	unsigned long long availPos = allStopFlag;
	const int offsetLR = pIsLeft ? 0 : AVAIL_ID_MAX;
	for (int i = 0; i < AVAIL_ID_MAX; ++i) {
		auto it = pSAData.data.cbegin() + i + offsetLR;
		unsigned long long availData = tableAvailable[it->availableID].data;
		availData = GetAvailShiftData(availData, it->tableFlag);

		/* 優先度による処理決定 */ {
			const bool isPrior = ((it->tableFlag & 0x4) != 0x0);
			if (isPrior) {	// 優先T
				availData &= availPos;	// 事前に非停止とされた停止位置を除く
				const int slip = GetAvailDistance(availData, pPushPos);
				if (slip >= 0) return (pPushPos + slip) % m_comaMax;
			} else {		// 非停止T
				availPos &= availData;
			}
		}
	}

	// 非停止Tによる位置決定結果を返す
	const int slipNum = GetAvailDistance(availPos, pPushPos);
	if (slipNum == -1) return -1;
	return (pPushPos + slipNum) % m_comaMax;
}

// [ret]停止位置ビット列 ただし無効なコマが1つでもあれば0
int CSlotControlManager::GetActiveFromAvailT(const SControlDataSA& pSAData, bool pIsLeft) {
	int ans = 0;
	for (int pushPos = 0; pushPos < m_comaMax; ++pushPos) {
		const int availPos = GetPosFromAvailT(pSAData, pushPos, pIsLeft);
		if (availPos == -1) return 0;	// 停止位置なしで強制終了
		ans |= (1 << availPos);
	}
	return ans;
}

// [act]停止可能な最も近い停止可能位置までの距離を出す。(0-4, 範囲外は-1とする)
int CSlotControlManager::GetAvailDistance(const unsigned long long pData, const int pPushPos) {
	int refPos = pPushPos; // LSBが0番のデータ
	for (int pos = 0; pos < 5; ++pos) {
		if ((pData >> refPos) & 0x1) return pos;	// シフト後LSB=1なら停止可能
		refPos = (refPos + 1) % m_comaMax;			// 滑った後のrefPosを算出
	}
	return -1;
}


bool CSlotControlManager::Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor) {
	DxLib::SetDrawScreen(pDrawFor);
	/* リール描画 */ {
		std::vector<int> drawPos; drawPos.resize(m_reelMax * 2, -1);	// 0PS, 1PS, 2PS
		const int reelPosByOrder[] = { Get2ndReel(posData.isWatchLeft), Get2ndReel(!posData.isWatchLeft) };
		/* 描画リール定義 */ {
			if (posData.currentOrder == 0) {			// 1st
				const auto ss = GetSS();
				drawPos[posData.stop1st * 2    ] = posData.cursorComa[0];
				drawPos[posData.stop1st * 2 + 1] = GetPosFromSlipT(*ss, posData.cursorComa[0]);
			} else if (Get2ndStyle() == 0x03) {			// 2nd以降 2nd/3rd共通
				const auto sa = GetSA();
				drawPos[posData.stop1st   * 2    ] = posData.cursorComa[0];
				drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
				drawPos[Get2ndReel(true)  * 2    ] = posData.cursorComa[1];
				drawPos[Get2ndReel(true)  * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[1], true);
				drawPos[Get2ndReel(false) * 2    ] = posData.cursorComa[2];
				drawPos[Get2ndReel(false) * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[2], false);
			} else if (posData.currentOrder == 1) {		// 2ndComSA以外
				drawPos[posData.stop1st   * 2    ] = posData.cursorComa[0];
				drawPos[reelPosByOrder[0] * 2    ] = posData.cursorComa[1];
				if (Get2ndStyle() == 0x00) {				// PSテーブル
					const unsigned char *const ss1 = GetSS(-1, true);	// -1:現在選択中のフラグ
					const unsigned char *const ss2 = GetSS(-1, false);
					drawPos[posData.stop1st   * 2 + 1] = GetPosFromSlipT(*ss1, posData.cursorComa[0]);
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromSlipT(*ss2, posData.cursorComa[1]);
				} else if (Get2ndStyle() == 0x01) {			// SSテーブル
					const unsigned char *const ss2 = GetSS(-1, false);
					drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromSlipT(*ss2, posData.cursorComa[1]);
				} else {									// SAテーブル
					const auto sa = GetSA();
					drawPos[posData.stop1st   * 2 + 1] = posData.cursorComa[0];
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromAvailT(*sa, posData.cursorComa[1], posData.isWatchLeft);
				}
			} else {									// 3rdComSA以外
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
		/* SReelDrawData初期作成 */ {
			drawReel.y = 1;
			drawReel.comaW = 77; drawReel.comaH = 35; drawReel.comaNum = 3;
			drawReel.offsetLower = 0; drawReel.offsetUpper = 0;
		}
		// リール描画
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
	}

	/* 各種情報描画(仮) */ {
		int tableNo = 0;
		std::string availFlag = "";
		const auto* const sd = GetDef();
		if (sd != nullptr) {
			tableNo = sd->availableID;
			const auto flag = sd->tableFlag;
			const std::string availType[] = {"Def", "1up", "1dn", "Rev"};
			availFlag += availType[flag & 0x3] + ", ";
			availFlag += (flag & 0x4) ? "Pri" : "Neg";
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

	/* テーブル描画・テーブル番号描画 */ {
		// x=275, 298 y = 176
		int xPos = 227;
		const int yPos = 176;
		// リール番号描画
		for (int i = 0; i < m_comaMax; ++i) {
			const int strY = yPos + i * BOX_H;
			DxLib::DrawFormatString(xPos-2, strY, 0x808080, "%02d", m_comaMax - i);
		}
		xPos += BOX_W;
		
		DrawSlipTable(xPos, yPos, posData.currentFlagID, pData);
		xPos += BOX_W;

		if (!isSilp()) {
			DrawStopTable(xPos, yPos, posData.currentFlagID);
			xPos += BOX_W * AVAIL_ID_MAX;
		}
		xPos += BOX_W;

		// 参考資料として他テーブルを描画
		for (int flagC = 0; flagC < m_flagMax; ++flagC) {
			if (flagC == posData.currentFlagID) continue;
			bool isNotStop = false;
			for (int i = 0; i < posData.currentOrder; ++i) 
				if (!GetCanStop(i, posData.cursorComa[i], flagC)) { isNotStop = true; continue; }
			if (isNotStop) continue;

			DrawSlipTable(xPos, yPos, flagC, pData);
			xPos += BOX_W;
		}
	}
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

bool CSlotControlManager::DrawSlipTable(int x, int y, int pFlagID, SSlotGameDataWrapper& pData) {
	const int color = (pFlagID == posData.currentFlagID) ? 0xFFFF00 : 0x808080;
	const std::string flagName  = pData.randManager.GetFlagName (pFlagID);
	const std::string bonusName = pData.randManager.GetBonusName(pFlagID);

	DxLib::DrawString(x - 1, y - BOX_H*2 + 14, flagName .c_str(), 0xFFFF00);
	DxLib::DrawString(x - 1, y - BOX_H   +  4, bonusName.c_str(), 0xFFFF00);
	if (isSilp()) {
		const auto ss = GetSS(pFlagID);
		if (ss == nullptr) return false;
		DrawComaBox(x-3, y-6, tableSlip[*ss].activePos, posData.cursorComa[posData.currentOrder]);
		for (int i = 0; i < m_comaMax; ++i) {
			const int posY = y + 26 * (m_comaMax - i - 1);
			int stopPos = GetPosFromSlipT(*ss, i);
			int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
			DxLib::DrawFormatString(x, posY, color, "%d", showVal);
		}
	} else {
		SControlDataSA* sa = GetSA(pFlagID);
		if(sa == nullptr) return false;
		const unsigned int stopFlag = m_isSuspend ? 0 : GetActiveFromAvailT(*sa, posData.isWatchLeft);
		DrawComaBox(x-3, y-6, stopFlag, posData.cursorComa[posData.currentOrder]);
		for (int i = 0; i < m_comaMax; ++i) {
			const int posY = y + BOX_H * (m_comaMax - i - 1);
			if (!m_isSuspend) {
				int stopPos = GetPosFromAvailT(*sa, i, posData.isWatchLeft);
				int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
				DxLib::DrawFormatString(x, posY, color, "%d", showVal);
			} else {
				DxLib::DrawString(x, posY, "X", 0xFF0000);
			}
		}
	}
	return true;
}

bool CSlotControlManager::DrawStopTable(int x, int y, int pFlagID) {
	const int color = (pFlagID == posData.currentFlagID) ? 0xFFFF00 : 0x808080;
	SControlDataSA* sa = GetSA(pFlagID);
	if(sa == nullptr) return false;
	for (int j = 0; j < AVAIL_ID_MAX; ++j) {
		const int posX = x + BOX_W * j;
		unsigned long long stopFlag = 0;
		/* 停止位置呼び出し */ {
			const int index = j + (posData.isWatchLeft ? 0 : AVAIL_ID_MAX);
			const auto nowTableID = sa->data[index].availableID;
			const int boxColor = (j == posData.selectAvailID) ? 0x8080FF : 0x404040;
			stopFlag = GetAvailShiftData(*sa, j, posData.isWatchLeft);
			DrawComaBox(posX-3, y-6, stopFlag & 0xFFFFFFFF, posData.cursorComa[posData.currentOrder], boxColor);
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


bool CSlotControlManager::ReadRestore(CRestoreManagerRead& pReader) {
	size_t sizeTemp = 0;
	size_t defSize[4] = { 0,0,0,0 };
	size_t loopMax[4] = { 0,0,0,0 };
	bool   canCountRef[4] = { true, true, true, true };	// ループカウントがdefSize以上となる場合破棄されるのでrefNumに追加しないようにするための変数

	// tableSlip
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > tableSlip.size()) { defSize[0] = tableSlip.size(); tableSlip.resize(sizeTemp, tableSlip[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		if (!pReader.ReadNum(tableSlip[i].data     )) return false;
		if (!pReader.ReadNum(tableSlip[i].activePos)) return false;
		// Version:3移行はctrlData読み込み時に計算する → 読み込むだけ読み込んで0初期化
		if (pReader.GetReadVer()<=2)
			if (!pReader.ReadNum(tableSlip[i].refNum   )) return false;
		tableSlip[i].refNum = 0;
	}
	if (defSize[0] > 0) { tableSlip.resize(defSize[0]); defSize[0] = 0; }
	
	// tableAvailable(activePosは使用しない)
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > tableAvailable.size()) { defSize[0] = tableAvailable.size(); tableAvailable.resize(sizeTemp, tableAvailable[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		if (!pReader.ReadNum(tableAvailable[i].data     )) return false;
		// Version:3移行はctrlData読み込み時に計算する → 読み込むだけ読み込んで0初期化
		if (pReader.GetReadVer()<=2)
			if (!pReader.ReadNum(tableAvailable[i].refNum   )) return false;
		tableAvailable[i].refNum = 0;
	}
	if (defSize[0] > 0) { tableAvailable.resize(defSize[0]); defSize[0] = 0; }

	// ctrlData
	if (!pReader.ReadNum(sizeTemp)) return false; loopMax[0] = sizeTemp;
	if (sizeTemp > ctrlData.size()) { defSize[0] = ctrlData.size(); ctrlData.resize(sizeTemp, ctrlData[0]); }
	for (size_t i = 0; i < loopMax[0]; ++i) {
		// Version<=3時 ctrlData[i].controlStyle更新用
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
			if (canCountRef[1]) tableSlip[nowData.controlData1st].refNum++;	// refNum使用実績確認
			if (!pReader.ReadNum(nowData.controlData2nd.activeFlag)) return false;

			// controlStyle2ndの更新
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
				if (canCountRef[2]) tableSlip[ps].refNum++;		// refNum使用実績確認
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
				if (canCountRef[2]) tableSlip[ss].refNum++;		// refNum使用実績確認
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
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum使用実績確認
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
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum使用実績確認
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
					if (canCountRef[3]) tableAvailable[sa.data[k].availableID].refNum++;	// refNum使用実績確認
				}
				if (defSize[3] > 0) { sa.data.resize(defSize[3]); defSize[3] = 0; }
			}
			if (defSize[2] > 0) { nowData.controlData3rd.availableData.resize(defSize[2]); defSize[2] = 0; }
		}
		if (defSize[1] > 0) { ctrlData[i].controlData.resize(defSize[1]); defSize[1] = 0; }
	}
	if (defSize[0] > 0) { ctrlData.resize(defSize[0]); defSize[0] = 0; }

	// refPos[0](No Data)の参照数付けなおし
	tableSlip[0].refNum = INT_MAX;	tableAvailable[0].refNum = INT_MAX;
	for (size_t i = 1; i < tableSlip.size(); ++i) tableSlip[0].refNum -= tableSlip[i].refNum;
	for (size_t i = 1; i < tableAvailable.size(); ++i) tableAvailable[0].refNum -= tableAvailable[i].refNum;

	// posData
	if (!pReader.ReadNum(posData.currentFlagID)) return false;
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

	AdjustPos();
	return true;
}

bool CSlotControlManager::WriteRestore(CRestoreManagerWrite& pWriter) const {
	// tableSlip
	if (!pWriter.WriteNum(tableSlip.size())) return false;
	for (size_t i = 0; i < tableSlip.size(); ++i) {
		if (!pWriter.WriteNum(tableSlip[i].data     )) return false;
		if (!pWriter.WriteNum(tableSlip[i].activePos)) return false;
		// Version=2で打ち切り(読み込み時に計算)
		//if (!pWriter.WriteNum(tableSlip[i].refNum   )) return false;
	}

	// tableAvailable(activePosは使用しない)
	if (!pWriter.WriteNum(tableAvailable.size())) return false;
	for (size_t i = 0; i < tableAvailable.size(); ++i) {
		if (!pWriter.WriteNum(tableAvailable[i].data  )) return false;
		// Version=2で打ち切り(読み込み時に計算)
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