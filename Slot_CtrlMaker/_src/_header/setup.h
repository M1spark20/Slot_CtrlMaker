#pragma once
#include "ISingleton.h"
class CSetup_S : public ISingleton<CSetup_S>{
// [act]DXライブラリ初期化処理。異常終了時はメッセージをログに残す
//		シングルトンで管理されている
	friend ISingleton<CSetup_S>;
	CSetup_S(){};
	~CSetup_S(){};
public:
	bool StartSetup(bool pIsMiniSize);
};
