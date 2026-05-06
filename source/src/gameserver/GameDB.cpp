#include "stdafx.h"
#include "Character.h"
#include "Player.h"
#include "GameDB.h"
#include "ChaAttrType.h"
#include "SubMap.h"
#include "Config.h"
#include "Guild.h"
#include "CommFunc.h"
#include "lua_gamectrl.h"

using namespace std;

char szDBLog[256] = "DBData";
//-------------------
// Check if character name exists
//-------------------
BOOL CTableCha::VerifyName(const char* pszName) {
	T_B
	// Use stored procedure to prevent SQL injection
	string buf[1];
	int l_retrow = 0;
	bool ret = _get_row_stored_procedure(buf, 1, "{CALL dbo.VerifyCharacterName(?)}", 
		"dbo", "VerifyCharacterName", &l_retrow, 1, pszName);
	if (ret && l_retrow > 0) {
		// Stored procedure returns 1 if name exists, 0 if not
		int nameExists = atoi(buf[0].c_str());
		return nameExists == 1 ? TRUE : FALSE;
	}
	return FALSE;
	T_E
}

std::string CTableCha::GetName(int cha_id) {
	// Use stored procedure to get character name
	string buf[1];
	int l_retrow = 0;
	bool ret = _get_row_stored_procedure(buf, 1, "{CALL dbo.GetCharacterName(?)}", 
		"dbo", "GetCharacterName", &l_retrow, 1, &cha_id);
	return (ret && l_retrow > 0) ? buf[0] : "";
}

#define defKITBAG_DATA_STRING_LEN 16384
#define defSKILLBAG_DATA_STRING_LEN 1500
#define defSHORTCUT_DATA_STRING_LEN 1500
#define defSSTATE_DATE_STRING_LIN 1024

const int g_cnCol = 64;
string g_buf[g_cnCol];
char g_sql[1024 * 1024]{};
char g_kitbag[defKITBAG_DATA_STRING_LEN] = {};
char g_kitbagTmp[defKITBAG_DATA_STRING_LEN] = {};
char g_equip[defKITBAG_DATA_STRING_LEN] = {};
char g_look[defLOOK_DATA_STRING_LEN] = {};
char g_skillbag[defSKILLBAG_DATA_STRING_LEN] = {};
char g_shortcut[defSHORTCUT_DATA_STRING_LEN] = {};
char g_skillstate[defSSTATE_DATE_STRING_LIN] = {};

// Add by lark.li 20080723 begin
char g_extendAttr[ROLE_MAXSIZE_DBMISCOUNT];
// End

// ?????????????
char g_szMisInfo[ROLE_MAXSIZE_DBMISSION];
char g_szRecord[ROLE_MAXSIZE_DBRECORD];
char g_szTrigger[ROLE_MAXSIZE_DBTRIGGER];
char g_szMisCount[ROLE_MAXSIZE_DBMISCOUNT];

bool CTableMaster::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				   cha_id1, cha_id2, finish, relation\
				   from %s \
				   (nolock) where 1=2",
			_get_table());

	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(master)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "master");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

unsigned int CTableMaster::GetMasterDBID(CPlayer* pPlayer) {
	if (!pPlayer) {
		return 0;
	}
	CCharacter* pCha = pPlayer->GetMainCha();

	if (!pCha) {
		return 0;
	}

	int nIndex = 0;
	char param[] = "cha_id2";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_id1=%d", pPlayer->GetDBChaId());
	int r = _get_row(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		return (unsigned int)(Str2Int(g_buf[nIndex++]));
	} else {
		return (unsigned int)0;
	}
}

bool CTableMaster::IsMasterRelation(int masterID, int prenticeID) {
	char param[] = "count(*)";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "(cha_id1=%d) and (cha_id2=%d)", prenticeID, masterID);
	int r = _get_row(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		if (Str2Int(g_buf[0]) > 0)
			return true;
	}

	return false;
}

