#pragma once
#include <string>
#include <fstream>
#include <vector>

class IRestoreManager {
protected:
	static const int VERSION = 4;
	const std::string cFileName;
	std::string GetDefaultFilePath() const;

public:
	IRestoreManager() : cFileName("/playData.sav") {}
};

class CRestoreManagerRead : public IRestoreManager {
	std::ifstream mIfs;
	int mDataVersion;

public:
	bool StartRead();
	bool ReadStr(std::string& pInputFor);
	template<class T> bool ReadNum(T& pInputFor) {
		mIfs.read((char*)&pInputFor, sizeof(pInputFor));
		return (bool)mIfs;
	}
	template<> bool ReadNum(bool& pInputFor) {
		char data = 0x0;
		mIfs.read(&data, sizeof(pInputFor));
		if(!mIfs) return false;
		pInputFor = (data != 0);
		return true;
	}
	template<class T, class U> bool ReadNum(std::pair<T, U>& pInputFor) {
		if (!ReadNum(pInputFor.first)) return false;
		if (!ReadNum(pInputFor.second)) return false;
		return true;
	}
	void CloseRead();
	bool IsSameDataVersion() { return mDataVersion == VERSION; }
	int GetReadVer() const { return mDataVersion; }
};

class CRestoreManagerWrite : public IRestoreManager {
	std::ofstream mOfs;
	unsigned char mCheckSum;
	bool mActivateFlag;
	bool mResetFlag;

public:
	CRestoreManagerWrite() {
		mCheckSum = 0x00;
		mActivateFlag = false;
		mResetFlag = true;
	}

	bool IsActivate();
	void ActivateFlagReset();
	void SetActivate();
	bool Process();

	bool StartWrite();
	bool WriteStr(std::string pStr);
	template<class T> bool WriteNum(T pValue) {
		mOfs.write((char*)&pValue, sizeof(pValue));
		if (!mOfs) return false;

		// チェックサム加算
		const unsigned char* dataPtr = (unsigned char*)&pValue;
		for (unsigned int i = 0; i < sizeof(pValue); ++i) {
			if (i > 0) ++dataPtr;
			unsigned char data = *dataPtr & 0xFF;
			mCheckSum = (mCheckSum + data) & 0xFF;
		}
		return true;
	}
	template<> bool WriteNum(bool pValue) {
		const char pushVal = pValue ? 0x1 : 0x0;
		return WriteNum(pushVal);
	}
	template<class T, class U> bool WriteNum(std::pair<T, U> pValue) {
		if (!WriteNum(pValue.first)) return false;
		if (!WriteNum(pValue.second)) return false;
		return true;
	}
	bool Flush();
};
