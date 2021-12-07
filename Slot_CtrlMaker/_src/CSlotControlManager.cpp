#include "_header/CSlotControlManager.hpp"
#include "_header/SSlotGameDataWrapper.hpp"
#include "_header/keyexport.h"
#include <DxLib.h>

bool CSlotControlManager::Init(const SSlotGameDataWrapper& pData) {
	// データ型初期化(ロード処理は後で)
	m_flagMax = pData.randManager.GetFlagNum();
	m_reelMax = pData.reelManager.GetReelNum();
	m_comaMax = pData.reelManager.GetCharaNum();
	m_allStopFlag = (unsigned long long)pow(2, pData.reelManager.GetCharaNum()) - 1ull;
	m_isSuspend = false;

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
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_1)) SetAvailCtrlPattern(0);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_2)) SetAvailCtrlPattern(1);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_3)) SetAvailCtrlPattern(2);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_4)) SetAvailCtrlPattern(3);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_5)) SetAvailShiftConf(0);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_6)) SetAvailShiftConf(1);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_7)) SetAvailShiftConf(2);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_8)) SetAvailShiftConf(3);

		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_Q)) ActionTableID(true);
		else if (shiftFlag && key.ExportKeyState(KEY_INPUT_A)) ActionTableID(false);

		AdjustPos();
	}
	return true;
}

void CSlotControlManager::AdjustPos() {
	// 各種上限値定義
	const int comaMax = m_comaMax;
	const int flagMax = m_flagMax;
	const int reelMax = m_reelMax;

	for (auto it = posData.cursorComa.begin(); it != posData.cursorComa.end(); ++it) {
		while (*it < 0) *it += comaMax;
		while (*it >= comaMax) *it -= comaMax;
	}

	while (posData.stop1st < 0) posData.stop1st += reelMax;
	while (posData.stop1st >= reelMax) posData.stop1st -= reelMax;

	int orderLim = reelMax;
	// 共通制御時2nd/3rdはcurrentOrder = 1で頭打ち
	if (Get2ndStyle() == 0x03) orderLim -= 1;
	while (posData.currentOrder < 0) posData.currentOrder += orderLim;
	while (posData.currentOrder >= orderLim) posData.currentOrder -= orderLim;

	while (posData.currentFlagID < 0) posData.currentFlagID += flagMax;
	while (posData.currentFlagID >= flagMax) posData.currentFlagID -= flagMax;

	int selectLim = posData.currentOrder + 1;
	// 共通制御時2nd/3rdは全リール操作可能とする
	if (posData.currentOrder >= 1 && Get2ndStyle() == 0x03) selectLim = m_reelMax;
	while (posData.selectReel < 0) posData.selectReel += selectLim;
	while (posData.selectReel >= selectLim) posData.selectReel -= selectLim;

	if (isSilp()) posData.selectAvailID = 0;
	while (posData.selectAvailID < 0) posData.selectAvailID += AVAIL_ID_MAX;
	while (posData.selectAvailID >= AVAIL_ID_MAX) posData.selectAvailID -= AVAIL_ID_MAX;
}

// [act]停止し得る場所へcomaPosを自動調整する
// [prm]pMoveOrder	: 今回変更するリールの押し順
// [ret]正しく位置を設定できたか(無効制御有の場合false)
bool CSlotControlManager::SetComaPos(const int pMoveOrder, const bool pIsReset, const bool pIsUp) {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	auto& srcData = nowMakeData.controlData[posData.stop1st];

	for (int i = pIsReset ? 0 : 1; i < m_comaMax; ++i) {
		int diffPos = pIsReset ? 0 : posData.cursorComa[pMoveOrder];
		diffPos += pIsUp ? i : -i;
		while (diffPos < 0)				diffPos += m_comaMax;
		while (diffPos >= m_comaMax)	diffPos -= m_comaMax;

		// 使用可能なリール位置でcontinueせず場所が確定(break)する仕組み
		if (posData.currentOrder == 0) {
			// 1st変更中：制御をかけない(対象が1stであるか確認する)
			if (pMoveOrder != 0) return false;
		} else if (posData.currentOrder == 1) {
			// 2nd変更中：制御方式PS以外 && 参照1st であれば制御をかける
			// ※制御方式ComSAにて3rdもこのループを参照、制御はかからない
			if(Get2ndStyle() != 0 && pMoveOrder == 0) {
				const auto activeData = srcData.controlData2nd.activeFlag;
				if ((activeData & (1u << diffPos)) == 0x0) continue;
			}
			// 条件通過 or 2nd でifを抜ける
		} else if (posData.currentOrder == 2) {
			// 3rd変更中：1st,2ndは必ず制御
			// ※制御方式ComSAはcurrentOrder == 1にて処理される
			if (pMoveOrder == 0) {
				// 1stに制御かけ
				const auto activeData = srcData.controlData3rd.activeFlag1st;
				if ((activeData & (1u << diffPos)) == 0x0) continue;
			} else if (pMoveOrder == 1){
				// 2ndに制御かけ
				const auto activeData = srcData.controlData3rd.activeFlag2nd[posData.cursorComa[0]];
				int shiftNum = diffPos + (posData.isWatchLeft ? 0 : m_comaMax);
				if ((activeData & (1ull << shiftNum)) == 0x0) continue;
			}
			// 条件通過 or 3rd でifを抜ける
		}
		// 場所確定
		posData.cursorComa[pMoveOrder] = diffPos; return true;
	}
	return false;
}

