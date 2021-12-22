#include "DxLib.h"
#include "_header/setup.h"
#include "_header/gameloop.h"
#include <new>

bool CSetup_S::StartSetup(bool pIsMiniSize){
// [act]ライブラリ初期化(ウィンドウモード)
//		ランタイムエラーor容量確保エラー時のログ書き込み
// [ret]プログラムの正常/異常終了フラグ
	DxLib::SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);	// UTF-8にてゲームを処理する
	DxLib::SetAlwaysRunFlag(TRUE);							// アクティブじゃないときも処理を行う

	DxLib::SetGraphMode(1366, 720, 32);					// 画面サイズ初期設定
#ifdef __ANDROID__
#elif __APPLE__
#else
	DxLib::ChangeWindowMode(1);								// ウィンドウモードに
	DxLib::SetFullSceneAntiAliasingMode(3,2);				// 3D描画のクオリティを指定
	if(pIsMiniSize) DxLib::SetWindowSizeExtendRate(0.5);	// ミニサイズフラグ指定
#endif
	if(DxLib::DxLib_Init()) return false;					// DxLib初期化処理

	bool isExtend = false;
#if __ANDROID__
	int wx, wy;
	DxLib::GetAndroidDisplayResolution(&wx, &wy);
	isExtend = wx >= (int)floorf(wy * 1.95f);
#elif __APPLE__
	int wx, wy;
	DxLib::GetDisplayResolution_iOS(&wx, &wy);
	isExtend = wx >= (int)floorf(wy * 1.95f);
#endif

	if(isExtend) DxLib::SetGraphMode(2160, 1080, 32);		// Extentionありのとき、画面サイズ変更
	DxLib::SetDrawScreen(DX_SCREEN_BACK);					// 裏画面書き込み
	DxLib::SetUseZBuffer3D(TRUE);							// Ｚバッファを有効にする
	DxLib::SetWriteZBuffer3D(TRUE);							// Ｚバッファへの書き込みを有効にする
	DxLib::SetDrawMode(DX_DRAWMODE_BILINEAR);				// バイリニア法による拡大縮小を行う

	bool Result = true;
	try {
		CGameLoop_S& LoopIns=CGameLoop_S::GetInstance();
		Result = LoopIns.StartGameLoop(isExtend);
	} catch(std::bad_alloc){
		DxLib::ErrorLogAdd(u8"メモリ確保時に問題が発生しました。\n");
		Result = false;
	}
	if(!Result)
		DxLib::ErrorLogAdd(u8"ソフトが強制終了しました。\n");
	DxLib_End();
	return Result;
}
