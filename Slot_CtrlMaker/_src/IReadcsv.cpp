#include <fstream>
#include <iostream>
#include "_header/IReadcsv.h"
#include "DxLib.h"
IReadCSVBase::IReadCSVBase(){
	m_ReadDataNowCurrent = 0;
}
bool IReadCSVBase::StartReadFile(const char* FileName){
// [act]メンバ変数m_ReadDataAllにファイルの文字列をすべて読み込む
// [ret]ファイルが読み込めたかどうか
	const int ReadLineStrMax = 8192;// 1行から読み込む最大の文字数
	DxLib::SetUseASyncLoadFlag(FALSE);
	const int fileHandle = FileRead_open(FileName);
	if (fileHandle == 0) {
		DxLib::ErrorLogFmtAdd(u8"CSVファイル\"%s\"が読み込めませんでした。", FileName);
		return false;
	}
	while (DxLib::FileRead_eof(fileHandle) == 0) {
		const char str = (char)DxLib::FileRead_getc(fileHandle);
		m_ReadDataAll += str;
	}
	m_ReadDataAll += '\0';

	return DxLib::FileRead_close(fileHandle) == 0;
}
void IReadCSVBase::GetStrSplitByComma(StringArr& StrArray){
// [prm]p1;戻り値として扱う。各配列に左からコンマで区切られた文字列が入る。
// [act]文字列を次の行までコンマで区切って、それを各配列に代入して返す。
//		コメントアウトは記述されずに次の行まで読み飛ばされる。
//		コメント行は空の文字列の物が1つpush_backされる。
	bool PastSlash=false;
	std::string NowStr;
	while(true){
		if(PastSlash&&m_ReadDataAll[m_ReadDataNowCurrent]!='/'){
			NowStr += '/';
			PastSlash=false;
		}
		switch(m_ReadDataAll[m_ReadDataNowCurrent]){
		case '\n':	// 改行
		case '\0':	// データの終わりの場合もここで処理
			StrArray.push_back(NowStr);
			m_ReadDataNowCurrent++;
			return;
		case '/':
			if(PastSlash){
				StrArray.push_back(NowStr);
				do { 
					m_ReadDataNowCurrent++;
					if(DataEOF())break;
				}while(m_ReadDataAll[m_ReadDataNowCurrent-1]!='\n');
				return;
			}
			PastSlash = true;
			break;
		case ',':
			StrArray.push_back(NowStr);
			NowStr="";
			break;
		default:
			NowStr += m_ReadDataAll[m_ReadDataNowCurrent];
			break;
		}
		m_ReadDataNowCurrent++;
	}
}
int IReadCSVBase::GetNowCurrent() const{
	return m_ReadDataNowCurrent;
}
void IReadCSVBase::SetNowCurrent(int NewCurrent){
	m_ReadDataNowCurrent = NewCurrent;
}
