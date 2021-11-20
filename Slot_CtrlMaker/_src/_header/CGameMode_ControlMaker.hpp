#pragma once
#include "IGameModeBase.h"
class CGameDataManage;
class IGameStateBase;

class CGameMode_ControlMaker : public IGameModeBase {
public:
	bool Init() override;
	enum class EChangeModeFlag Process(bool pExtendResolution) override;
	~CGameMode_ControlMaker();
};