bool CTableCha::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				cha_id, cha_name, motto, icon, version, pk_ctrl, mem_addr, act_id, guild_id, guild_stat, guild_permission, job, degree, exp, \
				hp, sp, ap, tp, gd, str, dex, agi, con, sta, luk, sail_lv, sail_exp, sail_left_exp, live_lv, live_exp, map, main_map, map_x, map_y, radius, \
				angle, look, skillbag, shortcut, mission, misrecord, mistrigger, miscount, birth, login_cha, live_tp, bank, \
				delflag, operdate, skill_state, kitbag, kitbag_tmp, kb_locked, credit, store_item \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(character)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "character");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableCha::ShowExpRank(CCharacter* pCha, int count) {
	bool ret = false;

	const char* sql_syntax = "select top %d cha_name,job,degree from %s where delflag =0 ORDER BY CASE WHEN (exp < 0) THEN (exp+4294967296) ELSE exp END DESC";
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, sql_syntax, count, _get_table());

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
		col_num = std::min<decltype(col_num)>(col_num, MAX_COL);
		col_num = std::min<decltype(col_num)>(col_num, _max_col);

		// Bind Column
		for (int i = 0; i < col_num; ++i) {
			SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
		}

		WPACKET l_wpk = GETWPACKET();
		WRITE_CMD(l_wpk, CMD_MC_RANK);

		int f_row = 0;
		for (; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			}

			WRITE_STRING(l_wpk, (char const*)_buf[0]);		// name
			WRITE_STRING(l_wpk, (char const*)_buf[1]);		// job
			WRITE_SHORT(l_wpk, atoi((char const*)_buf[2])); // lv
		}

		WRITE_SHORT(l_wpk, f_row);
		pCha->ReflectINFof(pCha, l_wpk);

		SQLFreeStmt(hstmt, SQL_UNBIND);
		ret = true;
	} catch (int& e) {
		LG("consortia system", "consult apply consortia process memeberODBC interface transfer error,position ID:%d\n", e);
	} catch (...) {
		LG("consortia system", "Unknown Exception raised when list rank\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}

//-----------------------
// ??????????????????
//-----------------------
bool CTableCha::ReadAllData(CPlayer* pPlayer, DWORD cha_id) {
	T_B if (!pPlayer) {
		// LG("enter_map", "??????????Player???.\n");
		LG("enter_map", "Loading database error??Player is empty.\n");
		return false;
	}
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha || (pPlayer->GetDBChaId() != cha_id)) {
		// LG("enter_map", "?????????????????????????.\n");
		LG("enter_map", "Loading database error,the Main character is inexistence or not matching.\n");
		return false;
	}

	int nIndex = 0;
	int r1 = 0;
	// Use stored procedure for ReadAllData
	int r = _get_row_stored_procedure(g_buf, g_cnCol, 
		"{CALL dbo.ReadAllData(?)}", "dbo", "ReadAllData", &r1, 1, &cha_id);
	// LG("enter_map", "???????????_get_row.\n");
	LG("enter_map", "Loading database succeed??_get_row_stored_procedure.\n");
	if (DBOK(r) && r1 > 0) {
		pPlayer->SetDBActId(Str2Int(g_buf[nIndex++]));
		pCha->SetGuildState(Str2Int(g_buf[nIndex++]));
		pCha->SetGuildID(Str2Int(g_buf[nIndex++]));
		// pCha->setAttr(ATTR_GUILD_STATE, Str2Int(g_buf[nIndex++]), 1);
		// pCha->setAttr(ATTR_GUILD, Str2Int(g_buf[nIndex++]), 1);
		// pCha->setAttr(ATTR_GUILD_TYPE, game_db.GetGuildTypeByID(pCha, pCha->getAttr(ATTR_GUILD)), 1);

		pCha->setAttr(ATTR_HP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_SP, Str2Int(g_buf[nIndex++]), 1);
		// pCha->setAttr(ATTR_CEXP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_CEXP, _atoi64(g_buf[nIndex++].c_str()), 1);

		pCha->SetRadius(Str2Int(g_buf[nIndex++]));
		pCha->SetAngle(Str2Int(g_buf[nIndex++]));
		pCha->SetName((char*)g_buf[nIndex++].c_str());
		Char szLogName[defLOG_NAME_LEN] = "";
		_snprintf_s(szLogName, sizeof(szLogName), _TRUNCATE, "Cha-%s+%u", pCha->GetName(), pCha->GetID());
		pCha->m_CLog.SetLogName(szLogName);

		pCha->SetMotto(g_buf[nIndex++].c_str());
		pCha->SetIcon(Str2Int(g_buf[nIndex++]));

		int lVer = Str2Int(g_buf[nIndex++]);
		if (pCha->getAttr(ATTR_HP) < 0) // ?�??
			lVer = defCHA_TABLE_NEW_VER;
		pCha->SetPKCtrl(Str2Int(g_buf[nIndex++]));

		strcpy(pCha->m_CChaAttr.m_szName, pCha->GetName());

		pCha->setAttr(ATTR_LV, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_JOB, g_GetJobID(g_buf[nIndex++].c_str()), 1);
		pCha->setAttr(ATTR_GD, _atoi64(g_buf[nIndex++].c_str()), 1);  // 64-bit for gold (100B cap)
		pCha->setAttr(ATTR_AP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_TP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BSTR, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BDEX, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BAGI, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BCON, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BSTA, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_BLUK, Str2Int(g_buf[nIndex++]), 1);

		pCha->setAttr(ATTR_SAILLV, Str2Int(g_buf[nIndex++]), 1);
		// pCha->setAttr(ATTR_CSAILEXP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_CSAILEXP, _atoi64(g_buf[nIndex++].c_str()), 1);
		pCha->setAttr(ATTR_CLEFT_SAILEXP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_LIFELV, Str2Int(g_buf[nIndex++]), 1);
		// pCha->setAttr(ATTR_CLIFEEXP, Str2Int(g_buf[nIndex++]), 1);
		pCha->setAttr(ATTR_CLIFEEXP, _atoi64(g_buf[nIndex++].c_str()), 1);
		pCha->setAttr(ATTR_LIFETP, Str2Int(g_buf[nIndex++]), 1);

		pCha->SetBirthMap(g_buf[nIndex++].c_str());
		int lPosX = Str2Int(g_buf[nIndex++]);
		int lPosY = Str2Int(g_buf[nIndex++]);
		pCha->SetPos(lPosX, lPosY);
		pCha->SetBirthCity(g_buf[nIndex++].c_str());
		// LG("enter_map", "???�????????????.\n");

		try {
			int nLookDataID = nIndex;
			if (!pCha->String2LookDate(g_buf[nIndex++])) {
				// LG("enter_map", "??????????????.\n");
				LG("enter_map", "Appearance data check sum error.\n");
				// LG("???????", "?????dbid %u??name %s??resid %u????L????????????.\n", cha_id, pCha->GetLogName(), pCha->GetKitbagRecDBID());
				LG("Check sum error", "the character (dbid %u??name %s??resid %u)'s change appearance data check sum error.\n", cha_id, pCha->GetLogName(), pCha->GetKitbagRecDBID());
				return false;
			}
			pCha->SetCat(pCha->m_SChaPart.sTypeID);
			// LG("enter_map", "???�???????.\n");
		} catch (...) {
			// LG("enter_map", "Strin2LookData????!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			LG("enter_map", "Strin2LookData error!!\n");
			// LG("enter_map", "???????? %s\n", g_buf[nIndex - 1]);
			LG("enter_map", "Appearance String %s\n", g_buf[nIndex - 1].c_str());
			throw;
		}

		int nSkillBagDataID = nIndex;
		String2SkillBagData(&pCha->m_CSkillBag, g_buf[nIndex++]);
		// LG("enter_map", "???�???????????.\n");

		int nSortcutDataID = nIndex;
		String2ShortcutData(&pCha->m_CShortcut, g_buf[nIndex++]);
		// LG("enter_map", "???�??????????.\n");

		// ??????????
		pPlayer->MisClear();
		memset(g_szMisInfo, 0, ROLE_MAXSIZE_DBMISSION);
		strncpy(g_szMisInfo, g_buf[nIndex++].c_str(), ROLE_MAXSIZE_DBMISSION - 1);
		if (!pPlayer->MisInit(g_szMisInfo)) {
			// pCha->SystemNotice( "?�???????�?????????'?????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00009));
		}
		// LG("enter_map", "???�??????1???.\n");

		memset(g_szRecord, 0, ROLE_MAXSIZE_DBRECORD);
		strncpy(g_szRecord, g_buf[nIndex++].c_str(), ROLE_MAXSIZE_DBRECORD - 1);
		if (!pPlayer->MisInitRecord(g_szRecord)) {
			// pCha->SystemNotice( "?�???????????�?????????'?????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00010));
		}
		// LG("enter_map", "???�??????2???.\n");

		memset(g_szTrigger, 0, ROLE_MAXSIZE_DBTRIGGER);
		strncpy(g_szTrigger, g_buf[nIndex++].c_str(), ROLE_MAXSIZE_DBTRIGGER - 1);
		if (!pPlayer->MisInitTrigger(g_szTrigger)) {
			// pCha->SystemNotice( "?�???????????????'?????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00011));
		}
		// LG("enter_map", "???�??????3???.\n");

		memset(g_szMisCount, 0, ROLE_MAXSIZE_DBMISCOUNT);
		strncpy(g_szMisCount, g_buf[nIndex++].c_str(), ROLE_MAXSIZE_DBMISCOUNT - 1);
		if (!pPlayer->MisInitMissionCount(g_szMisCount)) {
			// pCha->SystemNotice( "?�?????????????????'?????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00012));
		}
		// LG("enter_map", "???�??????4???.\n");

		string strList[2];
		Util_ResolveTextLine(g_buf[nIndex++].c_str(), strList, 2, ',');
		pPlayer->SetLoginCha(Str2Int(strList[0]), Str2Int(strList[1]));

		// if (lVer != defCHA_TABLE_NEW_VER) // ????????
		//	SaveTableVer(cha_id);

		pCha->SetKitbagRecDBID(Str2Int(g_buf[nIndex++]));

		pCha->SetKitbagTmpRecDBID(Str2Int(g_buf[nIndex++])); // ???????????????ID

		pPlayer->SetMapMaskDBID(Str2Int(g_buf[nIndex++]));
		g_strChaState[0] = g_buf[nIndex++];
		pPlayer->Strin2BankDBIDData(g_buf[nIndex++]);

		// ??????????
		int iLocked = Str2Int(g_buf[nIndex++]);
		pCha->m_CKitbag.SetPwdLockState(iLocked);

		// ????
		int nCredit = Str2Int(g_buf[nIndex++]);
		pCha->SetCredit(nCredit);

		pCha->SetStoreItemID(Str2Int(g_buf[nIndex++]));

		// Add by lark.li 20080723 begin
		Strin2ChaExtendAttr(pCha, g_buf[nIndex++]);

		pCha->guildPermission = stoull(g_buf[nIndex++]); // Str2Int(g_buf[nIndex++]);
		pCha->chatColour = Str2Int(g_buf[nIndex++]);
		pCha->SetIMP(Str2Int(g_buf[nIndex++]));
		// End

		// LG("enter_map", "?????????????.\n");
		LG("enter_map", "Set the whole data succeed.\n");
	} else {
		// LG("enter_map", "??????????_get_row()???????%d.%u\n", r);
		LG("enter_map", "Loading database error??_get_row() return value??%d.%u\n", r, r1);
		return false;
	}

	return true;
	T_E
}

//-----------------
// ?????????????
//-----------------
bool CTableCha::SaveAllData(CPlayer* pPlayer, char chSaveType) {
	T_B if (!pPlayer || !pPlayer->IsValid()) return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	CCharacter* pCCtrlCha = pPlayer->GetCtrlCha();
	if (pPlayer->GetLoginChaType() == enumLOGIN_CHA_BOAT) // ??????????�
	{
		CCharacter* pCLogCha = pPlayer->GetBoat(static_cast<DWORD>(pPlayer->GetLoginChaID()));
		if (pCLogCha != pCCtrlCha) // ????�???????
		{
			pCCtrlCha->SetToMainCha();
			pCCtrlCha = pCha;
			if (pCLogCha)
				// LG("??�??????????", "??�??? %s???????? %s??????? %s.\n", pCLogCha->GetLogName(), pCCtrlCha->GetLogName(), pCha->GetLogName());
				LG("logging character control error", "logging character %s,control character %s??Main character %s.\n", pCLogCha->GetLogName(), pCCtrlCha->GetLogName(), pCha->GetLogName());
			else
				// LG("??�??????????", "??�??? %s???????? %s??????? %s.\n", "", pCCtrlCha->GetLogName(), pCha->GetLogName());
				LG("logging character control error", "logging character %s,control character %s??Main character %s.\n", "", pCCtrlCha->GetLogName(), pCha->GetLogName());
			return false;
		}
	} else {
		if (pCha != pCCtrlCha) // ????�???????
		{
			pCCtrlCha = pCha;
			// LG("??�??????????", "??�??? %s???????? %s??????? %s.\n", pCCtrlCha->GetLogName(), pCCtrlCha->GetLogName(), pCha->GetLogName());
			LG("logging character control error", "logging character %s,control character %s??Main character %s.\n", pCCtrlCha->GetLogName(), pCCtrlCha->GetLogName(), pCha->GetLogName());
			return false;
		}
	}

	if (pCha) {
		// LG("enter_map", "%s ??'???�???????.\n", pCha->GetLogName());
		LG("enter_map", "%s start configure save data.\n", pCha->GetLogName());

		// pCha->m_CLog.Log("^^^^^^^^^^^^??'??????\n");
		pCha->m_CLog.Log("........... now you start save character\n");
		// pCha->m_CLog.Log("??? %d????? %s?????? [%d,%d]???????? %s.\n", pCha->m_CChaAttr.GetAttr(ATTR_LV), pCha->GetBirthMap(), pCha->GetPos().x, pCha->GetPos().y, pCha->GetBirthCity());
		pCha->m_CLog.Log("grade %d??map %s??coordinate [%d,%d]??birth city %s.\n", (int)pCha->m_CChaAttr.GetAttr(ATTR_LV), pCha->GetBirthMap(), pCha->GetPos().x, pCha->GetPos().y, pCha->GetBirthCity());
	}

	// char	szSaveCha[256] = "?????????";
	char szSaveCha[256];
	strncpy(szSaveCha, RES_STRING(GM_GAMEDB_CPP_00013), 256 - 1);

	// char	szSaveChaFile[256] = "log\\?????????.log";
	char szSaveChaFile[256];
	strncpy(szSaveChaFile, RES_STRING(GM_GAMEDB_CPP_00014), 256 - 1);

	char szLogMsg[1024] = "";
	// FILE	*fp;
	// if (!(fp = fopen(szSaveChaFile, "r")))
	//	LG(szSaveCha, "????\t???\t?????\t?????\t????\t???SQL\t???SQL[????(???)]\t????\t???????\n");
	// if (fp)
	//	fclose(fp);
	DWORD dwNowTick = GetTickCount();
	DWORD dwOldTick;
	DWORD dwTotalTick = 0;

	DWORD hp = (int)pCha->getAttr(ATTR_HP);
	DWORD sp = (int)pCha->getAttr(ATTR_SP);
	LONG64 exp = (int)pCha->getAttr(ATTR_CEXP);

	const char* map = pCCtrlCha->GetBirthMap();
	const char* main_map = pCha->GetBirthMap();
	DWORD map_x = pCha->GetShape().centre.x;
	DWORD map_y = pCha->GetShape().centre.y;
	DWORD radius = pCha->GetShape().radius;
	short angle = pCha->GetAngle();
	short degree = (short)pCha->getAttr(ATTR_LV);
	const char* job = g_GetJobName((short)pCha->getAttr(ATTR_JOB));
	__int64 gd = pCha->getAttr(ATTR_GD);
	DWORD ap = (int)pCha->getAttr(ATTR_AP);
	DWORD tp = (int)pCha->getAttr(ATTR_TP);
	DWORD str = (int)pCha->getAttr(ATTR_BSTR);
	DWORD dex = (int)pCha->getAttr(ATTR_BDEX);
	DWORD agi = (int)pCha->getAttr(ATTR_BAGI);
	DWORD con = (int)pCha->getAttr(ATTR_BCON);
	DWORD sta = (int)pCha->getAttr(ATTR_BSTA);
	DWORD luk = (int)pCha->getAttr(ATTR_BLUK);

	DWORD sail_lv = (int)pCha->getAttr(ATTR_SAILLV);
	DWORD sail_exp = (int)pCha->getAttr(ATTR_CSAILEXP);
	DWORD sail_left_exp = (int)pCha->getAttr(ATTR_CLEFT_SAILEXP);
	DWORD live_lv = (int)pCha->getAttr(ATTR_LIFELV);
	DWORD live_exp = (int)pCha->getAttr(ATTR_CLIFEEXP);
	DWORD live_tp = (int)pCha->getAttr(ATTR_LIFETP);

	DWORD nLocked = pCha->m_CKitbag.GetPwdLockState();

	DWORD dwCredit = (int)pCha->GetCredit();
	DWORD dwStoreItemID = pCha->GetStoreItemID();

	int chaIMP = pCha->GetIMP();
	int battlePower = pCha->GetBattlePower();

	char pk_ctrl = pCha->IsInPK();
	dwOldTick = dwNowTick;
	dwNowTick = GetTickCount();
	dwTotalTick += dwNowTick - dwOldTick;
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "%4u", dwNowTick - dwOldTick);
	// LG("enter_map", "???�????????????.\n");

	g_look[0] = 0;
	if (!LookData2String(&pCha->m_SChaPart, g_look, defLOOK_DATA_STRING_LEN, false)) {
		// LG("enter_map", "???%s\t??????????????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tsave data (surface) error!\n", pCha->GetLogName());
		return false;
	}
	// LG("enter_map", "???�???????.\n");

	dwOldTick = dwNowTick;
	dwNowTick = GetTickCount();
	dwTotalTick += dwNowTick - dwOldTick;
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%4u", dwNowTick - dwOldTick);

	g_skillbag[0] = 0;
	if (!SkillBagData2String(&pCha->m_CSkillBag, g_skillbag, defSKILLBAG_DATA_STRING_LEN)) {
		// LG("enter_map", "???%s\t???????????????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tsave data(skill) error!\n", pCha->GetLogName());
		return false;
	}
	// LG("enter_map", "???�???????????.\n");

	dwOldTick = dwNowTick;
	dwNowTick = GetTickCount();
	dwTotalTick += dwNowTick - dwOldTick;
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%4u", dwNowTick - dwOldTick);

	g_shortcut[0] = 0;
	if (!ShortcutData2String(&pCha->m_CShortcut, g_shortcut, defSHORTCUT_DATA_STRING_LEN)) {
		// LG("enter_map", "???%s\t?????????????????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tsave data(shortcut)error!\n", pCha->GetLogName());
		return false;
	}
	// LG("enter_map", "???�??????????.\n");

	dwOldTick = dwNowTick;
	dwNowTick = GetTickCount();
	dwTotalTick += dwNowTick - dwOldTick;
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%4u", dwNowTick - dwOldTick);

	// ???????????�???
	memset(g_szMisInfo, 0, ROLE_MAXSIZE_DBMISSION);
	if (!pPlayer->MisGetData(g_szMisInfo, ROLE_MAXSIZE_DBMISSION - 1)) {
		// pCha->SystemNotice( "?�???????????????????????!ID = %d", pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00015), pCha->GetID());
		// LG(szDBLog, "??????[ID: %d\tNAME: %s]???????????????????????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		LG(szDBLog, "save character[ID: %d\tNAME: %s]data info,Get mission data error!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}
	// LG("enter_map", "???�??????1???.\n");

	memset(g_szRecord, 0, ROLE_MAXSIZE_DBRECORD);
	if (!pPlayer->MisGetRecord(g_szRecord, ROLE_MAXSIZE_DBRECORD - 1)) {
		// pCha->SystemNotice( "?�???????????????????????!ID = %d", pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00015), pCha->GetID());
		// LG(szDBLog, "??????[ID: %d\tNAME: %s]?????????????????????�???????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		LG(szDBLog, "save character[ID: %d\tNAME: %s]data info,Get mission history data error !ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}
	// LG("enter_map", "???�??????2???.\n");

	memset(g_szTrigger, 0, ROLE_MAXSIZE_DBTRIGGER);
	if (!pPlayer->MisGetTrigger(g_szTrigger, ROLE_MAXSIZE_DBTRIGGER - 1)) {
		// pCha->SystemNotice( "?�?????????????????????????????!ID = %d", pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00016), pCha->GetID());
		// LG(szDBLog, "??????[ID: %d\tNAME: %s]???????????????????????????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		LG(szDBLog, "save character[ID: %d\tNAME: %s]data info??Get mission trigger data error!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}
	// LG("enter_map", "???�??????3???.\n");

	memset(g_szMisCount, 0, ROLE_MAXSIZE_DBMISCOUNT);
	if (!pPlayer->MisGetMissionCount(g_szMisCount, ROLE_MAXSIZE_DBMISCOUNT - 1)) {
		// pCha->SystemNotice( "?�?????????????????????????????!ID = %d", pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00017), pCha->GetID());
		// LG(szDBLog, "??????[ID: %d\tNAME: %s]?????????????????????????????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		LG(szDBLog, "save character[ID: %d\tNAME: %s]data info??Get randomicity mission take count of data error!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}
	// LG("enter_map", "???�??????4???.\n");

	const char* szBirthName = pCha->GetBirthCity();
	// dwOldTick = dwNowTick;
	// dwNowTick = GetTickCount();
	// dwTotalTick += dwNowTick - dwOldTick;
	// _snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%4u", dwNowTick - dwOldTick);

	char szLoginCha[50];
	_snprintf_s(szLoginCha, sizeof(szLoginCha), _TRUNCATE, "%u,%u", pPlayer->GetLoginChaType(), pPlayer->GetLoginChaID());

	if (chSaveType == enumSAVE_TYPE_OFFLINE) // ????
	{
		SStateData2String(pCha, g_skillstate, defSSTATE_DATE_STRING_LIN, chSaveType);
	} else if (!SStateData2String(pCha, g_skillstate, defSSTATE_DATE_STRING_LIN, chSaveType)) {
		LG("enter_map", "character %s\tsave data(shortcut)error!\n", pCha->GetLogName());
		return false;
	}

	// Add by lark.li 20080723 begin
	memset(g_extendAttr, 0, ROLE_MAXSIZE_DBMISCOUNT);
	if (!ChaExtendAttr2String(pCha, g_extendAttr, ROLE_MAXSIZE_DBMISCOUNT)) {
		LG("enter_map", "character %s\tsave data (extend attr) error!\n", pCha->GetLogName());
		return false;
	}

	// End
	char str_exp[32];
	_i64toa(exp, str_exp, 10); // C4996

	bool bWithPos = false;
	if (pCCtrlCha->GetSubMap())
		bWithPos = pCCtrlCha->GetSubMap()->CanSavePos();

	short sExec = -1;
	
	// Use stored procedures for SaveAllData
	if (bWithPos) {
		sExec = stored_procedure("{CALL dbo.SaveAllData(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}",
			"dbo", "SaveAllData", 43,
			&hp, &sp, str_exp, map, main_map, &map_x, &map_y, &radius, &angle, &pk_ctrl,
			&degree, job, &gd, &ap, &tp, &str, &dex, &agi, &con, &sta, &luk, 
			g_look, g_skillbag, g_shortcut, g_szMisInfo, g_szRecord, g_szTrigger, g_szMisCount,
			szBirthName, szLoginCha, &sail_lv, &sail_exp, &sail_left_exp, &live_lv, &live_exp,
			&live_tp, &nLocked, &dwCredit, &dwStoreItemID, g_skillstate, g_extendAttr, &chaIMP,
			&cha_id);
	} else {
		sExec = stored_procedure("{CALL dbo.SaveAllDataWithoutPos(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}",
			"dbo", "SaveAllDataWithoutPos", 38,
			&hp, &sp, str_exp, &radius, &pk_ctrl,
			&degree, job, &gd, &ap, &tp, &str, &dex, &agi, &con, &sta, &luk, 
			g_look, g_skillbag, g_shortcut, g_szMisInfo, g_szRecord, g_szTrigger, g_szMisCount,
			szBirthName, szLoginCha, &sail_lv, &sail_exp, &sail_left_exp, &live_lv, &live_exp,
			&live_tp, &nLocked, &dwCredit, &dwStoreItemID, g_skillstate, g_extendAttr, &chaIMP,
			&cha_id);
	}
	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t???SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character %u!\n", cha_id);
		return false;
	}

	// game_db.UpdateIMP(pPlayer);

	// LG("enter_map", "??????SQL?????.\n");

	dwOldTick = dwNowTick;
	dwNowTick = GetTickCount();
	dwTotalTick += dwNowTick - dwOldTick;
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%7u[%10u]", dwNowTick - dwOldTick, (unsigned int)strlen(g_sql));
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%6u", dwTotalTick);
	_snprintf_s(szLogMsg + strlen(szLogMsg), sizeof(szLogMsg + strlen(szLogMsg)), _TRUNCATE, "\t%s\n", pCha->m_CLog.GetLogName());
	// LG(szSaveCha, szLogMsg);

	// pCha->m_CLog.Log("^^^^^^^^^^^^?????????\n");
	pCha->m_CLog.Log("...............you finish save the character \n");
	// pCha->SystemNotice("??L????????????????? %d????? %s?????? [%d,%d]???????? %s.\n", pCha->m_CChaAttr.GetAttr(ATTR_LV), pCha->GetBirthMap(), pCha->GetPos().x, pCha->GetPos().y, pCha->GetBirthCity());
	// LG("enter_map", "??????????????????.\n", pCha->GetLogName());
	LG("enter_map", "save the main character whole data succeed!\n", pCha->GetLogName());

	return true;
	T_E
}

bool CTableCha::SavePos(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	CCharacter* pCCtrlCha = pPlayer->GetCtrlCha();
	if (!pCha || !pCCtrlCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SavePos
	const char* map = pCCtrlCha->GetBirthMap();
	const char* main_map = pCha->GetBirthMap();
	int map_x = pCha->GetPos().x;
	int map_y = pCha->GetPos().y;
	int angle = pCha->GetAngle();
	
	short sExec = stored_procedure("{CALL dbo.SavePos(?,?,?,?,?,?)}",
		"dbo", "SavePos", 6, map, main_map, &map_x, &map_y, &angle, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t????????�?SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out save position SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveMoney(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SaveMoney
	__int64 gd = pCha->getAttr(ATTR_GD);
	// Pass pointer to __int64 for BIGINT parameter binding
	short sExec = stored_procedure("{CALL dbo.SaveMoney(?,?)}",
		"dbo", "SaveMoney", 2, &gd, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t??????????SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out save money SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveKBagDBID(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SaveKBagDBIDEx
	int kitbag_id = pCha->GetKitbagRecDBID();
	short sExec = stored_procedure("{CALL dbo.SaveKBagDBIDEx(?,?)}",
		"dbo", "SaveKBagDBIDEx", 2, &kitbag_id, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t??????????????SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character%s\tcarry out save kitbag indexical SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveKBagTmpDBID(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SaveKBagTmpDBID
	int kitbag_tmp_id = pCha->GetKitbagTmpRecDBID();
	short sExec = stored_procedure("{CALL dbo.SaveKBagTmpDBID(?,?)}",
		"dbo", "SaveKBagTmpDBID", 2, &kitbag_tmp_id, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t???????????????????SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out save temp kitbag indexical SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveKBState(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SaveKBState
	int iLocked = pCha->m_CKitbag.GetPwdLockState();
	short sExec = stored_procedure("{CALL dbo.SaveKBState(?,?)}",
		"dbo", "SaveKBState", 2, &iLocked, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t????????????????SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out save kitbag lock state SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

BOOL CTableCha::SaveStoreItemID(DWORD cha_id, int lStoreItemID) {
	if (cha_id == 0) {
		return false;
	}

	// Use stored procedure for SaveStoreItemID
	short sExec = stored_procedure("{CALL dbo.SaveStoreItemID(?,?)}",
		"dbo", "SaveStoreItemID", 2, &lStoreItemID, &cha_id);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}

	return true;
}

BOOL CTableCha::AddMoney(DWORD cha_id, __int64 money) {
	if (cha_id == 0) {
		return false;
	}

	// Use stored procedure for AddMoney
	short sExec = stored_procedure("{CALL dbo.AddMoney(?,?)}",
		"dbo", "AddMoney", 2, &money, &cha_id);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}

	return true;
}

BOOL CTableCha::AddCreditByDBID(DWORD cha_id, int lCredit) {
	if (cha_id == 0) {
		return false;
	}

	// Use stored procedure for AddCreditByDBID
	short sExec = stored_procedure("{CALL dbo.AddCreditByDBID(?,?)}",
		"dbo", "AddCreditByDBID", 2, &lCredit, &cha_id);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}

	return true;
}

BOOL CTableCha::IsChaOnline(DWORD cha_id, BOOL& bOnline) {
	if (cha_id == 0) {
		return false;
	}

	// Use stored procedure for IsChaOnline
	int affect_rows = 0;
	if (!_get_row_stored_procedure(g_buf, g_cnCol, "{CALL dbo.IsChaOnline(?)}", 
		"dbo", "IsChaOnline", &affect_rows, 1, &cha_id)) {
		return false;
	}

	int lMemAddr = atoi(g_buf[0].c_str());
	if (lMemAddr > 0) {
		bOnline = true;
	} else {
		bOnline = false;
	}

	return true;
}

Long CTableCha::GetChaAddr(DWORD cha_id) {
	if (cha_id == 0)
		return false;

	// Use stored procedure for GetChaAddr (same as IsChaOnline)
	int affect_rows = 0;
	if (!_get_row_stored_procedure(g_buf, g_cnCol, "{CALL dbo.IsChaOnline(?)}", 
		"dbo", "IsChaOnline", &affect_rows, 1, &cha_id)) {
		return 0;
	}

	return atoi(g_buf[0].c_str());
}

bool CTableCha::SaveMMaskDBID(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	// Use stored procedure for SaveMMaskDBID
	int map_mask = pPlayer->GetMapMaskDBID();
	short sExec = stored_procedure("{CALL dbo.SaveMMaskDBID(?,?)}",
		"dbo", "SaveMMaskDBID", 2, &map_mask, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%d\t???????????????SQL????????!\n", pPlayer->GetDBChaId());
		LG("enter_map", "character %d\tcarry out save big map indexical SQL senternce error!\n", pPlayer->GetDBChaId());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveBankDBID(CPlayer* pPlayer) {
	if (!pPlayer || !pPlayer->IsValid())
		return false;
	DWORD cha_id = pPlayer->GetDBChaId();

	const short csIDBufLen = 200;
	char szIDBuf[csIDBufLen];
	if (!pPlayer->BankDBIDData2String(szIDBuf, csIDBufLen))
		return false;

	// Use stored procedure for SaveBankDBID
	short sExec = stored_procedure("{CALL dbo.SaveBankDBID(?,?)}",
		"dbo", "SaveBankDBID", 2, szIDBuf, &cha_id);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%d\t????????????????SQL????????!\n", pPlayer->GetDBChaId());
		LG("enter_map", "character %d\tcarry out save bank indexcial SQL sentence error!\n", pPlayer->GetDBChaId());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", cha_id);
		LG("enter_map", "Database couldn't find the character%u!\n", cha_id);
		return false;
	}

	return true;
}

bool CTableCha::SaveTableVer(DWORD cha_id) {
	// Use stored procedure for SaveTableVer
	int version = defCHA_TABLE_NEW_VER;
	short sExec = stored_procedure("{CALL dbo.SaveTableVer(?,?)}",
		"dbo", "SaveTableVer", 2, &version, &cha_id);

	return DBOK(sExec) && !DBNODATA(sExec);
}

BOOL CTableCha::SaveMissionData(CPlayer* pPlayer, DWORD cha_id) {
	T_B if (!pPlayer) return FALSE;
	CCharacter* pCha = pPlayer->GetMainCha();
	if (!pCha)
		return FALSE;

	// ???????????�???
	memset(g_szMisInfo, 0, ROLE_MAXSIZE_DBMISSION);
	if (!pPlayer->MisGetData(g_szMisInfo, ROLE_MAXSIZE_DBMISSION - 1)) {
		// pCha->SystemNotice( "SaveMissionData:?�???????????????????????!ID = %d", pCha->GetID() );
		// LG(szDBLog, "SaveMissionData:??????[ID: %d\tNAME: %s]???????????????????????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00018), pCha->GetID());
		LG(szDBLog, "SaveMissionData:save character[ID: %d\tNAME: %s]data info,Get mission data error!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}

	memset(g_szRecord, 0, ROLE_MAXSIZE_DBRECORD);
	if (!pPlayer->MisGetRecord(g_szRecord, ROLE_MAXSIZE_DBRECORD - 1)) {
		// pCha->SystemNotice( "SaveMissionData:?�???????????????????????!ID = %d", pCha->GetID() );
		// LG(szDBLog, "SaveMissionData:??????[ID: %d\tNAME: %s]?????????????????????�???????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00018), pCha->GetID());
		LG(szDBLog, "SaveMissionData:save character[ID: %d\tNAME: %s]data info,Get mission history data error !ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}

	memset(g_szTrigger, 0, ROLE_MAXSIZE_DBTRIGGER);
	if (!pPlayer->MisGetTrigger(g_szTrigger, ROLE_MAXSIZE_DBTRIGGER - 1)) {
		// pCha->SystemNotice( "SaveMissionData:?�?????????????????????????????!ID = %d", pCha->GetID() );
		// LG(szDBLog, "SaveMissionData:??????[ID: %d\tNAME: %s]???????????????????????????!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00019), pCha->GetID());
		LG(szDBLog, "SaveMissionData:save character[ID: %d\tNAME: %s]data info??Get mission trigger data error!ID = %d\n", cha_id, pCha->GetName(), pCha->GetID());
	}

	// Use stored procedure for SaveMissionData
	short sExec = stored_procedure("{CALL dbo.SaveMissionData(?,?,?,?)}",
		"dbo", "SaveMissionData", 4, g_szMisInfo, g_szRecord, g_szTrigger, &cha_id);
	return DBOK(sExec) && !DBNODATA(sExec);
	T_E
}

// Add by lark.li 20080521 begin
bool CTableLotterySetting::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select section from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(LotterySetting)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "LotterySetting");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableLotterySetting::GetCurrentIssue(int& issue) {
	issue = -1;
	string buf[1];
	char param[] = "issue";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state  =%d", enumCURRENT);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		issue = Str2Int(buf[0]);
		return true;
	}

	return false;
}

bool CTableLotterySetting::AddIssue(int issue) {
	// Use stored procedure for AddIssue
	int section = 1;
	int state = enumCURRENT;
	short sExec = stored_procedure("{CALL dbo.LotteryAddIssue(?,?,?)}",
		"dbo", "LotteryAddIssue", 3, &section, &issue, &state);
	if (DBOK(sExec)) {
		return true;
	}

	return false;
}

bool CTableLotterySetting::DisuseIssue(int issue, int state) {
	// Use stored procedure for DisuseIssue
	short sExec = stored_procedure("{CALL dbo.LotteryDisuseIssue(?,?)}",
		"dbo", "LotteryDisuseIssue", 2, &state, &issue);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("lottery", "lottery couldn't find the issue %d!\n", issue);
		return false;
	}

	return true;
}

bool CTableLotterySetting::SetWinItemNo(int issue, const char* itemno) {
	// Use stored procedure for SetWinItemNo
	short sExec = stored_procedure("{CALL dbo.LotterySetWinItemNo(?,?)}",
		"dbo", "LotterySetWinItemNo", 2, itemno, &issue);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("lottery", "lottery couldn't find the issue %d!\n", issue);
		return false;
	}

	return true;
}

bool CTableLotterySetting::GetWinItemNo(int issue, string& itemno) {
	string buf[1];
	char param[] = "itemno";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state  =%d and issue = %d", enumCURRENT, issue);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		itemno = buf[0];
		return true;
	}

	return false;
}

bool CTableTicket::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select itemno from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(Ticket)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "Ticket");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableTicket::AddTicket(int cha_id, int issue, char itemno[6][2]) {
	int index = -1;
	char no[10][6];

	for (int i = 0; i < 6; i++) {
		if (itemno[i][0] == 'X') {
			index = i;
			break;
		}
	}

	if (index > 0) {
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 6; j++) {
				if (j == index)
					no[i][j] = '0' + i;
				else
					no[i][j] = itemno[j][0];
			}
		}

		for (int i = 0; i < 10; i++)
			AddTicket(cha_id, issue, no[i][0], no[i][1], no[i][2], no[i][3], no[i][4], no[i][5], 0);
	}

	AddTicket(cha_id, issue, itemno[0][0], itemno[1][0], itemno[2][0], itemno[3][0], itemno[4][0], itemno[5][0]);

	return false;
}

bool CTableTicket::AddTicket(int cha_id, int issue, char itemno1, char itemno2, char itemno3, char itemno4, char itemno5, char itemno6, int real) {
	// Build itemno string
	char itemno[8];
	_snprintf_s(itemno, sizeof(itemno), _TRUNCATE, "%c%c%c%c%c%c", itemno1, itemno2, itemno3, itemno4, itemno5, itemno6);
	
	// Use stored procedure for AddTicket
	char issueStr[16];
	_snprintf_s(issueStr, sizeof(issueStr), _TRUNCATE, "%d", issue);
	short sExec = stored_procedure("{CALL dbo.TicketAddTicket(?,?,?,?)}",
		"dbo", "TicketAddTicket", 4, &cha_id, issueStr, itemno, &real);
	if (DBOK(sExec)) {
		return true;
	}

	return false;
}

bool CTableTicket::IsExist(int issue, char* itemno) {
	// Use stored procedure to prevent SQL injection
	string buf[1];
	int l_retrow = 0;
	bool ret = _get_row_stored_procedure(buf, 1, "{CALL dbo.CheckTicketExists(?, ?)}", 
		"dbo", "CheckTicketExists", &l_retrow, 2, &issue, itemno);
	if (ret && l_retrow > 0) {
		if (Str2Int(buf[0]) > 0)
			return true;
	}

	return false;
}

bool CTableTicket::CalWinTicket(int issue, int max, string& itemno) {
	int RANGE_MIN = 1;
	int RANGE_MAX = 2;

	// ?????2???h??????
	int probability = rand() % RANGE_MAX + RANGE_MIN;

	// ??????
	if (issue % probability == 0) {
		char sql[256];

		_snprintf_s(sql, sizeof(sql), _TRUNCATE, "SELECT top 10 * FROM\
						(\
						SELECT     itemno, COUNT(*) AS num\
						FROM         Ticket\
						WHERE     (issue = %d) AND real = 0\
						GROUP BY itemno\
						) \
						AS A\
						WHERE num <= %d\
						ORDER BY num",
				issue, max);

		vector<vector<string>> data;

		getalldata(sql, data);

		RANGE_MAX = (int)data.size();

		if (RANGE_MAX > 0) {
			int index = rand() % RANGE_MAX + RANGE_MIN;

			itemno = data[index][0];
			return true;
		}
	}

	char buffer[7];
	RANGE_MAX = 999999;

	while (1) {
		int num = rand() % RANGE_MAX + RANGE_MIN;
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%06d", num);

		if (!IsExist(issue, buffer))
			break;
	}

	itemno = buffer;

	return true;
}

bool CTableWinTicket::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select issue from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(WinTicket)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "WinTicket");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableWinTicket::GetTicket(int issue) {
	return true;
}

bool CTableWinTicket::AddTicket(int issue, char* itemno, int grade) {
	// Use stored procedure for AddTicket
	short sExec = stored_procedure("{CALL dbo.WinTicketAddTicket(?,?,?)}",
		"dbo", "WinTicketAddTicket", 3, &issue, itemno, &grade);
	if (DBOK(sExec)) {
		return true;
	}

	return false;
}

bool CTableWinTicket::Exchange(int issue, char* itemno) {
	// Use stored procedure for Exchange
	short sExec = stored_procedure("{CALL dbo.WinTicketExchange(?,?)}",
		"dbo", "WinTicketExchange", 2, &issue, itemno);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("lottery", "lottery couldn't find the issue %d!\n", issue);
		return false;
	}

	return true;
}

bool CTableAmphitheaterSetting::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select season from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(AmphitheaterSetting)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "AmphitheaterSetting");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

// ??�?j?????z?????
bool CTableAmphitheaterSetting::GetCurrentSeason(int& season, int& round) {
	season = -1;
	round = -1;
	string buf[2];
	char param[] = "season, [round]";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state  =%d", AmphitheaterSetting::enumCURRENT);

	int r = _get_row(buf, 2, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		season = Str2Int(buf[0]);
		round = Str2Int(buf[1]);
		return true;
	}

	return false;
}

// ???h??????
bool CTableAmphitheaterSetting::AddSeason(int season) {
	// Use stored procedure for AddSeason
	int section = 1;
	int round = 1;
	int state = AmphitheaterSetting::enumCURRENT;
	short sExec = stored_procedure("{CALL dbo.AmphitheaterAddSeason(?,?,?,?)}",
		"dbo", "AmphitheaterAddSeason", 4, &section, &season, &round, &state);
	if (DBOK(sExec)) {
		return true;
	}

	return false;
}

// ??????????
bool CTableAmphitheaterSetting::DisuseSeason(int season, int state, const char* winner) {
	// Use stored procedure for DisuseSeason
	short sExec = stored_procedure("{CALL dbo.AmphitheaterDisuseSeason(?,?,?)}",
		"dbo", "AmphitheaterDisuseSeason", 3, &state, winner, &season);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("AmphitheaterSetting", "amphitheater couldn't find the season %d!\n", season);
		return false;
	}

	return true;
}

// ???????
bool CTableAmphitheaterSetting::UpdateRound(int season, int round) {
	// Use stored procedure for UpdateRound
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateRound(?,?)}",
		"dbo", "AmphitheaterUpdateRound", 2, &round, &season);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("AmphitheaterSetting", "amphitheater couldn't find the season %d!\n", season);
		return false;
	}

	return true;
}

bool CTableAmphitheaterTeam::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select id from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(AmphitheaterTeam)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "AmphitheaterTeam");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

// ??�???????????
bool CTableAmphitheaterTeam::GetTeamCount(int& count) {
	count = -1;
	string buf[1];
	char param[] = "count(*)";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state > %d", AmphitheaterTeam::enumNotUse);

	int r = _get_row(buf, 2, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		count = Str2Int(buf[0]);
		return true;
	}
	return false;
}

// ??�?????
bool CTableAmphitheaterTeam::GetNoUseTeamID(int& teamID) {
	// ??�?????ID
	teamID = 0;
	string buf[1];
	char param[] = "top(1) id";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state = %d", AmphitheaterTeam::enumNotUse);

	int r = _get_row(buf, 2, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		teamID = Str2Int(buf[0]);
		return true;
	}

	return false;
}

// ??????? ?????????teamID
bool CTableAmphitheaterTeam::TeamSignUP(int& teamID, int captain, int member1, int member2) {
	// ??�?????ID
	if (teamID < 0) {
		if (!GetNoUseTeamID(teamID))
			return false;
	}

	char member[50];
	_snprintf_s(member, sizeof(member), _TRUNCATE, "%d,%d", member1, member2);

	// Use stored procedure for TeamSignUP
	int state = AmphitheaterTeam::enumUse;
	short sExec = stored_procedure("{CALL dbo.AmphitheaterTeamSignUP(?,?,?,?)}",
		"dbo", "AmphitheaterTeamSignUP", 4, &captain, member, &state, &teamID);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("AmphitheaterTeam", "amphitheater couldn't find the id %d!\n", teamID);
		return false;
	}

	return true;
}

// ???????
bool CTableAmphitheaterTeam::TeamCancel(int teamID) {
	// Use stored procedure for TeamCancel
	int state = AmphitheaterTeam::enumNotUse;
	short sExec = stored_procedure("{CALL dbo.AmphitheaterTeamCancel(?,?)}",
		"dbo", "AmphitheaterTeamCancel", 2, &state, &teamID);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		LG("AmphitheaterTeam", "amphitheater couldn't find the id %d!\n", teamID);
		return false;
	}

	return true;
}

// ???????
bool CTableAmphitheaterTeam::TeamUpdate(int teamID, int matchNo, int state, int winnum, int losenum, int relivenum) {

	return false;
}

// ???????K???
bool CTableAmphitheaterTeam::IsValidAmphitheaterTeam(int teamID, int captainID, int member1, int member2) {
	string buf[1];
	char param[] = "count(*)";
	char filter[80];
	int HasValid = 0;
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id = %d and captain = %d and (member = '%d,%d' or member = '%d,%d')", teamID, captainID, member1, member2, member2, member1);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		HasValid = Str2Int(buf[0]);
		if (HasValid > 0)
			return true;
		else
			return false;
	}

	return false;
}
// Add by sunny.sun20080714
// ????????????
bool CTableAmphitheaterTeam::IsLogin(int pActorID) {
	string buf[1];
	char param[] = "count(*)";
	char filter[80];
	int PActorIDNum = 0;

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "captain = %d or member like '%d,%' or member like '%,%d'", pActorID, pActorID, pActorID);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	// Note: Removed redundant exec_sql_direct call - _get_row already executed the query
	if (DBOK(r) && r1 > 0) {
		PActorIDNum = Str2Int(buf[0]);
		if (PActorIDNum > 0)
			return false;
		else
			return true;
	}
	return false;
}

