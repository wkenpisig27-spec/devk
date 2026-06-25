#pragma once

class CCharacter;
struct SItemGrid;

void ItemAuditLog(CCharacter* cha, const char* action, SItemGrid* grid, int qtyDelta,
                  CCharacter* counterparty = nullptr, __int64 goldDelta = 0, const char* detail = nullptr);
