#include "stdafx.h"
#include "Friend.h"
#include "Master.h"
#include "DBConnect.h"
#include "log.h"
#include "Guild.h"
#include "GroupServerApp.h"
#include "GameCommon.h"

using namespace std;

SQLRETURN Exec_sql_direct(const char* pszSQL, cfl_rs* pTable) {
	// LG("group_sql", "表[%s], 开始执行SQL语句[%s]\n", pTable->get_table(), pszSQL);
	LG("group_sql", "Table [%s], begin execute SQL [%s]\n", pTable->get_table(), pszSQL);
	SQLRETURN r = pTable->exec_sql_direct(pszSQL);
	if (DBOK(r)) {
		// LG("group_sql", "成功执行SQL!\n");
		LG("group_sql", "execute SQL success!");
	} else if (DBNODATA(r)) {
		// LG("group_sql", "执行SQL, 但无结果返回\n");
		LG("group_sql", "execute SQL, no result \n");
	} else {
		// LG("group_sql", "执行SQL, 出错!\n");
		LG("group_sql", "execute SQL, failed!\n");
	}
	return r;
}

//==========TBLSystem===============================
bool TBLAccounts::IsReady() {
	char sql[SQL_MAXLEN];
	strncpy_s(sql, sizeof(sql), "drop trigger [TR_D_Character_Friends]", _TRUNCATE);
	SQLRETURN l_ret = Exec_sql_direct(sql, this);
	if (!DBOK(l_ret)) {
		LG("Database", "SQL:%s execute failed !\n", sql);
	}
	strncpy_s(sql, sizeof(sql), "drop trigger [TR_I_Character]", _TRUNCATE);
	l_ret = Exec_sql_direct(sql, this);
	if (!DBOK(l_ret)) {
		LG("Database", "SQL:%s execute failed !\n", sql);
	}
	strncpy_s(sql, sizeof(sql), "CREATE TRIGGER TR_D_Character_Friends ON character \n\
				FOR DELETE \n\
				AS\n\
				BEGIN\n\
					declare @@stat tinyint\n\
					declare @@gid  int\n\
					select @@stat =guild_stat,@@gid =guild_id from deleted\n\
					DELETE friends where friends.cha_id1 IN(select cha_id from deleted)\n\
					if(@@gid >0)\n\
					BEGIN\n\
						update guild set try_total =try_total -(case when @@stat>0 then 1 else 0 end),\n\
								member_total =member_total -(case when @@stat >0 then 0 else 1 end)\n\
							where guild_id >0 and guild_id =@@gid\n\
					END\n\
				END\n\
		", _TRUNCATE);
	l_ret = Exec_sql_direct(sql, this);
	if (!DBOK(l_ret)) {
		LG("Database", "SQL:%s execute failed !\n", sql);
		return false;
	}
	strncpy_s(sql, sizeof(sql), "CREATE TRIGGER TR_I_Character ON character\n\
				FOR INSERT\n\
				AS\n\
				BEGIN\n\
					declare @l_icon smallint\n\
					select @l_icon =convert(smallint,SUBSTRING(inserted.look,5,1)) from inserted\n\
					update character set icon =@l_icon where cha_id in (select cha_id from inserted)\n\
				END\n\
		", _TRUNCATE);
	l_ret = Exec_sql_direct(sql, this);
	if (!DBOK(l_ret)) {
		LG("Database", "SQL:%s execute failed !\n", sql);
		return false;
	}
	return true;
}

//==========TBLAccounts===============================
void TBLAccounts::AddStatLog(int login, int play, int wgplay) {
	// Use stored procedure for better performance and security
	stored_procedure("{CALL AddStatLog(?, ?, ?)}", "dbo", "AddStatLog", 3, &login, &play, &wgplay);
}
bool TBLAccounts::SetDiscInfo(int actid, const char* cli_ip, const char* reason) {
	// Use stored procedure to prevent SQL injection
	const auto ret = stored_procedure("{CALL dbo.SetDiscInfo(?, ?, ?)}", "dbo", "SetDiscInfo", 3, cli_ip, reason, &actid);
	return DBOK(ret);
}
bool TBLAccounts::InsertRow(int act_id, const char* act_name, const char* cha_ids) {
	// Use stored procedure - auto-generates act_id
	const auto ret = stored_procedure("{CALL dbo.AccountSaveInsert(?, ?)}", "dbo", "AccountSaveInsert", 2, act_name, cha_ids);
	return DBOK(ret);
}
bool TBLAccounts::UpdateRow(int act_id, const char* cha_ids) {
	// Use stored procedure to update character IDs for account
	const auto ret = stored_procedure("{CALL dbo.AccountSaveUpdateChaIds(?, ?)}", "dbo", "AccountSaveUpdateChaIds", 2, cha_ids, &act_id);
	return DBOK(ret);
}
bool TBLAccounts::UpdatePassword(int act_id, const char szPassword[]) {
	// Use stored procedure to update password
	const auto ret = stored_procedure("{CALL dbo.AccountSaveUpdatePassword(?, ?)}", "dbo", "AccountSaveUpdatePassword", 2, szPassword, &act_id);
	return DBOK(ret);
}

