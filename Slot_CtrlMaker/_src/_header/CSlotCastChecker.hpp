#pragma once
#include "SCastData.hpp"
#include <vector>
class CGameDataManage;
struct SSlotGameDataWrapper;
class CSlotInternalDataManager;

class CSlotCastChecker {
	SCastData						m_castData;
	unsigned int					m_checkedBet;
	std::vector<const SPayData*>	m_castList;
	int								m_reachSoundID;

public:
	bool	Init(CGameDataManage& pData, int pFileID);
	bool	SetReachData(const SSlotGameDataWrapper& pData, int pBetNum, int pGameMode);
	bool	SetCastData(const SSlotGameDataWrapper& pData, int pBetNum, int pGameMode);
	void	ResetCastData();
	bool	IsReplay();
	int		GetPayout();
	int		GetReachSoundID() const;
	int		GetPayoutEffect() const;
	int		GetPayoutLineID() const;
};
