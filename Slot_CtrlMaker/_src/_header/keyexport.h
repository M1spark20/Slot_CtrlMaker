#pragma once
#include "ISingleton.h"
#include "keyinput.h"
#include "STouchInputList.hpp"

enum class EKeyExportStatus {
	eGameMain,
	eMenu,
	eStateMax
};

class CKeyExport_S : public ISingleton<CKeyExport_S>{
// [act]各クラスへのキー入力状態を出力
//		シングルトンで管理されている(_Sの意味)
	friend ISingleton<CKeyExport_S>;
// CKeyInput_SでKeyStateUpdate関数を使用させるためのfriend宣言
	friend CKeyInput_S;
	CKeyExport_S();
	~CKeyExport_S(){};
	void KeyStateUpdate(char* pInput, const STouchInputList& pTouch);
	bool CheckTouchRange(const STouchInput& pTouch, int xLow, int xHigh, int yLow, int yHigh);

// DxLibのキーボード取得時の配列マジックナンバー(m1)
	static const int GetInputArrayMax_C = 256;
// キーが連続で入力されているフレーム(m2)
	int m_NowKeyState[GetInputArrayMax_C];
// キーボード入力モード(ゲーム, メニュー, etc...)
	EKeyExportStatus mStatus;

public:
	bool ExportKeyState(int KeyHandle,int LessFrame=1,int MaxFrame=1) const;
	int ExportKeyStateFrame(int KeyHandle) const;
	void SetExportStatus(EKeyExportStatus pSetStatus) { mStatus = pSetStatus; }
	EKeyExportStatus GetExportStatus() const { return mStatus; }
};
