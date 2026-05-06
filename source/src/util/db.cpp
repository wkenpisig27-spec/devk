#include "util.h"
#include "db.h"

#if defined(__linux__)
#include <cstring> // strlen

// ODBC Driver 18 for SQL Server on Linux does not support SQL_C_DEFAULT (value 99)
// in SQLBindParameter. We must explicitly map SQL data types to their C equivalents.
static SQLSMALLINT MapSqlTypeToC(SQLSMALLINT sqlType) {
    switch (sqlType) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
#ifdef SQL_WCHAR
        case SQL_WCHAR:       // -8 on Linux (same as SQL_SS_WCHAR)
        case SQL_WVARCHAR:    // -9 on Linux (same as SQL_SS_WVARCHAR)
        case SQL_WLONGVARCHAR: // -10 on Linux (same as SQL_SS_WLONGVARCHAR)
#else
        case -8:  // SQL_SS_WCHAR
        case -9:  // SQL_SS_WVARCHAR
        case -10: // SQL_SS_WLONGVARCHAR
#endif
            return SQL_C_CHAR;
        case SQL_INTEGER:
            return SQL_C_SLONG;
        case SQL_SMALLINT:
            return SQL_C_SSHORT;
        case SQL_TINYINT:
            return SQL_C_STINYINT;
        case SQL_BIGINT:
            return SQL_C_SBIGINT;
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return SQL_C_DOUBLE;
        case SQL_REAL:
            return SQL_C_FLOAT;
        case SQL_BIT:
            return SQL_C_BIT;
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            return SQL_C_BINARY;
        case SQL_NUMERIC:
        case SQL_DECIMAL:
            return SQL_C_CHAR; // Bind as string, let driver convert
        case SQL_TYPE_DATE:
            return SQL_C_TYPE_DATE;
        case SQL_TYPE_TIME:
            return SQL_C_TYPE_TIME;
        case SQL_TYPE_TIMESTAMP:
            return SQL_C_TYPE_TIMESTAMP;
        default:
            return SQL_C_CHAR; // Safe fallback for unknown types
    }
}

static bool IsStringCType(SQLSMALLINT cType) {
    return (cType == SQL_C_CHAR || cType == SQL_C_BINARY);
}
#endif // __linux__

#define SQLERR_FORMAT "SQL Error State:%s, Native Error Code: %lX, ODBC Error: %s\n"
#define COLTRUNC_WARNG "Number of columns in display truncated to %u\n"
#define NULLDATASTRING ""

#define SQL_SUC(sr) (((sr == SQL_SUCCESS) || (sr == SQL_SUCCESS_WITH_INFO) || (sr == SQL_NO_DATA_FOUND)) ? true : false)
#define DBFAIL(sr) ((sr != SQL_SUCCESS) && (sr != SQL_SUCCESS_WITH_INFO))


//TODO: Could be loaded from .CFG 
#define LOG_UTIL_DB_ENABLE 0
#ifdef _DEBUG
#define LOG_UTIL_DB_ENABLE 1
#endif

const char g_cchLogUtilDb = LOG_UTIL_DB_ENABLE;

cfl_db::cfl_db() : _henv(SQL_NULL_HENV), _hdbc(SQL_NULL_HDBC), _rslist()
{
    _connected = false;
    _dump_errinfo = false;
	_connstr = "";

	_rslist.clear();
}

cfl_db::~cfl_db()
{
    if (_connected)
    {
        disconn();
		_rslist.clear();
        _connected = false;
    }
}

void cfl_db::add(cfl_rs* rs)
{
	if (rs == nullptr) return;

	vector<cfl_rs *>::iterator it = _rslist.begin();
	bool found = false;
	while (it != _rslist.end())
	{
		if (rs == *(it)) {found = true; break;}
		++ it;
	}

	if (!found) _rslist.push_back(rs);
}

bool cfl_db::handle_err(SQLHANDLE h, SQLSMALLINT t,
                        RETCODE r, char const* sql /* = nullptr */,
						bool reconn /* = false */)
{
	//if (!_connected) return;

#define DBLOG printf
    SQLRETURN sqlret;
    SQLCHAR state[SQL_SQLSTATE_SIZE + 1] = {0};
    SQLINTEGER error;
	SQLCHAR msg[SQL_MAX_MESSAGE_LENGTH + 1] = {0};
    SQLSMALLINT msg_len;

	sqlret = SQLGetDiagRec(t, h, 1, state, &error, msg, SQL_MAX_MESSAGE_LENGTH, &msg_len);
    if (sqlret == SQL_ERROR || sqlret == SQL_INVALID_HANDLE) return false;

	// Always log SQL errors unconditionally so DB issues are visible in production
	LG2("util_db_error", SQLERR_FORMAT, state, error, msg);
	if (sql != nullptr)
	{
		LG2("util_db_error", "[STMT:0x%x][SQLERR]: [%s]\n", h, sql);
	}

	if (_dump_errinfo && g_cchLogUtilDb == 1)
	{
		LG2("util_db", SQLERR_FORMAT, state, error, msg);
		if (sql != nullptr)
			LG2("util_db", "[STMT:0x%x][SQLERR]: [%s]\n", h, sql);
	}

	// Reconnect...
    if (reconn && _needreconn((char const *)state))
	{
		return _reconnt();
	}
	return false;
}

bool cfl_db::_needreconn(char const* state)
{
	bool ret = false;

	// Only reconnect for actual connection failures, NOT for query timeouts or warnings.
	// HYT00 = query timeout - the connection is likely fine, just a slow/blocked query.
	// 01000 = general warning - too broad, should not trigger a full reconnection.
	// HYT01 = connection timeout - connection may be dead, reconnect.
	// 08S01 = communication link failure - connection is dead, reconnect.
	if (strcmp(state, "HYT01") == 0) ret = true;
	else if (strcmp(state, "08S01") == 0) ret = true;

	if (ret)
		LG2("util_db_error", "[DB RECONNECT] Triggered by SQLSTATE: %s\n", state);

	return ret;
}

bool cfl_db::_reconnt()
{
	string err_info;
    cfl_rs* tmp = nullptr;
	constexpr int MAX_RECONNECT_RETRIES = 10; // Max 10 seconds of blocking

	printf( "connecting database ...\n" );
	LG2("util_db_error", "connecting database...\n");
    vector<cfl_rs *>::iterator it = _rslist.begin();
    while (it != _rslist.end())
    {
        tmp = (cfl_rs *)*(it);
		LG2("util_db_error", "notify %p the disconn message\n", tmp);
        tmp->notify_discon(this);

        ++ it;
    }

	disconn();

	printf( "ready connect database...\n" );
	LG2("util_db_error", "ready connect database...\n");

	int retryCount = 0;
	while (!_connect(err_info))
	{
		if (++retryCount >= MAX_RECONNECT_RETRIES)
		{
			printf("reconnect database failed after %d retries!\n", MAX_RECONNECT_RETRIES);
			LG2("util_db_error", "reconnect database FAILED after %d retries - giving up to avoid game loop freeze\n", MAX_RECONNECT_RETRIES);
			return false; // Don't block the game loop forever
		}
		LG2("util_db_error", "reconnect database failed (attempt %d/%d): %s\n", retryCount, MAX_RECONNECT_RETRIES, err_info.c_str());
		Sleep(1000);
		printf( "reconnect database... (attempt %d/%d)\n", retryCount, MAX_RECONNECT_RETRIES );
	}

	printf( "reconnect database success!\n" );
	LG2("util_db_error", "reconnect database success!\n");

	it = _rslist.begin();
	while (it != _rslist.end())
	{
		tmp = (cfl_rs *)*(it);
		LG2("util_db_error", "notify %p the reconnect message\n", tmp);
		tmp->notify_reconn(this);

		++ it;
	}

	printf( "reconnect database ok!\n" );
	LG2("util_db_error", "reconnect database ok!\n");
	return true;
}

bool cfl_db::_connect(string& errinfo)
{
	bool ret = true;
	SQLRETURN sqlret;
	char outbuf[2048];
	SQLSMALLINT outlen = 0;

	do
	{
		sqlret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_henv);
		if (sqlret == SQL_ERROR)
		{
			errinfo = "Unable to allocate an environment handle\n";
			ret = false;
			break;
		}

		// Let ODBC know this is an ODBC 3.0 application
		sqlret = SQLSetEnvAttr(_henv, SQL_ATTR_ODBC_VERSION,
			                   (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
		if (DBFAIL(sqlret))
		{
			handle_err(_henv, SQL_HANDLE_ENV, sqlret);

			errinfo = "Error in calling SQLSetEnvAttr\n";
			SQLFreeHandle(SQL_HANDLE_ENV, _henv);
			ret = false;
			break;
		}

		sqlret = SQLAllocHandle(SQL_HANDLE_DBC, _henv, &_hdbc);
		if (DBFAIL(sqlret))
		{
			handle_err(_henv, SQL_HANDLE_ENV, sqlret);

			errinfo = "Unable to allocate an dbc handle\n";
			SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
			SQLFreeHandle(SQL_HANDLE_ENV, _henv);
			ret = false;
			break;
		}

		//SQLSetConnectAttr(_hdbc, SQL_COPT_SS_MARS_ENABLED, (SQLPOINTER)SQL_MARS_ENABLED_YES, SQL_IS_UINTEGER);


		sqlret = SQLDriverConnectA(_hdbc, nullptr, (SQLCHAR *)_connstr.c_str(),
								  SQL_NTS, (SQLCHAR *)outbuf, sizeof outbuf,
								  &outlen, SQL_DRIVER_NOPROMPT);
		if (DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

            errinfo = "Unable to connect database\n";
			SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
			SQLFreeHandle(SQL_HANDLE_ENV, _henv);
			ret = false;
			break;
		}

		//char command[256];

		//sprintf(command, "use %s", DATABASE);
		//if (!SQL_SUCCEEDED(SQLExecDirect(_hdbc, (SQLCHAR *) command, SQL_NTS))) 
		//{
		//	handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

  //          errinfo = "Unable to connect database\n";
		//	SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
		//	SQLFreeHandle(SQL_HANDLE_ENV, _henv);
		//	ret = false;
		//	break;
		//}

        //handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

		//int v;
		//SQLGetConnectAttr(_hdbc, SQL_ATTR_CONNECTION_TIMEOUT,(SQLPOINTER)(&v), 0 , nullptr);
        sqlret = SQLSetConnectAttr(_hdbc, SQL_ATTR_CONNECTION_TIMEOUT,
								   (void *)10, 0);

		//SQLGetConnectAttr(_hdbc, SQL_ATTR_CONNECTION_TIMEOUT,(SQLPOINTER)(&v), 0 , nullptr);
		if (DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

            errinfo = "Unable set timeout to database\n";
			SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
			SQLFreeHandle(SQL_HANDLE_ENV, _henv);
			ret = false;
			break;
		}

		//exec_sql_direct(_openDatabase.c_str());

		// Set connect flag...
		_connected = true;

		// Set a global lock timeout to prevent indefinite lock waits
		// This applies to all queries, not just explicit transactions
		{
			SQLHSTMT hstmt_lt = SQL_NULL_HSTMT;
			SQLRETURN ltret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt_lt);
			if (!DBFAIL(ltret)) {
				ltret = SQLExecDirectA(hstmt_lt, (SQLCHAR*)"SET LOCK_TIMEOUT 5000", SQL_NTS);
				SQLFreeHandle(SQL_HANDLE_STMT, hstmt_lt);
			}
		}

	}
	while (0);

	return ret;
}