// ??????�?????????
bool CTableAmphitheaterTeam::IsMapFull(int MapID, int& PActorIDNum) {
	string buf[1];
	char param[] = "count(*)";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "map = %d ", MapID);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();

	if (DBOK(r) && r1 > 0) {
		PActorIDNum = Str2Int(buf[0]);
		if (PActorIDNum > 2)
			return false;
		else
			return true;
	}
	return false;
}
// ???�??mapflag
bool CTableAmphitheaterTeam::UpdateMapNum(int Teamid, int Mapid, int MapFlag) {
	// Use stored procedure for UpdateMapNum
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateMapNum(?,?,?)}",
		"dbo", "AmphitheaterUpdateMapNum", 3, &MapFlag, &Teamid, &Mapid);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}

	return true;
}

// ???mapflag?
bool CTableAmphitheaterTeam::GetMapFlag(int Teamid, int& Mapflag) {
	string buf[1];
	char param[] = "mapflag";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id = %d ", Teamid);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();

	if (DBOK(r) && r1 > 0) {
		Mapflag = Str2Int(buf[0]);
		if (Mapflag >= 2)
			return false;
		else
			return true;
	}
	return false;
}

// ??????????K??????????,state = 1 ??state= 3???????
bool CTableAmphitheaterTeam::SetMaxBallotTeamRelive(void) {
	string buf[1];
	char param[] = "count(*)";
	char filter[80];
	int state = 0;
	int OddOrEven = 0;

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state = %d", AmphitheaterTeam::enumPromotion);
	int r = _get_row(buf, 1, param, filter);
	if (DBOK(r)) {
		state = Str2Int(buf[0]);
		if (state % 2 == 0)
			OddOrEven = 2;
		else
			OddOrEven = 1;
	}
	
	// Use stored procedures for AmphitheaterSetMaxBallotRelive and AmphitheaterSetOutState
	int promotionState = AmphitheaterTeam::enumPromotion;
	int reliveState = AmphitheaterTeam::enumRelive;
	int outState = AmphitheaterTeam::enumOut;
	int useState = AmphitheaterTeam::enumUse;
	
	short sExec1 = stored_procedure("{CALL dbo.AmphitheaterSetMaxBallotRelive(?,?,?)}",
		"dbo", "AmphitheaterSetMaxBallotRelive", 3, &OddOrEven, &promotionState, &reliveState);

	short sExec2 = stored_procedure("{CALL dbo.AmphitheaterSetOutState(?,?,?)}",
		"dbo", "AmphitheaterSetOutState", 3, &outState, &reliveState, &useState);

	if (!DBOK(sExec1) || !DBOK(sExec2)) {
		return false;
	}
	return true;
}

// ???�??????????
bool CTableAmphitheaterTeam::SetMatchResult(int Teamid1, int Teamid2, int Id1state, int Id2state) {
	// Use stored procedure for SetMatchResult (two calls for two teams)
	short sExec1 = stored_procedure("{CALL dbo.AmphitheaterSetTeamState(?,?)}",
		"dbo", "AmphitheaterSetTeamState", 2, &Id1state, &Teamid1);
	short sExec2 = stored_procedure("{CALL dbo.AmphitheaterSetTeamState(?,?)}",
		"dbo", "AmphitheaterSetTeamState", 2, &Id2state, &Teamid2);

	if (!DBOK(sExec1) || !DBOK(sExec2)) {
		return false;
	}
	if (DBNODATA(sExec1) || DBNODATA(sExec2)) {
		return false;
	}
	return true;
}
// ????mapid ??�?????????????id
bool CTableAmphitheaterTeam::GetCaptainByMapId(int Mapid, string& Captainid1, string& Captainid2) {
	string NoCaptain = "";
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select captain from %s where map = %d", _get_table(), Mapid);
	vector<vector<string>> data;

	getalldata(g_sql, data);

	int MapCaptainNum = (int)data.size();
	if (MapCaptainNum > 2 || MapCaptainNum == 0) {
		return false;
	}
	if (MapCaptainNum < 2) {

		Captainid1 = data[0][0];
		Captainid2 = NoCaptain;
		return true;
	} else {
		Captainid1 = data[0][0];
		Captainid2 = data[1][0];
		return true;
	}
	return false;
}
// ????map???
bool CTableAmphitheaterTeam::UpdateMap(int Mapid) {
	// Use stored procedure for UpdateMap
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateMap(?)}",
		"dbo", "AmphitheaterUpdateMap", 1, &Mapid);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}
// ????????????
bool CTableAmphitheaterTeam::GetPromotionAndReliveTeam(vector<vector<string>>& dataPromotion, vector<vector<string>>& dataRelive) {
	// select A.id, A.captain, A.relivenum, B.cha_name from AmphitheaterTeam A,  character B where B.cha_id = A.captain
	// _snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select A.id, A.captain, B.cha_name, A.winnum from AmphitheaterTeam A,  character B where B.cha_id = A.captain and a.state = %d  order by A.winnum DESC", AmphitheaterTeam::enumPromotion );
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select B.cha_name,A.id,A.winnum from AmphitheaterTeam A, character B where B.cha_id = A.captain and a.state = %d  order by A.winnum DESC", AmphitheaterTeam::enumPromotion);

	if (!getalldata(g_sql, dataPromotion))
		return false;

	// _snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select A.id, A.captain, B.cha_name, A.relivenum from AmphitheaterTeam A,  character B where B.cha_id = A.captain and A.state = %d  order by A.relivenum DESC", AmphitheaterTeam::enumRelive );
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select B.cha_name, A.relivenum ,A.id from AmphitheaterTeam A,  character B where B.cha_id = A.captain and A.state = %d  order by A.relivenum DESC", AmphitheaterTeam::enumRelive);

	if (!getalldata(g_sql, dataRelive))
		return false;

	return true;
}

// ???????
bool CTableAmphitheaterTeam::UpdatReliveNum(int ReID) {
	string buf[1];
	char param[] = "relivenum";
	char filter[80];
	int relivenumber = 0;
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id = %d ", ReID);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		relivenumber = Str2Int(buf[0]) + 1;
		// Use stored procedure for UpdatReliveNum
		short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateReliveNum(?,?)}",
			"dbo", "AmphitheaterUpdateReliveNum", 2, &relivenumber, &ReID);
		if (!DBOK(sExec))
			return false;
		return true;
	}
	return false;
}

// ????h??�?�?????K??????
bool CTableAmphitheaterTeam::UpdateAbsentTeamRelive() {
	// Use stored procedure for UpdateAbsentTeamRelive
	int reliveState = AmphitheaterTeam::enumRelive;
	int useState = AmphitheaterTeam::enumUse;
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateAbsentTeamRelive(?,?)}",
		"dbo", "AmphitheaterUpdateAbsentTeamRelive", 2, &reliveState, &useState);

	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}
// ???�???????????map???
bool CTableAmphitheaterTeam::UpdateMapAfterEnter(int CaptainID, int MapID) {
	// Use stored procedure for UpdateMapAfterEnter
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateMapAfterEnter(?,?)}",
		"dbo", "AmphitheaterUpdateMapAfterEnter", 2, &MapID, &CaptainID);
	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}

// Add by sunny.sun20080806
// ????winnum??????????1
bool CTableAmphitheaterTeam::UpdateWinnum(int teamid) {
	// Use stored procedure for UpdateWinnum
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateWinnum(?)}",
		"dbo", "AmphitheaterUpdateWinnum", 1, &teamid);
	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}
// ???winnum??????h?K????id
bool CTableAmphitheaterTeam::GetUniqueMaxWinnum(int& teamid) {
	// select id from AmphitheaterTeam where winnum in (select  max(winnum) from AmphitheaterTeam)
	string buf[1];
	char param[] = "id";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "winnum in (select  max(winnum) from AmphitheaterTeam)");
	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		if (r1 == 1) {
			teamid = Str2Int(buf[0]);
			return true;
		} else
			return false;
	}
	return false;
}

bool CTableAmphitheaterTeam::SetMatchnoState(int teamid) {
	// Use stored procedure for SetMatchnoState
	short sExec = stored_procedure("{CALL dbo.AmphitheaterSetMatchnoState(?)}",
		"dbo", "AmphitheaterSetMatchnoState", 1, &teamid);
	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}

bool CTableAmphitheaterTeam::UpdateState() {
	// Use stored procedure for UpdateState
	int useState = AmphitheaterTeam::enumUse;
	int promotionState = AmphitheaterTeam::enumPromotion;
	short sExec = stored_procedure("{CALL dbo.AmphitheaterUpdateState(?,?)}",
		"dbo", "AmphitheaterUpdateState", 2, &useState, &promotionState);
	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}

bool CTableAmphitheaterTeam::CloseReliveByState(int& statenum) {
	string buf[1];
	char param[] = "count(*)";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "state = %d ", AmphitheaterTeam::enumUse);
	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		statenum = Str2Int(buf[0]);
		return true;
	}
	return false;
}

bool CTableAmphitheaterTeam::CleanMapFlag(int teamid1, int teamid2) {
	// Use stored procedure for CleanMapFlag
	short sExec = stored_procedure("{CALL dbo.AmphitheaterCleanMapFlag(?,?)}",
		"dbo", "AmphitheaterCleanMapFlag", 2, &teamid1, &teamid2);
	if (!DBOK(sExec)) {
		return false;
	}
	if (DBNODATA(sExec)) {
		return false;
	}
	return true;
}

bool CTableAmphitheaterTeam::GetStateByTeamid(int teamid, int& state) {
	string buf[1];
	char param[] = "state";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id = %d ", teamid);
	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		state = Str2Int(buf[0]);
		return true;
	}
	return false;
}

// personinfo ??'??
bool CTablePersoninfo::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select cha_id from %s (nolock) where 1=2", _get_table());

	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(AmphitheaterSetting)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "personinfo");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

// ???birthday
bool CTablePersoninfo::GetPersonBirthday(int chaid, int& birthday) {
	string buf[1];
	char param[] = "birthday";
	char filter[80];

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_id = %d ", chaid);

	int r = _get_row(buf, 1, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		if (strcmp(buf[0].c_str(), "NULL") == 0)
			return false;
		else
			birthday = Str2Int(buf[0]);
		return true;
	}
	return false;
}

// End

bool CTableResource::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				id, cha_id, type_id, content \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(resource)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "resource");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableResource::Create(int& lDBID, int lChaId, int lTypeId) {
	// Use stored procedure for Create - returns new ID
	int affect_rows = 0;
	bool ret = _get_row_stored_procedure(g_buf, g_cnCol, "{CALL dbo.ResourceCreate(?,?)}",
		"dbo", "ResourceCreate", &affect_rows, 2, &lChaId, &lTypeId);

	if (ret && affect_rows > 0) {
		lDBID = atoi(g_buf[0].c_str());
		return true;
	}

	return false;
}

bool CTableResource::ReadKitbagData(CCharacter* pCha) {
	if (!pCha) {
		// LG("enter_map", "??????????????????????.\n");
		LG("enter_map", "Load resource database error,character is inexistence\n");
		return false;
	}
	if (pCha->GetKitbagRecDBID() == 0) {
		int lDBID;
		if (!Create(lDBID, pCha->GetPlayer()->GetDBChaId(), enumRESDB_TYPE_KITBAG))
			return false;
		pCha->SetKitbagRecDBID(lDBID);
	}

	int nIndex = 0;
	char param[] = "cha_id, type_id, content";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id=%d", pCha->GetKitbagRecDBID());
	int r = _get_row3(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		DWORD dwChaId = Str2Int(g_buf[nIndex++]);
		char chType = Str2Int(g_buf[nIndex++]);
		if (dwChaId != pCha->GetPlayer()->GetDBChaId() || chType != enumRESDB_TYPE_KITBAG) {
			// LG("enter_map", "?????????????????????.\n");
			LG("enter_map", "Load resource database error??character is not matching.\n");
			return false;
		}
		if (!pCha->String2KitbagData(g_buf[nIndex++])) {
			// LG("enter_map", "???????????????.\n");
			LG("enter_map", "kitbag data check sum error.\n");
			// LG("???????", "?????%s???i????????resource_id %u?????????.\n", pCha->GetLogName(), pCha->GetKitbagRecDBID());
			LG("check sum error", "character(%s) kitbag data(resource_id %u) check sum error.\n", pCha->GetLogName(), pCha->GetKitbagRecDBID());
			return false;
		}
	} else {
		// LG("enter_map", "?????????????_get_row()???????%d.\n", r);
		LG("enter_map", "Load resource database error??_get_row()return value:%d.\n", r);
		return false;
	}
	// LG("enter_map", "????????????.\n");
	LG("enter_map", "Load kitbag data succeed.\n");
	return true;
}

bool CTableResource::SaveKitbagData(CCharacter* pCha) {
	if (!pCha || !pCha->IsValid())
		return false;

	// LG("enter_map", "??'??????????!\n");
	g_kitbag[0] = 0;
	if (!KitbagData2String(&pCha->m_CKitbag, g_kitbag, defKITBAG_DATA_STRING_LEN)) {
		// LG("enter_map", "???%s\t????????????????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tsave data(kitbag) error!\n", pCha->GetLogName());
		return false;
	}
	// LG("enter_map", "?????????????\n");

	// Use stored procedure for SaveResourceContent
	int recDBID = pCha->GetKitbagRecDBID();
	short sExec = stored_procedure("{CALL dbo.SaveResourceContent(?,?)}",
		"dbo", "SaveResourceContent", 2, g_kitbag, &recDBID);
	// LG("enter_map", "???SQL?????\n");
	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t???SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\t carry out SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????�??????%u!\n", pCha->GetKitbagRecDBID());
		LG("enter_map", "Database couldn't find the kitbag resource %u!\n", pCha->GetKitbagRecDBID());
		return false;
	}
	// LG("enter_map", "??????????.\n");
	LG("enter_map", "finish kitbag save.\n");

	return true;
}

bool CTableResource::ReadKitbagTmpData(CCharacter* pCha) {
	if (!pCha) {
		// LG("enter_map", "??????????????????????.\n");
		LG("enter_map", "Load resource database error,character is inexistence.\n");
		return false;
	}
	if (pCha->GetKitbagTmpRecDBID() == 0) {
		int lDBID;
		if (!Create(lDBID, pCha->GetPlayer()->GetDBChaId(), enumRESDB_TYPE_KITBAGTMP))
			return false;
		pCha->SetKitbagTmpRecDBID(lDBID);
	}

	int nIndex = 0;
	char param[] = "cha_id, type_id, content";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id=%d", pCha->GetKitbagTmpRecDBID());
	int r = _get_row3(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		DWORD dwChaId = Str2Int(g_buf[nIndex++]);
		char chType = Str2Int(g_buf[nIndex++]);
		if (dwChaId != pCha->GetPlayer()->GetDBChaId() || chType != enumRESDB_TYPE_KITBAGTMP) {
			// LG("enter_map", "?????????????????????.\n");
			LG("enter_map", "Load resource database error,character is not matching.\n");
			return false;
		}
		if (!pCha->String2KitbagTmpData(g_buf[nIndex++])) {
			// LG("enter_map", "??????????????????.\n");
			LG("enter_map", "Temp kitbag data check sum error.\n");
			// LG("???????", "?????%s????????????????resource_id %u?????????.\n", pCha->GetLogName(), pCha->GetKitbagTmpRecDBID());
			LG("check sum error", "character(%s) temp kitbag data(resource_id %u)check sum error.\n", pCha->GetLogName(), pCha->GetKitbagTmpRecDBID());
			return false;
		}
	} else {
		// LG("enter_map", "?????????????_get_row()???????%d.\n", r);
		LG("enter_map", "Load resource database error,_get_row() return value:%d.\n", r);
		return false;
	}
	// LG("enter_map", "???????????????.\n");
	LG("enter_map", "Load temp kitbag data succeed.\n");

	return true;
}

bool CTableResource::ReadKitbagTmpData(int lRecDBID, string& strData) {
	if (lRecDBID == 0) {
		return false;
	}

	BOOL ret = false;

	const char* sql_syntax = "select content from %s where id=%d";
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, sql_syntax, _get_table(), lRecDBID);

	// ?????????
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
		col_num = std::min<decltype(col_num)>(col_num, MAX_COL);
		col_num = std::min<decltype(col_num)>(col_num, _max_col);

		// Bind Column
		for (int i = 0; i < col_num; ++i) {
			SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
		}

		// Fetch each Row	int i; // ?????????
		for (int f_row = 0; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			}

			strData = (char const*)_buf[0];
		}
		SQLFreeStmt(hstmt, SQL_UNBIND);
		ret = true;
	} catch (int& e) {
		// LG("Store_msg", "ReadKitbagTmpData ODBC ?????�????????%d\n",e);
		LG("Store_msg", "ReadKitbagTmpData ODBC interface transfer error,position ID:%d\n", e);
	} catch (...) {
		LG("Store_msg", "Unknown Exception raised when ReadKitbagTmpData\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}

bool CTableResource::SaveKitbagTmpData(int lRecDBID, const string& strData) {
	if (lRecDBID == 0) {
		return false;
	}

	// Use stored procedure for SaveResourceContent
	short sExec = stored_procedure("{CALL dbo.SaveResourceContent(?,?)}",
		"dbo", "SaveResourceContent", 2, strData.c_str(), &lRecDBID);

	if (!DBOK(sExec)) {
		// LG("Store_msg", "???SQL????????!\n");
		LG("Store_msg", "carry out SQL sentence error!\n");
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("Store_msg", "?????�??????????????????%u!\n", lRecDBID);
		LG("Store_msg", "Database couldn't find the temp kitbag resource %u!\n", lRecDBID);
		return false;
	}
	// LG("Store_msg", "??????????????.\n");
	LG("Store_msg", "finish the temp kitbag save.\n");

	return true;
}

bool CTableResource::SaveKitbagTmpData(CCharacter* pCha) {
	if (!pCha || !pCha->IsValid())
		return false;

	g_kitbagTmp[0] = 0;
	if (!KitbagData2String(pCha->m_pCKitbagTmp.get(), g_kitbagTmp, defKITBAG_DATA_STRING_LEN)) {
		// LG("enter_map", "???%s\t???????????????????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tsave data(temp kitbag)error!\n", pCha->GetLogName());
		return false;
	}

	// Use stored procedure for SaveResourceContent
	int recDBID = pCha->GetKitbagTmpRecDBID();
	short sExec = stored_procedure("{CALL dbo.SaveResourceContent(?,?)}",
		"dbo", "SaveResourceContent", 2, g_kitbagTmp, &recDBID);

	if (!DBOK(sExec)) {
		// LG("enter_map", "???%s\t???SQL????????!\n", pCha->GetLogName());
		LG("enter_map", "character %s\tcarry out SQL sentence error!\n", pCha->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�??????????????????%u!\n", pCha->GetKitbagTmpRecDBID());
		LG("enter_map", "Database couldn't find the temp kitbag resource%u!\n", pCha->GetKitbagTmpRecDBID());
		return false;
	}
	// LG("enter_map", "??????????????.\n");
	LG("enter_map", "finish save the temp kitbag.\n");

	return true;
}

bool CTableResource::ReadBankData(CPlayer* pCPly, char chBankNO) {
	if (!pCPly) {
		// LG("enter_map", "??????????????????????.\n");
		LG("enter_map", "Load resource database error,character is inexistence.\n");
		return false;
	}
	if (pCPly->GetCurBankNum() == 0) {
		int lDBID;
		if (!Create(lDBID, pCPly->GetDBChaId(), enumRESDB_TYPE_BANK))
			return false;
		pCPly->AddBankDBID(lDBID);
	}

	char chStart, chEnd;
	if (chBankNO < 0) {
		chStart = 0;
		chEnd = pCPly->GetCurBankNum() - 1;
	} else {
		if (chBankNO >= pCPly->GetCurBankNum())
			return false;
		chStart = chEnd = chBankNO;
	}

	int nIndex = 0;
	char param[] = "cha_id, type_id, content";
	char filter[80];
	int r;
	int r1;
	for (char i = chStart; i <= chEnd; i++) {
		nIndex = 0;
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id=%d", pCPly->GetBankDBID(i));
		r = _get_row3(g_buf, g_cnCol, param, filter);
		r1 = get_affected_rows();
		if (DBOK(r) && r1 > 0) {
			DWORD dwChaId = Str2Int(g_buf[nIndex++]);
			char chType = Str2Int(g_buf[nIndex++]);
			if (dwChaId != pCPly->GetDBChaId() || chType != enumRESDB_TYPE_BANK) {
				// LG("enter_map", "?????????????????????.\n");
				LG("enter_map", "Load resource database error,character is not matching.\n");
				return false;
			}
			if (!pCPly->String2BankData(i, g_buf[nIndex++])) {
				// LG("enter_map", "???????????????.\n");
				LG("enter_map", "kitbag data check sum error.\n");
				// LG("???????", "????%u?????????????resource_id %u?????????.\n", pCPly->GetDBChaId(), pCPly->GetBankDBID(i));
				LG("check sum error", "player (%u) bank data(resource_id %u)check sum error.\n", pCPly->GetDBChaId(), pCPly->GetBankDBID(i));
				return false;
			}
		} else {
			// LG("enter_map", "?????????????_get_row()???????%d.\n", r);
			LG("enter_map", "Load resource database error??_get_row() return value:%d.\n", r);
			return false;
		}
	}
	// LG("enter_map", "????????????.\n");
	LG("enter_map", "Load bank data succeed.\n");
	return true;
}

bool CTableResource::SaveBankData(CPlayer* pCPly, char chBankNO) {
	if (!pCPly || !pCPly->IsValid())
		return false;
	if (pCPly->GetCurBankNum() == 0)
		return true;

	char chStart, chEnd;
	if (chBankNO < 0) {
		chStart = 0;
		chEnd = pCPly->GetCurBankNum() - 1;
	} else {
		if (chBankNO >= pCPly->GetCurBankNum())
			return false;
		chStart = chEnd = chBankNO;
	}

	// LG("enter_map", "??'????????????!\n");

	for (char i = chStart; i <= chEnd; i++) {
		if (!pCPly->BankWillSave(i))
			continue;
		pCPly->SetBankSaveFlag(i, false);
		g_kitbag[0] = 0;
		if (!KitbagData2String(pCPly->GetBank(i), g_kitbag, defKITBAG_DATA_STRING_LEN)) {
			// LG("enter_map", "???%u\t???????????????????!\n", pCPly->GetBankDBID(i));
			LG("enter_map", "character%u\tsave data(bank) error!\n", pCPly->GetBankDBID(i));
			return false;
		}
		// LG("enter_map", "?????????????\n");

		// Use stored procedure for SaveBankData
		int bankDBID = pCPly->GetBankDBID(i);
		short sExec = stored_procedure("{CALL dbo.SaveBankData(?,?)}",
			"dbo", "SaveBankData", 2, g_kitbag, &bankDBID);

		// LG("enter_map", "???SQL?????\n");
		if (!DBOK(sExec)) {
			// LG("enter_map", "???%u\t???SQL????????!\n", pCPly->GetDBChaId());
			LG("enter_map", "player %u\tcarry out SQL sentence error!\n", pCPly->GetDBChaId());
			return false;
		}
		if (DBNODATA(sExec)) {
			// LG("enter_map", "?????�???????????????%u!\n", pCPly->GetBankDBID(i));
			LG("enter_map", "Database couldn't find the bank resource%u!\n", pCPly->GetBankDBID(i));
			return false;
		}
	}
	// LG("enter_map", "??????????[%d->%d]????.\n", chStart, chEnd);
	LG("enter_map", "finish the whole bank[%d->%d]save\n", chStart, chEnd);
	return true;
}

bool CTableMapMask::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				id, cha_id, content1, content2, content3, content4, content5 \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(map_mask)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "map_mask");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableMapMask::GetColNameByMapName(const char* szMapName, char* szColName, int lColNameLen) {
	if (!szMapName || !szColName)
		return false;
	const char* szColBase = "content";
	if ((int)strlen(szColBase) + 1 >= lColNameLen)
		return false;

	char chIndex = 0;
	if (!strcmp(szMapName, "garner"))
		chIndex = 1;
	else if (!strcmp(szMapName, "magicsea"))
		chIndex = 2;
	else if (!strcmp(szMapName, "darkblue"))
		chIndex = 3;
	else if (!strcmp(szMapName, "winterland")) // Add by lark.li 20080812
		chIndex = 4;
	// else if (!strcmp(szMapName, "eastgoaf"))
	//	chIndex = 4;
	// else if (!strcmp(szMapName, "lonetower"))
	//	chIndex = 5;
	else
		return false;

	// Fix: Use lColNameLen instead of sizeof(szColName) - szColName is a pointer,
	// so sizeof(szColName) returns pointer size (4/8 bytes), not buffer size
	_snprintf_s(szColName, lColNameLen, _TRUNCATE, "%s%d", szColBase, chIndex);
	return true;
}

bool CTableMapMask::Create(int& lDBID, int lChaId) {
	// Use stored procedure for Create - returns new ID
	int affect_rows = 0;
	bool ret = _get_row_stored_procedure(g_buf, g_cnCol, "{CALL dbo.MapMaskCreate(?)}",
		"dbo", "MapMaskCreate", &affect_rows, 1, &lChaId);

	if (ret && affect_rows > 0) {
		lDBID = atoi(g_buf[0].c_str());
		return true;
	}

	return false;
}

bool CTableMapMask::ReadData(CPlayer* pCPly) {
	if (!pCPly || !pCPly->IsValid()) {
		// LG("enter_map", "??????????????????????????.\n");
		LG("enter_map", "Load map hideID database error,character is inexistence.\n");
		return false;
	}
	if (pCPly->GetMapMaskDBID() == 0) {
		int lDBID;
		if (!Create(lDBID, pCPly->GetDBChaId()))
			return false;
		pCPly->SetMapMaskDBID(lDBID);
	}

	char szMaskColName[30];
	if (!GetColNameByMapName(pCPly->GetMaskMapName(), szMaskColName, 30)) {
		// LG("enter_map", "????????????????.\n");
		LG("enter_map", "choice map hideID data error.\n");
		return false;
	}

	int nIndex = 0;
	char param[80];
	_snprintf_s(param, sizeof(param), _TRUNCATE, "cha_id, %s", szMaskColName);
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "id=%d", pCPly->GetMapMaskDBID());
	int r = _get_row3(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		DWORD dwChaId = Str2Int(g_buf[nIndex++]);
		if (dwChaId != pCPly->GetDBChaId()) {
			// LG("enter_map", "?????????????????????????.\n");
			LG("enter_map", "Load map hideID database error,character is not matching.\n");
			return false;
		}
		pCPly->SetMapMaskBase64(g_buf[nIndex++].c_str());
		// if (strcmp(g_buf[nIndex-1].c_str(), "0"))
		//	LG("????????", "??? %s?????? %u.\n", pCPly->GetMaskMapName(), strlen(g_buf[nIndex-1].c_str()));
	} else {
		// LG("enter_map", "?????????????????_get_row()???????%d.%u\n", r);
		LG("enter_map", "Load map hideID database error,_get_row() return value:%d.%u\n", r);
		return false;
	}
	// LG("enter_map", "????????????.\n");
	LG("enter_map", "Load big map data succeed.\n");
	return true;
}

bool CTableMapMask::SaveData(CPlayer* pCPly, BOOL bDirect) {
	if (!pCPly || !pCPly->IsValid())
		return false;

	char szMaskColName[30];
	if (!GetColNameByMapName(pCPly->GetMaskMapName(), szMaskColName, 30)) {
		// LG("enter_map", "????????????????.\n");
		LG("enter_map", "choice map hideID data error.\n");
		return false;
	}

	// LG("enter_map", "??'???????????!\n");
	LG("enter_map", "start save big map data!\n");
	
	// Use stored procedure with parameterized query for SQL injection prevention
	// Column index: 1=garner, 2=magicsea, 3=darkblue, 4=winterland
	int colIndex = 0;
	const char* mapName = pCPly->GetMaskMapName();
	if (!strcmp(mapName, "garner")) colIndex = 1;
	else if (!strcmp(mapName, "magicsea")) colIndex = 2;
	else if (!strcmp(mapName, "darkblue")) colIndex = 3;
	else if (!strcmp(mapName, "winterland")) colIndex = 4;
	
	int dbId = pCPly->GetMapMaskDBID();
	const char* maskData = pCPly->GetMapMaskBase64();
	
	// Use stored procedure: MapMaskSaveData(@colIndex, @maskData, @dbId)
	short sExec = stored_procedure("{CALL dbo.MapMaskSaveData(?,?,?)}",
		"dbo", "MapMaskSaveData", 3, &colIndex, maskData, &dbId);

	// LG("enter_map", "?????????????!\n");
	LG("enter_map", "organize big map data succeed!\n");

	return DBOK(sExec);
}

// ???????????????SQL???
BOOL CTableMapMask::_ExecSQL(const char* pszSQL) {
	MPTimer t;
	t.Begin();
	if (strlen(pszSQL) >= SQL_MAXLEN) {
		// FILE *pf = fopen("log\\SQL????????.txt", "a+");`
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", pszSQL);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return FALSE;
	}

	short sExec = exec_sql_direct(pszSQL);
	// LG("enter_map", "??????SQL???!\n");
	if (!DBOK(sExec)) {
		// LG("enter_map", "??????SQL????????sql = [%s]\n", pszSQL);
		LG("enter_map", "player carry out SQL sentence error sql = [%s]\n", pszSQL);
		return FALSE;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????�??????sql = [%s]\n", pszSQL);
		LG("enter_map", "Database couldn't find the map hideID sql = [%s]\n", pszSQL);
		return FALSE;
	}
	// LG("enter_map", "?????????????.\n");
	// LG("??????????", "??????????[%d], ????[%d]\n", t.End(), _SaveMapMaskList.size() - 1);
	LG("save data waste time", "save big map waste time[%d],queue[%d]\n", t.End(), _SaveMapMaskList.size() - 1);
	return TRUE;
}

// ????J???????????h???????
void CTableMapMask::SaveAll() {
	for (list<string>::iterator it = _SaveMapMaskList.begin(); it != _SaveMapMaskList.end(); it++) {
		string& strSQL = (*it);
		_ExecSQL(strSQL.c_str());
	}
	// LG("enter_map", "h??????????????????SQL, ???[%d]??!\n", _SaveMapMaskList.size());
	LG("enter_map", "one-off carry out every big map save SQL,totalize[%d] piece!\n", _SaveMapMaskList.size());
	_SaveMapMaskList.clear();
}

// ??????J???????????
void CTableMapMask::HandleSaveList() {
	//	yyy	add!
	// RECORDCALL( __FUNCTION__ );
	DWORD dwTick = GetTickCount();
	static DWORD g_dwLastSaveTick = 0;

	if ((dwTick - g_dwLastSaveTick) > 2000) // �????????h?d???????????
	{
		g_dwLastSaveTick = dwTick;

		int nSize = (int)(_SaveMapMaskList.size());
		if (nSize == 0)
			return;

		string& strSQL = _SaveMapMaskList.front();

		_ExecSQL(strSQL.c_str());

		_SaveMapMaskList.pop_front();

		// LG("??????????", "???????????[%d]\n", nSize - 1);
	}
}

bool CTableAct::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				act_id, act_name, gm, cha_ids, last_ip, disc_reason, last_leave \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(account)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "account");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

