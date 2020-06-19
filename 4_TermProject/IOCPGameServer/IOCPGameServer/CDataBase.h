#pragma once

// SQLBindCol_ref.cpp  
// compile with: odbc32.lib  
#include <windows.h>  
#include <stdio.h>  
#include <locale.h>
#define UNICODE  
#include <sqlext.h>  
#include <unordered_map>


#define NAME_LEN 20  

void show_error();

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

void LoadData();
