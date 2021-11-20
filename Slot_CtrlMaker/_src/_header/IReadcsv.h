#pragma once
#include <sstream>
#include <vector>
typedef std::vector<std::string> StringArr;
class IReadCSVBase {
// [act]csv読み込み制御クラス作成時の基底クラス
protected:
// 読み込んだファイルの文字列を格納する
	std::string m_ReadDataAll;
// 現在の読み込みカレントの場所
	unsigned int m_ReadDataNowCurrent;
	IReadCSVBase();
	bool StartReadFile(const char* FileName);
	void GetStrSplitByComma(StringArr& StrArray);
	int GetNowCurrent() const;
	void SetNowCurrent(int NewCurrent);
	bool DataEOF() const{
		return (m_ReadDataNowCurrent>=m_ReadDataAll.size()-1);
	}
	template<class T> void StrToNum(T& InputFor,std::string &p_Data){
// [act]テンプレート型、1つ目の引数に2つ目の文字列から抽出した数字を代入します
//		bool型の場合は整数で判別し、0がfalse、それ以外がtrueとなる模様です
		std::istringstream Data(p_Data);
		Data>>InputFor;
	};

	virtual ~IReadCSVBase(){};
};
