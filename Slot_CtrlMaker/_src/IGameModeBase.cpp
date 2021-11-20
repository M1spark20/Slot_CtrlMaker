#include "_header/IGameModeBase.h"
#include "_header/CReadDataListGeterFromCSV.hpp"

bool IGameModeBase::StartReadFile(const std::string pReadListFileName){
	m_pGameDataManage = new CGameDataManage;
	CReadDataListGeterFromCSV reader;
	CGameDataManage::ArrayIndex index;
	if (!reader.FileInit(pReadListFileName)) return false;
	if (!reader.MakeData(index)) return false;
	return m_pGameDataManage->StartReadFile(index);
}
