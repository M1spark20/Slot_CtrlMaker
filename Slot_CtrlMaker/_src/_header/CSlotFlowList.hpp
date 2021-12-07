#pragma once
#include "ISlotFlowManager.hpp"
#include <algorithm>

class CSlotFlowMakeControl1st : public ISlotFlowManager {
public:
	CSlotFlowMakeControl1st();
	bool Init(SSlotGameDataWrapper& pGameData) override;
	ESlotFlowFlag Process(SSlotGameDataWrapper& pGameData) override;
	bool Draw(SSlotGameDataWrapper& pGameData) override;
};