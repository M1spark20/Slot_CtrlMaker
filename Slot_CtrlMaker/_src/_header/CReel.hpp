#pragma once
#include <vector>
#include <algorithm>
#include "SReelChaData.hpp"
#include "SReelDrawData.hpp"
class CGameDataManage;
class CSlotTimerManager;
class IImageSourceManager;
class CImageColorManager;
class CImageColorController;
class CRestoreManagerRead;
class CRestoreManagerWrite;

enum class EReelStatus{
	eInitial,
	eStoping,
	eAccerating,
	eRotating,
	eSliping,
	eReelStatusMax
};

class CReel {
// [act]リール1個ずつのクラス
	SReelChaData					m_reelData;
	float							m_rotatePos;
	float							m_pushPos;
	float							m_speed;
	float							m_speedMax;
	float							m_accVal;
	unsigned int					m_comaPos;
	unsigned int					m_destination;
	long long						m_lastRotationTime;
	EReelStatus						m_nowStatus;
	EReelStatus						m_lastStatus;

	bool			DrawReelMain(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas, unsigned int pStartComa, bool pIsFixed) const;

public:
	bool			Init(const SReelChaData& pReelData);
	bool			ReelStart();
	bool			ReelStop(unsigned int pDest, bool pForceFlag);
	EReelStatus		GetReelStatus() const { return m_nowStatus; }
	float			GetReelRotatePos() const { return m_rotatePos; }
	int				GetReelPos() const { return m_comaPos; }
	unsigned int	GetReelID() const { return m_reelData.reelID; };
	size_t			GetComaNum() const { return m_reelData.arrayData.size(); }
	int				GetReelComaByReelPos(size_t pOffset) const;
	int				GetReelComaByFixedPos(int pComaID) const;
	int				GetReelDetailPos() const;
	int				GetSlipCount() const;

	bool			Process(CSlotTimerManager& pTimer);
	bool			DrawReel(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas) const;
	bool			DrawReel(const CGameDataManage& pDataManager, SReelDrawData pData, int pCanvas, unsigned int pComaStart) const;
};
