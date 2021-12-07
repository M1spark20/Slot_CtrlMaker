#include "_header/CSlotFlowList.hpp"

CSlotFlowMakeControl1st::CSlotFlowMakeControl1st() {
}

bool CSlotFlowMakeControl1st::Init(SSlotGameDataWrapper& pGameData) {
	return true;
}

ESlotFlowFlag CSlotFlowMakeControl1st::Process(SSlotGameDataWrapper& pGameData) {
	return ESlotFlowFlag::eFlowContinue;
}

bool CSlotFlowMakeControl1st::Draw(SSlotGameDataWrapper& pGameData) {
	return true;
}