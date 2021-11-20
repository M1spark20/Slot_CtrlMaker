#pragma once
#include <string>
enum EMainReadFileType{
// [act]ゲーム本体で読み込むファイルの種類を指定
//		どれで読み込むかはCSVのTypeで指定
	Graphic, Sound, Material, Text, Binary, Copy, ColorMap, Font
};
struct SMainReadFileIndex{
// [act]ゲーム本体で読み込むファイルの情報群の基底構造体
	EMainReadFileType FileType;
	std::string FileName;
	int DataReadSyncFlag;
	int DataHandle;
	int SoundStreamingFlag;
	unsigned long long FileSize;
};
