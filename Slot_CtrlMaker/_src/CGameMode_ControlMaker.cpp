#include "_header/CGameMode_ControlMaker.hpp"
//#include "_header/CGameState_ReadingData.hpp"
//#include "_header/CGameState_SlotGameMain.hpp"
#include "_header/keyexport.h"
#include "DxLib.h"

bool CGameMode_ControlMaker::Init() {
	//if (!StartReadFile("data/launchData/gameMain.act")) return false;
	//m_pGameStateManage = new CGameState_ReadingData;
	//return m_pGameStateManage->Init(*m_pGameDataManage);
	return true;
}

enum class EChangeModeFlag CGameMode_ControlMaker::Process(bool pExtendResolution) {
	/*EChangeStateFlag state = m_pGameStateManage->Process(*m_pGameDataManage, pExtendResolution);
	switch (state) {
	case eStateContinue:
	case eStateEnd:
		break;
	case eStateMainStart:
		delete m_pGameStateManage;
		m_pGameStateManage = nullptr;
		m_pGameStateManage = new CGameState_SlotGameMain;
		if (!m_pGameStateManage->Init(*m_pGameDataManage)) {
			DxLib::ErrorLogAdd(u8"ゲーム本体の初期化中にエラーが発生しました。");
			return eModeErrEnd;
		}
		break;
	case eStateErrEnd:
		DxLib::ErrorLogAdd(u8"ゲーム本体の処理中にエラーが発生しました。");
		return eModeErrEnd;
	default:
		DxLib::ErrorLogAdd(u8"不明なStateハンドルを取得しました。");
		return eModeErrEnd;
		break;
	}
	if (!m_pGameStateManage->Draw(*m_pGameDataManage)) {
		DxLib::ErrorLogAdd(u8"ゲーム画面描画中にエラーが発生しました。");
		return eModeErrEnd;
	}*/

	// テスト移行
	/*CKeyExport_S& key = CKeyExport_S::GetInstance();
	if (key.ExportKeyState(KEY_INPUT_F1)) return eModeControlTest;*/

	//return state == eStateEnd ? eModeEnd : eModeContinue;
	DxLib::DrawString(0, 0, "ControlMaker Test", 0xFFFF00);
	return EChangeModeFlag::eModeContinue;
}
CGameMode_ControlMaker::~CGameMode_ControlMaker() {
	// deleteはベースクラスで
}