int TBLAccounts::FetchRowByActName(const char szAccount[]) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	if (_get_row_stored_procedure(m_buf, 7, "{CALL dbo.GetAccountByName(?)}", 
		"dbo", "GetAccountByName", &l_retrow, 1, szAccount)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return l_retrow;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

int TBLAccounts::FetchRowByActID(int act_id) {
	int l_retrow = 0;
	const char* param = "act_name,gm,cha_ids,password,last_ip,disc_reason,convert(varchar(20),last_leave,120)";
	char filter[200];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "act_id=%d", act_id);
	if (_get_row(m_buf, 7, param, filter, &l_retrow)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return l_retrow;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}

//==========TBLCharacters===============================
bool TBLCharacters::ZeroAddr() {
	// Use stored procedure to reset all memory addresses on server startup
	const auto ret = stored_procedure("{CALL dbo.CharacterZeroAddr()}", "dbo", "CharacterZeroAddr", 0);
	return DBOK(ret);
}

bool TBLCharacters::SetAddr(int cha_id, unsigned long long addr) {
	// Use stored procedure to set memory address
	const auto ret = stored_procedure("{CALL dbo.CharacterSetAddr(?, ?)}", "dbo", "CharacterSetAddr", 2, &addr, &cha_id);
	return DBOK(ret);
}
bool TBLCharacters::InsertRow(const char* cha_name, int act_id, const char* birth, const char* map, const char* look) {
	// Use stored procedure to insert new character
	const auto ret = stored_procedure("{CALL dbo.CharacterInsert(?, ?, ?, ?, ?)}", "dbo", "CharacterInsert", 5, cha_name, &act_id, birth, map, look);
	return DBOK(ret);
}
bool TBLCharacters::UpdateInfo(unsigned int cha_id, unsigned short icon, const char* motto) {
	// Use stored procedure to update character info
	int l_icon = static_cast<int>(icon);
	int l_cha_id = static_cast<int>(cha_id);
	const auto ret = stored_procedure("{CALL dbo.CharacterUpdateInfo(?, ?, ?)}", "dbo", "CharacterUpdateInfo", 3, &l_icon, motto, &l_cha_id);
	return DBOK(ret);
}

int TBLCharacters::FetchRowByChaName(const char* cha_name) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	if (_get_row_stored_procedure(m_buf, 3, "{CALL dbo.GetCharacterByChaName(?)}", 
		"dbo", "GetCharacterByChaName", &l_retrow, 1, cha_name)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return l_retrow;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}
bool TBLCharacters::FetchAccidByChaName(const char* cha_name, int& cha_accid) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	if (_get_row_stored_procedure(m_buf, 1, "{CALL dbo.GetAccountIdByChaName(?)}", 
		"dbo", "GetAccountIdByChaName", &l_retrow, 1, cha_name)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			cha_accid = atoi(m_buf[0].c_str());
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool TBLCharacters::StartEstopTime(int cha_id) {
	// Use stored procedure to start estop time
	const auto ret = stored_procedure("{CALL dbo.CharacterStartEstop(?)}", "dbo", "CharacterStartEstop", 1, &cha_id);
	return DBOK(ret);
}

bool TBLCharacters::EndEstopTime(int cha_id) {
	// Use stored procedure to end estop time
	const auto ret = stored_procedure("{CALL dbo.CharacterEndEstop(?)}", "dbo", "CharacterEndEstop", 1, &cha_id);
	return DBOK(ret);
}

bool TBLCharacters::IsEstop(int cha_id) {
	int l_retrow = 0;
	const char* param = "estop";
	char filter[200];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_id = %d and dateadd(minute, estoptime, estop) > getdate()", cha_id);
	if (_get_row(m_buf, 1, param, filter, &l_retrow)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

bool TBLCharacters::Estop(const char* cha_name, uLong lTimes) {
	// Use stored procedure to set estop by character name
	int l_times = static_cast<int>(lTimes);
	const auto ret = stored_procedure("{CALL dbo.CharacterEstop(?, ?)}", "dbo", "CharacterEstop", 2, &l_times, cha_name);
	return DBOK(ret);
}

bool TBLCharacters::AddMoney(int cha_id, __int64 llMoney) {
	// Use stored procedure to add money to character (64-bit for 100B gold cap)
	const auto ret = stored_procedure("{CALL dbo.CharacterAddMoney(?, ?)}", "dbo", "CharacterAddMoney", 2, &llMoney, &cha_id);
	return DBOK(ret);
}

bool TBLCharacters::DelEstop(const char* cha_name) {
	// Use stored procedure to delete estop by character name
	const auto ret = stored_procedure("{CALL dbo.CharacterDelEstop(?)}", "dbo", "CharacterDelEstop", 1, cha_name);
	return DBOK(ret);
}

int TBLCharacters::FetchChaIDByCharName(cChar* cha_name) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	try {
		if (_get_row_stored_procedure(m_buf, 1, "{CALL dbo.GetChaIdByChaName(?)}", 
			"dbo", "GetChaIdByChaName", &l_retrow, 1, cha_name)) {
			if (l_retrow > 0) {
				return atoi(m_buf[0].c_str());
			}
		}
	} catch (...) {
		LG("group_sql", "TBLCharacters::FetchChaIDByCharName execute SQL, failed!,cha_id =%s\n", cha_name);
	}
	return 0;
}

int TBLCharacters::FetchActIDByCharName(cChar* cha_name) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	try {
		if (_get_row_stored_procedure(m_buf, 1, "{CALL dbo.GetActIdByChaName(?)}", 
			"dbo", "GetActIdByChaName", &l_retrow, 1, cha_name)) {
			if (l_retrow > 0) {
				return atoi(m_buf[0].c_str());
			}
		}
	} catch (...) {
		LG("group_sql", "TBLCharacters::FetchActIDByCharName execute SQL, failed!,cha_id =%s\n", cha_name);
	}
	return 0;
}

int TBLCharacters::FetchRowByChaID(int cha_id) {
	int l_retrow = 0;
	/*
		char* param = "c.cha_name,c.motto,c.icon,\
					  case when c.guild_stat =0 then c.guild_id else 0 end,\
					  case when c.guild_stat <>0 or c.guild_id =0 then '[无]' else g.guild_name end,\
					  c.job,c.degree,c.map,c.map_x,c.map_y,c.look,c.str,c.dex,c.agi,c.con,c.sta,c.luk\
					  ";
	*/
	string param = string("c.cha_name,c.motto,c.icon,\
				  case when c.guild_stat =0 then c.guild_id else 0 end,\
				  case when c.guild_stat <>0 or c.guild_id =0 then '") +
				   string(RES_STRING(GP_DBCONNECT_CPP_00001)) +
				   string("' else g.guild_name end,\
				  c.job,c.degree,c.map,c.map_x,c.map_y,c.look,c.str,c.dex,c.agi,c.con,c.sta,c.luk,c.guild_permission,c.chatColour");
	char filter[200];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "c.guild_id =g.guild_id and c.cha_id=%d", cha_id);
	std::string l_tblname = _tbl_name;
	_tbl_name = "character c,guild g";
	bool l_bret = false;
	try {
		l_bret = _get_row(m_buf, CHA_MAXCOL, const_cast<char*>(param.c_str()), filter, &l_retrow);
	} catch (...) {
		// LG("group_sql", "TBLCharacters::FetchRowByChaID执行SQL, 发生异常!,cha_id =%d\n", cha_id);
		LG("group_sql", "TBLCharacters::FetchRowByChaID execute SQL, failed!,cha_id =%d\n", cha_id);
	}
	_tbl_name = l_tblname;
	if (l_bret) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return l_retrow;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}
bool TBLCharacters::BackupRow(int cha_id) {
	// Use stored procedure to handle character deletion with guild count updates
	// The stored procedure handles: guild member/try count decrement and setting delflag
	const auto ret = stored_procedure("{CALL dbo.CharacterBackupRow(?)}", "dbo", "CharacterBackupRow", 1, &cha_id);
	if (!DBOK(ret)) {
		LG("GuildSystem", "BackupRow: stored procedure failed for cha_id = %d\n", cha_id);
		return false;
	}
	return true;
}

//==========TBLFriends===============================
int TBLFriends::GetFriendsCount(int cha_id1, int cha_id2) {
	int l_retrow = 0;
	char filter[200];

	const char* param1 = "count(*) num";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(cha_id1=%d AND cha_id2 =%d)OR(cha_id1=%d AND cha_id2 =%d)", cha_id1, cha_id2, cha_id2, cha_id1);

	if (_get_row(m_buf, FRD_MAXCOL, param1, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		return atoi(m_buf[0].c_str());
	} else {
		return -1;
	}
}

int TBLFriends::GetGroupCount(int cha_id1) {
	int l_retrow = 0;
	char filter[200];
	char buffer[255];
	memset(buffer, 0, sizeof(buffer));

	const char* param1 = "count(*) num";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "1=1");
	_tbl_name = "(select distinct friends.relation relation from friends\
						where friends.cha_id1 =%d and friends.cha_id2 = -1) cc";

	_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, _tbl_name.c_str(), cha_id1);

	_tbl_name = buffer;

	if (_get_row(m_buf, FRD_MAXCOL, param1, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		_tbl_name = "friends";
		return atoi(m_buf[0].c_str());
	} else {
		_tbl_name = "friends";
		return -1;
	}
}
uintptr_t TBLFriends::GetFriendAddr(int cha_id1, int cha_id2) {
	int l_retrow = 0;
	char filter[200];

	const char* param = "character.mem_addr addr";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(friends.cha_id1=%d AND friends.cha_id2 =%d)", cha_id1, cha_id2);
	_tbl_name = "character (nolock) INNER JOIN friends ON character.cha_id = friends.cha_id2";
	if (_get_row(m_buf, FRD_MAXCOL, param, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		_tbl_name = "friends";
		return static_cast<uintptr_t>(strtoull(m_buf[0].c_str(), nullptr, 10));
	} else {
		_tbl_name = "friends";
		return 0;
	}
}

bool TBLFriends::UpdateGroup(int cha_id1, int cha_id2, const char* newgroup) {
	// Use stored procedure to update friend group by character IDs
	const auto ret = stored_procedure("{CALL dbo.FriendsUpdateGroupById(?, ?, ?)}", "dbo", "FriendsUpdateGroupById", 3, newgroup, &cha_id1, &cha_id2);
	return DBOK(ret);
}

bool TBLFriends::UpdateGroup(int cha_id1, const char* oldgroup, const char* newgroup) {
	// Use stored procedure to update friend group by relation name
	const auto ret = stored_procedure("{CALL dbo.FriendsUpdateGroupByName(?, ?, ?)}", "dbo", "FriendsUpdateGroupByName", 3, newgroup, &cha_id1, oldgroup);
	return DBOK(ret);
}

bool TBLFriends::AddFriend(int cha_id1, int cha_id2) {
	// Use stored procedure to add bidirectional friendship
	const auto ret = stored_procedure("{CALL dbo.FriendsAdd(?, ?)}", "dbo", "FriendsAdd", 2, &cha_id1, &cha_id2);
	return DBOK(ret);
}
bool TBLFriends::DelFriend(int cha_id1, int cha_id2) {
	// Use stored procedure to delete bidirectional friendship
	const auto ret = stored_procedure("{CALL dbo.FriendsDelete(?, ?)}", "dbo", "FriendsDelete", 2, &cha_id1, &cha_id2);
	return DBOK(ret);
}

//==========TBLMaster===============================
int TBLMaster::GetMasterCount(int cha_id) {
	int l_retrow = 0;
	char filter[200];

	const char* param1 = "count(*) num";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(cha_id1=%d)", cha_id);
	if (_get_row(m_buf, MASTER_MAXCOL, param1, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		return atoi(m_buf[0].c_str());
	} else {
		return -1;
	}
}

int TBLMaster::GetPrenticeCount(int cha_id) {
	int l_retrow = 0;
	char filter[200];

	const char* param1 = "count(*) num";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(cha_id2=%d AND finish=0)", cha_id);
	if (_get_row(m_buf, MASTER_MAXCOL, param1, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		return atoi(m_buf[0].c_str());
	} else {
		return -1;
	}
}

int TBLMaster::HasMaster(int cha_id1, int cha_id2) {
	int l_retrow = 0;
	char filter[200];

	const char* param1 = "count(*) num";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(cha_id1=%d AND cha_id2=%d)", cha_id1, cha_id2);
	if (_get_row(m_buf, MASTER_MAXCOL, param1, filter, &l_retrow) && l_retrow == 1 && get_affected_rows() == 1) {
		return atoi(m_buf[0].c_str());
	} else {
		return -1;
	}
}

bool TBLMaster::AddMaster(int cha_id1, int cha_id2) {
	// Use stored procedure to add master-apprentice relationship
	const auto ret = stored_procedure("{CALL dbo.MasterAdd(?, ?)}", "dbo", "MasterAdd", 2, &cha_id1, &cha_id2);
	return DBOK(ret);
}

bool TBLMaster::DelMaster(int cha_id1, int cha_id2) {
	// Use stored procedure to delete master-apprentice relationship
	const auto ret = stored_procedure("{CALL dbo.MasterDelete(?, ?)}", "dbo", "MasterDelete", 2, &cha_id1, &cha_id2);
	return DBOK(ret);
}

bool TBLMaster::FinishMaster(int cha_id) {
	// Use stored procedure to mark master relationship as finished
	const auto ret = stored_procedure("{CALL dbo.MasterFinish(?)}", "dbo", "MasterFinish", 1, &cha_id);
	return DBOK(ret);
}

bool TBLMaster::InitMasterRelation(map<uLong, uLong>& mapMasterRelation) {
	static char const query_master_format[SQL_MAXLEN] =
		"select cha_id1 cha_id1,cha_id2 cha_id2 from %s";

	bool ret = false;
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, query_master_format, _get_table());

	// 执行查询操作
	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLSMALLINT col_num = 0;
	bool found = true;

	try {
		do {
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				throw 1;
			}

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				if (sqlret != SQL_SUCCESS_WITH_INFO)
					throw 2;
			}

			sqlret = SQLNumResultCols(hstmt, &col_num);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			// Bind Column
			for (int i = 0; i < col_num; ++i) {
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
			}

			// Fetch each Row
			for (int i = 0; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++i) {
				if (sqlret != SQL_SUCCESS)
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

				uLong ulPID = atoi((char const*)_buf[0]);
				uLong ulMID = atoi((char const*)_buf[1]);

				mapMasterRelation[ulPID] = ulMID;
			}

			SQLFreeStmt(hstmt, SQL_CLOSE);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
			SQLFreeStmt(hstmt, SQL_UNBIND);
			ret = true;

		} while (0);
	} catch (...) {
		LG("Master", "Unknown Exception raised when InitMasterRelation()\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}

bool TBLMaster::GetMasterData(master_dat* farray, int& array_num, unsigned int cha_id) {
	// NOTE(Ogge): Remember the size of farray before resetting array_num to be used as a return value
	const auto array_size{array_num};
	array_num = 0;

	constexpr const char query_master_format[] =
		"select '' relation,count(*) addr,0 cha_id,'' cha_name,0 icon,'' motto from ( \
		select distinct master.relation relation from character INNER JOIN \
		master ON character.cha_id = master.cha_id2 where master.cha_id1 = %d \
		) cc union select master.relation relation,count(character.mem_addr) addr,0 \
		cha_id,'' cha_name,1 icon,'' motto from character INNER JOIN master ON \
		character.cha_id = master.cha_id2 where master.cha_id1 = %d group by relation \
		union select master.relation relation,character.mem_addr addr,character.cha_id \
		cha_id,character.cha_name cha_name,character.icon icon,character.motto motto \
		from character INNER JOIN master ON character.cha_id = master.cha_id2 \
		where master.cha_id1 = %d order by relation,cha_id,icon";

	if (!farray || array_size <= 0 || cha_id == 0)
		return false;

	bool ret = false;
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, query_master_format, cha_id, cha_id, cha_id);

	// 执行查询操作
	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLSMALLINT col_num = 0;
	bool found = true;

	try {
		do {
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				throw 1;
			}

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				if (sqlret != SQL_SUCCESS_WITH_INFO)
					throw 2;
			}

			sqlret = SQLNumResultCols(hstmt, &col_num);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			// Bind Column
			for (int i = 0; i < col_num; ++i) {
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
			}

			// Fetch each Row
			int i = 0;
			for (; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++i) {
				if (i >= array_size) {
					break;
				}

				if (sqlret != SQL_SUCCESS)
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

				farray[i].relation = (char const*)_buf[0];
				farray[i].memaddr = strtoull((char const*)_buf[1], nullptr, 10);
				farray[i].cha_id = atoi((char const*)_buf[2]);
				farray[i].cha_name = (char const*)_buf[3];
				farray[i].icon_id = atoi((char const*)_buf[4]);
				farray[i].motto = (char const*)_buf[5];
			}

			array_num = i; // NOTE(Ogge): Return number of rows read through referenced argument

			SQLFreeStmt(hstmt, SQL_CLOSE);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
			SQLFreeStmt(hstmt, SQL_UNBIND);
			ret = true;

		} while (0);
	} catch (...) {
		LG("Master", "Unknown Exception raised when GetMasterData()\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}

bool TBLMaster::GetPrenticeData(master_dat* farray, int& array_num, unsigned int cha_id) {
	const auto array_size{array_num};
	array_num = 0;

	constexpr const char query_prentice_format[] =
		"select '' relation,count(*) addr,0 cha_id,'' cha_name,0 icon,'' motto from ( \
		select distinct master.relation relation from character INNER JOIN \
		master ON character.cha_id = master.cha_id1 where master.cha_id2 = %d \
		) cc union select master.relation relation,count(character.mem_addr) addr,0 \
		cha_id,'' cha_name,1 icon,'' motto from character INNER JOIN master ON \
		character.cha_id = master.cha_id1 where master.cha_id2 = %d group by relation \
		union select master.relation relation,character.mem_addr addr,character.cha_id \
		cha_id,character.cha_name cha_name,character.icon icon,character.motto motto \
		from character INNER JOIN master ON character.cha_id = master.cha_id1 \
		where master.cha_id2 = %d order by relation,cha_id,icon";

	if (!farray || array_size <= 0 || cha_id == 0) {
		return false;
	}

	bool ret = false;
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, query_prentice_format, cha_id, cha_id, cha_id);

	// 执行查询操作
	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLSMALLINT col_num = 0;
	bool found = true;

	try {
		do {
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
				throw 1;
			}

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				if (sqlret != SQL_SUCCESS_WITH_INFO)
					throw 2;
			}

			sqlret = SQLNumResultCols(hstmt, &col_num);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			// Bind Column
			for (int i = 0; i < col_num; ++i) {
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
			}

			// Fetch each Row
			int i = 0;
			for (; ((sqlret = SQLFetch(hstmt)) == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO); ++i) {
				if (i >= array_size) {
					break;
				}

				if (sqlret != SQL_SUCCESS)
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret, sql);

				farray[i].relation = (char const*)_buf[0];
				farray[i].memaddr = strtoull((char const*)_buf[1], nullptr, 10);
				farray[i].cha_id = atoi((char const*)_buf[2]);
				farray[i].cha_name = (char const*)_buf[3];
				farray[i].icon_id = atoi((char const*)_buf[4]);
				farray[i].motto = (char const*)_buf[5];
			}

			array_num = i; // 取出的行数

			SQLFreeStmt(hstmt, SQL_CLOSE);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
			SQLFreeStmt(hstmt, SQL_UNBIND);
			ret = true;

		} while (0);
	} catch (...) {
		LG("Master", "Unknown Exception raised when GetPrenticeData()\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}

//==========TBLGuilds===============================
bool TBLGuilds::IsReady() {
	int l_retrow = 0;
	const char* param = "count(*)";
	if (_get_row(m_buf, 1, param, 0, &l_retrow)) {
		if (l_retrow == 1 &&
			get_affected_rows() == 1 &&
			atoi(m_buf[0].c_str()) == 199) // NOTE: 199 is the maximum guild count
		{
			return true;
		}
	}
	return false;
}

int TBLGuilds::FetchRowByName(const char* guild_name) {
	// Use stored procedure to prevent SQL injection
	int l_retrow = 0;
	if (_get_row_stored_procedure(m_buf, 1, "{CALL dbo.GetGuildByName(?)}", 
		"dbo", "GetGuildByName", &l_retrow, 1, guild_name)) {
		if (l_retrow == 1 && get_affected_rows() == 1) {
			return l_retrow;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}
bool TBLGuilds::Disband(uLong gldid) {
	// Use stored procedure to disband guild - resets guild data and clears member guild associations
	int l_gldid = static_cast<int>(gldid);
	const auto ret = stored_procedure("{CALL dbo.GuildDisband(?)}", "dbo", "GuildDisband", 1, &l_gldid);
	if (!DBOK(ret)) {
		LG("Guild", "dismiss guild SQL failed! guild ID:%d\n", gldid);
		return false;
	}
	return true;
}
bool TBLGuilds::InitAllGuilds(char disband_days) {
	string sql_syntax = "";
	if (disband_days < 1) {
		return false;
	} else {
		/*
		sql_syntax =
			"	select g.guild_id, g.guild_name, g.motto, g.leader_id,g.type,g.stat,\
						g.money, g.exp, g.member_total, g.try_total,g.disband_date,\
						case when g.stat>0 then DATEDIFF(mi,g.disband_date,GETDATE()) else 0 end  解散考察累计分钟,\
						case when g.stat>0 then %d*24*60 -DATEDIFF(mi,g.disband_date,GETDATE()) else 0 end 解散考察剩余分钟\
					from guild As g\
					where (g.guild_id >0)\
			";
		*/
		sql_syntax =
			string("	select g.guild_id, g.guild_name, g.motto, g.leader_id,\
						g.exp, g.member_total, g.try_total,g.disband_date") +
			string(",g.level from guild As g where (g.guild_id >0) ");
	}

	bool l_ret = false;
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, sql_syntax.c_str(), disband_days);

	// 执行查询操作
	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLSMALLINT col_num = 0;
	bool found = true;

	try {
		sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			throw 1;
		}

		sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
		if (sqlret != SQL_SUCCESS) {
			handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			if (sqlret != SQL_SUCCESS_WITH_INFO)
				throw 2;
		}

		sqlret = SQLNumResultCols(hstmt, &col_num);
		col_num = min(col_num, MAX_COL);
		col_num = min(col_num, _max_col);

		// Bind Column
		for (int i = 0; i < col_num; ++i) {
			SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
		}

		// Fetch each Row	int i; // 取出的行数
		for (int f_row = 1; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			}
			Guild* l_gld = Guild::Alloc();
			l_gld->m_id = atoi((cChar*)_buf[0]);			// 公会ID
			strncpy_s(l_gld->m_name, sizeof(l_gld->m_name), (cChar*)_buf[1], _TRUNCATE);
			strncpy_s(l_gld->m_motto, sizeof(l_gld->m_motto), (cChar*)_buf[2], _TRUNCATE);
			l_gld->m_leaderID = atoi((cChar*)_buf[3]);		// 会长ID
			l_gld->m_remain_minute = atoi((cChar*)_buf[7]); // 公会解散剩余分钟数
			l_gld->m_tick = GetTickCount();

			l_gld->BeginRun();
		}

		SQLFreeStmt(hstmt, SQL_UNBIND);
		l_ret = true;
	} catch (int& e) {
		LG("Guild", "init guild ODBC interface failed, InitAllGuilds() error:%d\n", e);
	} catch (...) {
		LG("Guild", "Unknown Exception raised when InitAllGuilds()\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return l_ret;
}
bool TBLGuilds::SendGuildInfo(Player* ply) {
	WPacket l_togmSelf = g_gpsvr->GetWPacket();
	l_togmSelf.WriteCmd(CMD_PM_GUILDINFO);
	l_togmSelf.WriteLong(ply->m_chaid[ply->m_currcha]); // 角色DBID
	l_togmSelf.WriteLong(ply->m_guild[ply->m_currcha]); // 公会ID
	l_togmSelf.WriteLong(ply->GetGuild()->m_leaderID);	// 会长ID
	l_togmSelf.WriteString(ply->GetGuild()->m_name);	// 公会name
	l_togmSelf.WriteString(ply->GetGuild()->m_motto);	// 公会座佑名
	ply->m_gate->GetDataSock()->SendData(l_togmSelf);
	return true;
}
bool TBLGuilds::InitGuildMember(Player* ply, uLong chaid, uLong gldid, int mode) {
	bool l_ret = false;
	if (ply && gldid == 0) {
		WPacket l_toSelf = g_gpsvr->GetWPacket();
		l_toSelf.WriteCmd(CMD_PC_GUILD);
		l_toSelf.WriteChar(MSG_GUILD_START);

		l_toSelf.WriteLong(0);
		l_toSelf.WriteChar(0);

		g_gpsvr->SendToClient(ply, l_toSelf);
	} else {
		const char* sql_syntax = 0;
		char sql[SQL_MAXLEN];
		sql_syntax =
			"	select c.mem_addr,c.cha_id, c.cha_name, c.motto, c.job, c.degree, c.icon, c.guild_permission\
					from character As c\
					where (c.guild_stat =0) and (c.guild_id =%d) and (c.delflag = 0)\
			";
		_snprintf_s(sql, sizeof(sql), _TRUNCATE, sql_syntax, gldid);
		// 执行查询操作
		SQLRETURN sqlret;
		SQLHSTMT hstmt = SQL_NULL_HSTMT;
		SQLSMALLINT col_num = 0;
		bool found = true;

		try {
			sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
			if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
				handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);

				throw 1;
			}

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);

				if (sqlret != SQL_SUCCESS_WITH_INFO)
					throw 2;
			}

			sqlret = SQLNumResultCols(hstmt, &col_num);
			col_num = min(col_num, MAX_COL);
			col_num = min(col_num, _max_col);

			// Bind Column
			for (int i = 0; i < col_num; ++i) {
				SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
			}
			WPacket l_toGuild = g_gpsvr->GetWPacket();
			l_toGuild.WriteCmd(CMD_PC_GUILD);
			if (mode) {
				l_toGuild.WriteChar(MSG_GUILD_ADD);
			} else {
				l_toGuild.WriteChar(MSG_GUILD_ONLINE);
				l_toGuild.WriteLong(chaid);
			}

			WPacket l_toSelf, l_wpk0;
			if (ply) {
				l_wpk0 = g_gpsvr->GetWPacket();
				l_wpk0.WriteCmd(CMD_PC_GUILD);
				l_wpk0.WriteChar(MSG_GUILD_START);
			}
			bool l_hrd = false;

			Player* l_plylst[10240];
			short l_plynum = 0;

			int lPacketNum = 0;

			// Fetch each Row	int i; // 取出的行数
			int f_row = 1;
			for (; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
				if (sqlret != SQL_SUCCESS) {
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				}
				if (ply && (f_row % 20) == 1) {
					l_toSelf = l_wpk0;
				}
				if (ply && !l_hrd) {
					l_hrd = true;
					l_toSelf.WriteLong(ply->m_guild[ply->m_currcha]); // 公会ID
					l_toSelf.WriteString(ply->GetGuild()->m_name);	  // 公会name
					l_toSelf.WriteLong(ply->GetGuild()->m_leaderID);  // 会长ID
				}
				unsigned long long l_memaddr = strtoull((cChar*)_buf[0], nullptr, 10);
				if (l_memaddr) {
					Player* l_ply_ptr = (Player*)MakePointer(l_memaddr);
					// Validate pointer is registered before adding to list
					if (l_ply_ptr && g_gpsvr->IsPlayerRegistered(l_ply_ptr)) {
						l_plylst[l_plynum] = l_ply_ptr;
						l_plynum++;
					}
				}
				if (mode && chaid == atoi((cChar*)_buf[1])) {
					l_toGuild.WriteChar(l_memaddr ? 1 : 0);		  // online
					l_toGuild.WriteLong(atoi((cChar*)_buf[1]));	  // chaid
					l_toGuild.WriteString((cChar*)_buf[2]);		  // chaname
					l_toGuild.WriteString((cChar*)_buf[3]);		  // motto
					l_toGuild.WriteString((cChar*)_buf[4]);		  // job
					l_toGuild.WriteShort(atoi((cChar*)_buf[5]));  // degree
					l_toGuild.WriteShort(atoi((cChar*)_buf[6]));  // icon
					l_toGuild.WriteLong(stoull((cChar*)_buf[7])); // permission
				}
				if (ply) {
					l_toSelf.WriteChar(l_memaddr ? 1 : 0);		 // online
					l_toSelf.WriteLong(atoi((cChar*)_buf[1]));	 // chaid
					l_toSelf.WriteString((cChar*)_buf[2]);		 // chaname
					l_toSelf.WriteString((cChar*)_buf[3]);		 // motto
					l_toSelf.WriteString((cChar*)_buf[4]);		 // job
					l_toSelf.WriteShort(atoi((cChar*)_buf[5]));	 // degree
					l_toSelf.WriteShort(atoi((cChar*)_buf[6]));	 // icon
					l_toSelf.WriteLong(stoull((cChar*)_buf[7])); // permission
				}
				if (ply && !(f_row % 20)) {
					l_toSelf.WriteLong(lPacketNum);
					lPacketNum++;
					l_toSelf.WriteChar(((f_row - 1) % 20) + 1); // 本次包括的条数
					g_gpsvr->SendToClient(ply, l_toSelf);
				}
			}
			if (ply && (f_row % 20) == 1) {
				l_toSelf = l_wpk0;
			}
			if (ply && !l_hrd) {
				l_hrd = true;
				l_toSelf.WriteLong(ply->m_guild[ply->m_currcha]); // 公会ID
				l_toSelf.WriteString(ply->GetGuild()->m_name);	  // 公会name
				l_toSelf.WriteLong(ply->GetGuild()->m_leaderID);  // 会长ID
			}
			if (ply) {
				l_toSelf.WriteLong(lPacketNum);
				lPacketNum++;
				l_toSelf.WriteChar((f_row - 1) % 20);
				g_gpsvr->SendToClient(ply, l_toSelf);
			}
			LG("Guild", "online guild num:%d\n", l_plynum);
			g_gpsvr->SendToClient(l_plylst, l_plynum, l_toGuild);

			SQLFreeStmt(hstmt, SQL_UNBIND);
			l_ret = true;
		} catch (int& e) {
			LG("Guild", "init guild ODBC interface failed, InitGuildMember() error:%d, SQL:%s\n", e, sql);
		} catch (...) {
			LG("Guild", "Unknown Exception raised when InitGuildMember()\n");
		}

		if (hstmt != SQL_NULL_HSTMT) {
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			hstmt = SQL_NULL_HSTMT;
		}
	}

	return l_ret;
}

