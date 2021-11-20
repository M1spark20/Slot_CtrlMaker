#pragma once
#include "EChangeStateFlag.h"
// クラスの先行宣言
class CGameDataManage;

class IGameStateBase {
	// [act]ゲーム本体のモード選択用の基底クラス
public:
	virtual bool Init(CGameDataManage& pDataManageIns) = 0;
	virtual EChangeStateFlag Process(CGameDataManage& pDataManageIns, bool pExtendResolution) = 0;
	virtual bool Draw(CGameDataManage& pDataManageIns) = 0;
	virtual ~IGameStateBase(){};
};