bool CTableAct::ReadAllData(CPlayer* pPlayer, DWORD act_id) {
	T_B if (!pPlayer) return false;

	string buf[3];
	char param[] = "gm, act_name";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "act_id=%d", act_id);
	int r = _get_row(buf, 3, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		pPlayer->SetGMLev(Str2Int(buf[0]));
		pPlayer->SetActName(buf[1].c_str());
		pPlayer->SetIMP(Str2Int(buf[2].c_str()));
		return true;
	}

	return false;
	T_E
}

bool CTableAct::SaveIMP(CPlayer* pPlayer) {
	T_B int IMP = pPlayer->GetMainCha()->GetIMP();
	int actID = pPlayer->GetMainCha()->GetID();
	// Use stored procedure for SaveIMP
	short l_sqlret = stored_procedure("{CALL dbo.SaveIMP(?,?)}",
		"dbo", "SaveIMP", 2, &IMP, &actID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		return false;
	}
	return true;
	T_E
}

bool CTableAct::SaveGmLv(CPlayer* pPlayer) {
	T_B

		int gmLv = pPlayer->GetGMLev();
	int actID = pPlayer->GetDBActId();
	// Use stored procedure for SaveGmLv
	short l_sqlret = stored_procedure("{CALL dbo.SaveGmLv(?,?)}",
		"dbo", "SaveGmLv", 2, &gmLv, &actID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		return false;
	}
	return true;
	T_E
}

bool CTableBoat::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				boat_id, boat_berth, boat_name, boat_boatid, boat_header, boat_body, boat_engine, boat_cannon, boat_equipment, \
				boat_bag, boat_diecount, boat_isdead, boat_ownerid, boat_createtime, boat_isdeleted, cur_endure, mx_endure, cur_supply, mx_supply, skill_state, \
				map, map_x, map_y, angle, degree, exp \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(boat)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "boat");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

BOOL CTableBoat::Create(DWORD& dwBoatID, const BOAT_DATA& Data) {
	T_B
		string strKitbag = "";
	KitbagStringConv(Data.sCapacity, strKitbag);
	char szTime[128] = "";
	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);
	_snprintf_s(szTime, sizeof(szTime), _TRUNCATE, "%d-%d-%d %d:%d:%d", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
	
	// Use stored procedure for BoatCreate - returns new ID
	int sBerth = Data.sBerth;
	int sBoat = Data.sBoat;
	int sHeader = Data.sHeader;
	int sBody = Data.sBody;
	int sEngine = Data.sEngine;
	int sCannon = Data.sCannon;
	int sEquipment = Data.sEquipment;
	int sBagsize = Data.sCapacity;
	int diecount = 0;
	int isdead = 0;
	int dwOwnerID = Data.dwOwnerID;
	int isdeleted = 0;
	
	int affect_rows = 0;
	bool ret = _get_row_stored_procedure(g_buf, g_cnCol, 
		"{CALL dbo.BoatCreate(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}",
		"dbo", "BoatCreate", &affect_rows, 15, 
		Data.szName, &sBerth, &sBoat, &sHeader, &sBody,
		&sEngine, &sCannon, &sEquipment, &sBagsize, strKitbag.c_str(), &diecount,
		&isdead, &dwOwnerID, szTime, &isdeleted);

	if (ret && affect_rows > 0) {
		dwBoatID = atoi(g_buf[0].c_str());
		return TRUE;
	}

	return FALSE;
	T_E
}

BOOL CTableBoat::GetBoat(CCharacter& Boat) {
	T_B
		DWORD dwBoatID = (DWORD)Boat.getAttr(ATTR_BOAT_DBID);
	BOAT_DATA Data;
	memset(&Data, 0, sizeof(BOAT_DATA));
	int nIndex = 0;
	char param[] = "boat_name, boat_boatid, boat_berth, boat_header, boat_body, \
					boat_engine, boat_cannon, boat_equipment, boat_diecount, boat_isdead, \
					boat_ownerid, boat_isdeleted, \
					cur_endure, mx_endure, cur_supply, mx_supply, skill_state, \
					map, map_x, map_y, angle, degree, exp";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "boat_id=%d", dwBoatID);
	int r = _get_row3(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		// ???????????
		strncpy(Data.szName, g_buf[nIndex++].c_str(), BOAT_MAXSIZE_BOATNAME - 1);
		Data.sBoat = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sBerth = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sHeader = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sBody = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sEngine = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sCannon = (USHORT)Str2Int(g_buf[nIndex++]);
		Data.sEquipment = (USHORT)Str2Int(g_buf[nIndex++]);
		USHORT sDieCount = (USHORT)Str2Int(g_buf[nIndex++]);
		BYTE byIsDead = (BYTE)Str2Int(g_buf[nIndex++]);
		DWORD dwOwnerID = (DWORD)Str2Int(g_buf[nIndex++]);
		BYTE byIsDeleted = (BYTE)Str2Int(g_buf[nIndex++]);
		// if( dwOwnerID != Boat.GetPlayer()->GetDBChaId() )
		//{
		//	LG( "boat_error", "?????%s??ID[0x%X]?????ID[0x%X]??j?????%s??ID[0x%X]????.",
		//		Data.szName, dwBoatID, dwOwnerID,
		//		Boat.GetName(), Boat.GetPlayer()->GetDBChaId() );
		//	Boat.SystemNotice( "??????J????%s???????????????????????????????!??!" );
		//	return FALSE;
		// }

		if (byIsDeleted == 1) {
			/*LG( "boat_error", "?????%s??ID[0x%X]?????ID[0x%X]??????????j?????%s??????????????????.",
				Data.szName, dwBoatID, dwOwnerID,
				Boat.GetName() );*/
			LG("boat_error", "boat(%s)ID[0x%X]owner ID[0x%X]had delete,is not fall short of the currently character (%s) captain prove data.",
			   Data.szName, dwBoatID, dwOwnerID,
			   Boat.GetName());
			// Boat.SystemNotice( "??????J????%s???????j???????????????????!??!", Boat.GetName());
			Boat.SystemNotice(RES_STRING(GM_GAMEDB_CPP_00020), Boat.GetName());
			return FALSE;
		}

		Boat.SetName(Data.szName);
		Boat.setAttr(ATTR_BOAT_BERTH, Data.sBerth, 1);
		Boat.setAttr(ATTR_BOAT_SHIP, Data.sBoat, 1);
		Boat.setAttr(ATTR_BOAT_HEADER, Data.sHeader, 1);
		Boat.setAttr(ATTR_BOAT_BODY, Data.sBody, 1);
		Boat.setAttr(ATTR_BOAT_ENGINE, Data.sEngine, 1);
		Boat.setAttr(ATTR_BOAT_CANNON, Data.sCannon, 1);
		Boat.setAttr(ATTR_BOAT_PART, Data.sEquipment, 1);
		Boat.setAttr(ATTR_BOAT_DIECOUNT, sDieCount, 1);
		Boat.setAttr(ATTR_BOAT_ISDEAD, byIsDead, 1);

		// ????
		Boat.setAttr(ATTR_HP, Str2Int(g_buf[nIndex++]), 1);
		Boat.setAttr(ATTR_BMXHP, Str2Int(g_buf[nIndex++]), 1);
		Boat.setAttr(ATTR_SP, Str2Int(g_buf[nIndex++]), 1);
		Boat.setAttr(ATTR_BMXSP, Str2Int(g_buf[nIndex++]), 1);
		g_strChaState[1] = g_buf[nIndex++];
		// ???
		Boat.SetBirthMap(g_buf[nIndex++].c_str());
		int lPosX = Str2Int(g_buf[nIndex++]);
		int lPosY = Str2Int(g_buf[nIndex++]);
		Boat.SetPos(lPosX, lPosY);
		Boat.SetAngle(Str2Int(g_buf[nIndex++]));
		// ???
		Boat.setAttr(ATTR_LV, Str2Int(g_buf[nIndex++]), 1);
		Boat.setAttr(ATTR_CEXP, Str2Int(g_buf[nIndex++]), 1);

	} else
		return FALSE;

	if (!ReadCabin(Boat))
		return FALSE;

	//  ???????????�?????,??????
	SItemGrid* pGridCont = nullptr;
	CItemRecord* pItem = nullptr;
	Short sPos = 0;
	int i = 0;
	Short sUsedNum = Boat.m_CKitbag.GetUseGridNum();
	while (i < sUsedNum) {
		pGridCont = Boat.m_CKitbag.GetGridContByNum(i);
		if (pGridCont) {
			pItem = GetItemRecordInfo(pGridCont->sID);
			if (pItem) {
				if (enumITEM_PICKTO_KITBAG == pItem->chPickTo) {
					sPos = Boat.m_CKitbag.GetPosIDByNum(i);
					LG("boat_excp", "Character %s Remove %s.\n", Boat.GetName(), pItem->szName);
					Boat.m_CKitbag.Pop(pGridCont, sPos);
					sUsedNum = Boat.m_CKitbag.GetUseGridNum();
					i = 0;
					continue;
					// return FALSE;
				}
			}
		}
		i++;
	}

	return TRUE;
	T_E
}

BOOL CTableBoat::SaveBoatTempData(DWORD dwBoatID, DWORD dwOwnerID, BYTE byIsDeleted) {
	// Use stored procedure for SaveBoatTempData
	int isDeleted = byIsDeleted;
	short sExec = stored_procedure("{CALL dbo.SaveBoatTempData(?,?,?)}",
		"dbo", "SaveBoatTempData", 3, &dwOwnerID, &isDeleted, &dwBoatID);
	return DBOK(sExec) && !DBNODATA(sExec);
}

BOOL CTableBoat::SaveBoatDelTag(DWORD dwBoatID, BYTE byIsDeleted) {
	// Use stored procedure for SaveBoatDelTag
	int isDeleted = byIsDeleted;
	short sExec = stored_procedure("{CALL dbo.SaveBoatDelTag(?,?)}",
		"dbo", "SaveBoatDelTag", 2, &isDeleted, &dwBoatID);
	return DBOK(sExec) && !DBNODATA(sExec);
}

BOOL CTableBoat::SaveBoatTempData(CCharacter& Boat, BYTE byIsDeleted) {
	DWORD dwBoatID = (DWORD)Boat.getAttr(ATTR_BOAT_DBID);
	int dieCount = (int)Boat.getAttr(ATTR_BOAT_DIECOUNT);
	int isDead = (int)Boat.getAttr(ATTR_BOAT_ISDEAD);
	DWORD dwOwnerID = Boat.GetPlayer()->GetDBChaId();
	int isDeleted = byIsDeleted;

	// Use stored procedure for SaveBoatTempDataEx
	short sExec = stored_procedure("{CALL dbo.SaveBoatTempDataEx(?,?,?,?,?)}",
		"dbo", "SaveBoatTempDataEx", 5, &dieCount, &isDead, &dwOwnerID, &isDeleted, &dwBoatID);
	return DBOK(sExec) && !DBNODATA(sExec);
}

