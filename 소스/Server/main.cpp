﻿#include "stdafx.h"
#include "main.h"

int main()
{
	std::wcout.imbue(std::locale("korean"));

	if (g_networkFramework.OnInit_iocp()) return 1;
	
}
