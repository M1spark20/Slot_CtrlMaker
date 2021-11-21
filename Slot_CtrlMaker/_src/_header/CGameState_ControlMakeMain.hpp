#pragma once
#include "IGameStateBase.hpp"
#include "SSlotGameDataWrapper.hpp"

class ISlotFlowManager;
class CGameDataManage;

class CGameState_ControlMakerMain : public IGameStateBase {

	SSlotGameDataWrapper	m_data;

	ISlotFlowManager*		m_pFlowManager;
	int						mDisplayW, mDisplayH;
	int						mBGWindow;

public:
	bool Init(CGameDataManage& pDataManageIns) override;
	EChangeStateFlag Process(CGameDataManage& pDataManageIns, bool pExtendResolution) override;
	bool Draw(CGameDataManage& pDataManageIns) override;
	~CGameState_ControlMakerMain();
};