int CSlotControlManager::Get2ndReel(bool pIsLeft) {
	if (pIsLeft)	return posData.stop1st == 0 ? 1 : 0;	// 2nd左側の場合
	else			return posData.stop1st == 2 ? 1 : 2;	// 2nd右側の場合
}

bool CSlotControlManager::Action(int pNewInput) {
	bool setAns = true;
	auto refData = GetDef();
	if (refData != nullptr) {	// SAテーブル
		int comaIndex = posData.currentOrder;
		const auto styleData = Get2ndStyle();
		if(posData.currentOrder >= 1 && styleData == 0x3) comaIndex = posData.currentOrder;
		*refData = SetAvailT(setAns, refData->availableID, posData.cursorComa[comaIndex], pNewInput, refData->tableFlag & 0x4);
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
			return UpdateActiveFlag();
		}
	}
	return false;
}

SControlDataSA* CSlotControlManager::GetSA() {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
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

SControlAvailableDef* CSlotControlManager::GetDef() {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	const auto styleData = Get2ndStyle();
	auto& nowCtrlData = nowMakeData.controlData[posData.stop1st];

	if (posData.currentOrder == 0) return nullptr;	// 1st制御(=処理なし)
	if (styleData == 0x3) {							// 2nd以降 2nd/3rdCom制御
		const int index = posData.cursorComa[0];
		const int dataID = posData.selectAvailID + (posData.currentOrder - 1) * AVAIL_ID_MAX;
		return &(nowCtrlData.controlData2nd.controlDataComSA[index].data[dataID]);
	} else if (posData.currentOrder == 1) {			// 2nd制御
		if (styleData == 0x2) {						// 2ndStopAvailable(cursorComa[0]に制限あり)
			const int index = posData.cursorComa[0];
			const int dataID = posData.selectAvailID + posData.isWatchLeft ? 0 : AVAIL_ID_MAX;
			return &(nowCtrlData.controlData2nd.controlData2ndSA[index].data[dataID]);
		}
	} else if (posData.currentOrder == 2) {			// 3rd制御(非Com)
		const int index = posData.cursorComa[0] * m_comaMax + posData.cursorComa[1];
		const int dataID = posData.selectAvailID + posData.isWatchLeft ? 0 : AVAIL_ID_MAX;
		return &(nowCtrlData.controlData3rd.availableData[index].data[dataID]);
	}
	return nullptr;
}

// [act]現在参照中の該当スベリIDの入った変数ポインタを返す
unsigned char* CSlotControlManager::GetSS(bool pGet1st) {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	const auto styleData = Get2ndStyle();
	auto& nowCtrlData = nowMakeData.controlData[posData.stop1st];
	if (posData.currentOrder == 0 || pGet1st) {		// 1st制御
		return &nowCtrlData.controlData1st;
	} else if (posData.currentOrder == 1) {			// 2nd制御
		if (styleData == 0x0) {						// 2ndPushSlip
			int index = posData.cursorComa[0] + posData.isWatchLeft ? 0 : m_comaMax;
			return &nowCtrlData.controlData2nd.controlData2ndPS[index];
		}
		else if (styleData == 0x1) {				// 2ndStopSlip(cursorComa[0]に制限あり)
			int index = posData.cursorComa[0] + posData.isWatchLeft ? 0 : m_comaMax;;
			return &nowCtrlData.controlData2nd.controlData2ndSS[index];
		}
	}
	return nullptr;
}

unsigned char CSlotControlManager::Get2ndStyle() {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	return (nowMakeData.controlStyle >> (posData.stop1st * 2)) & 0x3;
}

// [act]制御パターン種別切り替え(1:PS, 2:SS, 3:SA, 4:Com)
void CSlotControlManager::SetAvailCtrlPattern(unsigned char pNewFlag) {
	auto& nowMakeData = ctrlData[posData.currentFlagID];
	const int offset = (posData.stop1st * 2);
	// 現在の設定削除
	nowMakeData.controlStyle &= ~(0x03 << offset);
	// 新規設定導入
	nowMakeData.controlStyle |= ((pNewFlag & 0x03) << offset);
}

// [act]SAテーブルタイプ決定(非停止T, 優先T)
void CSlotControlManager::SwitchATableType() {
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// 新規フラグ設定
	const auto nowFlag = refData->tableFlag & 0x04;
	refData->tableFlag = (refData->tableFlag & 0xFB) | (~nowFlag & 0x04);
}

// [act]SAシフト切り替え(0:シフト無, 1:下1コマ, 2:上1コマ, 3:反転)
void CSlotControlManager::SetAvailShiftConf(unsigned char pNewFlag) {
	SControlAvailableDef* refData = GetDef();
	if (refData == nullptr) return;
	// 新規フラグ設定
	refData->tableFlag = (refData->tableFlag & 0xfc) | (pNewFlag & 0x03);
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
SControlAvailableDef CSlotControlManager::SetAvailT(bool& pCHK, const size_t pSrcTableNo, const int pPushPos, const int pNewVal, const bool pIsPrior) {
	pCHK = true;
	SControlAvailableDef ans{ 0,0 };
	if (pSrcTableNo >= tableAvailable.size()) { pCHK = false; return ans; }
	if (pNewVal < 0 || pNewVal >= 5) { pCHK = false; return ans; }

	// データ作成
	unsigned long long newData = tableAvailable[pSrcTableNo].data;
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
			if (pIsPrior) ans.tableFlag |= 0x4;
			return ans;
		}
		// 参照がない最初のデータの取得
		if (firstNonRef == AVAIL_TABLE_MAX && tableAvailable[ref].refNum == 0) { firstNonRef = ref; }
	}
	// 一致のない場合: 参照のない最初のデータに新しいデータを代入する
	tableAvailable[firstNonRef].refNum = 1;
	tableAvailable[firstNonRef].data = newData;
	ans.availableID = (unsigned char)firstNonRef;
	ans.tableFlag = pIsPrior ? 0x4 : 0x0;
	return ans;
}

