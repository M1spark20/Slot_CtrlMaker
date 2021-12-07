#pragma once
#include "ESlotFlowFlag.hpp"
struct SSlotGameDataWrapper;

class ISlotFlowManager {
protected:
	static const int DRAW_SQ_SIZE   = 26;	// Draw時四角のサイズ
	static const int DRAW_STOP_SIZE =  6;	// Draw時停止有無のサイズ

public:
	virtual bool Init(SSlotGameDataWrapper& pGameData) = 0;
	virtual ESlotFlowFlag Process(SSlotGameDataWrapper& pGameData) = 0;
	virtual bool Draw(SSlotGameDataWrapper& pGameData) = 0;
	virtual ~ISlotFlowManager(){}
};
