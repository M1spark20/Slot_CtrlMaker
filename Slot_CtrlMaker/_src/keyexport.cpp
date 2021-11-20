#include "_header/keyexport.h"
#include "DxLib.h"
CKeyExport_S::CKeyExport_S(){
	for(int i=0;i<256;i++)
		m_NowKeyState[i]=0;
	mStatus = EKeyExportStatus::eGameMain;
}
void CKeyExport_S::KeyStateUpdate(char* pInput, const STouchInputList& pTouch){
// [prm]p1;キー入力情報が入った変数のポインタ(m1で指定したサイズの配列であること)
// [act]CKeyInput_Sからのアクセス専用。m2を更新する。
	bool isPush[GetInputArrayMax_C];
	// キーボード入力コピー
	for (int i = 0; i < GetInputArrayMax_C; i++) 
		isPush[i] = (pInput[i] != 0);

	for (size_t i = 0; i < pTouch.size(); ++i) {
		if (mStatus == EKeyExportStatus::eGameMain) {
			if (CheckTouchRange(pTouch[i], 103, 256, 372, 501)) { isPush[KEY_INPUT_3] = true; continue; }
			if (CheckTouchRange(pTouch[i], 103, 256, 524, 653)) { isPush[KEY_INPUT_2] = true; continue; }
			if (CheckTouchRange(pTouch[i], 103, 256, 676, 805)) { isPush[KEY_INPUT_1] = true; continue; }
			if (CheckTouchRange(pTouch[i], 283, 628, 318, 9999)) { isPush[KEY_INPUT_LEFT] = true; continue; }
			if (CheckTouchRange(pTouch[i], 629, 974, 318, 9999)) { isPush[KEY_INPUT_DOWN] = true; continue; }
			if (CheckTouchRange(pTouch[i], 975, 1320, 318, 9999)) { isPush[KEY_INPUT_RIGHT] = true; continue; }
			if (CheckTouchRange(pTouch[i], 0, 1602, 318, 9999)) { isPush[KEY_INPUT_UP] = true; continue; }
			if (CheckTouchRange(pTouch[i], 1611, 1914, 990, 1063)) { isPush[KEY_INPUT_M] = true; continue; }
		}
		if (mStatus == EKeyExportStatus::eMenu) {
			if (CheckTouchRange(pTouch[i], 171, 250, 190, 269)) { isPush[KEY_INPUT_LEFT] = true; continue; }
			if (CheckTouchRange(pTouch[i], 1051, 1130, 190, 269)) { isPush[KEY_INPUT_RIGHT] = true; continue; }
			if (CheckTouchRange(pTouch[i], 1251, 1330, 290, 369)) { isPush[KEY_INPUT_UP] = true; continue; }
			if (CheckTouchRange(pTouch[i], 1251, 1330, 810, 889)) { isPush[KEY_INPUT_DOWN] = true; continue; }
			if (CheckTouchRange(pTouch[i], 1351, 1430, 190, 269)) { isPush[KEY_INPUT_M] = true; continue; }
		}
	}

	for (int i = 0; i < GetInputArrayMax_C; i++) {
		if (isPush[i]) m_NowKeyState[i]++;
		else m_NowKeyState[i] = 0;
	}
}
bool CKeyExport_S::ExportKeyState(int KeyHandle,int LessFrame, int MaxFrame) const{
// [prm]p2;下限キーフレーム数(これ以上ならtrue)
// [act]指定キーがp2フレーム以上押されているか返す
//		p1が異常値( 0～m1-1 以外)ならfalseを返す。
	if(KeyHandle<0||KeyHandle>=GetInputArrayMax_C) return false;
	bool Ans = (m_NowKeyState[KeyHandle]>=LessFrame);
	Ans &= (m_NowKeyState[KeyHandle]<=MaxFrame);
	return Ans;
}
int CKeyExport_S::ExportKeyStateFrame(int KeyHandle) const{
// [act]指定のキーが連続で押されているフレーム数を返す。
//		p1が異常値なら-1を返す。
	if(KeyHandle<0||KeyHandle>=GetInputArrayMax_C) return -1;
	return m_NowKeyState[KeyHandle];
}
bool CKeyExport_S::CheckTouchRange(const STouchInput& pTouch, int xLow, int xHigh, int yLow, int yHigh) {
	if (pTouch.x < xLow || pTouch.x > xHigh) return false;
	if (pTouch.y < yLow || pTouch.y > yHigh) return false;
	return true;
}
