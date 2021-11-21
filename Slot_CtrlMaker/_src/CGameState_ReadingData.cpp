#include "_header/CGameState_ReadingData.hpp"
#include "_header/CGameDataManage.h"
#include "_header/CGetSysDataFromCSV.hpp"
#include "DxLib.h"

bool CGameState_ReadingData::Init(CGameDataManage& p_pDataManageIns){
// [act]何もしない。読み込みデータ一覧はCGameDataManageの初期化時に規定。
//		このクラスではその読み込み完了をDxLib関数によって監視するだけ。
	CGetSysDataFromCSV sysReader;
	sysReader.FileInit(p_pDataManageIns.GetDataHandle(0));
	mSystemFont = p_pDataManageIns.GetDataHandle(sysReader.GetSysDataID("systemFont"));
	return true;
}
EChangeStateFlag CGameState_ReadingData::Process(CGameDataManage& p_pDataManageIns, bool pExtendResolution){
// [act]ファイルの読み込みが終わったか確認し、
//		終わっていればシーンをプレイ画面に移す。
// [ret]次回のゲームモード(終了時にプレイ画面へ。)
	if (DxLib::GetASyncLoadNum() == 0){
		if (!p_pDataManageIns.CheckReadFile()) return EChangeStateFlag::eStateErrEnd;
		return EChangeStateFlag::eStateMainStart;
	}
	return EChangeStateFlag::eStateContinue;
}
bool CGameState_ReadingData::Draw(CGameDataManage& p_DataManageIns){
// [act]ロード中であることを知らせる画面の描画を行います。
// [ret]関数が成功したかどうか。
	DxLib::DrawStringToHandle(0, 0, u8"NowLoading...", 0xFFFFFF, mSystemFont);
	return true;
}
