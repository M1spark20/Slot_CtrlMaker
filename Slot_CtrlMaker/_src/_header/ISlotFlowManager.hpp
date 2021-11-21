#pragma once
#include "ESlotFlowFlag.hpp"
struct SSlotGameDataWrapper;

class ISlotFlowManager {
public:
	virtual bool Init(SSlotGameDataWrapper& pGameData) = 0;
	virtual ESlotFlowFlag Process(SSlotGameDataWrapper& pGameData) = 0;
	virtual ~ISlotFlowManager(){}
};