bool cfl_db::connect(char* source, char* userid, char* passwd, string& err_info)
{
    if (_connected)
    {
        err_info = "Already connected\n";
        return false;
    }

	bool ret = true;
	SQLRETURN sqlret;

    do
    {
        sqlret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_henv);
        if (sqlret == SQL_ERROR)
        {
            err_info = "Unable to allocate an environment handle\n";
            ret = false;
            break;
        }

        // Let ODBC know this is an ODBC 3.0 application
        sqlret = SQLSetEnvAttr(_henv, SQL_ATTR_ODBC_VERSION,
                               (SQLPOINTER)SQL_OV_ODBC3,
                               SQL_IS_INTEGER);
        if (DBFAIL(sqlret))
        {
            handle_err(_henv, SQL_HANDLE_ENV, sqlret);

            err_info = "Error in calling SQLSetEnvAttr\n";
            SQLFreeHandle(SQL_HANDLE_ENV, _henv);
            ret = false;
            break;
        }

        sqlret = SQLAllocHandle(SQL_HANDLE_DBC, _henv, &_hdbc);
        if (DBFAIL(sqlret))
        {
            handle_err(_henv, SQL_HANDLE_ENV, sqlret);

            err_info = "Unable to allocate an dbc handle\n";
            SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, _henv);
            ret = false;
            break;
        }

        sqlret = SQLConnect(_hdbc, (SQLCHAR *)source, SQL_NTS,
                            (SQLCHAR *)userid, SQL_NTS,
						    (SQLCHAR *)passwd, SQL_NTS);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

            err_info = "Unable to connect database\n";
            SQLFreeHandle(SQL_HANDLE_DBC, _hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, _henv);
            ret = false;
            break;
        }

        handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

        // Set connect flag...
        _connected = true;
    }
	while (0);

    return ret;
}

bool cfl_db::connect(const char* servername, const char* database, const char* userid, const char* passwd,
                     string& err_info)
{
    if (_connected)
    {
        err_info = "Already connected\n";
        return false;
    }

	char conn_str[1024];    
    //sprintf(conn_str, "DRIVER={SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s",
#ifdef _WIN32
	_snprintf_s(conn_str, sizeof(conn_str), _TRUNCATE, "DRIVER={SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s",
            servername, userid, passwd, database);
#else
	_snprintf_s(conn_str, sizeof(conn_str), _TRUNCATE, "DRIVER={ODBC Driver 18 for SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s;TrustServerCertificate=yes;Encrypt=optional",
            servername, userid, passwd, database);
#endif

	// Save connect string
	_connstr = conn_str;

	// Add by lark.li 20080902 begin
	_openDatabase = string("use ") + string(database) + string(";");
	// End

	return _connect(err_info);
}

void cfl_db::disconn()
{
    if (_connected)
    {
        SQLDisconnect(_hdbc);

        SQLFreeHandle(SQL_HANDLE_DBC, _hdbc); _hdbc = SQL_NULL_HDBC;

        SQLFreeHandle(SQL_HANDLE_ENV, _henv); _henv = SQL_NULL_HENV;

        _connected = false;
    }
}

SQLRETURN cfl_db::exec_sql_direct(char const* sql, unsigned short timeout /* = 5 */)
{
	if (!_connected) return SQL_ERROR;

RECONNECT:

	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;

	do
	{
		sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		if (DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			break;
		}

        sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
		if (DBFAIL(sqlret))		
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			break;
		}

		sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
		switch (sqlret)
		{
		case SQL_SUCCESS_WITH_INFO:
			//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
			// fall through
		case SQL_SUCCESS:
			break;

		case SQL_ERROR:
			{
				if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			}
			return 0;

		default:
			if (g_cchLogUtilDb == 1)
				LG2("util_db", "SQLExecDirect return %d\n", sqlret);
		}
	}
	while (0);

	if (hstmt != SQL_NULL_HSTMT) SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	return sqlret;
}

bool cfl_db::begin_tran()
{
#if 1
	if (!_connected) return false;

	// Set lock timeout to 5 seconds (5000 milliseconds) to prevent indefinite deadlocks
	// This ensures transactions don't hang forever waiting for locks
	const char* timeout_sql = "SET LOCK_TIMEOUT 5000";
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLRETURN sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
	if (DBFAIL(sqlret)) {
		return false;
	}
	
	sqlret = SQLExecDirectA(hstmt, (SQLCHAR*)timeout_sql, SQL_NTS);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	
	if (DBFAIL(sqlret)) {
		LG("db_error", "Failed to set lock timeout");
		// Continue anyway - timeout setting failure shouldn't block transactions
	}

	sqlret = SQLSetConnectAttr(_hdbc, SQL_ATTR_AUTOCOMMIT,
										 (SQLPOINTER)SQL_AUTOCOMMIT_OFF,
                                         SQL_IS_POINTER);
	return (DBFAIL(sqlret)) ? false : true;
#endif

    return true;
}

bool cfl_db::commit_tran()
{
#if 1
	if (!_connected) return false;

	SQLEndTran(SQL_HANDLE_DBC, _hdbc, SQL_COMMIT);

	SQLRETURN sqlret = SQLSetConnectAttr(_hdbc, SQL_ATTR_AUTOCOMMIT,
										 (SQLPOINTER)SQL_AUTOCOMMIT_ON,
                                         SQL_IS_POINTER);
	return (DBFAIL(sqlret)) ? false : true;
#endif

    return true;
}

bool cfl_db::rollback()
{
#if 1
	if (!_connected) return false;

	SQLEndTran(SQL_HANDLE_DBC, _hdbc, SQL_ROLLBACK);

	SQLRETURN sqlret = SQLSetConnectAttr(_hdbc, SQL_ATTR_AUTOCOMMIT,
										 (SQLPOINTER)SQL_AUTOCOMMIT_ON,
										 SQL_IS_POINTER);
	return (DBFAIL(sqlret)) ? false : true;
#endif

    return true;
}



cfl_rs::cfl_rs(cfl_db* db) :
	_db(db), _hdbc(SQL_NULL_HDBC), _hstmt(SQL_NULL_HSTMT), _max_col(MAX_COL)
{
	attach_db(db);
	db->add(this);
}

cfl_rs::cfl_rs(cfl_db* db, char const* tbl_name, int max_col)
    : _db(db), _hdbc(SQL_NULL_HDBC), _hstmt(SQL_NULL_HSTMT), _tbl_name(tbl_name)
{
	attach_db(db);
	db->add(this);

    _max_col = max_col;
    _col_num = 0;
    _row_num = 0;
    _param_num = 0;
}

cfl_rs::~cfl_rs()
{
    if (_hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, _hstmt);
        _hstmt = SQL_NULL_HSTMT;
    }

	_hdbc = SQL_NULL_HDBC;
    _db = nullptr;
}

void cfl_rs::attach_db(cfl_db* db)
{
	if (db != _db) return;

	_hdbc = db->get_dbc();
	//SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &_hstmt);
}

void cfl_rs::notify_discon(cfl_db* db)
{
    if (_db != db) return ;

#if 1
    if (_hstmt != SQL_NULL_HSTMT)
    {
		LG2("util_db_error", "notify_discon\n");
        SQLFreeHandle(SQL_HANDLE_STMT, _hstmt);
        _hstmt = SQL_NULL_HSTMT;
    }
#endif
}

void cfl_rs::notify_reconn(cfl_db* db)
{
	attach_db(db);
}

