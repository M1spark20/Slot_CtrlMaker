#pragma once
// State Pattern & Singleton
#include "ISingleton.h"
#include "EChangeModeFlag.h"
// クラスの先行宣言
class IGameModeBase;
class CSelectMode_S : public ISingleton<CSelectMode_S>{
// [act]ゲームの現在のモードを参照し、それぞれのスタートクラスへ処理を渡す。
//		ステートパターン・シングルトンで管理されている。
	friend ISingleton<CSelectMode_S>;
	CSelectMode_S();
	~CSelectMode_S();
// 現在のモードのインスタンスのポインタ(m1)
	IGameModeBase* m_pNowMode; 
public:
	bool MainLoopProcess(bool& Ans, bool pExtendResolution);
};