bool TBLParam::InitParam(void) {
	string strSQL = "select param1,param2,param3,param4,param5,param6,param7,param8,param9,param10 from param where id = 1";
	//	string strSQL = "select param1 from param where id = 1";

	SQLRETURN sqlret;
	SQLHSTMT hstmt = SQL_NULL_HSTMT;
	SQLLEN buf_len[MAXORDERNUM + MAXORDERNUM];
	//	SQLINTEGER		nID = 0,nlen;
	bool found = true;

	try {
		sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			throw 1;
		}
		sqlret = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0);
		if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			throw 1;
		}
		SQLBindCol(hstmt, 1, SQL_C_SLONG, &m_nOrder[0].nid, 0, &(buf_len[0]));
		SQLBindCol(hstmt, 2, SQL_C_SLONG, &m_nOrder[1].nid, 0, &buf_len[1]);
		SQLBindCol(hstmt, 3, SQL_C_SLONG, &m_nOrder[2].nid, 0, &buf_len[2]);
		SQLBindCol(hstmt, 4, SQL_C_SLONG, &m_nOrder[3].nid, 0, &buf_len[3]);
		SQLBindCol(hstmt, 5, SQL_C_SLONG, &m_nOrder[4].nid, 0, &buf_len[4]);

		SQLBindCol(hstmt, 6, SQL_C_SLONG, &m_nOrder[0].nfightpoint, 0, &buf_len[5]);
		SQLBindCol(hstmt, 7, SQL_C_SLONG, &m_nOrder[1].nfightpoint, 0, &buf_len[6]);
		SQLBindCol(hstmt, 8, SQL_C_SLONG, &m_nOrder[2].nfightpoint, 0, &buf_len[7]);
		SQLBindCol(hstmt, 9, SQL_C_SLONG, &m_nOrder[3].nfightpoint, 0, &buf_len[8]);
		SQLBindCol(hstmt, 10, SQL_C_SLONG, &m_nOrder[4].nfightpoint, 0, &buf_len[9]);
		sqlret = SQLExecDirect(hstmt, (SQLCHAR*)const_cast<char*>(strSQL.c_str()), SQL_NTS);
		if (sqlret != SQL_SUCCESS) {
			handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			if (sqlret != SQL_SUCCESS_WITH_INFO)
				throw 2;
		}

		sqlret = SQLFetch(hstmt);
		if (sqlret != SQL_SUCCESS) {
			// Modfi by lark.li 20080714 begin
			if (sqlret != SQL_NO_DATA)
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			// End
		}
		SQLFreeStmt(hstmt, SQL_CLOSE);
		SQLFreeStmt(hstmt, SQL_UNBIND);
	} catch (int& e) {
		LG("Garner2", "init guild ODBC interface failed, InitParam() error:%d\n", e);
	} catch (...) {
		LG("Garner2", "Unknown Exception raised when InitParam()\n");
	}

	char buff[255];
	int nlev;
	try {
		sqlret = SQLAllocHandle(SQL_HANDLE_STMT, _hdbc, &hstmt);
		if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO)) {
			handle_err(_hdbc, SQL_HANDLE_DBC, sqlret);
			throw 1;
		}
		SQLBindCol(hstmt, 1, SQL_C_CHAR, _buf[0], MAX_DATALEN, &buf_len[0]);
		SQLBindCol(hstmt, 2, SQL_C_CHAR, _buf[1], MAX_DATALEN, &buf_len[1]);
		SQLBindCol(hstmt, 3, SQL_C_SLONG, &nlev, 0, &buf_len[2]);
		for (int n = 0; n < MAXORDERNUM; n++) {
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, "select cha_name,job,degree from character where cha_id = %d ", m_nOrder[n].nid);

			sqlret = SQLExecDirect(hstmt, (SQLCHAR*)buff, SQL_NTS);
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				if (sqlret != SQL_SUCCESS_WITH_INFO)
					throw 2;
			}
			int i = 0;
			if ((sqlret = SQLFetch(hstmt)) != SQL_NO_DATA) {
				if (sqlret == SQL_NO_DATA) {
					LG("Garner2", "cha name query failed .cha ID：%d\n", m_nOrder[n].nid);
					continue;
				}
				if (sqlret != SQL_SUCCESS) {
					handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
				}
				if (buf_len[0] > 20) {
					LG("Garner2", "cha name query failed.\n");
					return false;
				}
				memcpy(m_nOrder[n].strname, _buf[0], buf_len[0]);
				m_nOrder[n].strname[buf_len[0]] = '\\0';
				memcpy(m_nOrder[n].strjob, _buf[1], buf_len[1]);
				m_nOrder[n].strjob[buf_len[1]] = '\\0';
				m_nOrder[n].nlev = nlev;
			}
			SQLFreeStmt(hstmt, SQL_CLOSE);
		}
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	} catch (int& e) {
		LG("Garner2", "init guild ODBC interface failed, InitParam() erro :%d\n", e);
	} catch (...) {
		LG("Garner2", "Unknown Exception raised when InitParam()\n");
	}
	return true;
}