int cfl_rs::get_affected_rows()
{
	string	 l_buf[1];
	int		 l_affected_rows	=0;
	char const	*param		=" @@ROWCOUNT ";
	string	 l_tbl_name =_tbl_name;
	_tbl_name			="";
	bool l_ret = _get_row(l_buf,1,param,0,&l_affected_rows);
    _tbl_name			=l_tbl_name;
	if(!l_ret)
	{
		return -1;	//SQLï¿½ï¿½ï¿½ï¿½
	}else if(l_affected_rows !=1)
	{
		return -2;	//ï¿½ï¿½È¡Öµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	}
	return atoi(l_buf[0].c_str());
}
int cfl_rs::get_identity()
{
	string	 l_buf[1];
	int		 l_affected_rows	=0;
	char const	*param		=" ISNULL(@@IDENTITY,0) ";
	string	 l_tbl_name =_tbl_name;
	_tbl_name			="";
	bool l_ret = _get_row(l_buf,1,param,0,&l_affected_rows);
	_tbl_name			=l_tbl_name;
	if(!l_ret)
	{
		return -1;	//SQLï¿½ï¿½ï¿½ï¿½
	}else if(l_affected_rows !=1)
	{
		return -2;	//ï¿½ï¿½È¡Öµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	}
	return atoi(l_buf[0].c_str());
}

// Add bynlark.li 20080808 begin
bool cfl_rs::_get_bin_field(char* field_text, SQLLEN& len, char* param, char* filter, int* affect_rows)
{
    bool ret = false;
    char sql[SQL_MAXLEN];
    int i = 0;

    try {
        if (param == nullptr) {
            //sprintf(sql, "select * from %s", _tbl_name.c_str());
			_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select * from %s", _tbl_name.c_str());
        } else {
		    if (_tbl_name.length() !=0) {
	            //sprintf(sql, "select %s from %s", param, _tbl_name.c_str());
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s from %s", param, _tbl_name.c_str());
		    } else {
			    //sprintf(sql, "select %s",param);
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s",param);
		    }
        }

        if ((strstr(sql, "select") == nullptr) &&
            (strstr(sql, "select") == nullptr) &&
            (strstr(_tbl_name.c_str(), "(nolock)") == nullptr) &&
            (strstr(_tbl_name.c_str(), "(NOLOCK)") == nullptr)) {
            //strcat(sql, " (nolock) ");
				strncat_s(sql, sizeof(sql), " (nolock) ", _TRUNCATE);
        }

        if (filter == nullptr) {
        } else {
            //strcat(sql, " where ");
			strncat_s(sql, sizeof(sql), " where ", _TRUNCATE);
            //strcat(sql, filter);
			strncat_s(sql, sizeof(sql), filter, _TRUNCATE);
        }
    } catch (...) {
		if (g_cchLogUtilDb == 1)
	        LG2("util_db", "exception raised from _get_row exec\n");
        return false;
    }

	if (g_cchLogUtilDb == 1)
	    LG2("util_db", "_get_row [SQL]: [%s]\n", sql);

RECONNECT:

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    do
    {
        try {        
            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }
            } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row alloc\n"); break;}

            try {
			constexpr uint64_t timeout{10};
			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (DBFAIL(sqlret))
            {
                if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			    return false;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n"); break;}

        try {
            sqlret = SQLNumResultCols(hstmt, &col_num);

            col_num = min(col_num, 1);

            if (col_num <= 0)
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "col_num = 0 (<=0)\n", col_num);
                break;
            }

                SQLBindCol(hstmt, UWORD(1), SQL_C_BINARY, _buf[0], len, &_buf_len[0]);

        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row bind\n"); break;}

        try {
            sqlret = SQLFetch(hstmt); // only fetch the next row
            if (DBNODATA(sqlret))
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
                found = false;
            }
            else if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
                if (sqlret != SQL_SUCCESS_WITH_INFO)
                {
                    break;
                }
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row fetch\n"); break;}

        try {
            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
            if (found)
            {
                // È¡ï¿½ï¿½ï¿½ï¿½
				len = _buf_len[0];
                if (len == SQL_NULL_DATA)
                {
                    field_text[0] = 0x0;
                }
                else
                {
                    memcpy(field_text, _buf[0], len);
                }

                if (affect_rows != nullptr)
                    *affect_rows = 1;
            }
            else
            {
                // Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
                if (affect_rows != nullptr)
                    *affect_rows = 0;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row copydata\n"); break;}

        ret = true;

    }
	while (0);

    try {
        if (hstmt != SQL_NULL_HSTMT) {    
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row freestmt\n"); ret = false;}

    return ret;
}

SQLRETURN cfl_rs::cache_stored_procedure(int numberOfParams, char const* schema,char const* procedure) {

	SQLRETURN	   sqlret = SQL_SUCCESS;
	SQLHSTMT	   hstmt	= SQL_NULL_HSTMT;
	SQLSMALLINT	   col_num	= 0;
	unsigned short timeout	= 10;
	bool		   found	= true;
	SQLINTEGER	   data_len = SQL_NTS;
	
	// Special case: procedures with 0 parameters don't need SQLProcedureColumns lookup
	// Just cache an empty parameter vector and return success
	if(numberOfParams == 0)
	{
		std::vector<SQLParamData> empty_params;
		_cached_map.insert({std::string(procedure), empty_params});
		if(g_cchLogUtilDb == 1)
			LG2("util_db", "Cached 0-parameter procedure: [%s]\n", procedure);
		return SQL_SUCCESS;
	}
	
	do
	{
		try
		{
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if(DBFAIL(sqlret))
			{
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				break;
			}

		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from stored_procedure alloc\n");
			break;
		}
		try
		{
			constexpr auto timeout{10};
			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
			if(DBFAIL(sqlret))
			{
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				break;
			}
			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			MPTimer t1;
			t1.Begin();
			SQLSMALLINT tempDataType;
			SQLLEN		tempColumnSize;
			SQLLEN		templenDataType;
			SQLLEN		templenColumnSize;
			SQLSMALLINT tempColumnType;     // COLUMN_TYPE (col 5): 1=INPUT, 2=INOUT, 3=RESULT, 4=OUTPUT, 5=RETURN_VALUE
			SQLLEN		templenColumnType;

			SQLSMALLINT DataType[MAX_PARAM_NUM]	  = {0};
			SQLLEN		ColumnSize[MAX_PARAM_NUM] = {0};

			sqlret = SQLProcedureColumns(hstmt, nullptr, 0, (SQLCHAR*)schema, SQL_NTS, (SQLCHAR*)procedure, SQL_NTS,
										 (SQLCHAR*)"%", SQL_NTS);

			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			// Bind column 5 (COLUMN_TYPE) to distinguish return values from actual parameters
			sqlret = SQLBindCol(hstmt, 5, SQL_C_SHORT, &tempColumnType, sizeof(tempColumnType), &templenColumnType);
			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			sqlret = SQLBindCol(hstmt, 6, SQL_C_SHORT, &tempDataType, sizeof(tempDataType), &templenDataType);
			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}
#if defined(__linux__)
			// On Linux x64, SQLLEN is 8 bytes (int). Use SQL_C_SBIGINT to match.
			sqlret = SQLBindCol(hstmt, 8, SQL_C_SBIGINT, &tempColumnSize, sizeof(tempColumnSize), &templenColumnSize);
#else
			sqlret = SQLBindCol(hstmt, 8, SQL_C_LONG, &tempColumnSize, sizeof(tempColumnSize), &templenColumnSize);
#endif
			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			int paramIdx = 0;
			// Fetch up to numberOfParams+2 rows to handle possible return value row + all params
			for(int fetchCount = 0; fetchCount < numberOfParams + 2 && paramIdx < numberOfParams; fetchCount++)
			{
				tempColumnType = 0;
				sqlret = SQLFetch(hstmt);
				// SQL_NO_DATA is not an error - it means no more rows
				if(sqlret == SQL_NO_DATA)
				{
					sqlret = SQL_SUCCESS; // Reset to success - end of data is expected
					break;
				}
				if(DBFAIL(sqlret))
				{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
				}
				// Skip return value rows (COLUMN_TYPE = 5) and result column rows (COLUMN_TYPE = 3)
				if(tempColumnType == SQL_RETURN_VALUE || tempColumnType == SQL_RESULT_COL)
					continue;
				DataType[paramIdx]   = tempDataType;
				ColumnSize[paramIdx] = tempColumnSize;
				paramIdx++;
			}

#if defined(__linux__)
			// Log what we cached so we can diagnose binding issues
			LG2("util_db", "cache_stored_procedure [%s]: fetched %d params (expected %d)\n", procedure, paramIdx, numberOfParams);
			for(int d = 0; d < paramIdx; d++) {
				LG2("util_db", "  param[%d]: sql_type=%d, col_size=%lld\n", d, (int)DataType[d], (long long)ColumnSize[d]);
			}
			// Safety: if we got fewer params than expected or any type is 0, don't cache garbage
			bool hasInvalidType = false;
			for(int d = 0; d < paramIdx; d++) {
				if(DataType[d] == 0) { hasInvalidType = true; break; }
			}
			if(paramIdx < numberOfParams || hasInvalidType) {
				LG2("util_db", "cache_stored_procedure [%s]: incomplete/invalid data (got %d/%d, invalidType=%d) - NOT caching\n",
					procedure, paramIdx, numberOfParams, hasInvalidType);
				// Don't cache - let caller use fallback
				break;
			}
#endif

			std::vector<SQLParamData> vect_params;
			// Store to cache.
			for(int i = 0; i < paramIdx; i++)
			{
				SQLParamData param;
				param.sql_data_type = DataType[i];
				param.column_length = ColumnSize[i];
				vect_params.push_back(param);
			}
			_cached_map.insert({std::string(procedure), vect_params});

			if(DBFAIL(sqlret))
			{
				break;
			}

		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n");
			break;
		}
	} while(0);
	if(hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return sqlret;
	}




bool cfl_rs::_get_row_stored_procedure(string field_text[], int field_max_cnt, char const* sql, char const* schema, char const* procedure,
							 int* affect_rows, ...)

{
	bool ret = false;
	//char sql[SQL_MAXLEN];
	int i = 0;
	va_list arg_list;
	va_start(arg_list, affect_rows);
	int numberOfParameters = va_arg(arg_list, int);

	// Flag to indicate if we need to use fallback parameter binding
	bool use_fallback_params = false;
	// Fallback parameter data when cache lookup fails
	std::vector<SQLParamData> fallback_params;

    // Time complexity to find in an unordered_map: O(1) (average).
	std::vector<SQLParamData>* cached_params = nullptr;
	auto cache =  _cached_map.find(std::string(procedure));
	if (cache == _cached_map.end()) {
		// Procedure wasn't cached yet.
		cache_stored_procedure(numberOfParameters, schema, procedure);
		cache = _cached_map.find(std::string(procedure));

		if(cache != _cached_map.end())
		{
			cached_params = &(*cache).second;

		}
		else
		{
			// Cache lookup failed - use fallback default parameters (SQL_VARCHAR, 255 length)
			// This allows stored procedures to work even when SQLProcedureColumns doesn't return metadata
			LG2("util_db", "Could not find cached map for procedure: [%s] - using fallback VARCHAR params\n", procedure);
			use_fallback_params = true;
			for (int p = 0; p < numberOfParameters; p++) {
				SQLParamData param;
				param.sql_data_type = SQL_VARCHAR;
				param.column_length = 255;
				fallback_params.push_back(param);
			}
			cached_params = &fallback_params;
		}

	}
	else
	{
		cached_params = &(*cache).second;
		
		// Validate cached parameter count matches expected count
		if(cached_params && cached_params->size() != numberOfParameters)
		{
			LG2("util_db", "Parameter count mismatch for [%s]: expected %d, cached %d - invalidating cache\n", 
				procedure, numberOfParameters, (int)cached_params->size());
			_cached_map.erase(cache);
			cache_stored_procedure(numberOfParameters, schema, procedure);
			cache = _cached_map.find(std::string(procedure));
			if(cache != _cached_map.end())
			{
				cached_params = &(*cache).second;
			}
			else
			{
				// Cache still failed after re-caching - use fallback
				LG2("util_db", "Re-cache failed for [%s] - using fallback VARCHAR params\n", procedure);
				use_fallback_params = true;
				for (int p = 0; p < numberOfParameters; p++) {
					SQLParamData param;
					param.sql_data_type = SQL_VARCHAR;
					param.column_length = 255;
					fallback_params.push_back(param);
				}
				cached_params = &fallback_params;
			}
		}
	}
	//

	try
	{


	} catch(...)
	{
		if(g_cchLogUtilDb == 1)
			LG2("util_db", "exception raised from stored_procedure exec\n");
		return false;
	}

	if(g_cchLogUtilDb == 1)
		LG2("util_db", "stored_procedure[SQL]: [%s]\n", procedure);

	// the bit bellow is simply safe_sql, but because of variadic arguments we cant call it here.
RECONNECT:

	SQLRETURN	   sqlret;
	SQLHSTMT	   hstmt	= SQL_NULL_HSTMT;
	SQLSMALLINT	   col_num	= 0;
	unsigned short timeout	= 10;
	bool		   found	= true;
	SQLINTEGER	   data_len = SQL_NTS;
	do
	{
		try
		{
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if(DBFAIL(sqlret))
			{
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				break;
			}

		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from stored_procedure alloc\n");
			break;
		}
		try
		{
			constexpr auto timeout{10};
			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
			if(DBFAIL(sqlret))
			{
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				break;
			}
			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			if(DBFAIL(sqlret))
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
			}

			// Time to bind parameters.
#if defined(__linux__)
			SQLLEN paramLenIndicators[MAX_PARAM_NUM] = {0};
#endif
			for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
			{
				if (!cached_params) {

					break;

				}
				SQLSMALLINT sql_data_type = cached_params->at(i).sql_data_type;
				SQLULEN column_length = cached_params->at(i).column_length;
				const char* param_pointer = va_arg(arg_list, const char*);

#if defined(__linux__)
				SQLSMALLINT c_type = MapSqlTypeToC(sql_data_type);
				SQLLEN bufferLen = 0;
				if (IsStringCType(c_type)) {
					paramLenIndicators[i] = SQL_NTS;
					bufferLen = param_pointer ? (SQLLEN)strlen(param_pointer) + 1 : 0;
				}
				sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, sql_data_type, column_length, 0,
											(SQLPOINTER)param_pointer, bufferLen, &paramLenIndicators[i]);
#else
				sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, sql_data_type, column_length, 0,
											(SQLPOINTER)param_pointer, 0, 0);
#endif

				if(DBFAIL(sqlret))
				{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
				}
				
			}
			va_end(arg_list);

			if(DBFAIL(sqlret))
			{
				break;
			}
			
			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			switch(sqlret)
			{
			case SQL_SUCCESS_WITH_INFO:
				//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				// fall through
			case SQL_SUCCESS:
				break;

			case SQL_ERROR:
			{
				if(handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true))
				{
					goto RECONNECT;
				}
			}
			break;

			case SQL_NO_DATA: // update or delete
				break;

			case SQL_NEED_DATA:
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				break;
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n");
			break;
		}

		try
		{
			sqlret = SQLNumResultCols(hstmt, &col_num);

			col_num = min(col_num, field_max_cnt);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			if(col_num <= 0)
			{
				if(g_cchLogUtilDb == 1)
					LG2("util_db", "col_num = 0 (<=0)\n", col_num);
				break;
			}

			SQLLEN numRows;
			auto	   retCode = SQLRowCount(hstmt, &numRows);

			for(i = 0; i < col_num; ++i)
			{
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
						   MAX_DATALEN, &_buf_len[i]);
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row bind\n");
			break;
		}

		try
		{
			sqlret = SQLFetch(hstmt); // only fetch the next row
			if(DBNODATA(sqlret))
			{
				if(g_cchLogUtilDb == 1)
					LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
				found = false;
			} else if(sqlret != SQL_SUCCESS)
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				if(sqlret != SQL_SUCCESS_WITH_INFO)
				{
					break;
				}
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row fetch\n");
			break;
		}

		try
		{
			// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
			if(found)
			{
				// È¡ï¿½ï¿½ï¿½ï¿½
				for(i = 0; i < col_num; ++i)
				{
					if(_buf_len[i] == SQL_NULL_DATA)
					{
					  field_text[i] = NULLDATASTRING;
					} else
					{
					  field_text[i] = (char*)(_buf[i]);
					}
				}

				if(affect_rows != nullptr)
					*affect_rows = 1;
			} else
			{
				// Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
				if(affect_rows != nullptr)
					*affect_rows = 0;
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row copydata\n");
			break;
		}

		ret = true;

	} while(0);

	if(hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}
	return ret;
}

bool cfl_rs::_get_row_stored_procedure_whitespace(string field_text[], int field_max_cnt, char const* sql, char const* schema, char const* procedure,
									   int* affect_rows, ...)

{
		bool ret = false;
		int	 i = 0;

		va_list arg_list;
		va_start(arg_list, affect_rows);
		int numberOfParameters = va_arg(arg_list, int);

		// Flag to indicate if we need to use fallback parameter binding
		bool use_fallback_params = false;
		// Fallback parameter data when cache lookup fails
		std::vector<SQLParamData> fallback_params;

		// Time complexity to find in an unordered_map: O(1) (average).
		std::vector<SQLParamData>* cached_params = nullptr;
		auto					   cache		 = _cached_map.find(std::string(procedure));
		if(cache == _cached_map.end())
		{
		// Procedure wasn't cached yet.
		cache_stored_procedure(numberOfParameters, schema, procedure);
		cache = _cached_map.find(std::string(procedure));

		if(cache != _cached_map.end())
		{
			cached_params = &(*cache).second;

		} else
		{
			// Cache lookup failed - use fallback default parameters (SQL_VARCHAR, 255 length)
			LG2("util_db", "Could not find cached map for procedure: [%s] - using fallback VARCHAR params\n", procedure);
			use_fallback_params = true;
			for (int p = 0; p < numberOfParameters; p++) {
				SQLParamData param;
				param.sql_data_type = SQL_VARCHAR;
				param.column_length = 255;
				fallback_params.push_back(param);
			}
			cached_params = &fallback_params;
		}

		} else
		{
		cached_params = &(*cache).second;
		
		// Validate cached parameter count matches expected count
		if(cached_params && cached_params->size() != numberOfParameters)
		{
			LG2("util_db", "Parameter count mismatch for [%s]: expected %d, cached %d - invalidating cache\n", 
				procedure, numberOfParameters, (int)cached_params->size());
			_cached_map.erase(cache);
			cache_stored_procedure(numberOfParameters, schema, procedure);
			cache = _cached_map.find(std::string(procedure));
			if(cache != _cached_map.end())
			{
				cached_params = &(*cache).second;
			}
			else
			{
				// Cache still failed after re-caching - use fallback
				LG2("util_db", "Re-cache failed for [%s] - using fallback VARCHAR params\n", procedure);
				use_fallback_params = true;
				for (int p = 0; p < numberOfParameters; p++) {
					SQLParamData param;
					param.sql_data_type = SQL_VARCHAR;
					param.column_length = 255;
					fallback_params.push_back(param);
				}
				cached_params = &fallback_params;
			}
		}
		}
		//
		if(g_cchLogUtilDb == 1)
			LG2("util_db", "_get_row3 [SQL]: [%s]\n", sql);

	RECONNECT:

		// Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
		SQLRETURN	sqlret;
		SQLHSTMT	hstmt	= SQL_NULL_HSTMT;
		SQLSMALLINT col_num = 0;
		bool		found	= true;

		do
		{
			try
			{
				sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
				if(DBFAIL(sqlret))
				{
					handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
					break;
				}
			} catch(...)
			{
				if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 alloc\n");
				break;
			}

			try
			{
				constexpr auto timeout{10};
				sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
				if(DBFAIL(sqlret))
				{
					handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
					break;
				}


				// Time to bind parameters.
#if defined(__linux__)
				SQLLEN paramLenIndicators2[MAX_PARAM_NUM] = {0};
#endif
				for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
				{
					if(!cached_params)
					{
					break;
					}
					SQLSMALLINT sql_data_type = cached_params->at(i).sql_data_type;
					SQLULEN column_length = cached_params->at(i).column_length;
					const char* param_pointer = va_arg(arg_list, const char*);

#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(sql_data_type);
					SQLLEN bufferLen = 0;
					if (IsStringCType(c_type)) {
						paramLenIndicators2[i] = SQL_NTS;
						bufferLen = param_pointer ? (SQLLEN)strlen(param_pointer) + 1 : 0;
					}
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, sql_data_type, column_length, 0,
											  (SQLPOINTER)param_pointer, bufferLen, &paramLenIndicators2[i]);
#else
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, sql_data_type, column_length, 0,
											  (SQLPOINTER)param_pointer, 0, 0);
#endif

					if(DBFAIL(sqlret))
					{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
					}
				}
				va_end(arg_list);

				if(DBFAIL(sqlret))
				{
					break;
				}

				sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
				if(DBFAIL(sqlret))
				{
					if(handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true))
					{
					  goto RECONNECT;
					}
					return false;
				}
			} catch(...)
			{
				if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 exec\n");
				break;
			}

			try
			{
				sqlret = SQLNumResultCols(hstmt, &col_num);

				col_num = min(col_num, field_max_cnt);
				col_num = min(col_num, MAX_COL);
				col_num = min(col_num, _max_col);

				if(col_num <= 0)
				{
					if(g_cchLogUtilDb == 1)
					  LG2("util_db", "col_num = 0 (<=0)\n", col_num);
					break;
				}

				for(i = 0; i < col_num; ++i)
				{
					SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
							   MAX_DATALEN, &_buf_len[i]);
				}
			} catch(...)
			{
				if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 bind\n");
				break;
			}

			try
			{
				sqlret = SQLFetch(hstmt); // only fetch the next row
				if(DBNODATA(sqlret))
				{
					if(g_cchLogUtilDb == 1)
					  LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
					found = false;
				} else if(sqlret != SQL_SUCCESS)
				{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
					if(sqlret != SQL_SUCCESS_WITH_INFO)
					{
					  break;
					}
				}
			} catch(...)
			{
				if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 fetch\n");
				break;
			}

			try
			{
				// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
				if(found)
				{
					// È¡ï¿½ï¿½ï¿½ï¿½
					for(i = 0; i < col_num; ++i)
					{
					  if(_buf_len[i] == SQL_NULL_DATA)
					  {
						field_text[i] = NULLDATASTRING;
					  } else
					  {
						char* p = strchr((char*)_buf[i], ' ');
						if(p != nullptr)
						{
						  *p = '\0';
						}

						field_text[i] = (char*)(_buf[i]);
					  }
					}

					if(affect_rows != nullptr)
					  *affect_rows = 1;
				} else
				{
					// Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
					if(affect_rows != nullptr)
					  *affect_rows = 0;
				}
			} catch(...)
			{
				if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 copydata\n");
				break;
			}

			ret = true;

		} while(0);

		try
		{
			if(hstmt != SQL_NULL_HSTMT)
			{
				SQLFreeStmt(hstmt, SQL_CLOSE);
				SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
				SQLFreeStmt(hstmt, SQL_UNBIND);
				SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 freestmt\n");
			ret = false;
		}

		return ret;
}


bool cfl_rs::_get_rowSafely(string field_text[], int field_max_cnt, char const* param,
							char const* filter, int* affect_rows, ...)
{
	bool ret = false;
	char sql[SQL_MAXLEN];
	int	 i = 0;

    va_list arg_list;
	va_start(arg_list, affect_rows);
    int numberOfParameters = va_arg(arg_list, int);
	SQLULEN		   dataLen[MAX_PARAM_NUM];
	SQLSMALLINT	   dataTypes[MAX_PARAM_NUM];
	SQLSMALLINT	   dataDecimalDigits[MAX_PARAM_NUM];
	SQLSMALLINT	   dataNullable[MAX_PARAM_NUM];

	try
	{
		if(param == nullptr)
		{
			//sprintf(sql, "select * from %s", _tbl_name.c_str());
			_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select * from %s", _tbl_name.c_str());
		} else
		{
			if(_tbl_name.length() != 0)
			{
				//sprintf(sql, "select %s from %s", param, _tbl_name.c_str());
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s from %s", param, _tbl_name.c_str());
			} else
			{
				//sprintf(sql, "select %s",param);
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s", param);
			}
		}

		if((strstr(sql, "select") == nullptr) &&
		   (strstr(sql, "select") == nullptr) &&
		   (strstr(_tbl_name.c_str(), "(nolock)") == nullptr) &&
		   (strstr(_tbl_name.c_str(), "(NOLOCK)") == nullptr))
		{
			//strcat(sql, " (nolock) ");
			strncat_s(sql, sizeof(sql), " (nolock) ", _TRUNCATE);
		}

		if(filter == nullptr)
		{
		} else
		{
			//strcat(sql, " where ");
			strncat_s(sql, sizeof(sql), " where ", _TRUNCATE);
			//strcat(sql, filter);
			strncat_s(sql, sizeof(sql), filter, _TRUNCATE);
		}
	} catch(...)
	{
		if(g_cchLogUtilDb == 1)
			LG2("util_db", "exception raised from _get_row exec\n");
		return false;
	}

	if(g_cchLogUtilDb == 1)
		LG2("util_db", "_get_row [SQL]: [%s]\n", sql);

    // the bit bellow is simply safe_sql, but because of variadic arguments we cant call it here.
RECONNECT:

	SQLRETURN	   sqlret;
	SQLHSTMT	   hstmt   = SQL_NULL_HSTMT;
	SQLSMALLINT	   col_num = 0;
	unsigned short timeout = 10;
	bool		   found = true;
	do
	{
        try{
		    sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		    if(DBFAIL(sqlret))
		    {
			    handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			    break;
		    }
	    }
	    catch(...)
	    {
		if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row alloc\n");
		break;
	    }
		try
		{
		constexpr auto timeout{10};
		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
		if(DBFAIL(sqlret))
		{
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				break;
		}
		sqlret = SQLPrepare(hstmt, (SQLCHAR*)sql, SQL_NTS);

		if(DBFAIL(sqlret))
		{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
		}
		for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
		{
			    // Get info for each parameter from the prepared statement.
			    sqlret = SQLDescribeParam(hstmt, i + 1, &dataTypes[i], &dataLen[i], &dataDecimalDigits[i], &dataNullable[i]);
		}
        if(DBFAIL(sqlret))
		{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				break;
		}

		// Time to bind parameters.
#if defined(__linux__)
		SQLLEN paramLenInd4[MAX_PARAM_NUM];
		for(int p = 0; p < MAX_PARAM_NUM; p++) paramLenInd4[p] = 0;
#endif
		for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
		{
				const char*	  param_pointer		= va_arg(arg_list, const char*);
				
                if (dataTypes[i] == SQL_VARBINARY || dataTypes[i] == SQL_VARCHAR) {
                    // parameter provided will be null-terminated.
#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(dataTypes[i]);
					paramLenInd4[i] = SQL_NTS;
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, param_pointer ? (SQLLEN)strlen(param_pointer) + 1 : 0, &paramLenInd4[i]);
#else
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, 0);
#endif
				} else
				{
                    // Parameter size is given by data length.
#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(dataTypes[i]);
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, reinterpret_cast<SQLLEN*>(& dataLen[i]));
#else
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, reinterpret_cast<SQLLEN*>(& dataLen[i]));
#endif
                                                                                      // casting unsigned to signed... why is the type different for sizes?
				}
				
				if(DBFAIL(sqlret))
				{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
				}
		}
		va_end(arg_list);

		if(DBFAIL(sqlret))
		{
				break;
		}

		sqlret = SQLExecute(hstmt);
		switch(sqlret)
		{
		case SQL_SUCCESS_WITH_INFO:
				//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				// fall through
		case SQL_SUCCESS:
				break;

		case SQL_ERROR:
		{
				if(handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true))
				{
					goto RECONNECT;
				}
		}
		break;

		case SQL_NO_DATA: // update or delete
				break;

		case SQL_NEED_DATA:
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				break;
		}
		}
		catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n");
			break;
		}

        try
		{
			sqlret = SQLNumResultCols(hstmt, &col_num);

			col_num = min(col_num, field_max_cnt);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			if(col_num <= 0)
			{
				if(g_cchLogUtilDb == 1)
					LG2("util_db", "col_num = 0 (<=0)\n", col_num);
				break;
			}

            SQLLEN numRows;
			auto	   retCode = SQLRowCount(hstmt, &numRows);

			for(i = 0; i < col_num; ++i)
			{
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
						   MAX_DATALEN, &_buf_len[i]);
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row bind\n");
			break;
		}

		try
		{
			sqlret = SQLFetch(hstmt); // only fetch the next row
			if(DBNODATA(sqlret))
			{
				if(g_cchLogUtilDb == 1)
					LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
				found = false;
			} else if(sqlret != SQL_SUCCESS)
			{
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
				if(sqlret != SQL_SUCCESS_WITH_INFO)
				{
					break;
				}
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row fetch\n");
			break;
		}

		try
		{
			// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
			if(found)
			{
				// È¡ï¿½ï¿½ï¿½ï¿½
				for(i = 0; i < col_num; ++i)
				{
					if(_buf_len[i] == SQL_NULL_DATA)
					{
					  field_text[i] = NULLDATASTRING;
					} else
					{
					  field_text[i] = (char*)(_buf[i]);
					}
				}

				if(affect_rows != nullptr)
					*affect_rows = 1;
			} else
			{
				// Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
				if(affect_rows != nullptr)
					*affect_rows = 0;
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row copydata\n");
			break;
		}

		ret = true;


	} while(0);

	if(hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}
	
	return ret;
;

}