bool CSlotControlManager::isSilp() {
	if (posData.currentOrder == 0) return true;
	return !(Get2ndStyle() & 0x2);
}

// [act]現在選択中フラグの停止可能位置をアップデートする
bool CSlotControlManager::UpdateActiveFlag() {
	m_isSuspend = false;	// m_isSuspendリセット
	auto& nowData = ctrlData[posData.currentFlagID];
	int orderCnt = 0;
	for (auto ctrlIt = nowData.controlData.begin(); ctrlIt != nowData.controlData.end(); ++ctrlIt, ++orderCnt) {
		const unsigned char IDFirst = ctrlIt->controlData1st;
		const int active1st = tableSlip[IDFirst].activePos;
		ctrlIt->controlData2nd.activeFlag = active1st;
		ctrlIt->controlData3rd.activeFlag1st = active1st;

		const unsigned char useTable2nd = Get2ndStyle();
		for (int pushPos = 0; pushPos < m_comaMax*2; ++pushPos) {	// 0:LR, 1:LR, ...
			const bool lrFlag = ((pushPos % m_comaMax) == 0);
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
				// 引込不可時無効データ -> m_isSuspendをtrueにして終了
				if (active2nd == 0) { m_isSuspend = true; return true; }
			} else if (useTable2nd == 0x3) {
				const int ref2nd = pushPos % m_comaMax;
				active2nd = GetActiveFromAvailT(ctrlIt->controlData2nd.controlDataComSA[ref2nd], lrFlag);
				// 引込不可時無効データ -> m_isSuspendをtrueにして終了
				if (active2nd == 0) { m_isSuspend = true; return true; }
			} else {
				return false;
			}

			if (lrFlag) {
				ctrlIt->controlData3rd.activeFlag2nd[pushPos / 2] = active2nd;
			} else {
				ctrlIt->controlData3rd.activeFlag2nd[pushPos / 2] |= (active2nd << m_comaMax);
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
				if (slip >= 0) return slip;
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
		refPos = (refPos + 1) % m_comaMax;			// 滑った後のrefPosを算出
		if ((pData >> refPos) & 0x1) return pos;	// シフト後LSB=1なら停止可能
	}
	return -1;
}


bool CSlotControlManager::Draw(SSlotGameDataWrapper& pData, CGameDataManage& pDataManageIns, int pDrawFor) {
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
					const unsigned char *const ss1 = GetSS(true);
					const unsigned char *const ss2 = GetSS(false);
					drawPos[posData.stop1st   * 2 + 1] = GetPosFromSlipT(*ss1, posData.cursorComa[0]);
					drawPos[reelPosByOrder[0] * 2 + 1] = GetPosFromSlipT(*ss2, posData.cursorComa[1]);
				} else if (Get2ndStyle() == 0x01) {			// SSテーブル
					const unsigned char *const ss2 = GetSS(false);
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
			drawReel.x = 502 + 78 * (i/2); drawReel.reelID = i/2;
			int castPos = (m_comaMax * 2 - 3 - drawPos[i]) % m_comaMax;
			pData.reelManager.DrawReel(pDataManageIns, drawReel, castPos, pDrawFor);
			drawReel.x = 768 + 78 * (i/2);
			castPos = (m_comaMax * 2 - 3 - drawPos[i + 1]) % m_comaMax;
			pData.reelManager.DrawReel(pDataManageIns, drawReel, castPos, pDrawFor);
		}
	}

	/* 各種情報描画(仮) */ {
		DxLib::DrawFormatString(277,  0, 0xFFFF00, "currentFlagID : %d", posData.currentFlagID);
		DxLib::DrawFormatString(277, 20, 0xFFFF00, "selectReel    : %d", posData.selectReel);
		DxLib::DrawFormatString(277, 40, 0xFFFF00, "currentOrder  : %d", posData.currentOrder);
		DxLib::DrawFormatString(277, 60, 0xFFFF00, "stop1st       : %d", posData.stop1st);
		const std::string ctrlType[] = {"PushS", "StopS", "2ndSA", "ComSA"};
		DxLib::DrawFormatString(277, 80, 0xFFFF00, "ctrlType      : %s", ctrlType[Get2ndStyle()].c_str());
		DxLib::DrawFormatString(277,100, 0xFFFF00, "currentComa   : %d, %d, %d", 
			posData.cursorComa[0], posData.cursorComa[1], posData.cursorComa[2]);
	}

	/* テーブル描画・テーブル番号描画 */ {
		if (isSilp()) {
			const auto ss = GetSS();
			if (ss == nullptr) return false;
			for (int i = 0; i < m_comaMax; ++i) {
				const int posY = 176 + 26 * (m_comaMax - i - 1);
				int stopPos = GetPosFromSlipT(*ss, i);
				int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
				DxLib::DrawFormatString(281, posY, 0xFFFF00, "%d", showVal);
			}
		} else {
			SControlDataSA* sa = GetSA();
			if(sa == nullptr) return false;	// この関数内でsa初期化
			for (int i = 0; i < m_comaMax; ++i) {
				const int posY = 176 + 26 * (m_comaMax - i - 1);
				int stopPos = GetPosFromAvailT(*sa, i, posData.isWatchLeft);
				int showVal = ((stopPos + m_comaMax) - i) % m_comaMax;
				DxLib::DrawFormatString(281, posY, 0xFFFF00, "%d", showVal);
			}
			for (int j = 0; j < AVAIL_ID_MAX; ++j) {
				const int posX = 307 + 26 * j;
				unsigned long long stopFlag = 0;
				/* 停止位置呼び出し */ {
					const int index = j + posData.isWatchLeft ? 0 : AVAIL_ID_MAX;
					const auto nowTableID = sa->data[index].availableID;
					stopFlag = GetAvailShiftData(*sa, j, posData.isWatchLeft);
					if (stopFlag == 0) return false;
				}
				for (int pos = 0; pos < m_comaMax; ++pos) {
					const int posY = 176 + 26 * (m_comaMax - pos - 1);
					const std::string drawStr = (stopFlag & 0x1) ? "@" : "-";
					DxLib::DrawFormatString(posX, posY, 0xFFFF00, "%s", drawStr.c_str());
					stopFlag >>= 1;
				}
			}
			
		}
	}
	return true;
}