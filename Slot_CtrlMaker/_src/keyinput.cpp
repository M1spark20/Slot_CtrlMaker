#include "DxLib.h"
#include "_header/keyinput.h"
#include "_header/keyexport.h"
#include "_header/STouchInputList.hpp"

bool CKeyInput_S::GetKeyState(){
// [act]キーステート取得(DxLib::GetHitKeyStateAll)
//		結果をCKeyExport_Sクラスへ伝播(ExIns.KeyStateUpdate)
// [ret]関数が成功したか

	// DxLibのキーボード取得時の配列マジックナンバー
	const int GetInputArrayMax_C = 256;
	char KeyState[GetInputArrayMax_C];

	// キー入力(iOSでは実施しない)
#ifndef __APPLE__
	if(DxLib::GetHitKeyStateAll(KeyState)) return false;
#else
	for (auto i = 0; i < GetInputArrayMax_C; ++i) KeyState[i] = 0;
#endif

	// タッチスクリーン(KeyStateの更新はキーコンフィグの将来対応のためExport側に実装)
	STouchInputList touchList;
	const int touchNum = DxLib::GetTouchInputNum();
	touchList.resize(touchNum);
	for (size_t i = 0; i < touchList.size(); ++i)
		if (DxLib::GetTouchInput(i, &touchList[i].x, &touchList[i].y, &touchList[i].ID, &touchList[i].dev) == -1) return false;

	// Exportへ出力(friendクラス)
	CKeyExport_S& ExIns=CKeyExport_S::GetInstance();
	ExIns.KeyStateUpdate(KeyState, touchList);
	return true;
}
