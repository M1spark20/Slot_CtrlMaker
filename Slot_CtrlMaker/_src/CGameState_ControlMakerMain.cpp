#include "_header/CGameState_ControlMakeMain.hpp"

#include "_header/CGameDataManage.h"
#include "_header/CGetSysDataFromCSV.hpp"
#include "_header/CSlotFlowList.hpp"
#include "_header/keyexport.h"
#include "_header/CRestoreManager.hpp"
#include "DxLib.h"

bool CGameState_ControlMakerMain::Init(CGameDataManage& pDataManageIns) {
	CGetSysDataFromCSV sysReader;
	sysReader.FileInit(pDataManageIns.GetDataHandle(0));
	if (!m_data.reelManager.Init(pDataManageIns, sysReader))						return false;
	if (!m_data.randManager.Init(pDataManageIns, sysReader.GetSysDataID("flags")))	return false;
	if (!m_data.castChecker.Init(pDataManageIns, sysReader.GetSysDataID("cast")))	return false;
	if (!m_data.timeManager.Init(m_data.reelManager.GetReelNum()))					return false;
	if (!m_data.reelChecker.Init(
		pDataManageIns, sysReader.GetSysDataID("spot"), sysReader.GetSysDataID("collection"), m_data.reelManager)
		) return false;
	mBGHandle = sysReader.GetSysDataID("BG");

	// controlManagerをデータ有で再初期化
	if (!m_controlManager.Init(m_data))	return false;

	// 保存データ読み込み
	CRestoreManagerRead reader;
	if (reader.StartRead()) {
		if (!m_controlManager.ReadRestore(reader)) return false;
	}

	// 縮小用画面生成
	DxLib::GetScreenState(&mDisplayW, &mDisplayH, NULL);
	// mBGWindow(縮小用)はProcess初回呼び出し時に定義
	mBGWindow = INT_MIN;

	m_data.timeManager.Process();
	m_pFlowManager = new CSlotFlowMakeControl1st;
	return m_pFlowManager->Init(m_data);
}

EChangeStateFlag CGameState_ControlMakerMain::Process(CGameDataManage& pDataManageIns, bool pExtendResolution) {
	// mBGWindow生成(拡大縮小用に1枚画面をかませる)
	if (mBGWindow == INT_MIN) {
		if (pExtendResolution) mBGWindow = DxLib::MakeScreen(2160, 1080);
		else mBGWindow = DxLib::MakeScreen(1920, 1080);
	}

	CKeyExport_S& key = CKeyExport_S::GetInstance();
	if (key.GetExportStatus() == EKeyExportStatus::eGameMain && key.ExportKeyState(KEY_INPUT_ESCAPE))
		return EChangeStateFlag::eStateEnd;

	m_data.timeManager.Process();
	m_controlManager.Process();
	ESlotFlowFlag flow = m_pFlowManager->Process(m_data);
	if (flow != ESlotFlowFlag::eFlowContinue) {
		delete m_pFlowManager;	m_pFlowManager = nullptr;
		switch (flow) {
		case ESlotFlowFlag::eFlowMake1st:
			m_pFlowManager = new CSlotFlowMakeControl1st;
			break;
		case ESlotFlowFlag::eFlowEnd:
			return EChangeStateFlag::eStateEnd;
			break;
		case ESlotFlowFlag::eFlowErrEnd:
			return EChangeStateFlag::eStateErrEnd;
			break;
		default:
			return EChangeStateFlag::eStateErrEnd;
			break;
		}
		if (!m_pFlowManager->Init(m_data)) return EChangeStateFlag::eStateErrEnd;
	}

	m_data.reelManager.Process(m_data.timeManager);

	// データ保存
	if (m_controlManager.RefreshFlag()) {
		m_data.restoreManager.SetActivate();	// 自分で有効化してチェックする
		if (!m_data.restoreManager.IsActivate()) {
			if (!m_data.restoreManager.StartWrite()) return EChangeStateFlag::eStateErrEnd;
			if (!m_controlManager.WriteRestore(m_data.restoreManager)) return EChangeStateFlag::eStateErrEnd;
			if (!m_data.restoreManager.Flush()) return EChangeStateFlag::eStateErrEnd;
		}
	}

	return EChangeStateFlag::eStateContinue;
}

bool CGameState_ControlMakerMain::Draw(CGameDataManage& pDataManageIns) {
	DxLib::SetDrawScreen(mBGWindow);
	DxLib::ClearDrawScreen();

	DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DxLib::DrawGraph(0, 0, pDataManageIns.GetDataHandle(mBGHandle), TRUE);

	DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
	m_controlManager.Draw(m_data, pDataManageIns, mBGWindow);

	DxLib::SetDrawScreen(DX_SCREEN_BACK);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
	DxLib::DrawGraph(0, 0, mBGWindow, FALSE);
	return true;
}

CGameState_ControlMakerMain::~CGameState_ControlMakerMain() {
	delete m_pFlowManager;
}