BOOL CTableBoat::SaveBoat(CCharacter& Boat, char chSaveType) {
	T_B
		// LG("enter_map", "?? %s (%s)??'???�???????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		DWORD dwBoatID = (DWORD)Boat.getAttr(ATTR_BOAT_DBID);
	USHORT sBerthID = (USHORT)Boat.getAttr(ATTR_BOAT_BERTH);
	if (chSaveType == enumSAVE_TYPE_OFFLINE) // ????
		g_skillstate[0] = '\0';
	else // ??????
	{
		if (!SStateData2String(&Boat, g_skillstate, defSSTATE_DATE_STRING_LIN, chSaveType)) {
			// LG("enter_map", "?? %s (%s)?????????????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
			LG("enter_map", "boat %s (%s)organize state data failed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
			return false;
		}
	}
	g_kitbag[0] = '\0';
	KitbagData2String(&Boat.m_CKitbag, g_kitbag, defKITBAG_DATA_STRING_LEN);
	// LG("enter_map", "????????????.\n");

	bool bWithPos = false;
	if (Boat.GetPlyCtrlCha()->GetSubMap())
		bWithPos = Boat.GetPlyCtrlCha()->GetSubMap()->CanSavePos();
	
	short sExec;
	if (bWithPos) {
		// Use stored procedure for BoatSaveWithPos
		int berthID = sBerthID;
		int ownerID = (Boat.GetPlayer()) ? Boat.GetPlayer()->GetDBChaId() : 0;
		int curEndure = (int)Boat.getAttr(ATTR_HP);
		int mxEndure = (int)Boat.getAttr(ATTR_BMXHP);
		int curSupply = (int)Boat.getAttr(ATTR_SP);
		int mxSupply = (int)Boat.getAttr(ATTR_BMXSP);
		int mapX = Boat.GetPos().x;
		int mapY = Boat.GetPos().y;
		int angle = Boat.GetAngle();
		int degree = (int)Boat.getAttr(ATTR_LV);
		int boatExp = (int)Boat.getAttr(ATTR_CEXP);
		int boatID = dwBoatID;
		char mapName[64];
		strncpy_s(mapName, sizeof(mapName), Boat.GetBirthMap(), _TRUNCATE);
		
		sExec = stored_procedure("{CALL dbo.SaveBoatExWithPos(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}",
			"dbo", "SaveBoatExWithPos", 15, &berthID, &ownerID, &curEndure, &mxEndure,
			&curSupply, &mxSupply, g_skillstate, mapName, &mapX, &mapY, &angle,
			&degree, &boatExp, g_kitbag, &boatID);
	} else {
		// Use stored procedure for SaveBoatEx (BoatSaveNoPos was incorrect)
		int berthID = sBerthID;
		int ownerID = (Boat.GetPlayer()) ? Boat.GetPlayer()->GetDBChaId() : 0;
		int curEndure = (int)Boat.getAttr(ATTR_HP);
		int mxEndure = (int)Boat.getAttr(ATTR_BMXHP);
		int curSupply = (int)Boat.getAttr(ATTR_SP);
		int mxSupply = (int)Boat.getAttr(ATTR_BMXSP);
		int degree = (int)Boat.getAttr(ATTR_LV);
		int boatExp = (int)Boat.getAttr(ATTR_CEXP);
		int boatID = dwBoatID;
		
		sExec = stored_procedure("{CALL dbo.SaveBoatEx(?,?,?,?,?,?,?,?,?,?,?)}",
			"dbo", "SaveBoatEx", 11, &berthID, &ownerID, &curEndure, &mxEndure,
			&curSupply, &mxSupply, g_skillstate, &degree, &boatExp, g_kitbag, &boatID);
	}
	// LG("enter_map", "???SQL?????.\n");

	if (!DBOK(sExec)) {
		// LG("enter_map", "?? %s (%s)???????????????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		LG("enter_map", "boat %s (%s)save basic data failed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", dwBoatID);
		LG("enter_map", "Database couldn't find the character%u!\n", dwBoatID);
		return false;
	}
	// LG("enter_map", "???????????????.\n");

	// if (!SaveCabin(Boat, chSaveType))
	//	return false;

	// LG("enter_map", "?? %s (%s)????????????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
	LG("enter_map", "boat %s (%s) the whole data save succeed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());

	return true;
	T_E
}

bool CTableBoat::SaveAllData(CPlayer* pPlayer, char chSaveType) {
	if (!pPlayer)
		return false;
	CCharacter* pCBoat;
	for (BYTE i = 0; i < pPlayer->GetNumBoat(); i++) {
		pCBoat = pPlayer->GetBoat(i);
		if (!pCBoat)
			continue;
		if (!SaveBoat(*pCBoat, chSaveType))
			return false;
	}
	// LG("enter_map", "???????????????.\n");
	LG("enter_map", "save the whole boat data succeed\n");

	return true;
}

bool CTableBoat::SaveCabin(CCharacter& Boat, char chSaveType) {
	DWORD dwBoatID = (DWORD)Boat.getAttr(ATTR_BOAT_DBID);
	int kb_capacity = Boat.m_CKitbag.GetCapacity();
	g_kitbag[0] = '\0';
	KitbagData2String(&Boat.m_CKitbag, g_kitbag, defKITBAG_DATA_STRING_LEN);

	// Use stored procedure for SaveCabin
	short sExec = stored_procedure("{CALL dbo.SaveCabin(?,?)}",
		"dbo", "SaveCabin", 2, g_kitbag, &dwBoatID);

	if (!DBOK(sExec)) {
		// LG("enter_map", "?? %s (%s)?J?????????????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		LG("enter_map", "boat %s (%s) cabin data save failed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		return false;
	}
	if (DBNODATA(sExec)) {
		// LG("enter_map", "?????�???????????%u!\n", dwBoatID);
		LG("enter_map", "Database couldn't find the character%u!\n", dwBoatID);
		return false;
	}
	// LG("enter_map", "?? %s (%s)?J????????????.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
	LG("enter_map", "boat %s (%s)cabin data save succeed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());

	return true;
}

bool CTableBoat::SaveAllCabin(CPlayer* pPlayer, char chSaveType) {
	CCharacter* pCBoat;
	for (BYTE i = 0; i < pPlayer->GetNumBoat(); i++) {
		pCBoat = pPlayer->GetBoat(i);
		if (!pCBoat)
			continue;
		if (!SaveCabin(*pCBoat, chSaveType))
			return false;
	}

	return true;
}

bool CTableBoat::ReadCabin(CCharacter& Boat) // ???????
{
	DWORD dwBoatID = (DWORD)Boat.getAttr(ATTR_BOAT_DBID);
	int nIndex = 0;
	char param[] = "boat_bag";
	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "boat_id=%d", dwBoatID);
	int r = _get_row3(g_buf, g_cnCol, param, filter);
	int r1 = get_affected_rows();
	if (DBOK(r) && r1 > 0) {
		// LG("enter_map", "?? %u (%s, %s)?J??????? %s.\n", dwBoatID, Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName(), g_buf[nIndex].c_str());
		if (!Boat.String2KitbagData(g_buf[nIndex++])) {
			// LG("enter_map", "???????????????.\n");
			LG("enter_map", "cabin data check sum error.\n");
			// LG("???????", "????%s???????????boat_id %u?????????.\n", Boat.GetLogName(), Boat.getAttr( ATTR_BOAT_DBID ));
			LG("check sum error", "boat (%s) cabin data (boat_id %u)check sum error.\n", Boat.GetLogName(), Boat.getAttr(ATTR_BOAT_DBID));
			return false;
		}

		// LG("enter_map", "?? %s (%s)?J??????????�??.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		LG("enter_map", "boat %s (%s) cabin data set succeed.\n", Boat.GetLogName(), Boat.GetPlyMainCha()->GetLogName());
		return true;
	}

	return false;
}

BOOL CGameDB::Init() {
	T_B
		m_bInitOK = FALSE;

	_connect.enable_errinfo();

	printf("Connecting database [%s : %s]... ", g_Config.m_szDBIP, g_Config.m_szDBName);

	string err_info;

	bool r = _connect.connect(g_Config.m_szDBIP, g_Config.m_szDBName, g_Config.m_szDBUsr, g_Config.m_szDBPass, err_info);
	if (!r) {
		char msg[256];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Database [%s] Connection Failed!", g_Config.m_szDBName);
		MessageBox(nullptr, msg, "Database Connection Error", MB_ICONERROR | MB_OK);
		LG("gamedb", "msgDatabase [%s] Connect Failed!, ERROR REPORT[%d]", g_Config.m_szDBName, err_info.c_str());
		return FALSE;
	}
	C_PRINT("sucess!\n");

	_tab_cha = new CTableCha(&_connect);
	_tab_master = new CTableMaster(&_connect);
	_tab_res = new CTableResource(&_connect);

	// Add by lark.li 20080521 begin
	_tab_setting = new CTableLotterySetting(&_connect);
	_tab_ticket = new CTableTicket(&_connect);
	_tab_winticket = new CTableWinTicket(&_connect);

	_tab_amp_setting = new CTableAmphitheaterSetting(&_connect);
	_tab_amp_team = new CTableAmphitheaterTeam(&_connect);
	// End

	_tab_mmask = new CTableMapMask(&_connect);
	_tab_act = new CTableAct(&_connect);
	_tab_gld = new CTableGuild(&_connect);
	_tab_boat = new CTableBoat(&_connect);
	_tab_log = new CTableLog(&_connect);
	_tab_item = new CTableItem(&_connect);

	// Modify by lark.li 20080521
	// if (!_tab_cha || !_tab_act || !_tab_gld || !_tab_boat || !_tab_master)
	if (!_tab_cha || !_tab_act || !_tab_gld || !_tab_boat || !_tab_master || !_tab_setting || !_tab_ticket || !_tab_winticket || !_tab_amp_setting || !_tab_amp_team || !_tab_log || !_tab_item)
		return FALSE;

	// if (!_tab_cha->Init() || !_tab_act->Init() || !_tab_gld->Init() || !_tab_boat->Init() || !_tab_res->Init() || !_tab_mmask->Init() || !_tab_master->Init())
	if (!_tab_cha->Init() || !_tab_act->Init() || !_tab_gld->Init() || !_tab_boat->Init() || !_tab_res->Init() || !_tab_mmask->Init() || !_tab_master->Init() || !_tab_setting->Init() || !_tab_ticket->Init() || !_tab_winticket->Init() || !_tab_amp_setting->Init() || !_tab_amp_team->Init())
		return FALSE;

	// int issue;
	//_tab_setting->GetCurrentIssue(issue);
	//_tab_setting->AddIssue(10);
	//_tab_setting->DisuseIssue(1, enumDISUSE);
	//_tab_ticket->AddTicket(1 , "123456", 1);
	//_tab_ticket->AddTicket(1 , "12345X", 1);
	//_tab_ticket->AddTicket(1 , "12345X", 1);
	//_tab_ticket->Exchange(1 , "123456");

	if (!_tab_log) {
		// LG("init", "gamelog???????'?????\n");
		LG("init", "gamelog data list init failed\n");
	}

	m_bInitOK = TRUE;

	return TRUE;
	T_E
}

bool CGameDB::ReadPlayer(CPlayer* pPlayer, DWORD cha_id) {
	if (!_tab_cha->ReadAllData(pPlayer, cha_id))
		return false;

	int lKbDBID = pPlayer->GetMainCha()->GetKitbagRecDBID();
	int lkbTmpDBID = pPlayer->GetMainCha()->GetKitbagTmpRecDBID(); // ???????ID
	int lMMaskDBID = pPlayer->GetMapMaskDBID();
	int lBankNum = pPlayer->GetCurBankNum();
	if (!_tab_res->ReadKitbagData(pPlayer->GetMainCha())) {
		return false;
	}
	if (lKbDBID == 0) {
		if (!SavePlayerKBagDBID(pPlayer)) {
			return false;
		}
	}
	if (!_tab_res->ReadKitbagTmpData(pPlayer->GetMainCha())) {
		LG("enter_map", "ReadKitbagTmpData fail!.\n");
		return false;
	}
	if (lkbTmpDBID == 0) {
		if (!SavePlayerKBagTmpDBID(pPlayer)) {
			return false;
		}
	}
	pPlayer->GetMainCha()->LogAssets(enumLASSETS_INIT);

	if (!_tab_res->ReadBankData(pPlayer))
		return false;
	if (lBankNum == 0)
		if (!_tab_cha->SaveBankDBID(pPlayer))
			return false;

	// if (g_Config.m_chMapMask > 0)
	{
		// ???????????????????????
		_tab_mmask->ReadData(pPlayer);
		if (lMMaskDBID == 0)
			SavePlayerMMaskDBID(pPlayer);
	}

	if (!_tab_act->ReadAllData(pPlayer, pPlayer->GetDBActId()))
		return false;

	// ??????
	if (pPlayer->m_lGuildID > 0) {
		_tab_gld->GetGuildInfo(pPlayer->GetMainCha(), pPlayer->m_lGuildID);
		// int	lType = _tab_gld->GetTypeByID(pPlayer->GetMainCha()->getAttr(ATTR_GUILD));
		// if (lType >= 0)
		//	pPlayer->GetMainCha()->setAttr(ATTR_GUILD_TYPE, lType, 1);
	}
	// LG("enter_map", "??????????????.\n");
	LG("enter_map", "Load the character whole data succeed.\n");

	// ??�???l?
	CKitbag* pCKb;
	CCharacter* pCMainC = pPlayer->GetMainCha();
	short sItemNum = pCMainC->m_CKitbag.GetUseGridNum();
	g_kitbag[0] = '\0';
	// _snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, "?????%u ?????%d@", pCMainC->getAttr(ATTR_GD), sItemNum);
	_snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00021), pCMainC->getAttr(ATTR_GD), sItemNum);
	SItemGrid* pGridCont;
	CItemRecord* pCItem;
	pCKb = &(pCMainC->m_CKitbag);
	for (short i = 0; i < sItemNum; i++) {
		pGridCont = pCKb->GetGridContByNum(i);
		if (!pGridCont)
			continue;
		pCItem = GetItemRecordInfo(pGridCont->sID);
		if (!pCItem)
			continue;
		_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
	}
	TL(CHA_ENTER, pCMainC->GetName(), "", g_kitbag);

	short sItemTmpNum = pCMainC->m_pCKitbagTmp->GetUseGridNum();
	g_kitbagTmp[0] = '\0';
	// _snprintf_s(g_kitbagTmp, sizeof(g_kitbagTmp), _TRUNCATE, "?????????%d@", sItemTmpNum);
	_snprintf_s(g_kitbagTmp, sizeof(g_kitbagTmp), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00022), sItemTmpNum);
	pCKb = pCMainC->m_pCKitbagTmp.get();
	for (short i = 0; i < sItemTmpNum; i++) {
		pGridCont = pCKb->GetGridContByNum(i);
		if (!pGridCont)
			continue;
		pCItem = GetItemRecordInfo(pGridCont->sID);
		if (!pCItem)
			continue;
		_snprintf_s(g_kitbagTmp + strlen(g_kitbagTmp), sizeof(g_kitbagTmp) - strlen(g_kitbagTmp), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
	}
	TL(CHA_ENTER, pCMainC->GetName(), "", g_kitbagTmp);

	char chStart = 0, chEnd = pPlayer->GetCurBankNum() - 1;
	for (char i = chStart; i <= chEnd; i++) {
		// _snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, "????ID(%d):", i + 1 );
		_snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00023), i + 1);
		pCKb = pPlayer->GetBank(i);
		sItemNum = pCKb->GetUseGridNum();
		_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "[%d]%d@;", i + 1, sItemNum);
		for (short i = 0; i < sItemNum; i++) {
			pGridCont = pCKb->GetGridContByNum(i);
			if (!pGridCont)
				continue;
			pCItem = GetItemRecordInfo(pGridCont->sID);
			if (!pCItem)
				continue;
			_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
		}
		TL(CHA_ENTER, pCMainC->GetName(), "", g_kitbag);
	}

	g_equip[0] = '\0';
	// _snprintf_s(g_equip, sizeof(g_equip), _TRUNCATE, "?????%d@", enumEQUIP_NUM);
	_snprintf_s(g_equip, sizeof(g_equip), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00024), enumEQUIP_NUM);
	for (short i = 0; i < enumEQUIP_NUM; i++) {
		pGridCont = &pCMainC->m_SChaPart.SLink[i];
		if (!pGridCont)
			continue;
		pCItem = GetItemRecordInfo(pGridCont->sID);
		if (!pCItem)
			continue;
		_snprintf_s(g_equip + strlen(g_equip), sizeof(g_equip) - strlen(g_equip), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
	}
	TL(CHA_EQUIP, pCMainC->GetName(), "", g_equip);

	//
	return true;
}

bool CGameDB::SavePlayer(CPlayer* pPlayer, char chSaveType) {
	if (!pPlayer || !pPlayer->GetMainCha())
		return FALSE;

	if (pPlayer->GetMainCha()->GetPlayer() != pPlayer) {
		// LG("????????????", "?????Player??? %p[dbid %u]???????????? %s???�????Player??? %p??\n",
		LG("save character great error", "save Player address %p[dbid %u]??the character main player %s??the character 's Player address %p??\n",
		   pPlayer, pPlayer->GetDBChaId(), pPlayer->GetMainCha()->GetLogName(), pPlayer->GetMainCha()->GetPlayer());
		// pPlayer->SystemNotice("??????????????????????");
		pPlayer->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00025));
		return FALSE;
	}

	bool bSaveMainCha = false, bSaveBoat = false, bSaveKitBag = false, bSaveMMask = false, bSaveBank = false;
	bool bSaveKitBagTmp = false;
	bool bSaveKBState = false;
	BeginTran();
	try {
		DWORD dwStartTick = GetTickCount();

		bSaveMainCha = _tab_cha->SaveAllData(pPlayer, chSaveType); // ?????????
		DWORD dwSaveMainTick = GetTickCount();
		bSaveKitBag = _tab_res->SaveKitbagData(pPlayer->GetMainCha());
		// ???????????
		bSaveKitBagTmp = _tab_res->SaveKitbagTmpData(pPlayer->GetMainCha());
		// ??????????????
		// bSaveKBState = _tab_cha->SaveKBState(pPlayer);
		DWORD dwSaveKbTick = GetTickCount();
		bSaveBank = _tab_res->SaveBankData(pPlayer);
		DWORD dwSaveBankTick = GetTickCount();
		if ((chSaveType != enumSAVE_TYPE_TIMER) && (g_Config.m_chMapMask > 0)) {
			if (pPlayer->IsMapMaskChange()) {
				bSaveMMask = _tab_mmask->SaveData(pPlayer);
				pPlayer->ResetMapMaskChange();
			}
		} else
			bSaveMMask = true;
		DWORD dwSaveMMaskTick = GetTickCount();
		bSaveBoat = _tab_boat->SaveAllData(pPlayer, chSaveType); // ????
		DWORD dwSaveBoatTick = GetTickCount();

		// LG("??????????", "???%-8d???????%-8d???????????%-8d??????%-8d??????%-8d????%-8d.[%d %s]\n",
		LG("save data waste time", "totalize %-8d??main character %-8d??main character kitbag %-8d??bank %-8d??big map %-8d??boat %-8d.[%d %s]\n",
		   dwSaveBoatTick - dwStartTick, dwSaveMainTick - dwStartTick, dwSaveKbTick - dwSaveMainTick, dwSaveBankTick - dwSaveKbTick, dwSaveMMaskTick - dwSaveBankTick, dwSaveBoatTick - dwSaveMMaskTick,
		   pPlayer->GetDBChaId(), pPlayer->GetMainCha()->GetLogName());
	} catch (...) {
		// LG("enter_map", "??????????????L??????????.\n");
		LG("enter_map", "It's abnormity when saving the character's whole data.\n");
	}

	if (!bSaveMainCha || !bSaveBoat || !bSaveKitBag) {
		RollBack();
		return false;
	}
	CommitTran();

	// LG("enter_map", "?????????????????.\n");
	LG("enter_map", "save character whole data succeed.\n");
	// ??�???l?
	if (chSaveType != enumSAVE_TYPE_TIMER) {
		CKitbag* pCKb;
		CCharacter* pCMainC = pPlayer->GetMainCha();
		short sItemNum = pCMainC->m_CKitbag.GetUseGridNum();
		g_kitbag[0] = '\0';
		// _snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, "?????%u ????????%d@", pCMainC->getAttr(ATTR_GD), sItemNum);
		_snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00026), pCMainC->getAttr(ATTR_GD), sItemNum);
		SItemGrid* pGridCont;
		CItemRecord* pCItem;
		pCKb = &(pCMainC->m_CKitbag);
		for (short i = 0; i < sItemNum; i++) {
			pGridCont = pCKb->GetGridContByNum(i);
			if (!pGridCont)
				continue;
			pCItem = GetItemRecordInfo(pGridCont->sID);
			if (!pCItem)
				continue;
			_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
		}
		TL(CHA_OUT, pCMainC->GetName(), "", g_kitbag);

		short sItemTmpNum = pCMainC->m_pCKitbagTmp->GetUseGridNum();
		g_kitbagTmp[0] = '\0';
		// _snprintf_s(g_kitbagTmp, sizeof(g_kitbagTmp), _TRUNCATE, "?????????%d@", sItemTmpNum);
		_snprintf_s(g_kitbagTmp, sizeof(g_kitbagTmp), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00022), sItemTmpNum);
		pCKb = pCMainC->m_pCKitbagTmp.get();
		for (short i = 0; i < sItemTmpNum; i++) {
			pGridCont = pCKb->GetGridContByNum(i);
			if (!pGridCont)
				continue;
			pCItem = GetItemRecordInfo(pGridCont->sID);
			if (!pCItem)
				continue;
			_snprintf_s(g_kitbagTmp + strlen(g_kitbagTmp), sizeof(g_kitbagTmp) - strlen(g_kitbagTmp), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
		}
		TL(CHA_OUT, pCMainC->GetName(), "", g_kitbagTmp);

		g_equip[0] = '\0';
		// _snprintf_s(g_equip, sizeof(g_equip), _TRUNCATE, "?????%d@", enumEQUIP_NUM);
		_snprintf_s(g_equip, sizeof(g_equip), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00024), enumEQUIP_NUM);
		for (short i = 0; i < enumEQUIP_NUM; i++) {
			pGridCont = &pCMainC->m_SChaPart.SLink[i];
			if (!pGridCont)
				continue;
			pCItem = GetItemRecordInfo(pGridCont->sID);
			if (!pCItem)
				continue;
			_snprintf_s(g_equip + strlen(g_equip), sizeof(g_equip) - strlen(g_equip), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
		}
		TL(CHA_EQUIP, pCMainC->GetName(), "", g_equip);

		char chStart = 0, chEnd = pPlayer->GetCurBankNum() - 1;
		// _snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, "????ID(%d):", pPlayer->GetCurBankNum());
		_snprintf_s(g_kitbag, sizeof(g_kitbag), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00023), pPlayer->GetCurBankNum());
		for (char i = chStart; i <= chEnd; i++) {
			pCKb = pPlayer->GetBank(i);
			sItemNum = pCKb->GetUseGridNum();
			_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "[%d]%d@;", i + 1, sItemNum);
			for (short i = 0; i < sItemNum; i++) {
				pGridCont = pCKb->GetGridContByNum(i);
				if (!pGridCont)
					continue;
				pCItem = GetItemRecordInfo(pGridCont->sID);
				if (!pCItem)
					continue;
				_snprintf_s(g_kitbag + strlen(g_kitbag), sizeof(g_kitbag) - strlen(g_kitbag), _TRUNCATE, "%s[%d],%d;", pCItem->szName, pGridCont->sID, pGridCont->sNum);
			}
		}
		TL(CHA_BANK, pCMainC->GetName(), "", g_kitbag);
	}
	//

	return true;
}

/*
// ????????????
#include "lua_gamectrl.h"
extern char g_TradeName[][8];
#include "SystemDialog.h"

void CGameDB::Log(const char *type, const char *c1, const char *c2, const char *c3, const char *c4, const char *p, BOOL bAddToList)
{
	if(g_Config.m_bLogDB==FALSE) return;

	if(!_tab_log) return;

	char szSQL[8192];

	_snprintf_s(szSQL, sizeof(szSQL), _TRUNCATE, "insert gamelog (action, c1, c2, c3, c4, content) \
				   values('%s', '%s', '%s', '%s', '%s', '%s')", type, c1, c2, c3, c4, p);

	//if(bAddToList)
	{
		// '??SendMessage????????, ???????????????, ???????????????????
	//	extern HWND g_SysDlg;
	//	PostMessage(g_SysDlg, WM_USER_LOG, 0, 0);
	}
	//else
	{
		ExecLogSQL(szSQL);
	}
}

void CGameDB::Log1(int nType, const char *cha1, const char *cha2, const char *pszContent)
{
	Log( g_TradeName[nType], cha1, "", cha2, "", pszContent);
}


void CGameDB::Log2(int nType, CCharacter *pCha1, CCharacter *pCha2, const char *pszContent)
{
	if(!_tab_log) return;

	char szName1[32]    = "";
	char szName2[32]    = "";
	char szActName1[32] = "";
	char szActName2[32] = "";

	if(pCha1)
	{
		strcpy(szName1, pCha1->GetName());
		if(pCha1->GetPlayer()) strcpy(szActName1, pCha1->GetPlayer()->GetActName());
	}
	if(pCha2)
	{
		strcpy(szName2, pCha2->GetName());
		if(pCha2->GetPlayer()) strcpy(szActName1, pCha2->GetPlayer()->GetActName());
	}

	Log(g_TradeName[nType], szName1, szActName1, szName2, szActName2, pszContent);
}*/

//===============CTableGuild Begin===========================================
bool CTableGuild::Init(void) {
	_snprintf_s(g_sql, sizeof(g_sql), _TRUNCATE, "select \
				guild_id, guild_name, motto, passwd, leader_id, exp, member_total,\
				try_total, disband_date \
				from %s \
				(nolock) where 1=2",
			_get_table());
	if (strlen(g_sql) >= SQL_MAXLEN) {
		// FILE	*pf = fopen("log\\SQL????????.txt", "a+");
		FILE* pf = fopen("log/SQLsentence_length_slopover.txt", "a+");
		if (pf) {
			fprintf(pf, "%s\n\n", g_sql);
			fclose(pf);
		}
		// LG("enter_map", "SQL????????!\n");
		LG("enter_map", "SQL sentence length slop over\n");
		return false;
	}
	short sExec = exec_sql_direct(g_sql);
	if (!DBOK(sExec)) {
		// MessageBox(0, "?????(guild)??'??????????", "????", MB_OK);
		char buffer[255];
		_snprintf_s(buffer, sizeof(buffer), _TRUNCATE, RES_STRING(GM_GAMEDB_CPP_00001), "guild");
		MessageBox(0, buffer, RES_STRING(GM_GAMEDB_CPP_00002), MB_OK);
		return false;
	}

	return true;
}

int CTableGuild::Create(CCharacter* pCha, char* guildname, cChar* passwd) {
	T_B int l_ret_guild_id = 0;
	string buf[1];

	while (true) {
		// ?????????ID
		const char* param = "isnull(min(guild_id),0)";
		char filter[80];
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id >0 and leader_id =0");
		bool ret = _get_row(buf, 1, param, filter);
		if (!ret) {
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00027));
			LG("consortia system", "found consortia system occur SQL operator error.");
			return 0;
		}
		l_ret_guild_id = atoi(buf[0].c_str());
		if (!l_ret_guild_id) {
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00030));
			return 0;
		}

		// Use stored procedure for GuildCreate
		int chaID = pCha->GetID();
		int guildID = l_ret_guild_id;
		SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildCreate(?,?,?,?)}",
			"dbo", "GuildCreate", 4, &chaID, passwd, guildname, &guildID);
		if (!DBOK(l_sqlret)) // ???????????????
		{
			// pCha->SystemNotice("???????????????'??");
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00031));
			return 0;
		}
		if (get_affected_rows() != 1) {
			continue;
		}

		break;
	}
	
	// Use stored procedure for GuildSetCharacterGuild
	int guildID = l_ret_guild_id;
	int guildPerm = emGldPermMax;
	int chaID = pCha->GetID();
	stored_procedure("{CALL dbo.GuildSetCharacterGuild(?,?,?)}",
		"dbo", "GuildSetCharacterGuild", 3, &guildID, &guildPerm, &chaID);

	WPACKET l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_CREATE);
	WRITE_LONG(l_wpk, l_ret_guild_id);									// ????ID
	WRITE_STRING(l_wpk, guildname);										// ????Name
	WRITE_STRING(l_wpk, g_GetJobName(uShort(pCha->getAttr(ATTR_JOB)))); // ??
	WRITE_SHORT(l_wpk, uShort(pCha->getAttr(ATTR_LV)));					// ???
	pCha->ReflectINFof(pCha, l_wpk);

	return l_ret_guild_id; // ?????????,???????ID
	T_E
}

