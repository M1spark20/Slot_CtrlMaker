#pragma once
#include "SReachCollectionData.hpp"
#include <deque>
class CGameDataManage;
class CRestoreManagerRead;
class CRestoreManagerWrite;
class CReelManager;
class CSlotInternalDataManager;

class CSlotReachCollectionData {
	SReachCollectionData mCollectionData;
	int mListImageID;
	int mSecretImageID;
	std::vector<SReachCollectionElement*> mLatch;
	std::deque<SReachCollectionElement*> mRecentComp;
	bool mLatchFlag;

	bool DrawColleElement(const SReachCollectionElement& pData, int pPos, bool pIsLastComp, int pOffsetX, int pOffsetY, int pFontHandle) const;

public:
	bool Init(const CGameDataManage& pDataManager, int pListFileID, int reelNum);
	bool JudgeCollection(const CReelManager& pReelData, const int pFirstStop, const std::vector<int> pStopPos);
	bool Latch(bool isRefresh);
	void SetLatchFlag(bool flag) { mLatchFlag = flag; }

	int GetCollectionNum() const { return mCollectionData.elem.size(); }
	int GetCompCount(int pIndex) const;
	std::string GetFirstCompDate(int pIndex) const;
	std::string GetLastCompDate(int pIndex) const;
	bool Draw(int pBeginPos, int pOffsetX, int pOffsetY, int pFontHandle) const;

	bool ReadRestore(CRestoreManagerRead& pReader);
	bool WriteRestore(CRestoreManagerWrite& pWriter) const;

	size_t GetLatchNum() const { return mLatch.size(); }
	int GetLatchReachID(int pIndex) const;
};
