#pragma once
#include "CReelManager.hpp"
#include "CRandManager.hpp"
#include "CSlotTimerManager.hpp"
#include "CSlotCastChecker.hpp"
#include "CRestoreManager.hpp"
#include "CSlotControlManager.hpp"
#include "CReelChecker.hpp"

struct SSlotGameDataWrapper{
	CReelManager				reelManager;
	CRandManager				randManager;
	CSlotTimerManager			timeManager;
	CSlotCastChecker			castChecker;
	CRestoreManagerWrite		restoreManager;
	CReelChecker				reelChecker;
};