bool CTableGuild::ListAll(CCharacter* pCha, char disband_days) {

	const char* sql_syntax = 0;
	if (!pCha || disband_days < 1) {
		return false;
	} else {
		sql_syntax =
			"select gld.guild_id, gld.guild_name, gld.motto, gld.leader_id, cha.cha_name leader_name, gld.exp, gld.member_total "
			"from guild As gld, character As cha where gld.leader_id =cha.cha_id";
	}
	bool ret = false;
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, "%s", sql_syntax);

	// ?????????
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
		col_num = std::min<decltype(col_num)>(col_num, MAX_COL);
		col_num = std::min<decltype(col_num)>(col_num, _max_col);

		// Bind Column
		for (int i = 0; i < col_num; ++i) {
			SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
		}

		WPACKET l_wpk, l_wpk0 = GETWPACKET();
		WRITE_CMD(l_wpk0, CMD_MC_LISTGUILD);

		// Fetch each Row	int i; // ?????????
		int f_row = 1;
		for (; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			}
			if ((f_row % 20) == 1) {
				l_wpk = l_wpk0;
			}
			WRITE_LONG(l_wpk, atoi((char const*)_buf[0]));		   // guild id
			WRITE_STRING(l_wpk, (char const*)_buf[1]);			   // guild name
			WRITE_STRING(l_wpk, (char const*)_buf[2]);			   // guild motto
			WRITE_STRING(l_wpk, (char const*)_buf[4]);			   // leader name
			WRITE_SHORT(l_wpk, atoi((const char*)_buf[6]));		   // guild member count
			LLong l_exp = _atoi64((const char*)_buf[5]); // guild exp
			WRITE_LONG(l_wpk, uLong(l_exp % 0x100000000));
			WRITE_LONG(l_wpk, uLong(l_exp / 0x100000000));

			if (!(f_row % 20)) {
				WRITE_CHAR(l_wpk, ((f_row - 1) % 20) + 1); // ?????????????
				pCha->ReflectINFof(pCha, l_wpk);
			}
		}
		if ((f_row % 20) == 1) {
			l_wpk = l_wpk0;
		}
		WRITE_LONG(l_wpk, (f_row - 1) % 20); // ?????????????
		pCha->ReflectINFof(pCha, l_wpk);

		SQLFreeStmt(hstmt, SQL_UNBIND);
		ret = true;
	} catch (int& e) {
		// LG("??????", "???????????ODBC ?????�????????%d\n",e);
		LG("consortia system", "found consortia process ODBC interfance transfer error,position ID:%d\n", e);
	} catch (...) {
		// LG("??????", "Unknown Exception raised when list all guilds\n");
		LG("consortia system", "Unknown Exception raised when list all guilds\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}
void CTableGuild::TryFor(CCharacter* pCha, uLong guildid) {
	if (pCha->HasGuild()) {
		// pCha->SystemNotice( "??????????%s?????,????????????????!", pCha->GetGuildName() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00032), pCha->GetGuildName());
		return;
	} else if (guildid == pCha->GetGuildID()) {
		// pCha->SystemNotice( "????????????????%s??!", pCha->GetGuildName() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00033), pCha->GetGuildName());
		return;
	}

	string buf[3];
	char filter[80];
	const char* param = "guild_id";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "leader_id >0 and guild_id =%d", guildid);
	int l_ret = _get_row(buf, 3, param, filter);
	if (!DBOK(l_ret)) {
		// pCha->SystemNotice("????????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00034));
		// LG("??????","?�?[%s]?????????[ID=%d]???SQL??????.\n",pCha->GetName(),guildid);
		LG("consortia system", "character[%s]apply join in consortia [ID=%d] carry out SQL failed.\n", pCha->GetName(), guildid);
		return;
	} else if (get_affected_rows() != 1) {
		// pCha->SystemNotice("???????L??????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00035));
		return;
	}
	param = "c.guild_id, c.guild_stat, g.guild_name";
	string l_tbl_name = _tbl_name;
	_tbl_name = "character c,guild g";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "c.guild_id =g.guild_id and c.cha_id =%d and g.guild_id <>%d", pCha->GetID(), guildid);
	l_ret = _get_row(buf, 3, param, filter);
	_tbl_name = l_tbl_name;
	if (!DBOK(l_ret) || get_affected_rows() != 1) {
		// pCha->SystemNotice("????????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00034));
		// LG("??????","?�?[%s]?????????[ID=%d]???SQL??????.\n",pCha->GetName(),guildid);
		LG("consortia system", "character[%s]apply join in consortia [ID=%d] carry out SQL failed.\n", pCha->GetName(), guildid);
		return;
	}

	// ??????????????
	string bufnew[1];
	param = "guild_name";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	l_ret = _get_row(bufnew, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {
	} else {
		// LG( "??????", "TryFor?????%s??????ID[0x%X]??????!", pCha->GetName(), guildid );
		LG("consortia system", "TryFor: character %s apply consortia ID[0x%X]is inexistence!", pCha->GetName(), guildid);
		// pCha->SystemNotice( "??????L???????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00036));
		return;
	}

	// ????????????L???????
	strncpy(pCha->GetPlayer()->m_szTempGuildName, bufnew[0].c_str(), defGUILD_NAME_LEN - 1);

	if (const auto guild_id = std::stoi(buf[0]); guild_id) {
		if (const auto status = std::stoi(buf[1]); status == emGldMembStatNormal) {
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00037), buf[2].c_str());
			return;
		} else if (status == emGldMembStatTry && pCha->GetPlayer()->m_GuildState.IsFalse(emGuildReplaceOldTry)) {
			pCha->GetPlayer()->m_GuildState.SetBit(emGuildReplaceOldTry);
			pCha->GetPlayer()->m_lTempGuildID = guildid;
			WPACKET l_wpk = GETWPACKET();
			WRITE_CMD(l_wpk, CMD_MC_GUILD_TRYFORCFM);
			WRITE_STRING(l_wpk, buf[2].c_str());
			pCha->ReflectINFof(pCha, l_wpk);
			return;
		}
	} else {
		TryForConfirm(pCha, guildid);
	}
}
void CTableGuild::TryForConfirm(CCharacter* pCha, uLong guildid) {
	if (pCha->HasGuild()) {
		// pCha->SystemNotice( "??????????%s?????,????????????????!", pCha->GetGuildName() );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00038), pCha->GetGuildName());
		return;
	}

	DWORD dwOldGuildID = pCha->GetGuildID();

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00039));
		return;
	}

	// Use stored procedure for GuildTryForApply
	int guildID = guildid;
	int chaID = pCha->GetID();
	int maxTryMembers = emMaxTryMemberNum;
	int maxMembers = emMaxMemberNum;
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildTryForApply(?,?,?,?)}",
		"dbo", "GuildTryForApply", 4, &guildID, &chaID, &maxTryMembers, &maxMembers);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "????????????????????�??????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00040));
		return;
	}

	// Use stored procedure for GuildIncrementTryTotal
	l_sqlret = stored_procedure("{CALL dbo.GuildIncrementTryTotal(?)}",
		"dbo", "GuildIncrementTryTotal", 1, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "????????????????????�??????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00040));
		return;
	}

	// ?????????????????????
	if (dwOldGuildID && pCha->GetPlayer()->m_GuildState.IsTrue(emGuildReplaceOldTry)) {
		// Use stored procedure for GuildDecrementTryTotal
		int oldGuildID = dwOldGuildID;
		SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildDecrementTryTotal(?)}",
			"dbo", "GuildDecrementTryTotal", 1, &oldGuildID);
		if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
			this->rollback();
			// pCha->SystemNotice( "?????????????????????????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00041));
			return;
		}
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "????????????????????�??????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00040));
		return;
	}

	// ?????�??????????
	pCha->SetGuildID(guildid);
	pCha->SetGuildState(emGldMembStatTry);

	pCha->SetGuildName(pCha->GetPlayer()->m_szTempGuildName);
	// pCha->SystemNotice( "???!???????????%s?????,?????j????????????????.", pCha->GetGuildName() );
	pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00042), pCha->GetGuildName());
}

bool CTableGuild::GetGuildBank(uLong guildid, CKitbag* bag) {
	string buf[3];
	char filter[80];
	const char* param = "bank";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {
		if (buf[0].length() == 0) {
			bag->SetCapacity(48);
			return true;
		}
		if (String2KitbagData(bag, buf[0])) {
			return true;
		}
	}
	return false;
}

int CTableGuild::GetGuildLeaderID(uLong guildid) {
	string buf[3];
	char filter[80];
	const char* param = "leader_id";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {
		return atoi(buf[0].c_str());
	}
	return 0;
}

bool CTableGuild::UpdateGuildBank(uLong guildid, CKitbag* bag) {
	char bagStr[defKITBAG_DATA_STRING_LEN];
	if (KitbagData2String(bag, bagStr, defKITBAG_DATA_STRING_LEN)) {
		// Use stored procedure for GuildUpdateBank
		int guildID = guildid;
		SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildUpdateBank(?,?)}",
			"dbo", "GuildUpdateBank", 2, bagStr, &guildID);
		if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
			// this->rollback(); // dont think we need to rollback??
			return false;
		}
		return true;
	}
	return false;
}

bool CTableGuild::UpdateGuildBankGold(int guildID, long long money) {
	// Use stored procedure for proper BIGINT handling and overflow protection (caps at 0-100B)
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildUpdateBankGold(?,?)}",
		"dbo", "GuildUpdateBankGold", 2, &money, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		return false;
	}
	return true;
}

unsigned long long CTableGuild::GetGuildBankGold(uLong guildid) {
	string buf[1];
	char filter[80];
	const char* param = "gold";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {
		return stoll(buf[0]);
	}
	return false;
}

std::vector<CTableGuild::BankLog> CTableGuild::GetGuildLog(uLong guildid) {
	string buf[1];
	string logList[1024]; // Max number of strings (1 log = 5 strings)
	std::vector<CTableGuild::BankLog> logs;

	char filter[SQL_MAXLEN];
	const char* param = "banklog";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {

		int n = Util_ResolveTextLine(buf[0].c_str(), logList, 1024, '-', ';');
		int i = 0;
		while (i < n) {
			BankLog p;
			p.type = Str2Int(logList[i].c_str());
			p.time = (time_t)_atoi64(logList[i + 1].c_str());  // 64-bit for time_t on x64
			p.parameter = (unsigned long long)_atoi64(logList[i + 2].c_str());  // 64-bit for gold values > 2B
			p.quantity = Str2Int(logList[i + 3].c_str());
			p.userID = Str2Int(logList[i + 4].c_str());
			logs.push_back(p);
			i += 5;
		}
	}

	if (logs.size() == 200) {
		logs.erase(logs.begin()); // size is 200, let's erase the first log
	}

	return logs;
}

bool CTableGuild::SetGuildLog(std::vector<BankLog> log, uLong guild_id) {
	char data[8000];
	data[0] = '\0';
	size_t currentLen = 0;
	const size_t maxLen = sizeof(data) - 1;  // Reserve space for null terminator
	
	for (size_t i = 0; i < log.size(); i++) {
		if (log.at(i).userID == 0) {
			continue;
		}
		char buf[100];
		int written = _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d-%lld-%lld-%d-%d;", 
			log.at(i).type, log.at(i).time, log.at(i).parameter, log.at(i).quantity, log.at(i).userID);
		
		// Check if we have enough space before appending
		if (written > 0 && currentLen + written < maxLen) {
			strcat(data, buf);
			currentLen += written;
		} else {
			// Buffer would overflow, stop adding more logs
			LG("bank_warning", "Guild bank log truncated due to size limit: guild_id=%d entries=%zu",
			   guild_id, i);
			break;
		}
	}

	// Use stored procedure for GuildSetBankLog
	int guildID = guild_id;
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildSetBankLog(?,?)}",
		"dbo", "GuildSetBankLog", 2, data, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		return false;
	}
	return true;
}

bool CTableGuild::GetGuildInfo(CCharacter* pCha, uLong guildid) {
	string buf[4];
	char filter[80];

	const char* param = "guild_name, motto";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", guildid);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 2, param, filter, &l_retrow);
	if (l_retrow == 1) {
		pCha->SetGuildName(buf[0].c_str());
		pCha->SetGuildMotto(buf[1].c_str());
		return true;
	} else {
		return false;
	}
}

bool CTableGuild::ListTryPlayer(CCharacter* pCha, char disband_days) {
	bool ret = false;

	if (!pCha || !pCha->HasGuild()) {
		return ret;
	}

	string buf[10];
	char filter[80];

	const char* sql_syntax = "g.guild_id, g.guild_name,g.motto, c.cha_name, g.member_total,g.exp, g.level";

	char param[500];
	// _snprintf_s(param , sql_syntax , disband_days);
	_snprintf_s(param, sizeof(param), _TRUNCATE, "%s", sql_syntax);

	string l_tbl_name = _tbl_name;
	_tbl_name = "character c,guild g";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "g.leader_id =c.cha_id and g.guild_id =%d", pCha->GetGuildID());
	int l_retrow = 0;

	bool l_ret = _get_row(buf, 10, param, filter, &l_retrow);
	_tbl_name = l_tbl_name;
	if (!l_ret || !l_retrow || this->get_affected_rows() != 1) {
		return ret;
	}
	WPACKET l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MC_GUILD_LISTTRYPLAYER);
	WRITE_LONG(l_wpk, atoi(buf[0].c_str()));  // guild_id
	WRITE_STRING(l_wpk, buf[1].c_str());	  // guild_name
	WRITE_STRING(l_wpk, buf[2].c_str());	  // motto
	WRITE_STRING(l_wpk, buf[3].c_str());	  // cha_name (leader)
	WRITE_SHORT(l_wpk, atoi(buf[4].c_str())); // member_total
	WRITE_SHORT(l_wpk, g_Config.m_sGuildNum); // max members
	l_wpk.WriteLongLong(_atoi64(buf[5].c_str())); // exp

	WRITE_LONG(l_wpk, 0); // remain time (not used, client expects this)

	sql_syntax =
		"select c.cha_id,c.cha_name,c.job,c.degree\
			from character c\
			where (c.guild_stat =1 and c.guild_id =%d and c.delflag =0)\
		";
	char sql[SQL_MAXLEN];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE, sql_syntax, pCha->GetGuildID());

	// ?????????
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
		col_num = std::min<decltype(col_num)>(col_num, MAX_COL);
		col_num = std::min<decltype(col_num)>(col_num, _max_col);

		// Bind Column
		for (int i = 0; i < col_num; ++i) {
			SQLBindCol(hstmt, UWORD(i + 1), SQL_C_CHAR, _buf[i], MAX_DATALEN, &_buf_len[i]);
		}

		// Fetch each Row	int i; // ?????????
		int f_row = 0;
		for (; (sqlret = SQLFetch(hstmt)) == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO; ++f_row) {
			if (sqlret != SQL_SUCCESS) {
				handle_err(hstmt, SQL_HANDLE_STMT, sqlret);
			}

			WRITE_LONG(l_wpk, atoi((char const*)_buf[0]));	// ID
			WRITE_STRING(l_wpk, (char const*)_buf[1]);		// ????
			WRITE_STRING(l_wpk, (char const*)_buf[2]);		// ??
			WRITE_SHORT(l_wpk, atoi((char const*)_buf[3])); // ???
		}

		WRITE_LONG(l_wpk, f_row); // ?????????????
		pCha->ReflectINFof(pCha, l_wpk);

		SQLFreeStmt(hstmt, SQL_UNBIND);
		ret = true;
	} catch (int& e) {
		// LG("??????", "?????????????????ODBC ?????�????????%d\n",e);
		LG("consortia system", "consult apply consortia process memeberODBC interface transfer error,position ID:%d\n", e);
	} catch (...) {
		// LG("??????", "Unknown Exception raised when list all guilds\n");
		LG("consortia system", "Unknown Exception raised when list all guilds\n");
	}

	if (hstmt != SQL_NULL_HSTMT) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hstmt = SQL_NULL_HSTMT;
	}

	return ret;
}
bool CTableGuild::Approve(CCharacter* pCha, uLong chaid) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	string buf[3];
	char filter[80];

	const char* param = "c.cha_id";
	string l_tbl_name = _tbl_name;
	_tbl_name = "character c";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "c.cha_id =%d and c.guild_id =%d and c.guild_permission & %d =%d", pCha->GetID(), pCha->GetGuildID(), emGldPermRecruit, emGldPermRecruit);
	int retrow = 0;
	bool l_ret = _get_row(buf, 3, param, filter, &retrow);
	_tbl_name = l_tbl_name;
	if (!l_ret) {
		// pCha->SystemNotice("?????????????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00043));
		return false;
	}
	if (!retrow) {
		// pCha->SystemNotice("??�?????L??????");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00044));
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00045));
		return false;
	}

	// Use stored procedure for GuildApproveUpdateGuild
	int guildID = pCha->GetGuildID();
	int maxMembers = g_Config.m_sGuildNum;
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildApproveUpdateGuild(?,?)}",
		"dbo", "GuildApproveUpdateGuild", 2, &guildID, &maxMembers);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00046));
		return false;
	}

	// Use stored procedure for GuildApproveUpdateCharacter
	int guildPerm = emGldPermDefault;
	int targetChaID = chaid;
	l_sqlret = stored_procedure("{CALL dbo.GuildApproveUpdateCharacter(?,?,?)}",
		"dbo", "GuildApproveUpdateCharacter", 3, &guildPerm, &targetChaID, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00046));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00046));
		return false;
	}

	WPACKET l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MM_GUILD_APPROVE);
	WRITE_LONG(l_wpk, chaid);
	WRITE_LONG(l_wpk, pCha->GetGuildID());
	WRITE_STRING(l_wpk, pCha->GetValidGuildName());
	WRITE_STRING(l_wpk, pCha->GetValidGuildMotto());
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_APPROVE);
	WRITE_LONG(l_wpk, chaid);
	pCha->ReflectINFof(pCha, l_wpk);

	const std::string cha_name = game_db.GetChaNameByID(chaid);

	char msg[SQL_MAXLEN];
	_snprintf_s(msg, sizeof(msg), _TRUNCATE, "%s has been accepted to the guild!", cha_name.c_str());
	DWORD guildIDDword = pCha->GetGuildID();
	g_pGameApp->GuildNotice(guildIDDword, msg);

	return true;
}
bool CTableGuild::Reject(CCharacter* pCha, uLong chaid) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	string buf[3];
	char filter[80];

	const char* param = "c.cha_id";
	string l_tbl_name = _tbl_name;
	_tbl_name = "character c";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "c.cha_id =%d and c.guild_id =%d and c.guild_permission & %d =%d", pCha->GetID(), pCha->GetGuildID(), emGldPermRecruit, emGldPermRecruit);
	int retrow = 0;
	bool l_ret = _get_row(buf, 3, param, filter, &retrow);
	_tbl_name = l_tbl_name;
	if (!l_ret) {
		// pCha->SystemNotice("?????????????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00047));
		return false;
	}
	if (!retrow) {
		// pCha->SystemNotice("??�?????L??????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00048));
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00045));
		return false;
	}

	// Use stored procedure for GuildRejectUpdateCharacter
	int targetChaID = chaid;
	int guildID = pCha->GetGuildID();
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildRejectUpdateCharacter(?,?)}",
		"dbo", "GuildRejectUpdateCharacter", 2, &targetChaID, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????!?????�???????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00049));
		return false;
	}

	// Use stored procedure for GuildDecrementTryTotal
	l_sqlret = stored_procedure("{CALL dbo.GuildDecrementTryTotal(?)}",
		"dbo", "GuildDecrementTryTotal", 1, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????!?????�???????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00049));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????!?????�???????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00049));
		return false;
	}

	WPACKET l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MM_GUILD_REJECT);
	WRITE_LONG(l_wpk, chaid);
	WRITE_STRING(l_wpk, pCha->GetGuildName());
	pCha->ReflectINFof(pCha, l_wpk);
	return true;
}
bool CTableGuild::Kick(CCharacter* pCha, uLong chaid) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	string buf[3];
	char filter[80];

	const char* param = "c.cha_id";
	string l_tbl_name = _tbl_name;
	_tbl_name = "character c";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "c.cha_id =%d and c.guild_id =%d and c.guild_permission & %d =%d", pCha->GetID(), pCha->GetGuildID(), emGldPermKick, emGldPermKick);
	int retrow = 0;
	bool l_ret = _get_row(buf, 3, param, filter, &retrow);
	_tbl_name = l_tbl_name;
	if (!l_ret) {
		// pCha->SystemNotice("??????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00050));
		return false;
	}
	if (!retrow) {
		// pCha->SystemNotice("??�?????L??????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00048));
		return false;
	}

	if (chaid == pCha->GetID()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00051));
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00052));
		return false;
	}

	// Use stored procedure for GuildKickUpdateCharacter
	int targetChaID = chaid;
	int guildID = pCha->GetGuildID();
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildKickUpdateCharacter(?,?)}",
		"dbo", "GuildKickUpdateCharacter", 2, &targetChaID, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "?????????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00053));
		return false;
	}

	// Use stored procedure for GuildDecrementMemberTotal
	l_sqlret = stored_procedure("{CALL dbo.GuildDecrementMemberTotal(?)}",
		"dbo", "GuildDecrementMemberTotal", 1, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "?????????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00053));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "?????????????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00053));
		return false;
	}

	WPACKET l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MM_GUILD_KICK);
	WRITE_LONG(l_wpk, chaid);
	WRITE_STRING(l_wpk, pCha->GetGuildName());
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_KICK);
	WRITE_LONG(l_wpk, chaid);
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MC_GUILD_KICK);
	pCha->ReflectINFof(pCha, l_wpk);

	return true;
}
bool CTableGuild::Leave(CCharacter* pCha) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00054));
		return false;
	}

	// Use stored procedure for GuildLeaveUpdateCharacter
	int chaID = pCha->GetID();
	int guildID = pCha->GetGuildID();
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildLeaveUpdateCharacter(?,?)}",
		"dbo", "GuildLeaveUpdateCharacter", 2, &chaID, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "????j?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00055));
		return false;
	}

	// Use stored procedure for GuildDecrementMemberTotal
	l_sqlret = stored_procedure("{CALL dbo.GuildDecrementMemberTotal(?)}",
		"dbo", "GuildDecrementMemberTotal", 1, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "????j?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00055));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "????j?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00055));
		return false;
	}

	char msg[SQL_MAXLEN];
	_snprintf_s(msg, sizeof(msg), _TRUNCATE, "%s has left the guild!", pCha->GetName());
	DWORD guildIDDword = pCha->GetGuildID();
	g_pGameApp->GuildNotice(guildIDDword, msg);

	pCha->SetGuildID(0);
	pCha->SetGuildName("");
	pCha->SetGuildMotto("");
	pCha->SyncGuildInfo();
	// pCha->SystemNotice("?????????!");
	pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00056));

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_LEAVE);
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MC_GUILD_LEAVE);
	pCha->ReflectINFof(pCha, l_wpk);
	return true;
}
bool CTableGuild::Disband(CCharacter* pCha, cChar* passwd) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	string buf[6];
	char filter[80];
	const char* param = "challlevel";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 6, param, filter, &l_retrow);
	if (l_retrow == 1) {
		if (atoi(buf[0].c_str()) > 0) {
			// pCha->SystemNotice( "???????????????h????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00057));
			return false;
		} else {
			l_retrow = 0;
			_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challid =%d", pCha->GetValidGuildID());
			bool l_ret = _get_row(buf, 6, param, filter, &l_retrow);
			if (!l_ret) {
				// pCha->SystemNotice( "?????L????????????!" );
				pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00058));
				return false;
			}
			if (l_retrow >= 1) {
				// pCha->SystemNotice( "?????????????????????!" );
				pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00059));
				return false;
			}
		}
	} else {
		// pCha->SystemNotice( "?????L????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00060));
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00061));
		return false;
	}

	// Use stored procedure for GuildDisbandUpdateGuild
	int guildID = pCha->GetGuildID();
	char passwdCopy[64];
	strncpy_s(passwdCopy, sizeof(passwdCopy), passwd, _TRUNCATE);
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildDisbandUpdateGuild(?,?)}",
		"dbo", "GuildDisbandUpdateGuild", 2, &guildID, passwdCopy);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00062));
		return false;
	}

	// Use stored procedure for GuildDisbandUpdateCharacters
	l_sqlret = stored_procedure("{CALL dbo.GuildDisbandUpdateCharacters(?)}",
		"dbo", "GuildDisbandUpdateCharacters", 1, &guildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00062));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00062));
		return false;
	}
	pCha->guildPermission = 0;
	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_DISBAND);
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MM_GUILD_DISBAND);
	WRITE_LONG(l_wpk, guildID);
	pCha->ReflectINFof(pCha, l_wpk);

	return true;
}
bool CTableGuild::Motto(CCharacter* pCha, cChar* motto) {
	if (!pCha || !pCha->HasGuild()) {
		return false;
	}

	// Use stored procedure for GuildUpdateMotto
	char mottoCopy[256];
	strncpy_s(mottoCopy, sizeof(mottoCopy), motto, _TRUNCATE);
	int guildID = pCha->GetGuildID();
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildUpdateMotto(?,?)}",
		"dbo", "GuildUpdateMotto", 2, mottoCopy, &guildID);
	if (!DBOK(l_sqlret)) {
		// pCha->SystemNotice("??L????????????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00063));
		return false; // ???SQL????
	}
	if (get_affected_rows() != 1) {
		// pCha->SystemNotice("??????????L?????????.");
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00064));
		return false;
	}

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MM_GUILD_MOTTO);
	WRITE_LONG(l_wpk, pCha->GetGuildID());
	WRITE_STRING(l_wpk, motto);
	pCha->ReflectINFof(pCha, l_wpk);

	l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_GUILD_MOTTO);
	WRITE_STRING(l_wpk, motto);
	pCha->ReflectINFof(pCha, l_wpk);

	return true;
}

bool CTableGuild::GetGuildName(int lGuildID, std::string& strGuildName) {
	char filter[80];

	const char* param = "guild_name";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", lGuildID);
	int l_retrow = 0;
	return _get_row(&strGuildName, 1, param, filter, &l_retrow);
}

bool CTableGuild::Leizhu(CCharacter* pCha, BYTE byLevel, long long dwMoney) {
	if (!pCha || !pCha->HasGuild() || byLevel < 1 || byLevel > 3) {
		return false;
	}

	if (dwMoney == 0) {
		// pCha->SystemNotice( "?????????????????????0??!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00065));
		return false;
	}

	string buf[6];
	char filter[80];
	const char* param1 = "guild_id, guild_name, challid, challmoney, leader_id, challstart";
	if (pCha->GetValidGuildID() > 0) {
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
		int l_retrow = 0;
		bool l_ret = _get_row(buf, 6, param1, filter, &l_retrow);
		if (l_retrow == 1) {
			if (pCha->GetID() == atoi(buf[4].c_str())) {
				// ?????????
			} else {
				return false;
			}
		} else {
			// pCha->SystemNotice( "??????L?????????!?????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00066));
			return false;
		}

		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challid =%d", pCha->GetValidGuildID());
		l_ret = _get_row(buf, 6, param1, filter, &l_retrow);
		if (l_retrow >= 1) {
			// pCha->SystemNotice( "??�????????h????????????????%s?????????^_^!", buf[1].c_str() );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00067), buf[1].c_str());
			return false;
		}
	} else {
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00068));
		return false;
	}

	char sql[SQL_MAXLEN];
	char szGuild[64];
	memset(szGuild, 0, 64);
	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	const char* param = "guild_id, guild_name, challid, challmoney";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", byLevel);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 4, param, filter, &l_retrow);
	if (l_retrow == 1) {
		// pCha->SystemNotice( "????%s?????????????%d??????!", buf[1].c_str(), byLevel );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00069), buf[1].c_str(), byLevel);
		return false;
	} else {
		const char* param1 = "challlevel";
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
		bool l_ret = _get_row(buf, 4, param1, filter, &l_retrow);
		if (l_retrow == 1) {
			if (atoi(buf[0].c_str()) > 0) {
				// pCha->SystemNotice( "???L??????????????%d????????!", atoi(buf[0].c_str()) );
				pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00070), atoi(buf[0].c_str()));
				return false;
			}
		}

		DWORD dwMoneyArray[3] = {5000000, 3000000, 1000000};
		if (dwMoney < dwMoneyArray[byLevel - 1] || !pCha->HasMoney(dwMoney)) {
			// pCha->SystemNotice( "??????????%d?L???????????%uG??!", byLevel, dwMoneyArray[byLevel-1] );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00071), byLevel, dwMoneyArray[byLevel - 1]);
			return false;
		}

		// Use stored procedure for GuildLeizhuSetLevel
		int byLevelInt = byLevel;
		int validGuildID = pCha->GetValidGuildID();
		SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildLeizhuSetLevel(?,?)}",
			"dbo", "GuildLeizhuSetLevel", 2, &byLevelInt, &validGuildID);
		if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
			this->rollback();
			// LG( "???????", "???????????????????????????????�?????????????!????????!????ID = %d.??????????%d", pCha->GetValidGuildID(), byLevel );
			LG("challenge consortia", "challenge consortia over,leizhu failed:update lost consortia data operater failed! consortiaID = %d.consortia level:%d", pCha->GetValidGuildID(), byLevel);
			return false;
		}

		if (!commit_tran()) {
			this->rollback();
			// pCha->SystemNotice( "????????????????????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00072));
			return false;
		}
		// if( pCha->TakeMoney( "??", dwMoney ) )
		if (pCha->TakeMoney(RES_STRING(GM_GAMEDB_CPP_00073), dwMoney)) {
			// pCha->SystemNotice( "?????L???%s?????????????%d???????!", pCha->GetGuildName(), byLevel );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00074), pCha->GetGuildName(), byLevel);
		}
		this->ListChallenge(pCha);
	}
	return true;
}

