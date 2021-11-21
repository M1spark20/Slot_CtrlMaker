#include "_header/ErrClass.hpp"
#include "DxLib.h"

ErrUndeclaredVar::ErrUndeclaredVar(std::string varName) : mInvalidVarName(varName) {
}

void ErrUndeclaredVar::WriteErrLog() {
	const std::string str =
		"Invalid Effect Variable name: \"" + mInvalidVarName + "\" has called.";
	DxLib::ErrorLogFmtAdd(str.c_str());
}

ErrLessCSVDefinition::ErrLessCSVDefinition(std::vector<std::string> data, int required) :
	mErrArray(data), requiredNum(required) {
}

void ErrLessCSVDefinition::WriteErrLog() {
	std::string str =
		"Less CSV definition - num:" + std::to_string(mErrArray.size()) +
		" / required:" + std::to_string(requiredNum);
	DxLib::ErrorLogFmtAdd(str.c_str());
	str = std::string("LineData: ");
	for (auto data : mErrArray) str += data + ", ";
	DxLib::ErrorLogFmtAdd(str.c_str());
}

ErrInternalVarUndeclared::ErrInternalVarUndeclared(std::string varName) : mInvalidVarName(varName) {
}

void ErrInternalVarUndeclared::WriteErrLog() {
	const std::string str =
		"Internal Variable: \"" + mInvalidVarName + "\" was empty or null.";
	DxLib::ErrorLogFmtAdd(str.c_str());
}

ErrIllegalCSVDefinition::ErrIllegalCSVDefinition(int pRowCount, std::string pDefinition)
	: mRowCount(pRowCount), mCSVDefinition(pDefinition) {}

void ErrIllegalCSVDefinition::WriteErrLog() {
	const std::string str =
		"Illegal CSV definition(Line:" + std::to_string(mRowCount) + ") : " + mCSVDefinition;
	DxLib::ErrorLogFmtAdd(str.c_str());

}

void ErrDivideBy0::WriteErrLog() {
	const std::string str =
		"Divide by 0 has detected while calcurating user variable.";
	DxLib::ErrorLogFmtAdd(str.c_str());
}
