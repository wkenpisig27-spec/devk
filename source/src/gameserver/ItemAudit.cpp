#include "stdafx.h"
#include "ItemAudit.h"
#include "Character.h"
#include "GameDB.h"
#include "Kitbag.h"

void ItemAuditLog(CCharacter* cha, const char* action, SItemGrid* grid, int qtyDelta,
                  CCharacter* counterparty, __int64 goldDelta, const char* detail) {
	if (!cha || !action || !grid)
		return;

	const int chaId = cha->GetPlayer() ? (int)cha->GetPlayer()->GetDBChaId() : 0;
	const int counterpartyId = (counterparty && counterparty->GetPlayer())
		? (int)counterparty->GetPlayer()->GetDBChaId()
		: -1;
	const int itemDbId = (int)grid->dwDBID;
	const int itemId = (int)grid->sID;
	const char* safeDetail = detail ? detail : "";

	char sql[1024];
	_snprintf_s(sql, sizeof(sql), _TRUNCATE,
		"{CALL dbo.ItemAuditInsert(%d,'%s',%d,%d,%d,%d,%lld,N'%s')}",
		chaId, action, itemDbId, itemId, qtyDelta, counterpartyId, goldDelta, safeDetail);

	if (game_db.ExecAuditSql(sql))
		return;

	LG("Security", "ItemAudit fallback cha=%d action=%s item=%d dbid=%d qty=%d gold=%lld cp=%d detail=%s\n",
	   chaId, action, itemId, itemDbId, qtyDelta, goldDelta, counterpartyId, safeDetail);
}