bool CTableGuild::Challenge(CCharacter* pCha, BYTE byLevel, long long dwMoney) {
	if (!pCha || !pCha->HasGuild() || byLevel < 1 || byLevel > 3) {
		return false;
	}

	if (dwMoney == 0) {
		// pCha->SystemNotice( "??????????????????0??!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00075));
		return false;
	}

	string buf[6];
	char filter[80];
	const char* param1 = "guild_id, guild_name, challid, challmoney, leader_id, challstart";
	if (pCha->GetValidGuildID() > 0) {
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
		int l_retrow = 0;
		bool l_ret = _get_row(buf, 6, param1, filter, &l_retrow);
		if (l_retrow == 1) {
			if (pCha->GetID() == atoi(buf[4].c_str())) {
				// ?????????
			} else {
				return false;
			}
		} else {
			// pCha->SystemNotice( "??????L?????????!?????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00066));
			return false;
		}

		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challid =%d", pCha->GetValidGuildID());
		l_ret = _get_row(buf, 6, param1, filter, &l_retrow);
		if (l_retrow >= 1) {
			// pCha->SystemNotice( "??�????????h????????????????%s?????????^_^!", buf[1].c_str() );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00067), buf[1].c_str());
			return false;
		}
	} else {
		return false;
	}

	// ??'????
	if (!begin_tran()) {
		// pCha->SystemNotice( "?????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00068));
		return false;
	}

	char sql[SQL_MAXLEN];
	char szGuild[64];
	memset(szGuild, 0, 64);
	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	const char* param = "guild_id, guild_name, challid, challmoney";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", byLevel);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 4, param, filter, &l_retrow);
	if (l_retrow == 1) {
		dwGuildID = atoi(buf[0].c_str());
		strncpy(szGuild, buf[1].c_str(), 63);
		dwChallID = atoi(buf[2].c_str());
		dwChallMoney = _atoi64(buf[3].c_str());
	} else {
		DWORD dwMoneyArray[3] = {5000000, 3000000, 1000000};
		if (dwMoney < dwMoneyArray[byLevel - 1] || !pCha->HasMoney(dwMoney)) {
			// pCha->SystemNotice( "??????????%d?L???????????%uG??!", byLevel, dwMoneyArray[byLevel-1] );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00077), byLevel, dwMoneyArray[byLevel - 1]);
			return false;
		}

		// Use stored procedure for GuildLeizhuSetLevel
		int byLevelInt = byLevel;
		int validGuildID = pCha->GetValidGuildID();
		SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildLeizhuSetLevel(?,?)}",
			"dbo", "GuildLeizhuSetLevel", 2, &byLevelInt, &validGuildID);
		if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
			this->rollback();
			// LG( "???????", "???????????????????????????????�?????????????!????????!????ID = %d.??????????%d", pCha->GetValidGuildID(), byLevel );
			LG("challenge consortia", "challenge consortia over,leizhu failed:update lost consortia data operater failed! consortiaID = %d.consortia level:%d", pCha->GetValidGuildID(), byLevel);
			return false;
		}

		if (!commit_tran()) {
			this->rollback();
			// pCha->SystemNotice( "????????????????????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00072));
			return false;
		}
		// if( pCha->TakeMoney( "??", dwMoney ) )
		if (pCha->TakeMoney(RES_STRING(GM_GAMEDB_CPP_00073), dwMoney)) {
			// pCha->SystemNotice( "?????L???%s?????????????%d???????!", pCha->GetGuildName(), byLevel );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00074), pCha->GetGuildName(), byLevel);
		}
		this->ListChallenge(pCha);
		return true;
	}

	BYTE byLvData = 0;
	const char* param2 = "challlevel";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
	l_ret = _get_row(buf, 4, param2, filter, &l_retrow);
	if (l_retrow == 1) {
		byLvData = (BYTE)atoi(buf[0].c_str());
	} else {
		// pCha->SystemNotice( "?????L?????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00078));
		return false;
	}

	if (dwGuildID == 0) {
		// pCha->SystemNotice( "????????????!GID = %d, LV = %d", dwGuildID, byLevel );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00079), dwGuildID, byLevel);
		return false;
	}

	if (byLvData != 0 && byLevel > byLvData) {
		// pCha->SystemNotice( "???????????????????????L???!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00080));
		return false;
	}

	if (pCha->GetPlayer()->GetDBChaId() == dwChallID) {
		// pCha->SystemNotice( "????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00081));
		return false;
	} else if (pCha->GetValidGuildID() == dwGuildID) {
		// pCha->SystemNotice( "?????????????L???!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00082));
		return false;
	} else if (dwMoney < dwChallMoney + 50000) {
		// pCha->SystemNotice( "????????????!???%u??", dwMoney );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00083), dwMoney);
		return false;
	}

	if (!pCha->HasMoney(dwMoney)) {
		// pCha->SystemNotice( "?????????�?????�??!???%u??", dwMoney );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00084), dwMoney);
		return false;
	}

	// ?????�???????? - Use stored procedure for GuildChallengeUpdate
	int challID = pCha->GetGuildID();
	long long challMoney = dwMoney;
	int targetGuildID = dwGuildID;
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildChallengeUpdate(?,?,?)}",
		"dbo", "GuildChallengeUpdate", 3, &challID, &challMoney, &targetGuildID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00085));
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// pCha->SystemNotice( "???????????????????????!" );
		pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00085));
		return false;
	}

	// ???
	// pCha->TakeMoney( "??", dwMoney );
	pCha->TakeMoney(RES_STRING(GM_GAMEDB_CPP_00073), dwMoney);
	// ???j????????????????
	if (dwChallID > 0 && dwChallMoney > 0) {
		WPacket l_wpk = GETWPACKET();
		WRITE_CMD(l_wpk, CMD_MP_GUILD_CHALLMONEY);
		WRITE_LONG(l_wpk, dwChallID);
		WRITE_LONGLONG(l_wpk, dwChallMoney);
		WRITE_STRING(l_wpk, szGuild);
		WRITE_STRING(l_wpk, pCha->GetGuildName());
		pCha->ReflectINFof(pCha, l_wpk);
	}

	ListChallenge(pCha);
	return true;
}

void CTableGuild::ListChallenge(CCharacter* pCha) {
	string buf1[6];
	string buf2[6];
	char filter[80];

	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	DWORD dwLeaderID = 0;
	BYTE byStart = 0;

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MC_GUILD_LISTCHALL);

	const char* param = "guild_id, guild_name, challid, challmoney, leader_id, challstart";
	if (pCha->GetValidGuildID() > 0) {
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pCha->GetValidGuildID());
		int l_retrow = 0;
		bool l_ret = _get_row(buf1, 6, param, filter, &l_retrow);
		if (l_retrow == 1) {
			if (pCha->GetID() == atoi(buf1[4].c_str())) {
				// ?????????
				WRITE_CHAR(l_wpk, 1);
			} else {
				WRITE_CHAR(l_wpk, 0);
			}
		} else {
			// pCha->SystemNotice( "??????L?????????!?????????!" );
			pCha->SystemNotice(RES_STRING(GM_GAMEDB_CPP_00066));
			return;
		}
	} else {
		WRITE_CHAR(l_wpk, 0);
	}

	for (int i = 1; i <= 3; ++i) {
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", i);
		int l_retrow = 0;
		bool l_ret = _get_row(buf1, 6, param, filter, &l_retrow);
		if (l_retrow == 1) {
			dwGuildID = atoi(buf1[0].c_str());
			dwChallID = atoi(buf1[2].c_str());
			dwChallMoney = _atoi64(buf1[3].c_str());
			byStart = (BYTE)atoi(buf1[5].c_str());

			if (dwChallID != 0) {
				_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", dwChallID);
				bool l_ret = _get_row(buf2, 6, param, filter, &l_retrow);
				if (l_retrow == 1) {
					WRITE_CHAR(l_wpk, i);
					WRITE_CHAR(l_wpk, byStart);
					WRITE_STRING(l_wpk, buf1[1].c_str());
					WRITE_STRING(l_wpk, buf2[1].c_str());
					WRITE_LONGLONG(l_wpk, dwChallMoney);
				} else {
					WRITE_CHAR(l_wpk, 0);
				}
			} else {
				WRITE_CHAR(l_wpk, i);
				WRITE_CHAR(l_wpk, byStart);
				WRITE_STRING(l_wpk, buf1[1].c_str());
				WRITE_STRING(l_wpk, "");
				WRITE_LONGLONG(l_wpk, dwChallMoney);
			}
		} else {
			WRITE_CHAR(l_wpk, 0);
		}
	}
	pCha->ReflectINFof(pCha, l_wpk);
}

bool CTableGuild::HasGuildLevel(CCharacter* pChar, BYTE byLevel) {
	if (!pChar->HasGuild()) {
		return false;
	}

	string buf[1];
	char filter[80];
	BYTE byData = 0;
	const char* param = "challlevel";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", pChar->GetValidGuildID());
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 1, param, filter, &l_retrow);
	if (l_retrow == 1) {
		byData = (BYTE)atoi(buf[0].c_str());
		return byLevel == byData;
	}
	return false;
}

bool CTableGuild::HasCall(BYTE byLevel) {
	string buf[5];
	char filter[80];

	char szGuild[64];
	memset(szGuild, 0, 64);
	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	const char* param = "guild_id, guild_name, challid, challmoney, challstart";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", byLevel);
	int l_retrow = 0;
	BYTE byStart = 0;
	bool l_ret = _get_row(buf, 5, param, filter, &l_retrow);
	if (l_retrow == 1) {
		dwGuildID = atoi(buf[0].c_str());
		strncpy(szGuild, buf[1].c_str(), 63);
		dwChallID = atoi(buf[2].c_str());
		dwChallMoney = _atoi64(buf[3].c_str());
		byStart = (BYTE)atoi(buf[4].c_str());
		return dwChallID != 0 && byStart == 1;
	}
	return false;
}

bool CTableGuild::StartChall(BYTE byLevel) {
	// LG( "???????", "?????????%d???????'????...\n", byLevel );
	LG("challenge consortia", "range level %d challenge start treat with....\n", byLevel);
	string buf[4];
	char filter[80];

	char szGuild[64];
	memset(szGuild, 0, 64);
	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	const char* param = "guild_id, guild_name, challid, challmoney";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", byLevel);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 4, param, filter, &l_retrow);
	if (l_retrow == 1) {
		dwGuildID = atoi(buf[0].c_str());
		strncpy(szGuild, buf[1].c_str(), 63);
		dwChallID = atoi(buf[2].c_str());
		dwChallMoney = _atoi64(buf[3].c_str());
	} else {
		return false;
	}

	if (dwGuildID == 0) {
		return false;
	}

	// ?????�???????? - Use stored procedure for GuildStartChallenge
	int guildIDInt = dwGuildID;
	SQLRETURN l_sqlret = stored_procedure("{CALL dbo.GuildStartChallenge(?)}",
		"dbo", "GuildStartChallenge", 1, &guildIDInt);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		// LG( "???????", "?????????????????!??????????'???????????!" );
		LG("challenge consortia", "challenge consortia data operator failed!consortia battle already start or inexistence!");
		return false;
	}

	// LG( "???????", "??????%d????????????'!GUILD1 = %d, GUILD2 = %d, Money = %u.\n", byLevel, dwGuildID, dwChallID, dwChallMoney );
	LG("challenge consortia", "range level %d challenge start succeed !GUILD1 = %d, GUILD2 = %d, Money = %lld.\n", byLevel, dwGuildID, dwChallID, dwChallMoney);
	return true;
}

void CTableGuild::EndChall(DWORD dwGuild1, DWORD dwGuild2, BOOL bChall) {
	// LG( "???????", "???????????????'????????GUILD1 = %d, GUILD2 = %d...\n", dwGuild1, dwGuild2 );
	LG("challenge consortia", "arranger level consortia game start operator finish GUILD1 = %d, GUILD2 = %d...\n", dwGuild1, dwGuild2);
	string buf[5];
	char filter[80];

	char szGuild[64];
	memset(szGuild, 0, 64);
	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	BYTE byLevel = 0;
	BYTE byStart = 0;
	const char* param = "challstart, guild_name, challid, challmoney, challlevel";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", dwGuild1);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 5, param, filter, &l_retrow);
	if (l_retrow == 1) {
		byStart = (BYTE)atoi(buf[0].c_str());
		strncpy(szGuild, buf[1].c_str(), 63);
		dwChallID = atoi(buf[2].c_str());
		dwChallMoney = _atoi64(buf[3].c_str());
		byLevel = (BYTE)atoi(buf[4].c_str());
		if (dwChallID == dwGuild2) {
			ChallMoney(byLevel, bChall, dwGuild1, dwGuild2, dwChallMoney);
			// LG( "???????", "??????%d???????????!GUILD1 = %d, GUILD2 = %d, Money = %u.\n", byLevel, dwGuild1, dwGuild2, dwChallMoney );
			LG("challenge consortia", "range level %d consortia challenge over!GUILD1 = %d, GUILD2 = %d, Money = %lld.\n", byLevel, dwGuild1, dwGuild2, dwChallMoney);
			return;
		}
	}

	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", dwGuild2);
	l_ret = _get_row(buf, 5, param, filter, &l_retrow);
	if (l_retrow == 1) {
		byStart = (BYTE)atoi(buf[0].c_str());
		strncpy(szGuild, buf[1].c_str(), 63);
		dwChallID = atoi(buf[2].c_str());
		dwChallMoney = _atoi64(buf[3].c_str());
		byLevel = (BYTE)atoi(buf[4].c_str());
		if (dwChallID == dwGuild1) {
			ChallMoney(byLevel, !bChall, dwGuild2, dwGuild1, dwChallMoney);
			// LG( "???????", "??????%d???????????!GUILD1 = %d, GUILD2 = %d, Money = %u.\n", byLevel, dwGuild2, dwGuild1, dwChallMoney );
			LG("challenge consortia", "range level %d consortia challenge over!GUILD1 = %d, GUILD2 = %d, Money = %lld.\n", byLevel, dwGuild2, dwGuild1, dwChallMoney);
			return;
		}
	}

	// LG( "???????", "?????????????????!GUILD1 = %d, GUILD2 = %d, ChallFlag = %d.\n", dwGuild1, dwGuild2, ( bChall ) ? 1 : 0 );
	LG("challenge consortia", "consortia challenge result disposal failed!GUILD1 = %d, GUILD2 = %d, ChallFlag = %d.\n", dwGuild1, dwGuild2, (bChall) ? 1 : 0);
}

bool CTableGuild::ChallWin(BOOL bUpdate, BYTE byLevel, DWORD dwWinGuildID, DWORD dwFailerGuildID) {
	// ??'????
	if (!begin_tran()) {
		// LG( "???????", "???????????????�????????'???????!" );
		LG("challenge consortia", "challenge consortia finish,update consortia data start affair failed!");
		return false;
	}

	// ?????�????????
	char sql[SQL_MAXLEN];
	SQLRETURN l_sqlret;
	if (!bUpdate) {
		// _snprintf_s(sql, sizeof(sql), _TRUNCATE, "update guild set challid = 0, challstart = 0, challmoney = 0, challlevel = 0 where guild_id =%d",
		//	dwFailerGuildID );
		// l_sqlret =exec_sql_direct(sql);
		// if( !DBOK( l_sqlret ) || get_affected_rows() == 0 )
		//{
		//	this->rollback();
		//	LG( "???????", "???????????????????????????????�?????????????!????????!????ID = %d.??????????%d", dwFailerGuildID, byLevel );
		//	return false;
		// }
	} else {
		string buf[5];
		char filter[80];

		BYTE byLvData = 0;
		const char* param = "challlevel";
		_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_id =%d", dwWinGuildID);
		int l_retrow = 0;
		bool l_ret = _get_row(buf, 5, param, filter, &l_retrow);
		if (l_retrow == 1) {
			byLvData = (BYTE)atoi(buf[0].c_str());
		} else {
			// LG( "???????", "???????????????????????????????????????!GUILDID = %d, WINID = %d.\n", dwFailerGuildID, dwWinGuildID );
			LG("challenge consortia", "finish challenge consortia??leizhu failed:inquire about failed consortia level info failed!GUILDID = %d, WINID = %d.\n", dwFailerGuildID, dwWinGuildID);
			return false;
		}

		if (byLvData > 0) {
			// ????????
			if (byLvData < byLevel) {
				BYTE byTemp = byLevel;
				byLevel = byLvData;
				byLvData = byTemp;
			}

			// Use stored procedure for GuildChallWinUpdateLoser
			int byLvDataInt = byLvData;
			int failerID = dwFailerGuildID;
			l_sqlret = stored_procedure("{CALL dbo.GuildChallWinUpdateLoser(?,?)}",
				"dbo", "GuildChallWinUpdateLoser", 2, &byLvDataInt, &failerID);
			if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
				this->rollback();
				// LG( "???????", "???????????????????????????????�?????????????!????????!????ID = %d.??????????%d.\n", dwFailerGuildID, byLevel );
				LG("challenge consortia", "challenge consortia over,leizhu failed:update lost consortia data operater failed! consortiaID = %d.consortia level:%d.\n", dwFailerGuildID, byLevel);
				return false;
			}
		} else {
			// Use stored procedure for GuildChallWinUpdateLoserNoLevel
			int failerID = dwFailerGuildID;
			l_sqlret = stored_procedure("{CALL dbo.GuildChallWinUpdateLoserNoLevel(?)}",
				"dbo", "GuildChallWinUpdateLoserNoLevel", 1, &failerID);
			if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
				this->rollback();
				// LG( "???????", "???????????????????????????????�?????????????!????????!????ID = %d.??????????%d.\n", dwFailerGuildID, byLevel );
				LG("challenge consortia", "challenge consortia over,leizhu failed:update lost consortia data operater failed! consortiaID = %d.consortia level:%d.\n", dwFailerGuildID, byLevel);
				return false;
			}
		}
	}

	// Use stored procedure for GuildChallWinUpdateWinner
	int byLevelInt = byLevel;
	int winnerID = dwWinGuildID;
	l_sqlret = stored_procedure("{CALL dbo.GuildChallWinUpdateWinner(?,?)}",
		"dbo", "GuildChallWinUpdateWinner", 2, &byLevelInt, &winnerID);
	if (!DBOK(l_sqlret) || get_affected_rows() == 0) {
		this->rollback();
		// LG( "???????", "??????????????????�?????�?????????????!????????!????ID = %d.??????????%d.\n", dwWinGuildID, byLevel );
		LG("challenge consortia", "challenge consortia over,update winner consortia data operator failed!inexistence consortia!consortiaID = %d.consortia level??%d.\n", dwWinGuildID, byLevel);
		return false;
	}

	if (!commit_tran()) {
		this->rollback();
		// LG( "???????", "????????????????????????!.\n" );
		LG("challenge consortia", "challenge consortia data referring failed,retry later on\n");
		return false;
	}
	return true;
}

void CTableGuild::ChallMoney(BYTE byLevel, BOOL bChall, DWORD dwGuildID, DWORD dwChallID, long long dwMoney) {
	if (bChall) {
		// LG( "?????????", "????????????ID = %d, ?????ID = %d, ?????%u, ??????%d.\n", dwGuildID, dwChallID, dwMoney, byLevel  );
		LG("challenge consortia result", "challenge failed: winner:ID = %d,loser:ID = %d, money = %lld,level:%d.\n", dwGuildID, dwChallID, dwMoney, byLevel);
		if (!ChallWin(FALSE, byLevel, dwGuildID, dwChallID)) {
			return;
		}

		if (dwChallID != 0) {
			dwMoney = (long long)(float(dwMoney * 80) / 100);
			WPacket l_wpk = GETWPACKET();
			WRITE_CMD(l_wpk, CMD_MP_GUILD_CHALL_PRIZEMONEY);
			WRITE_LONG(l_wpk, dwGuildID);
			WRITE_LONGLONG(l_wpk, dwMoney);
			SENDTOGROUP(l_wpk);
		}
	} else {
		// LG( "?????????", "?????????????ID = %d, ?????ID = %d, ?????%u, ??????%d.\n", dwChallID, dwGuildID, dwMoney, byLevel  );
		LG("challenge consortia result", "challenge succeed??winner:ID = %d,loser:ID = %d, money = %lld,level:%d.\n", dwChallID, dwGuildID, dwMoney, byLevel);
		if (!ChallWin(TRUE, byLevel, dwChallID, dwGuildID)) {
			return;
		}

		dwMoney = (long long)(float(dwMoney * 80) / 100);
		WPacket l_wpk = GETWPACKET();
		WRITE_CMD(l_wpk, CMD_MP_GUILD_CHALL_PRIZEMONEY);
		WRITE_LONG(l_wpk, dwChallID);
		WRITE_LONGLONG(l_wpk, dwMoney);
		SENDTOGROUP(l_wpk);
	}
}

bool CTableGuild::GetChallInfo(BYTE byLevel, DWORD& dwGuildID1, DWORD& dwGuildID2, long long& dwMoney) {
	string buf[3];
	char filter[80];

	DWORD dwGuildID = 0;
	DWORD dwChallID = 0;
	long long dwChallMoney = 0;
	const char* param = "guild_id, challid, challmoney";
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "challlevel =%d", byLevel);
	int l_retrow = 0;
	bool l_ret = _get_row(buf, 3, param, filter, &l_retrow);
	if (l_retrow == 1) {
		dwGuildID1 = atoi(buf[0].c_str());
		dwGuildID2 = atoi(buf[1].c_str());
		dwMoney = _atoi64(buf[2].c_str());

		return true;
	}
	return false;
}

bool CTableCha::SetGuildPermission(int cha_id, unsigned int permission, int guild_id) {
	// Use stored procedure for SetGuildPermission
	int perm = (int)permission;
	short sExec = stored_procedure("{CALL dbo.SetGuildPermission(?,?,?)}",
		"dbo", "SetGuildPermission", 3, &perm, &cha_id, &guild_id);
	if (!DBOK(sExec))
		return false;

	if (DBNODATA(sExec))
		return false;

	return true;
}

bool CTableCha::SetChaAddr(DWORD cha_id, Long addr) {
	// Use stored procedure for SetChaAddr
	int mem_addr = -1;  // Original code always sets to -1
	short sExec = stored_procedure("{CALL dbo.SetChaAddr(?,?)}",
		"dbo", "SetChaAddr", 2, &mem_addr, &cha_id);
	if (!DBOK(sExec))
		return false;

	if (DBNODATA(sExec))
		return false;

	return true;
}

//===============CTableGuild End===========================================
//	2008-7-28	yyy	add	function	begin!

bool CTableItem::LockItem(SItemGrid* sig, int iChaId) {
	char param[80];
	_snprintf_s(param, sizeof(param), _TRUNCATE, "TOP 1 id");

	char filter[80];
	_snprintf_s(filter, sizeof(filter), _TRUNCATE, "ORDER BY id DESC");

	std::string buf[1];

	int r1 = 0;
	int r = _get_rowOderby(buf, 1, param, filter, &r1);
	DWORD dwDropertyID;

	if (DBOK(r) && r1 > 0) {
		dwDropertyID = atoi(buf[0].c_str()) + 1;
	} else {
		dwDropertyID = 1;
	}
	if (sig && !sig->dwDBID) {
		sig->dwDBID = dwDropertyID;
		string s;
		int lnCheckSum = 0;

		if (SItemGrid2String(s, lnCheckSum, sig, 0)) {
			// Use stored procedure for PropertyInsert
			int propID = dwDropertyID;
			int chaID = iChaId;
			int checkSum = lnCheckSum;
			char contextStr[4096];
			strncpy_s(contextStr, sizeof(contextStr), s.c_str(), _TRUNCATE);
			short sExec = stored_procedure("{CALL dbo.PropertyInsert(?,?,?,?)}",
				"dbo", "PropertyInsert", 4, &propID, &chaID, contextStr, &checkSum);
			return DBOK(sExec);
		};
	};
	return false;
};

