#include "stdafx.h"

void errorDisplay(const int errNum, const char* msg)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, errNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
	std::wcout << errNum << " [" << msg << " Error] " << lpMsgBuf << std::endl;
	//while (true);
	LocalFree(lpMsgBuf);
}