bool TBLParam::SaveParam(void) {
	// Use stored procedure to save ranking parameters
	const auto ret = stored_procedure("{CALL dbo.ParamSave(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)}", "dbo", "ParamSave", 10,
		&m_nOrder[0].nid, &m_nOrder[1].nid, &m_nOrder[2].nid, &m_nOrder[3].nid, &m_nOrder[4].nid,
		&m_nOrder[0].nfightpoint, &m_nOrder[1].nfightpoint, &m_nOrder[2].nfightpoint, &m_nOrder[3].nfightpoint, &m_nOrder[4].nfightpoint);
	if (!DBOK(ret)) {
		LG("ParamErr", "Save Param Error via stored procedure");
	}
	return true;
}

bool TBLParam::IsReady() {
	int l_retrow = 0;
	const char* param = "count(*)";
	if (_get_row(m_buf, 1, param, 0, &l_retrow)) {
		if (l_retrow == 1 && get_affected_rows() == 1 && atoi(m_buf[0].c_str()) >= 199) {
			return true;
		}
	}
	return false;
}

void TBLParam::UpdateOrder(ORDERINFO& Order) {
	ORDERINFO ordertemp[MAXORDERNUM];

	memcpy(ordertemp, m_nOrder, sizeof(ORDERINFO) * MAXORDERNUM);

	int i = 0;
	int oldid = 0;
	for (i = 0; i < MAXORDERNUM; i++) {
		if (ordertemp[i].nfightpoint >= Order.nfightpoint) {
			if (ordertemp[i].nid == Order.nid)
				break;
			continue;
		} else {
			oldid = i;
			if (ordertemp[i].nid == Order.nid) {
				m_nOrder[i].nfightpoint = Order.nfightpoint;
				break;
			}
			memcpy(&m_nOrder[i++], &Order, sizeof(ORDERINFO));

			int n = -1;
			for (int a = i; a < MAXORDERNUM;) {
				if (ordertemp[a + n].nid == Order.nid) {
					n++;
					continue;
				}

				if (a + n < MAXORDERNUM)
					memcpy(&m_nOrder[a], &ordertemp[a + n], sizeof(ORDERINFO));
				else {
					strcpy(m_nOrder[a].strjob, "");
					strcpy(m_nOrder[a].strname, "");
					m_nOrder[a].nid = -1;
					m_nOrder[a].nlev = 0;
					m_nOrder[a].nfightpoint = 0;
				}
				a++;
			}

			SaveParam();
			WPacket l_wpk = g_gpsvr->GetWPacket();
			l_wpk.WriteCmd(CMD_PM_GARNER2_UPDATE);
			for (i = 0; i < MAXORDERNUM; i++) {
				l_wpk.WriteLong(m_nOrder[i].nid);
			}
			l_wpk.WriteLong(oldid);
			l_wpk.WriteLong(0);

			for (auto& gate : g_gpsvr->m_gate) {
				if (gate.GetDataSock()) {
					gate.GetDataSock()->SendData(l_wpk);
					break;
				}
			}
			LG("Garner2", "order chaned\n");
			break;
		}
	}
}