bool CTableItem::UnlockItem(SItemGrid* sig, int iChaId) {
	if (sig) {
		// Use stored procedure for PropertyDelete
		int propID = sig->dwDBID;
		int chaID = iChaId;
		short sExec = stored_procedure("{CALL dbo.PropertyDelete(?,?)}",
			"dbo", "PropertyDelete", 2, &propID, &chaID);
		sig->dwDBID = 0;
		return DBOK(sExec);
	}
	return false;
}

// ============================================
// Offline Stall System Database Functions
// ============================================

#include "OfflineStall.h"
#include "Config.h"

// Helper function to decode hex string to binary
static size_t HexDecode(const char* hexStr, BYTE* outBuf, size_t outBufSize) {
	if (!hexStr || !outBuf || outBufSize == 0) return 0;
	
	const char* p = hexStr;
	size_t outLen = 0;
	
	// Skip optional "0x" prefix
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		p += 2;
	}
	
	while (*p && *(p+1) && outLen < outBufSize) {
		char high = *p++;
		char low = *p++;
		
		unsigned char val = 0;
		
		// Decode high nibble
		if (high >= '0' && high <= '9') val = (high - '0') << 4;
		else if (high >= 'A' && high <= 'F') val = (high - 'A' + 10) << 4;
		else if (high >= 'a' && high <= 'f') val = (high - 'a' + 10) << 4;
		else return outLen; // Invalid char, stop
		
		// Decode low nibble
		if (low >= '0' && low <= '9') val |= (low - '0');
		else if (low >= 'A' && low <= 'F') val |= (low - 'A' + 10);
		else if (low >= 'a' && low <= 'f') val |= (low - 'a' + 10);
		else return outLen; // Invalid char, stop
		
		outBuf[outLen++] = val;
	}
	
	return outLen;
}

bool CGameDB::LoadOfflineStalls(const char* szMapName, COfflineStallMgr* pMgr) {
	if (!pMgr || !_tab_cha) return false;
	
	LG("offline_stall", "LoadOfflineStalls called for map: %s\n", szMapName ? szMapName : "ALL");
	
	T_B
	
	// Build SQL query to call stored procedure
	char sql[SQL_MAXLEN];
	if (szMapName) {
		_snprintf_s(sql, sizeof(sql), _TRUNCATE, "EXEC dbo.OfflineStall_LoadAll @map_name='%s'", szMapName);
	} else {
		// Don't pass @map_name parameter at all to get default NULL behavior
		strcpy_s(sql, sizeof(sql), "EXEC dbo.OfflineStall_LoadAll");
	}
	
	// Execute and get all results
	vector<vector<string>> data;
	LG("offline_stall", "Executing SQL: %s\n", sql);
	if (!_tab_cha->getalldata(sql, data)) {
		LG("offline_stall", "Failed to query offline stalls: SQL error\n");
		return false;
	}
	
	LG("offline_stall", "SQL returned %zu rows\n", data.size());
	
	// Process each row
	int rowCount = 0;
	for (size_t rowIdx = 0; rowIdx < data.size(); rowIdx++) {
		vector<string> row = data[rowIdx]; // Copy, not reference
		
		// Verify we have enough columns
		if (row.size() < 17) {
			LG("offline_stall", "LoadOfflineStalls: row %zu has only %zu columns, expected 17\n", rowIdx, row.size());
			continue;
		}
		
		SOfflineStallInfo* pInfo = new SOfflineStallInfo();
		if (!pInfo) continue;
		
		// Parse columns in order from stored procedure:
		// stall_id, cha_id, cha_name, act_id, stall_title, look_face, look_hair, job,
		// map_name, pos_x, pos_y, created_time, expire_time, item_count, item_data, kitbag_snapshot, pending_gold
		pInfo->dwStallID = (DWORD)atoi(row[0].c_str());
		pInfo->dwChaID = (DWORD)atoi(row[1].c_str());
		strncpy_s(pInfo->szChaName, sizeof(pInfo->szChaName), row[2].c_str(), _TRUNCATE);
		pInfo->dwActID = (DWORD)atoi(row[3].c_str());
		strncpy_s(pInfo->szStallTitle, sizeof(pInfo->szStallTitle), row[4].c_str(), _TRUNCATE);
		pInfo->sLookFace = (short)atoi(row[5].c_str());
		pInfo->sLookHair = (short)atoi(row[6].c_str());
		pInfo->sJob = (short)atoi(row[7].c_str());
		pInfo->byStallLevel = 1; // Default, will be overridden from DB if available
		strncpy_s(pInfo->szMapName, sizeof(pInfo->szMapName), row[8].c_str(), _TRUNCATE);
		pInfo->nPosX = atoi(row[9].c_str());
		pInfo->nPosY = atoi(row[10].c_str());
		// Skip timestamps row[11], row[12] - use defaults
		pInfo->tCreated = time(nullptr);
		pInfo->tExpire = pInfo->tCreated + (24 * 3600);
		pInfo->byItemCount = (BYTE)atoi(row[13].c_str());
		
		// Parse item data - row[14] is binary varbinary column (returned as hex string)
		string itemDataStr = row[14];
		LG("offline_stall", "Item data string length: %zu, first chars: %.20s\n", itemDataStr.size(), itemDataStr.c_str());
		if (itemDataStr.size() > 0) {
			// Decode hex string to binary - buffer needs to handle full SItemGrid per item.
			// CRITICAL: Zero-init the buffer because ODBC's SQL_C_CHAR binding strips trailing
			// zero bytes from varbinary during hex conversion. For example, a 200-byte struct
			// ending in 7 zero bytes will decode to only 193 bytes of hex. The zero-init
			// ensures the stripped trailing zeros are implicitly present in the buffer.
			BYTE itemDataBuf[8192];
			memset(itemDataBuf, 0, sizeof(itemDataBuf));
			size_t decodedSize = HexDecode(itemDataStr.c_str(), itemDataBuf, sizeof(itemDataBuf));
			LG("offline_stall", "Decoded %zu bytes from hex string (expected per item: %zu)\n", 
			   decodedSize, sizeof(SOfflineStallItem));
			
			// Basic sanity check - need at least some data (but allow less than sizeof(SOfflineStallItem)
			// because ODBC strips trailing zeros from varbinary when converting to SQL_C_CHAR hex).
			if (decodedSize < 10) {
				LG("offline_stall", "WARNING: Item data too small! Decoded %zu bytes. Skipping stall %u.\n",
				   decodedSize, pInfo->dwStallID);
				delete pInfo;
				continue;
			}
			
			// Use DeserializeStallItems to properly read both items AND sold items tracking data.
			// Pass the FULL BUFFER SIZE (not decodedSize) because the zero-initialized buffer
			// already contains the implicit trailing zeros that ODBC stripped.
			size_t fullDataSize = sizeof(SOfflineStallItem) * pInfo->byItemCount + 1 + sizeof(SSoldItemInfo) * OFFLINE_STALL_MAX_ITEMS;
			if (fullDataSize > sizeof(itemDataBuf)) fullDataSize = sizeof(itemDataBuf);
			if (!COfflineStallMgr::DeserializeStallItems(itemDataBuf, fullDataSize, pInfo->byItemCount, pInfo)) {
				LG("offline_stall", "WARNING: Failed to deserialize item data for stall %u. Skipping.\n", pInfo->dwStallID);
				delete pInfo;
				continue;
			}
			
			// Validate item IDs
			bool bValid = true;
			for (BYTE itemIdx = 0; itemIdx < pInfo->byItemCount; itemIdx++) {
				if (pInfo->items[itemIdx].itemGrid.sID <= 0 || pInfo->items[itemIdx].itemGrid.sID > 10000) {
					LG("offline_stall", "WARNING: Invalid item ID %d at index %d in stall %u, skipping entire stall\n",
					   pInfo->items[itemIdx].itemGrid.sID, itemIdx, pInfo->dwStallID);
					delete pInfo;
					pInfo = nullptr;
					bValid = false;
					break;
				}
				LG("offline_stall", "Loaded item %d: grid=%d, id=%d, count=%d, price=%lld, forge=%d\n",
					itemIdx, pInfo->items[itemIdx].byGrid, pInfo->items[itemIdx].itemGrid.sID,
					pInfo->items[itemIdx].byCount, pInfo->items[itemIdx].llMoney,
					pInfo->items[itemIdx].itemGrid.chForgeLv);
			}
			if (!bValid) continue;
			
			LG("offline_stall", "Loaded stall %u: %d items, %d sold records\n",
			   pInfo->dwStallID, pInfo->byItemCount, pInfo->bySoldCount);
		}
		
		// Skip kitbag_snapshot row[15] for now
		
		// pending_gold (column 17, row[16])
		pInfo->llPendingGold = _atoi64(row[16].c_str());
		
		// stall_level from row[17]
		if (row.size() > 17 && !row[17].empty()) {
			pInfo->byStallLevel = (BYTE)atoi(row[17].c_str());
			if (pInfo->byStallLevel == 0) pInfo->byStallLevel = 1;
		}
		
		// Load equipment look data from row[18] (equip_look_data, varbinary)
		memset(pInfo->equipLook, 0, sizeof(pInfo->equipLook));
		if (row.size() > 18 && !row[18].empty()) {
			string equipLookStr = row[18];
			if (equipLookStr.size() > 0) {
				// Zero-init: ODBC may strip trailing zero bytes from varbinary→text conversion
				BYTE equipLookBuf[8192];
				memset(equipLookBuf, 0, sizeof(equipLookBuf));
				size_t decodedSize = HexDecode(equipLookStr.c_str(), equipLookBuf, sizeof(equipLookBuf));
				if (decodedSize > 0 && decodedSize <= sizeof(pInfo->equipLook)) {
					memcpy(pInfo->equipLook, equipLookBuf, sizeof(pInfo->equipLook));
					LG("offline_stall", "Loaded equipment look data (%zu bytes decoded, %zu total with zero-fill) for stall %u\n",
					   decodedSize, sizeof(pInfo->equipLook), pInfo->dwStallID);
				} else {
					LG("offline_stall", "Equipment look data size issue for stall %u (got %zu, expected %zu) - NPC will appear without gear\n",
					   pInfo->dwStallID, decodedSize, sizeof(pInfo->equipLook));
				}
			}
		}
		
		// angle from row[19]
		pInfo->sAngle = 0;
		if (row.size() > 19 && !row[19].empty()) {
			pInfo->sAngle = (short)atoi(row[19].c_str());
		}
		
		// Initialize runtime state
		pInfo->pVirtualNPC = nullptr;
		pInfo->dwWorldID = 0;
		pInfo->bActive = false;
		
		// Add to manager
		LG("offline_stall", "Adding stall: ID=%u, Name=%s, Map=%s, Items=%d\n",
			pInfo->dwStallID, pInfo->szChaName, pInfo->szMapName, pInfo->byItemCount);
		if (pMgr->AddLoadedStall(pInfo)) {
			rowCount++;
		} else {
			LG("offline_stall", "FAILED to add stall ID %u\n", pInfo->dwStallID);
			delete pInfo;
		}
	}
	
	LG("offline_stall", "Loaded %d offline stalls from database\n", rowCount);
	
	T_E
	return true;
}

DWORD CGameDB::CreateOfflineStall(DWORD dwChaID, const char* szChaName, DWORD dwActID,
                                  const char* szStallTitle, short sLookFace, short sLookHair,
                                  short sJob, const char* szMapName, int nPosX, int nPosY,
                                  short sAngle,
                                  DWORD dwDurationHours, BYTE byItemCount,
                                  const BYTE* pItemData, DWORD dwItemDataSize,
                                  const BYTE* pKitbagSnapshot, DWORD dwKitbagSnapshotSize,
                                  const BYTE* pEquipLookData, DWORD dwEquipLookDataSize,
                                  BYTE byStallLevel) {
	if (!_tab_cha) return 0;
	
	static DWORD s_dwNextStallID = 1;
	DWORD dwStallID = 0;
	
	T_B
	
	// Log the stall creation attempt
	LG("offline_stall", "CreateOfflineStall: cha_id=%u, name=%s, stall=%s, map=%s, pos=(%d,%d), items=%d\n",
	   dwChaID, szChaName, szStallTitle, szMapName, nPosX, nPosY, byItemCount);
	
	// Convert binary item data to hex string for stored procedure
	// Buffer must hold "0x" + 2 hex chars per byte. Max item data is ~5500 bytes → ~11000 hex chars.
	static char hexItemData[16384];
	{
		char* p = hexItemData;
		*p++ = '0'; *p++ = 'x';
		for (DWORD i = 0; i < dwItemDataSize; i++) {
			sprintf(p, "%02X", pItemData[i]);
			p += 2;
		}
		*p = '\0';
		LG("offline_stall", "CreateOfflineStall: hexItemData length=%zu (dataSize=%u)\n", strlen(hexItemData), dwItemDataSize);
	}
	
	static char hexKitbagSnapshot[1024];
	{
		char* p = hexKitbagSnapshot;
		*p++ = '0'; *p++ = 'x';
		for (DWORD i = 0; i < dwKitbagSnapshotSize; i++) {
			sprintf(p, "%02X", pKitbagSnapshot[i]);
			p += 2;
		}
		*p = '\0';
	}
	
	// Convert equipment look data to hex string for stored procedure
	// SOfflineStallEquipLook * 34 slots = significant data, need large buffer
	static char hexEquipLookData[8192];
	{
		char* p = hexEquipLookData;
		*p++ = '0'; *p++ = 'x';
		if (pEquipLookData && dwEquipLookDataSize > 0) {
			for (DWORD i = 0; i < dwEquipLookDataSize; i++) {
				sprintf(p, "%02X", pEquipLookData[i]);
				p += 2;
			}
		}
		*p = '\0';
		LG("offline_stall", "CreateOfflineStall: hexEquipLookData length=%zu (dataSize=%u)\n", strlen(hexEquipLookData), dwEquipLookDataSize);
	}
	
	// Escape single quotes in string params to prevent SQL issues
	auto escapeSql = [](const char* src, char* dst, size_t dstSize) {
		size_t j = 0;
		for (size_t i = 0; src[i] && j < dstSize - 2; i++) {
			if (src[i] == '\'') { dst[j++] = '\''; } // double the quote
			dst[j++] = src[i];
		}
		dst[j] = '\0';
	};
	char safeChaName[102], safeStallTitle[130], safeMapName[130];
	escapeSql(szChaName, safeChaName, sizeof(safeChaName));
	escapeSql(szStallTitle, safeStallTitle, sizeof(safeStallTitle));
	escapeSql(szMapName, safeMapName, sizeof(safeMapName));
	
	// Build direct SQL string with hex literals embedded inline.
	// This avoids ODBC parameter binding issues with VARBINARY(MAX) columns
	// (the parameterized stored_procedure call fails with HY004 "Invalid SQL data type").
	// SQL Server natively understands 0xABCD... as varbinary literals.
	static char sqlBuf[32768];
	_snprintf_s(sqlBuf, sizeof(sqlBuf), _TRUNCATE,
		"EXEC dbo.OfflineStall_Create "
		"@cha_id=%d, @cha_name='%s', @act_id=%d, @stall_title=N'%s', "
		"@look_face=%d, @look_hair=%d, @job=%d, "
		"@map_name='%s', @pos_x=%d, @pos_y=%d, @angle=%d, "
		"@duration_hours=%d, @item_count=%d, "
		"@item_data=%s, @kitbag_snapshot=%s, @equip_look_data=%s, @stall_level=%d",
		(int)dwChaID, safeChaName, (int)dwActID, safeStallTitle,
		(int)sLookFace, (int)sLookHair, (int)sJob,
		safeMapName, nPosX, nPosY, (int)sAngle,
		(int)dwDurationHours, (int)byItemCount,
		hexItemData, hexKitbagSnapshot, hexEquipLookData, (int)byStallLevel);
	
	LG("offline_stall", "CreateOfflineStall SQL length: %zu\n", strlen(sqlBuf));
	
	// Use getalldata instead of exec_sql_direct so we can read the stall_id
	// returned by the stored procedure's SELECT statement.
	// The SP returns: SELECT @new_id AS stall_id (for INSERT)
	//            or:  SELECT [stall_id] FROM offline_stalls WHERE cha_id = @cha_id (for UPDATE)
	vector<vector<string>> resultData;
	if (!_tab_cha->getalldata(sqlBuf, resultData, 30)) {
		LG("offline_stall", "Failed to create offline stall: SQL error\n");
		return 0;
	}
	
	// Read the actual stall_id from the result set
	if (resultData.size() > 0 && resultData[0].size() > 0) {
		dwStallID = (DWORD)atoi(resultData[0][0].c_str());
		LG("offline_stall", "Created offline stall with DB stall_id: %u\n", dwStallID);
	} else {
		LG("offline_stall", "WARNING: SP succeeded but returned no stall_id! Using fallback.\n");
		dwStallID = s_dwNextStallID++;
		LG("offline_stall", "Fallback stall_id: %u\n", dwStallID);
	}
	
	T_E
	return dwStallID;
}

bool CGameDB::DeleteOfflineStall(DWORD dwStallID) {
	if (!_tab_cha) return false;
	
	bool bResult = false;
	
	T_B
	
	int stallId = (int)dwStallID;
	short sExec = _tab_cha->stored_procedure("{CALL dbo.OfflineStall_Delete(?)}",
		"dbo", "OfflineStall_Delete", 1, &stallId);
	
	bResult = DBOK(sExec);
	LG("offline_stall", "DeleteOfflineStall: stall_id=%u, result=%d\n", dwStallID, bResult);
	
	T_E
	return bResult;
}

__int64 CGameDB::DeleteOfflineStallByCharId(DWORD dwChaID) {
	if (!_tab_cha) return 0;
	
	__int64 llPendingGold = 0;
	
	T_B
	
	int chaId = (int)dwChaID;
	
	// Use stored procedure - it returns pending gold
	string buf[1];
	int affectRows = 0;
	
	bool ret = _tab_cha->_get_row_stored_procedure(buf, 1, 
		"{CALL dbo.OfflineStall_DeleteByCharId(?)}",
		"dbo", "OfflineStall_DeleteByCharId", &affectRows, 1, &chaId);
	
	if (ret && affectRows > 0) {
		llPendingGold = _atoi64(buf[0].c_str());
	}
	
	LG("offline_stall", "DeleteOfflineStallByCharId: cha_id=%u, pending_gold=%lld\n", dwChaID, llPendingGold);
	
	T_E
	return llPendingGold;
}

bool CGameDB::HasOfflineStall(DWORD dwChaID) {
	if (!_tab_cha) return false;
	
	bool hasStall = false;
	
	T_B
	
	int chaId = (int)dwChaID;
	string buf[1];
	int affectRows = 0;
	
	bool ret = _tab_cha->_get_row_stored_procedure(buf, 1,
		"{CALL dbo.OfflineStall_GetByCharId(?)}",
		"dbo", "OfflineStall_GetByCharId", &affectRows, 1, &chaId);
	
	hasStall = (ret && affectRows > 0);
	
	LG("offline_stall", "HasOfflineStall: cha_id=%u, has_stall=%d\n", dwChaID, hasStall);
	
	T_E
	return hasStall;
}

bool CGameDB::UpdateOfflineStallItems(DWORD dwStallID, BYTE byItemCount,
                                      const BYTE* pItemData, DWORD dwItemDataSize,
                                      __int64 llPendingGold) {
	if (!_tab_cha) return false;
	
	bool bResult = false;
	
	T_B
	
	// Convert binary to hex string for stored procedure
	// Buffer must hold "0x" + 2 hex chars per byte. Max data is ~5500 bytes → ~11000 hex chars.
	static char hexItemData[16384];
	{
		char* p = hexItemData;
		*p++ = '0'; *p++ = 'x';
		for (DWORD i = 0; i < dwItemDataSize; i++) {
			sprintf(p, "%02X", pItemData[i]);
			p += 2;
		}
		*p = '\0';
		LG("offline_stall", "UpdateOfflineStallItems: hexItemData length=%zu (dataSize=%u)\n", strlen(hexItemData), dwItemDataSize);
	}
	
	// Use direct SQL with hex literals embedded inline (same approach as CreateOfflineStall).
	// The parameterized stored_procedure call has ODBC binding issues with VARBINARY(MAX) params.
	static char sqlBuf[32768];
	_snprintf_s(sqlBuf, sizeof(sqlBuf), _TRUNCATE,
		"EXEC dbo.OfflineStall_UpdateItems @stall_id=%d, @item_count=%d, @item_data=%s, @gold_earned=%lld",
		(int)dwStallID, (int)byItemCount, hexItemData, llPendingGold);
	
	short sExec = _tab_cha->exec_sql_direct(sqlBuf, 30);
	bResult = DBOK(sExec);
	
	LG("offline_stall", "UpdateOfflineStallItems: stall_id=%u, items=%d, gold=%lld, hexLen=%zu, sqlLen=%zu, result=%d\n",
	   dwStallID, byItemCount, llPendingGold, strlen(hexItemData), strlen(sqlBuf), bResult);
	
	T_E
	return bResult;
}

bool CGameDB::ExtendOfflineStallTime(DWORD dwStallID, int nExtendHours) {
	if (!_tab_cha) return false;
	
	bool bResult = false;
	
	T_B
	
	int stallId = (int)dwStallID;
	int extendHours = nExtendHours;
	short sExec = _tab_cha->stored_procedure("{CALL dbo.OfflineStall_ExtendTime(?,?)}",
		"dbo", "OfflineStall_ExtendTime", 2, &stallId, &extendHours);
	
	bResult = DBOK(sExec);
	LG("offline_stall", "ExtendOfflineStallTime: stall_id=%u, hours=%d, result=%d\n",
	   dwStallID, nExtendHours, bResult);
	
	T_E
	return bResult;
}

bool CGameDB::GetOfflineStallByCharId(DWORD dwChaID, SOfflineStallInfo* pOutInfo) {
	if (!pOutInfo || !_tab_cha) return false;
	
	bool bResult = false;
	
	T_B
	
	int chaId = (int)dwChaID;
	string buf[13];  // SP returns 13 columns
	int affectRows = 0;
	
	bool ret = _tab_cha->_get_row_stored_procedure(buf, 13,
		"{CALL dbo.OfflineStall_GetByCharId(?)}",
		"dbo", "OfflineStall_GetByCharId", &affectRows, 1, &chaId);
	
	if (ret && affectRows > 0) {
		// Parse results - SP returns 13 columns:
		// 0: stall_id, 1: cha_name, 2: stall_title, 3: map_name,
		// 4: pos_x, 5: pos_y, 6: created_time, 7: expire_time,
		// 8: item_count, 9: item_data, 10: kitbag_snapshot,
		// 11: pending_gold, 12: is_active
		pOutInfo->dwStallID = (DWORD)Str2Int(buf[0]);
		strncpy_s(pOutInfo->szChaName, sizeof(pOutInfo->szChaName), buf[1].c_str(), _TRUNCATE);
		strncpy_s(pOutInfo->szStallTitle, sizeof(pOutInfo->szStallTitle), buf[2].c_str(), _TRUNCATE);
		strncpy_s(pOutInfo->szMapName, sizeof(pOutInfo->szMapName), buf[3].c_str(), _TRUNCATE);
		pOutInfo->nPosX = Str2Int(buf[4]);
		pOutInfo->nPosY = Str2Int(buf[5]);
		// Skip buf[6] (created_time) and buf[7] (expire_time) - not needed at runtime
		pOutInfo->byItemCount = (BYTE)Str2Int(buf[8]);
		// Skip buf[9] (item_data) and buf[10] (kitbag_snapshot) - binary, not used here
		pOutInfo->llPendingGold = _atoi64(buf[11].c_str());
		// Skip buf[12] (is_active)
		
		LG("offline_stall", "GetOfflineStallByCharId: cha_id=%u, found stall_id=%u\n", dwChaID, pOutInfo->dwStallID);
		bResult = true;
	}
	
	T_E
	return bResult;
}

bool CGameDB::GetPlayerLoginInfo(const char* actName, std::string& outIP, std::string& outMAC) {
	if (!_tab_cha || !actName) return false;

	bool bResult = false;

	T_B

	string buf[2];
	int affectRows = 0;

	bool ret = _tab_cha->_get_row_stored_procedure(buf, 2,
		"{CALL dbo.GetPlayerLoginInfo(?)}",
		"dbo", "GetPlayerLoginInfo", &affectRows, 1, actName);

	if (ret && affectRows > 0) {
		outIP = buf[0];
		outMAC = buf[1];
		bResult = true;
	}

	T_E
	return bResult;
}

CGameDB game_db;

