#pragma once
#include "stdafx.h"

#define NAME_LEN 20

int try_login_db(char* mess);
void show_error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);