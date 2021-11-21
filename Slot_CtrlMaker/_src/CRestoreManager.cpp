#include "_header/CRestoreManager.hpp"
#include "DxLib.h"
 
std::string IRestoreManager::GetDefaultFilePath() const {
	char filePath[512] = "\0";
#ifdef __ANDROID__
	GetInternalDataPath(filePath, sizeof(filePath));
#elif __APPLE__
	GetLibraryPreferencesDirPath(filePath, sizeof(filePath));
#else
	strcpy_s(filePath, ".");
#endif
	return std::string(filePath);
}

bool CRestoreManagerRead::StartRead() {
	mIfs.open(GetDefaultFilePath() + cFileName, std::ios::binary);
	if (!mIfs) return false;

	// チェックサム確認
	unsigned char checkSum = 0x00;
	while (mIfs.eof()) {
		unsigned char data;
		ReadNum(data);
		checkSum = (checkSum + data) & 0xFF;
	}
	if (checkSum != 0x00) return false;

	// mDataVersion読み込み, 初期位置設定
	mIfs.clear();
	mIfs.seekg(0);
	ReadNum(mDataVersion);

	return true;
}

bool CRestoreManagerRead::ReadStr(std::string& pInputFor) {
	char data = '\0';
	size_t dataSize = 0;
	if (!ReadNum(dataSize)) return false;
	for (size_t i = 0; i < dataSize; ++i) {
		if(!ReadNum(data)) return false;
		pInputFor += data;
	}
	return true;
}

void CRestoreManagerRead::CloseRead() {
	mIfs.close();
	mIfs.clear();
}


bool CRestoreManagerWrite::IsActivate() {
	const bool ans = (mActivateFlag && !mResetFlag);
	if(ans) mResetFlag = true;
	mActivateFlag = false;
	return ans;
}

void CRestoreManagerWrite::ActivateFlagReset() {
	mResetFlag = false;
}

void CRestoreManagerWrite::SetActivate() {
	mActivateFlag = true;
}

bool CRestoreManagerWrite::StartWrite() {
	mOfs.open(GetDefaultFilePath() + cFileName, std::ios::binary|std::ios::trunc);
	if (!mOfs) return false;

	mCheckSum = 0x00;
	WriteNum(VERSION);
	return true;
}

bool CRestoreManagerWrite::WriteStr(std::string pStr) {
	char data;
	if (!WriteNum((size_t)pStr.size())) return false;
	for (size_t i = 0; i < pStr.size(); ++i) {
		data = pStr[i];
		if(!WriteNum(data)) return false;
	}
	return true;
}

bool CRestoreManagerWrite::Flush() {
	const unsigned char checkSum = ~mCheckSum + (unsigned char)1;
	if (!WriteNum(checkSum)) return false;
	if (mCheckSum != 0x00) return false;
	mOfs.close();
	mOfs.clear();
	return true;
}
