#pragma once
#include "IGameStateBase.hpp"
class CGameDataManage;

class CGameState_ReadingData : public IGameStateBase{
// [act]ゲーム画面のグラッフィック読み込み関数
	int mSystemFont;
public:
	bool Init(CGameDataManage& pDataManageIns) override;
	EChangeStateFlag Process(CGameDataManage& p_pDataManageIns, bool pExtendResolution)override;
	bool Draw(CGameDataManage& p_DataManageIns) override;
};
