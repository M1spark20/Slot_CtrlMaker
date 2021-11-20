#pragma once
#include "ISingleton.h"
class CGameLoop_S : public ISingleton<CGameLoop_S>{
// [act]ゲームのメインループを作る。キー入力監視をする。
//		シングルトンで管理されている。
	friend ISingleton<CGameLoop_S>;
	CGameLoop_S(){};
	~CGameLoop_S(){};
public:
	bool StartGameLoop(bool pExtendResolution);
};
