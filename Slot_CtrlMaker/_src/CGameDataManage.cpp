#include "DxLib.h"
#include "_header/CGameDataManage.h"
#include <cstdlib>
#include <sstream>
bool CGameDataManage::StartReadFile(ArrayIndex &InputData){
// [prm]p1;データインデックスの参照
// [act]データインデックスをコピーし、それぞれのファイルの読み込みを開始する
//		なお複製はファイル読み込み中に行うとエラーが返るようなので、
//		データチェック時にまとめて複製を行います。
	bool Ans=true;
	for(unsigned int MainC=0;MainC<InputData.size();MainC++){
		m_MainData.push_back(InputData[MainC]);
		const int GetSyncFlag=(m_MainData[MainC].DataReadSyncFlag == 0) ? FALSE:TRUE;
		// 0でfalse、それ以外ならtrue。非同期読み込み設定
		DxLib::SetUseASyncLoadFlag(GetSyncFlag);
		const std::string nowReadFileName = m_MainData[MainC].FileName;
		m_MainData[MainC].FileSize = DxLib::FileRead_size(nowReadFileName.c_str());
		if (m_MainData[MainC].FileSize == 0){
			ErrorLogFmtAdd(u8"IndexNo: %d, Name: \"%s\"が空か見つかりません。", MainC, nowReadFileName.c_str());
		}
		switch(m_MainData[MainC].FileType){
		case Graphic:
			// プログラム内で自由に書き換え可能
			m_MainData[MainC].DataHandle = 
				DxLib::LoadGraph(nowReadFileName.c_str());
			break;
		case ColorMap:
			m_MainData[MainC].DataHandle =
				DxLib::LoadSoftImage(nowReadFileName.c_str());
			break;
		case Sound:
			SetCreateSoundDataType(m_MainData[MainC].SoundStreamingFlag);
			m_MainData[MainC].DataHandle = 
				DxLib::LoadSoundMem(nowReadFileName.c_str());
			break;
		case Material:
			m_MainData[MainC].DataHandle = 
				DxLib::MV1LoadModel(nowReadFileName.c_str());
			break;
		case Text:
		case Binary:
			// AsyncのフラグをTRUEとするとメモリリークが発生したので省略
			// SetUseASyncLoadFlag()で既に非同期読み込みの設定は完了しているかも?
			m_MainData[MainC].DataHandle = 
				DxLib::FileRead_open(nowReadFileName.c_str());
			break;
		case Font:
			m_MainData[MainC].DataHandle =
				DxLib::LoadFontDataToHandle(nowReadFileName.c_str());
		case Copy:
			// 処理は読み込み後に実行することになります
			break;
		default:
			ErrorLogFmtAdd(u8"IndexNo.%dのType値が異常です。",MainC);
			Ans = false;
		}
	}
	DxLib::SetUseASyncLoadFlag(FALSE);
	return Ans;
}
bool CGameDataManage::CheckReadFile(){
// [act]全ての非同期ファイル読み込みが終わった後に関数を呼び出す。
//		ファイルが正常に読み込めたかチェックする。
//		複製指定されたファイルを複製する。
// [ret]全ての非同期読み込みファイルが正常に読み込めたかどうか
	bool Ans=true;
	for(unsigned int MainC=0;MainC<m_MainData.size();MainC++){
		if(m_MainData[MainC].FileType!=Copy){
			if(m_MainData[MainC].DataReadSyncFlag == 0) continue;
			const int Handle = m_MainData[MainC].DataHandle;
			if(CheckHandleASyncLoad(Handle) == -1){
				DxLib::ErrorLogFmtAdd(u8"IndexNo.%d,ファイル名\"%s\"の読み込みに失敗しました。",
					MainC, m_MainData[MainC].FileName.c_str());
				m_MainData[MainC].DataHandle = -1;
				Ans = false;
			}
		} else {
			int ForkID;
			/* ForkID定義 */{
				std::stringstream Temp(m_MainData[MainC].FileName);
				Temp>>ForkID;
			}
			if(ForkID<0||ForkID>=(signed)MainC){
				DxLib::ErrorLogFmtAdd(u8"IndexNo.%dのCopy元IDが異常です。",MainC);
				Ans = false;
				break;
			}
			switch(m_MainData[ForkID].FileType){
			case Graphic: 
			case ColorMap:	//複製機能がないためハンドルをコピー
				m_MainData[MainC].DataHandle = m_MainData[ForkID].DataHandle;
				DxLib::ErrorLogFmtAdd(u8"IndexNo.%dのCopyが無効です。同じハンドルを渡します。",MainC);
				break;
			case Sound:
				// 制約違反で複製に失敗したら-1が返ってくるって私信じてる(((
				m_MainData[MainC].DataHandle = 
					DxLib::DuplicateSoundMem(m_MainData[ForkID].DataHandle);
				if(m_MainData[MainC].DataHandle == -1){
					m_MainData[MainC].DataHandle = m_MainData[ForkID].DataHandle;
					DxLib::ErrorLogFmtAdd(u8"IndexNo.%dのCopyに失敗しました。同じハンドルを渡します。",MainC);
				}
				break;
			case Material:
				m_MainData[MainC].DataHandle =
					DxLib::MV1DuplicateModel(m_MainData[ForkID].DataHandle);
				break;
			case Text:
			case Binary: //複製機能がないためハンドルをコピー
			case Font:
				m_MainData[MainC].DataHandle = m_MainData[ForkID].DataHandle;
				DxLib::ErrorLogFmtAdd(u8"IndexNo.%dのCopyが無効です。同じハンドルを渡します。",MainC);
				break;
			default:
				DxLib::ErrorLogFmtAdd(u8"IndexNo.%dのCopy元IDが異常です。",MainC);
				Ans = false;
				break;
			}
		}
	}
	return Ans;
}
int CGameDataManage::GetDataHandle(unsigned int DataID) const{
// [act]指定されたデータIDのデータハンドルを返す
// [ret]指定されたIDのデータハンドル
//		引数が範囲外、または読み込みに失敗したファイルなら-1を返す
	if(DataID>=m_MainData.size()) return -1;
	return m_MainData[DataID].DataHandle;
}
const SMainReadFileIndex* CGameDataManage::GetDataIndex(unsigned int DataID) const{
// [act]指定されたデータIDのデータインデックスを返す
// [ret]指定されたIDのデータインデックスのポインタ
//		引数が範囲外ならNULLを返す
	if(DataID>=m_MainData.size()) return nullptr;
	return &(m_MainData[DataID]);
}
bool CGameDataManage::DeleteData(unsigned int DeletePos,EMainReadFileType DataType){
// [act]指定のタイプに対し、1番目の引数に格納されているデータを解放する
//		ただしコピーの場合は呼び出す前に対象のデータタイプを格納すること
//		コピータイプが引数に指定された場合は消去に失敗したとみなす。
	if(DeletePos >= m_MainData.size()){
		const char* Err = u8"ファイル解放時に不正なハンドルを受け取りました。\n";
		DxLib::ErrorLogAdd(Err);
		return false;
	}
	const int DataHandle = m_MainData.at(DeletePos).DataHandle;
	switch(DataType){
	case Graphic:
		DxLib::DeleteGraph(DataHandle);
		break;
	case ColorMap:
		DxLib::DeleteSoftImage(DataHandle);
		break;
	case Sound:
		DxLib::DeleteSoundMem(DataHandle);
		break;
	case Material:
		DxLib::MV1DeleteModel(DataHandle);
		break;
	case Text:
	case Binary:
		DxLib::FileRead_close(DataHandle);
		break;
	case Font:
		DxLib::DeleteFontToHandle(DataHandle);
		break;
	case Copy:
		const char* Err = u8"CopyフラグはDeleteDataでは解放できません。\n";
		DxLib::ErrorLogAdd(Err);
		return false;
		break;
	}
	return true;
}
CGameDataManage::~CGameDataManage(){
// [act]メインで読み込まれたデータの解放を行う
	DxLib::SetUseASyncLoadFlag(FALSE);
	for(unsigned int MainC=0;MainC<m_MainData.size();MainC++){
		if(m_MainData.at(MainC).FileType == Copy){
			std::stringstream Temp(m_MainData.at(MainC).FileName);
			int ForkID;
			Temp>>ForkID;
			DeleteData(MainC,m_MainData.at(ForkID).FileType);
		} else {
			DeleteData(MainC,m_MainData.at(MainC).FileType);
		}
	}
}
