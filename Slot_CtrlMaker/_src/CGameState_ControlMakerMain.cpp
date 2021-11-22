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
	/*if (m_data.restoreManager.IsActivate()) {
		if (!m_data.restoreManager.StartWrite()) return eStateErrEnd;
		if (!m_data.internalDataManager.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.dataCounter.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.reelManager.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.timeManager.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.effectManager.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.reachCollection.WriteRestore(m_data.restoreManager)) return eStateErrEnd;
		if (!m_data.restoreManager.Flush()) return eStateErrEnd;
	}*/

	return EChangeStateFlag::eStateContinue;
}

bool CGameState_ControlMakerMain::Draw(CGameDataManage& pDataManageIns) {
	DxLib::SetDrawScreen(mBGWindow);
	DxLib::ClearDrawScreen();

	SReelDrawData drawReel;
	/* SReelDrawData初期作成 */ {
		drawReel.y = 1;
		drawReel.comaW = 77; drawReel.comaH = 35; drawReel.comaNum = 3;
		drawReel.offsetLower = 10; drawReel.offsetUpper = 10;
	}
	for (size_t i = 0; i < 3; ++i) {
		drawReel.x = 502 + 78 * i; drawReel.reelID = i;
		m_data.reelManager.DrawReel(pDataManageIns, drawReel, mBGWindow);
		drawReel.x = 768 + 78 * i;
		m_data.reelManager.DrawReel(pDataManageIns, drawReel, mBGWindow);
	}
	/*m_data.effectManager.Draw(pDataManageIns, m_data.timeManager, mBGWindow);
	m_data.effectManager.RingSound(m_data.timeManager, pDataManageIns);
	m_menuManager.Draw(mBGWindow);*/

	DxLib::SetDrawScreen(DX_SCREEN_BACK);
	DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
	DxLib::DrawGraph(0, 0, mBGWindow, FALSE);
	return true;
}

CGameState_ControlMakerMain::~CGameState_ControlMakerMain() {
	delete m_pFlowManager;
}