bool cfl_rs::_get_row(string field_text[], int field_max_cnt, char const* param,
                      char const* filter, int* affect_rows /* = NULL */)
{
    bool ret = false;
    char sql[SQL_MAXLEN];
    int i = 0;

    try {
        if (param == nullptr) {
            //sprintf(sql, "select * from %s", _tbl_name.c_str());
			_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select * from %s", _tbl_name.c_str());
        } else {
		    if (_tbl_name.length() !=0) {
	            //sprintf(sql, "select %s from %s", param, _tbl_name.c_str());
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s from %s", param, _tbl_name.c_str());
		    } else {
			    //sprintf(sql, "select %s",param);
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s",param);
		    }
        }

        if ((strstr(sql, "select") == nullptr) &&
            (strstr(sql, "select") == nullptr) &&
            (strstr(_tbl_name.c_str(), "(nolock)") == nullptr) &&
            (strstr(_tbl_name.c_str(), "(NOLOCK)") == nullptr)) {
            //strcat(sql, " (nolock) ");
				strncat_s(sql, sizeof(sql), " (nolock) ", _TRUNCATE);
        }

        if (filter == nullptr) {
        } else {
            //strcat(sql, " where ");
			strncat_s(sql, sizeof(sql), " where ", _TRUNCATE);
            //strcat(sql, filter);
			strncat_s(sql, sizeof(sql), filter, _TRUNCATE);
        }
    } catch (...) {
		if (g_cchLogUtilDb == 1)
	        LG2("util_db", "exception raised from _get_row exec\n");
        return false;
    }

	if (g_cchLogUtilDb == 1)
	    LG2("util_db", "_get_row [SQL]: [%s]\n", sql);

RECONNECT:

  
    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    do
    {
        try {        
            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }
            } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row alloc\n"); break;}

            try {
			  constexpr uint64_t timeout{10};
			  sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (DBFAIL(sqlret))
            {
                if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			    return false;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n"); break;}

        try {
            sqlret = SQLNumResultCols(hstmt, &col_num);

            col_num = min(col_num, field_max_cnt);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

            if (col_num <= 0)
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "col_num = 0 (<=0)\n", col_num);
                break;
            }

            for (i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
                        MAX_DATALEN, &_buf_len[i]);
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row bind\n"); break;}

        try {
            sqlret = SQLFetch(hstmt); // only fetch the next row
            if (DBNODATA(sqlret))
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
                found = false;
            }
            else if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
                if (sqlret != SQL_SUCCESS_WITH_INFO)
                {
                    break;
                }
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row fetch\n"); break;}

        try {
            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
            if (found)
            {
                // È¡ï¿½ï¿½ï¿½ï¿½
                for (i = 0; i < col_num; ++ i)
                {
                    if (_buf_len[i] == SQL_NULL_DATA)
                    {
                        field_text[i] = NULLDATASTRING;
                    }
                    else
                    {
                        field_text[i] = (char *)(_buf[i]);
                    }
                }

                if (affect_rows != nullptr)
                    *affect_rows = 1;
            }
            else
            {
                // Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
                if (affect_rows != nullptr)
                    *affect_rows = 0;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row copydata\n"); break;}

        ret = true;

    }
	while (0);

    try {
        if (hstmt != SQL_NULL_HSTMT) {    
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row freestmt\n"); ret = false;}

    return ret;
}

bool cfl_rs::_get_row3(string field_text[], int field_max_cnt, char* param,
                       char* filter, int* affect_rows /* = NULL */)
{
    bool ret = false;
    char sql[SQL_MAXLEN];
    int i = 0;

    try {
        if (param == nullptr)
        {
            //sprintf(sql, "select * from %s", _tbl_name.c_str());
			_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select * from %s", _tbl_name.c_str());
        }
        else
        {
            if(_tbl_name.length() !=0)
            {
                //sprintf(sql, "select %s from %s", param, _tbl_name.c_str());
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s from %s", param, _tbl_name.c_str());
            }else
            {
                //sprintf(sql, "select %s",param);
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s",param);
            }
        }

        if (filter == nullptr)
        {
        }
        else
        {
            //strcat(sql, " where ");
			strncat_s(sql, sizeof(sql), " where ", _TRUNCATE);
            //strcat(sql, filter);
			strncat_s(sql, sizeof(sql), filter, _TRUNCATE);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 exec\n"); return false;}

	if (g_cchLogUtilDb == 1)
	    LG2("util_db", "_get_row3 [SQL]: [%s]\n", sql);

RECONNECT:

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    do
    {
        try {

            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 alloc\n"); break;}

        try {
			constexpr uint64_t timeout{10};
		  sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (DBFAIL(sqlret))
            {
                if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
                return false;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 exec\n"); break;}

        try {
            sqlret = SQLNumResultCols(hstmt, &col_num);

            col_num = min(col_num, field_max_cnt);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

            if (col_num <= 0)
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "col_num = 0 (<=0)\n", col_num);
                break;
            }

            for (i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
                    MAX_DATALEN, &_buf_len[i]);
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 bind\n"); break;}

        try {
            sqlret = SQLFetch(hstmt); // only fetch the next row
            if (DBNODATA(sqlret))
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
                found = false;
            }
            else if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
                if (sqlret != SQL_SUCCESS_WITH_INFO)
                {
                    break;
                }
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 fetch\n"); break;}

        try {
            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
            if (found)
            {
                // È¡ï¿½ï¿½ï¿½ï¿½
                for (i = 0; i < col_num; ++ i)
                {
                    if (_buf_len[i] == SQL_NULL_DATA)
                    {
                        field_text[i] = NULLDATASTRING;
                    }
                    else
                    {
                        char* p = strchr((char *)_buf[i], ' ');
                        if (p != nullptr)
                        {
                            *p = '\0';
                        }

                        field_text[i] = (char *)(_buf[i]);
                    }
                }

                if (affect_rows != nullptr)
                    *affect_rows = 1;
            }
            else
            {
                // Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
                if (affect_rows != nullptr)
                    *affect_rows = 0;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 copydata\n"); break;}

        ret = true;

    }
    while (0);

    try {
        if (hstmt != SQL_NULL_HSTMT)
        {    
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 freestmt\n"); ret = false;}

    return ret;
}

bool cfl_rs::_get_rowOderby(string field_text[], int field_max_cnt, char* param,
					char* filter, int* affect_rows/* = NULL*/)
{
    bool ret = false;
    char sql[SQL_MAXLEN];
    int i = 0;

    try {
        if (param == nullptr)
        {
            //sprintf(sql, "select * from %s", _tbl_name.c_str());
			_snprintf_s(sql, sizeof(sql), _TRUNCATE,  "select * from %s  with(nolock)", _tbl_name.c_str());
        }
        else
        {
            if(_tbl_name.length() !=0)
            {
                //sprintf(sql, "select %s from %s", param, _tbl_name.c_str());
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s from %s  with(nolock)", param, _tbl_name.c_str());
            }else
            {
                //sprintf(sql, "select %s",param);
				_snprintf_s(sql, sizeof(sql), _TRUNCATE, "select %s  with(nolock)",param);
            }
        }

        if (filter == nullptr)
        {
        }
        else
        {
            //strcat(sql, " ");
			strncat_s(sql, sizeof(sql), " ", _TRUNCATE);
            //strcat(sql, filter);
			strncat_s(sql, sizeof(sql), filter, _TRUNCATE);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 exec\n"); return false;}

	if (g_cchLogUtilDb == 1)
	    LG2("util_db", "_get_row3 [SQL]: [%s]\n", sql);

RECONNECT:

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    do
    {
        try {

            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 alloc\n"); break;}

        try {
			constexpr uint64_t timeout{10};
		  sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (DBFAIL(sqlret))
            {
                if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
                return false;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 exec\n"); break;}

        try {
            sqlret = SQLNumResultCols(hstmt, &col_num);

            col_num = min(col_num, field_max_cnt);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

            if (col_num <= 0)
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "col_num = 0 (<=0)\n", col_num);
                break;
            }

            for (i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
                    MAX_DATALEN, &_buf_len[i]);
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 bind\n"); break;}

        try {
            sqlret = SQLFetch(hstmt); // only fetch the next row
            if (DBNODATA(sqlret))
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
                found = false;
            }
            else if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
                if (sqlret != SQL_SUCCESS_WITH_INFO)
                {
                    break;
                }
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 fetch\n"); break;}

        try {
            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
            if (found)
            {
                // È¡ï¿½ï¿½ï¿½ï¿½
                for (i = 0; i < col_num; ++ i)
                {
                    if (_buf_len[i] == SQL_NULL_DATA)
                    {
                        field_text[i] = NULLDATASTRING;
                    }
                    else
                    {
                        char* p = strchr((char *)_buf[i], ' ');
                        if (p != nullptr)
                        {
                            *p = '\0';
                        }

                        field_text[i] = (char *)(_buf[i]);
                    }
                }

                if (affect_rows != nullptr)
                    *affect_rows = 1;
            }
            else
            {
                // Ã»ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
                if (affect_rows != nullptr)
                    *affect_rows = 0;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 copydata\n"); break;}

        ret = true;

    }
    while (0);

    try {
        if (hstmt != SQL_NULL_HSTMT)
        {    
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row3 freestmt\n"); ret = false;}

	return ret;
}

SQLRETURN cfl_rs::exec_sql_direct(char const* sql, unsigned short timeout /* = 5 */)
{
RECONNECT:

	SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
	//LG2("util_db", "SQL Statement Length: %d\n", strlen(sql));

	do
	{
        sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
            break;
        }

		constexpr uint64_t timeout{10};
		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
		if (DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			break;
		}

		sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
		switch (sqlret)
		{
		case SQL_SUCCESS_WITH_INFO:
			//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
			// fall through
		case SQL_SUCCESS:
			break;

		case SQL_ERROR:
			{
				if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			}
			break;

		case SQL_NO_DATA: // update or delete
			break;
		}
	}
	while (0);

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    
	return sqlret;
}

SQLRETURN cfl_rs::stored_procedure(char const* sql,char const* schema, char const* procedure, ...)
{
	bool ret = false;
	//char sql[SQL_MAXLEN];
	int	 i = 0;
	va_list arg_list;
	va_start(arg_list, procedure);
	int numberOfParameters = va_arg(arg_list, int);
	double	time_1, time_2;
	MPTimer t1;

	// Flag to indicate if we need to use fallback parameter binding
	bool use_fallback_params = false;
	// Fallback parameter data when cache lookup fails
	std::vector<SQLParamData> fallback_params;

	std::vector<SQLParamData>* cached_params = nullptr;
	auto					   cache		 = _cached_map.find(std::string(procedure));
	bool cache_mismatch = false;
	
	if(cache == _cached_map.end())
	{
		// Procedure wasn't cached yet.
		cache_stored_procedure(numberOfParameters, schema, procedure);
		cache = _cached_map.find(std::string(procedure));

		if(cache != _cached_map.end())
		{
			cached_params = &(*cache).second;

		} else
		{
			// Cache lookup failed - use fallback default parameters (SQL_VARCHAR, 255 length)
			LG2("util_db", "Could not find cached map for procedure: [%s] - using fallback VARCHAR params\n", procedure);
			use_fallback_params = true;
			for (int p = 0; p < numberOfParameters; p++) {
				SQLParamData param;
				param.sql_data_type = SQL_VARCHAR;
				param.column_length = 255;
				fallback_params.push_back(param);
			}
			cached_params = &fallback_params;
		}

	} else
	{
		cached_params = &(*cache).second;
		
		// Validate cached parameter count matches expected count
		// If mismatch, procedure definition may have changed - invalidate cache and retry
		if(cached_params && cached_params->size() != numberOfParameters)
		{
			LG2("util_db", "Parameter count mismatch for [%s]: expected %d, cached %d - invalidating cache\n", 
				procedure, numberOfParameters, (int)cached_params->size());
			_cached_map.erase(cache);
			cache_stored_procedure(numberOfParameters, schema, procedure);
			cache = _cached_map.find(std::string(procedure));
			if(cache != _cached_map.end())
			{
				cached_params = &(*cache).second;
				// Double-check after refresh
				if(cached_params->size() != numberOfParameters)
				{
					LG2("util_db", "ERROR: Parameter count still mismatched after refresh for [%s]\n", procedure);
					cache_mismatch = true;
				}
			}
			else
			{
				// Cache still failed after re-caching - use fallback
				LG2("util_db", "Re-cache failed for [%s] - using fallback VARCHAR params\n", procedure);
				use_fallback_params = true;
				for (int p = 0; p < numberOfParameters; p++) {
					SQLParamData param;
					param.sql_data_type = SQL_VARCHAR;
					param.column_length = 255;
					fallback_params.push_back(param);
				}
				cached_params = &fallback_params;
			}
		}
	}
	try
	{
	} catch(...)
	{
		if(g_cchLogUtilDb == 1)
			LG2("util_db", "exception raised from stored_procedure exec\n");
		return SQL_ERROR;
	}

	if(g_cchLogUtilDb == 1)
		LG2("util_db", "stored_procedure[SQL]: [%s]\n", procedure);

	// the bit bellow is simply safe_sql, but because of variadic arguments we cant call it here.
RECONNECT:

	SQLRETURN	   sqlret;
	SQLHSTMT	   hstmt	= SQL_NULL_HSTMT;
	SQLSMALLINT	   col_num	= 0;
	unsigned short timeout	= 10;
	bool		   found	= true;
	SQLINTEGER	   data_len = SQL_NTS;
	do
	{
		try
		{
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if(DBFAIL(sqlret))
			{
					handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
					break;
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from stored_procedure alloc\n");
			break;
		}
		try
		{
			constexpr auto timeout{10};
			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
			if(DBFAIL(sqlret))
			{
					handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
					break;
			}
			if(DBFAIL(sqlret))
			{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
			}

			if(DBFAIL(sqlret))
			{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
			}


			// Time to bind parameters.
#if defined(__linux__)
			SQLLEN paramLenIndicators3[MAX_PARAM_NUM] = {0};
#endif
			for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
			{
					if(!cached_params)
					{
					break;
					}
					SQLSMALLINT sql_data_type = cached_params->at(i).sql_data_type;
					SQLULEN column_length = cached_params->at(i).column_length;
					const char* param_pointer = va_arg(arg_list, const char*);

#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(sql_data_type);
					SQLLEN bufferLen = 0;
					if (IsStringCType(c_type)) {
						paramLenIndicators3[i] = SQL_NTS;
						bufferLen = param_pointer ? (SQLLEN)strlen(param_pointer) + 1 : 0;
					}
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, sql_data_type, column_length, 0,
											  (SQLPOINTER)param_pointer, bufferLen, &paramLenIndicators3[i]);
#else
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, sql_data_type, column_length, 0,
											  (SQLPOINTER)param_pointer, 0, 0);
#endif

					if(DBFAIL(sqlret))
					{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
					}
			};
			va_end(arg_list);

			if(DBFAIL(sqlret))
			{
					break;
			}

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			switch(sqlret)
			{
			case SQL_SUCCESS_WITH_INFO:
					//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
					// fall through
			case SQL_SUCCESS:
					break;

			case SQL_ERROR:
			{
					if(handle_err(hstmt, SQL_HANDLE_STMT, sqlret, procedure, true))
					{
					goto RECONNECT;
					}
			}
			break;

			case SQL_NO_DATA: // update or delete
					break;

			case SQL_NEED_DATA:
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret, procedure);
					break;
			}
		} catch(...)
		{
			if(g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row exec\n");
			break;
		}
	} while(0);

	if(hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return sqlret;
}


SQLRETURN cfl_rs::safe_sql(char const* sql, ...)
{
RECONNECT:

	SQLRETURN  sqlret;
	SQLHSTMT   hstmt	= SQL_NULL_HSTMT;
	unsigned short timeout	= 10;
    va_list arg_list;
	va_start(arg_list, sql);

	int numberOfParameters = va_arg(arg_list,int);
	SQLULEN	       dataLen[MAX_PARAM_NUM]; 
    SQLSMALLINT	   dataTypes[MAX_PARAM_NUM];
	SQLSMALLINT	   dataDecimalDigits[MAX_PARAM_NUM];
	SQLSMALLINT	   dataNullable[MAX_PARAM_NUM];

	do
	{
		sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		if(DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			break;
		}

		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
		if(DBFAIL(sqlret))
		{
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			break;
		}

		sqlret = SQLPrepare(hstmt, (SQLCHAR*)sql, SQL_NTS);
		if(DBFAIL(sqlret))
		{
			handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			break;
		}

        for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
		{
			// Get info for each parameter from the prepared statement.
			sqlret = SQLDescribeParam(hstmt, i + 1, &dataTypes[i], &dataLen[i], &dataDecimalDigits[i], &dataNullable[i]);
		}

		if(DBFAIL(sqlret))
		{
			handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			break;
		}

		// Time to bind parameters.
#if defined(__linux__)
		SQLLEN paramLenInd5[MAX_PARAM_NUM];
		for(int p = 0; p < MAX_PARAM_NUM; p++) paramLenInd5[p] = 0;
#endif
		for(SQLUSMALLINT i = 0; i < numberOfParameters; i++)
		{
			const char* param_pointer = va_arg(arg_list, const char*);

			if(dataTypes[i] == SQL_VARBINARY || dataTypes[i] == SQL_VARCHAR)
			{
#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(dataTypes[i]);
					paramLenInd5[i] = SQL_NTS;
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, param_pointer ? (SQLLEN)strlen(param_pointer) + 1 : 0, &paramLenInd5[i]);
#else
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, 0);
#endif
			} else
			{
#if defined(__linux__)
					SQLSMALLINT c_type = MapSqlTypeToC(dataTypes[i]);
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, c_type, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, reinterpret_cast<SQLLEN*>(&dataLen[i]));
#else
					// Parameter size is given by data length.
					sqlret = SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_DEFAULT, dataTypes[i], dataLen[i], dataDecimalDigits[i],
											  (SQLPOINTER)param_pointer, 0, reinterpret_cast<SQLLEN*>(&dataLen[i]));
#endif
					// casting unsigned to signed... why is the type different for sizes?
			}

			if(DBFAIL(sqlret))
			{
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
					break;
			}
		}
		va_end(arg_list);


		if(DBFAIL(sqlret))
		{
			break;
		}

		sqlret = SQLExecute(hstmt);
		switch(sqlret)
		{
		case SQL_SUCCESS_WITH_INFO:
			//handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
			// fall through
		case SQL_SUCCESS:
			break;

		case SQL_ERROR:
		{
			if(handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true))
			{
					goto RECONNECT;
			}
		}
		break;

		case SQL_NO_DATA: // update or delete
			break;

		case SQL_NEED_DATA:
			handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
			break;
		}
	} while(0);

	if(hstmt != SQL_NULL_HSTMT)
	{
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return sqlret;
}




SQLRETURN cfl_rs::exec_sql(char const* sql, char const* pdata, int len, unsigned short timeout /* = 30 */)
{
RECONNECT:

    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLLEN data_len = SQL_NTS;

    do
    {
        sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
            break;
        }

		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
            break;
        }

        sqlret = SQLPrepare(hstmt, (SQLCHAR *)sql, SQL_NTS);
        if (DBFAIL(sqlret))
        {
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
            break;
        }

        sqlret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, len - 1, 0,
            (SQLPOINTER)pdata, len, &data_len);
        if (DBFAIL(sqlret))
        {
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
            break;
        }

        sqlret = SQLExecute(hstmt);
        switch (sqlret)
        {
        case SQL_SUCCESS_WITH_INFO:
            //handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
            // fall through
        case SQL_SUCCESS:
            break;

        case SQL_ERROR:
			{
				if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			}
            break;

        case SQL_NO_DATA: // update or delete
            break;

        case SQL_NEED_DATA:
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
            break;
        }
    }
    while (0);

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeStmt(hstmt, SQL_UNBIND);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    return sqlret;
}

// Add by lark.li 20080808 begin
SQLRETURN cfl_rs::exec_param_sql(char const* sql, char const* pdata, int len, unsigned short timeout /* = 30 */)
{
RECONNECT:

    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLLEN data_len = len;

    do
    {
        sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
            break;
        }

		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
        if (DBFAIL(sqlret))
        {
            handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
            break;
        }

        sqlret = SQLPrepare(hstmt, (SQLCHAR *)sql, SQL_NTS);
        if (DBFAIL(sqlret))
        {
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
            break;
        }

        sqlret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, len, 0,
            (SQLPOINTER)pdata, 0, &data_len);
        if (DBFAIL(sqlret))
        {
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
            break;
        }

        sqlret = SQLExecute(hstmt);
        switch (sqlret)
        {
        case SQL_SUCCESS_WITH_INFO:
            //handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
            // fall through
        case SQL_SUCCESS:
            break;

        case SQL_ERROR:
			{
				if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
			}
            break;

        case SQL_NO_DATA: // update or delete
            break;

        case SQL_NEED_DATA:
            handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
            break;
        }
    }
    while (0);

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeStmt(hstmt, SQL_UNBIND);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    return sqlret;
}

// End

bool cfl_rs::getalldata(const char* sql, vector< vector< string > >& data, unsigned short timeout)
{
	bool ret;

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    try
    {
        do
        {
            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

                if (sqlret != SQL_SUCCESS_WITH_INFO) break;
            }

            sqlret = SQLNumResultCols(hstmt, &col_num);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

            // Bind Column
            for (int i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
            }

            // Fetch each Row
            for (int i = 0; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++ i)
            {
				vector< string > rowV;

                if (sqlret != SQL_SUCCESS) handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

				for(int j=0; j< col_num; j++)
				{
					rowV.push_back((char const *)_buf[j]);
				}

				data.push_back(rowV);
            }

            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            ret = true;

        } while (0);
    }
    catch (...)
    {
		if (g_cchLogUtilDb == 1)
	        LG2("util_db", "Exception raised when get friend data:\n%s\n", sql);
    }

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    return ret;
}

// Add by lark.li 20080809 begin
bool cfl_rs::get_page_data(char* tablename, char* param, int pagesize, int pageindex, char* filter, char* sort, int sorttype, int& totalpage, int& totalrecord,
					  vector< vector< string > > &data, unsigned short timeout)
{
	bool ret;

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    try
    {
        do
        {
			SQLLEN sql_nts = SQL_NTS;
			//SQLINTEGER num[2];

			//char tablename[32];
			//strncpy(tablename, _tbl_name.c_str(), sizeof(tablename));

			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

			SQLFreeStmt(hstmt, SQL_UNBIND);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

			if((SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, (SQLUINTEGER)strlen(tablename), 0, tablename, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, (SQLUINTEGER)strlen(param), 0, param, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, (SQLUINTEGER)sizeof(pagesize), 0, &pagesize, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, (SQLUINTEGER)sizeof(pageindex), 0, &pageindex, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, (SQLUINTEGER)strlen(filter), 0, filter, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, (SQLUINTEGER)strlen(sort), 0, sort, 0, &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, (SQLUINTEGER)sizeof(sorttype), 0, &sorttype, 0, &sql_nts) != SQL_SUCCESS)
				/*||	(SQLBindParameter(hstmt, 8, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 10, 0, &num[0], sizeof(num[0]), &sql_nts) != SQL_SUCCESS)
				||	(SQLBindParameter(hstmt, 9, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, 10, 0, &num[1], sizeof(num[1]),  &sql_nts) != SQL_SUCCESS)*/)
			{
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
			}

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *) "{call sys_Paging(?,?,?,?,?,?,?)}", SQL_NTS);
            if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, "{call sys_Paging(?,?,?,?,?,?,?)}");

                if (sqlret != SQL_SUCCESS_WITH_INFO) break;
            }

            sqlret = SQLNumResultCols(hstmt, &col_num);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

			// Clear
			memset(_buf, 0, MAX_COL * MAX_DATALEN * sizeof(UCHAR));

            // Bind Column
            for (int i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
            }

            // Fetch each Row
            for (int i = 0; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++ i)
            {
				vector< string > rowV;

                if (sqlret != SQL_SUCCESS) handle_err(hstmt, SQL_HANDLE_STMT, sqlret, "{call sys_Paging(?,?,?,?,?,?,?,?,?)}");

				for(int j=0; j< col_num; j++)
				{
					rowV.push_back((char const *)_buf[j]);
				}

				data.push_back(rowV);
            }

			if(SQLMoreResults(hstmt) == SQL_SUCCESS)
			{
				sqlret = SQLNumResultCols(hstmt, &col_num);
				col_num = min(col_num, MAX_COL);
				col_num = min(col_num, _max_col);

				// Bind Column
				for (int i = 0; i < col_num; ++ i)
				{
					SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
				}

				// Fetch each Row
				for (int i = 0; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++ i)
				{
					totalpage = atoi((const char*)_buf[0]);
					totalrecord = atoi((const char*)_buf[1]);
				}
			}
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            ret = true;

        } while (0);
    }
    catch (...)
    {
		if (g_cchLogUtilDb == 1)
	        LG2("util_db", "Exception raised when get friend data:\n%s\n", "{call sys_Paging2(?,?,?,?,?,?,?,?,?)}");
    }

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    return ret;
}
// End

bool cfl_rs::get(char const* sql, char const* pdata, int len, unsigned short timeout /* = 30 */)
{
RECONNECT:

    bool ret = false;
    //char sql[SQL_MAXLEN];
    int i = 0;

	if (g_cchLogUtilDb == 1)
	    LG2("util_db", "get() [SQL]: [%s]\n", sql);

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    do
    {
        try {

            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from get() alloc\n"); break;}

        try {
			constexpr uint64_t timeout{10};
		    sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if (DBFAIL(sqlret))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (DBFAIL(sqlret))
            {
                if( handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql, true) )
				{
					goto RECONNECT;
				}
                return false;
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from get() exec\n"); break;}

        try {
            sqlret = SQLNumResultCols(hstmt, &col_num);
            if (col_num <= 0)
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "col_num = 0 (<=0)\n", col_num);
                break;
            }

            for (i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i],
                    MAX_DATALEN, &_buf_len[i]);
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row bind\n"); break;}

        try {
            sqlret = SQLFetch(hstmt); // only fetch the next row
            if (DBNODATA(sqlret))
            {
				if (g_cchLogUtilDb == 1)
	                LG2("util_db", "SQL didn't fetch any data [%s]\n", sql);
                found = false;
            }
            else if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);
                if (sqlret != SQL_SUCCESS_WITH_INFO)
                {
                    break;
                }
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row fetch\n"); break;}

        try {
            // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
            if (found)
            {
                // È¡ï¿½ï¿½ï¿½ï¿½
                memcpy((void *)pdata, _buf[0], len);
            }
            else
            {
            }
        } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row copydata\n"); break;}

        ret = true;
    }
    while (0);

    try {
        if (hstmt != SQL_NULL_HSTMT)
        {    
            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    } catch (...) {if (g_cchLogUtilDb == 1) LG2("util_db", "exception raised from _get_row freestmt\n"); ret = false;}

    return ret;
}


// friend table
static char const query_friend_format[SQL_MAXLEN] =
"select '' relation,count(*) addr,0 cha_id,'' cha_name,0 icon,'' motto from ( \
select distinct friends.relation relation from character with(nolock) INNER JOIN \
friends with(nolock) ON character.cha_id = friends.cha_id2 where friends.cha_id1 = %d) cc	\
union select '' cha_name,0 addr, -1 cha_id,friends.relation relation,0 icon,'' motto from friends	\
where friends.cha_id1 = %d and friends.cha_id2 = -1	\
union select friends.relation relation,count(character.mem_addr) addr,0 \
cha_id,'' cha_name,1 icon,'' motto from character INNER JOIN friends ON \
character.cha_id = friends.cha_id2 where friends.cha_id1 = %d group by relation \
union select friends.relation relation,character.mem_addr addr,character.cha_id \
cha_id,character.cha_name cha_name,character.icon icon,character.motto motto \
from character INNER JOIN friends ON character.cha_id = friends.cha_id2 \
where friends.cha_id1 = %d order by relation,cha_id,icon";

bool friend_tbl::get_friend_dat(friend_dat* farray, int& array_num, unsigned int cha_id, bool* drop /* = nullptr */)
{
    if (farray == nullptr || array_num <= 0 || cha_id == 0) return false;

    bool ret = false;
    char sql[SQL_MAXLEN];
	// Modify by lark.li 20080804 begin
    //sprintf(sql, query_friend_format, cha_id, cha_id, cha_id);
	//sprintf(sql, query_friend_format, cha_id, cha_id, cha_id, cha_id);
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, query_friend_format, cha_id, cha_id, cha_id, cha_id);
	// End

    // Ö´ï¿½Ð²ï¿½Ñ¯ï¿½ï¿½ï¿½ï¿½
    SQLRETURN sqlret;
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    SQLSMALLINT col_num = 0;
    bool found = true;

    try
    {
        do
        {
            sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            constexpr uint64_t timeout{10};
			sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT, SQLPOINTER(timeout), SQL_IS_UINTEGER);
            if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
            {
                handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
                break;
            }

            sqlret = SQLExecDirect(hstmt, (SQLCHAR *)sql, SQL_NTS);
            if (sqlret != SQL_SUCCESS)
            {
                handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

                if (sqlret != SQL_SUCCESS_WITH_INFO) break;
            }

            sqlret = SQLNumResultCols(hstmt, &col_num);
            col_num = min(col_num, MAX_COL);
            col_num = min(col_num, _max_col);

            // Bind Column
			int i = 0;

            for (i = 0; i < col_num; ++ i)
            {
                SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
            }

            // Fetch each Row
            for (i = 0; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++ i)
            {
                if (i >= array_num)
                {
                    if (drop != nullptr)
                        *drop = true;

                    break;
                }

                if (sqlret != SQL_SUCCESS) handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

                farray[i].relation = (char const *)_buf[0];
                farray[i].memaddr = strtoull((char const *)_buf[1], nullptr, 10);
                farray[i].cha_id = atoi((char const *)_buf[2]);
                farray[i].cha_name = (char const *)_buf[3];
                farray[i].icon_id = atoi((char const *)_buf[4]);
                farray[i].motto = (char const *)_buf[5];
            }

            array_num = i; // È¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½

            SQLFreeStmt(hstmt, SQL_CLOSE);
            SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            SQLFreeStmt(hstmt, SQL_UNBIND);
            ret = true;

        } while (0);
    }
    catch (...)
    {
		if (g_cchLogUtilDb == 1)
	        LG2("util_db", "Exception raised when get friend data:\n%s\n", sql);
    }

    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    return ret;
}

// ï¿½ï¿½ï¿½ï¿½É¹ï¿½×ªï¿½ï¿½ï¿½ò·µ»ï¿½0
int Util::ConvertDBParam(const char* param, char* buf, size_t size, size_t& count)
{
	count = 0;

	if(param == nullptr)
		return -1;

	size_t len = strlen(param);

	// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½Ö·ï¿½ï¿½ï¿½ï¿½ï¿½\0
	if(len >= size)
		return -1;

	const char* p1 = param;
	const char* p2 = nullptr;

	while((p2 = strchr(p1, '\'')) != nullptr)
	{

#if (_MSC_VER >= 1400 )
		strncpy_s(buf + count, (size - count), p1, (p2 - p1));
#else
		strncpy(buf + count, p1, (size - count));
#endif
		count += (p2 - p1);

#if (_MSC_VER >= 1400 )
		strncpy_s(buf + count, (size - count), "''", 2);
#else
		strncpy(buf + count, "''", (size - count));
#endif
		count += 2;

		p1 = p2 + 1;
	}

	if((p1 - param) < (int)len)
	{
#if (_MSC_VER >= 1400 )
		strncpy_s(buf + count, (size - count), p1, _TRUNCATE);
#else
		strncpy(buf + count, p1, (size - count));
#endif
		count += (p1 - param);
	}

	return 0;
}
