#pragma once
#include "EChangeModeFlag.h"
#include "CGameDataManage.h"
#include "IGameStateBase.hpp"

class IGameModeBase{
// [act]ゲーム全体でのモード(メニューなど)のステートパターン用基底クラス
protected:
	CGameDataManage*	m_pGameDataManage;
	IGameStateBase*		m_pGameStateManage;

	bool StartReadFile(const std::string pReadListFileName);

public:
	IGameModeBase()
		: m_pGameDataManage(nullptr),
		  m_pGameStateManage(nullptr){};
	virtual bool Init()=0;
	virtual EChangeModeFlag Process(bool pExtendResolution)=0;
	virtual ~IGameModeBase(){
		delete m_pGameDataManage;
		delete m_pGameStateManage;
	}
};
