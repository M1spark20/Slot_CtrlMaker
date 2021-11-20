#include "DxLib.h"
#include "_header/gameloop.h"
#include "_header/keyexport.h"
#include "_header/selectmode.h"

bool CGameLoop_S::StartGameLoop(bool pExtendResolution){
// [act]ゲームのメインループを作る。キー入力監視をする。
// [ret]ゲームループが正常に動作したかを返す。
	CKeyInput_S& KeyIn_Ins = CKeyInput_S::GetInstance();
	CSelectMode_S& GameIns = CSelectMode_S::GetInstance();
	CKeyExport_S& KeyOutIns = CKeyExport_S::GetInstance();
	bool Check=true,Ans=true;
	while( Check && !DxLib::ProcessMessage() && !DxLib::ClearDrawScreen() ){
		clsDx();
		Check =  KeyIn_Ins.GetKeyState();
		Check &= GameIns.MainLoopProcess(Ans, pExtendResolution);
		ScreenFlip();
	}
	return Ans;
}
