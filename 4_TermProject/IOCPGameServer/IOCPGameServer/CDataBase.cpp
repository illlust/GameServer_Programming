#include <unordered_map>
#include "CDataBase.h"

struct DATABASE
{
	int id;
	char Name[20];
	int x, y;
};

void show_error() {
	printf("error\n");
}

/************************************************************************
/* HandleDiagnosticRecord : display error/warning information /*
/* Parameters:
/*      hHandle ODBC handle
/*      hType Type of handle (SQL_HANDLE_STMT, SQL_HANDLE_ENV, SQL_HANDLE_DBC)
/*      RetCode Return code of failing command
/************************************************************************/
void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode) {
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

	if (RetCode == SQL_INVALID_HANDLE)
	{
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS)
	{
		// Hide data truncated.. 
		if (wcsncmp(wszState, L"01004", 5))
		{
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

/*
void LoadData() {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;
	SQLWCHAR szUser_Name[NAME_LEN];
	SQLINTEGER dUser_id, dUser_x, dUser_y;

	setlocale(LC_ALL, "korean");

	SQLLEN cbName = 0, cbID = 0, cbX, cbY = 0;

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					printf("ODBC Connect OK \n");

					//SELECT 다음에 오는건 항목들, FROM 다음에 오는건 테이블 이름, ORDER BY 다음에 오는건 sorting 할 것. 지금은 user_name으로 정렬.
					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_alluserid 0", SQL_NTS);
					//C:\Program Files\Microsoft SQL Server\MSSQL15.MSSQLSERVER\MSSQL\DATA
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						printf("Select OK \n");

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &dUser_id, 100, &cbID);
						retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szUser_Name, NAME_LEN, &cbName);
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &dUser_x, 100, &cbX);
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &dUser_y, 100, &cbY);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
							{
								show_error();
								HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
							}
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								wprintf(L"%d: %d %s %d , %d\n", i + 1, dUser_id, szUser_Name, dUser_x, dUser_y);
							}
							else
								break;
						}
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}
				else
					HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);

			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

	wprintf(L"database end \n");
}

*/