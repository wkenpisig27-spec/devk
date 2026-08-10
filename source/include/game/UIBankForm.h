//--------------------------------------------------------------
// 名称:用户界面银行管理类
// 设计思想:管理界面银行
//--------------------------------------------------------------

#ifndef UI_BANK_FORM_H
#define UI_BANK_FORM_H

#include "UIGlobalVar.h"
#include "NetProtocol.h"

namespace GUI {
class CForm;
class CGoodsGrid;
class CLabel;

struct stNumBox;

class CBankMgr : public CUIInterface {
public:
	void ShowBank();
	void CloseBankUi(bool hideInventory = true);
	void RefreshBankUi();

	CGoodsGrid* GetBankGoodsGrid() { return grdBank; }
	CForm* GetBankForm() { return frmBank; } // legacy data host (never shown)
	bool IsBankOpen() const;

	bool PushToBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem);
	bool PopFromBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem);
	bool BankToBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem);

	// Index-based transfer (no CDrag::GetParent); pile prompt unchanged.
	bool MoveBagToBank(int bagIndex, int bankSlot = -1);
	bool MoveBankToBag(int bankIndex, int bagSlot = -1);
	bool MoveBankItem(int srcBankIndex, int dstBankIndex);

protected:
	virtual bool Init();
	virtual void CloseForm();

private:
	static void _MoveItemsEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey);
	static void _MoveAItemEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey);
	static void _evtBankToBank(CGuiData* pSender, int nFirst, int nSecond, bool& isSwap);
	static void _evtOnClose(CForm* pForm, bool& IsClose);
	static void _evtBankUseCommand(CCommandObj* pSender, bool& isUse);

private:
	stNumBox* m_pkNumberBox;
	stNetBank m_kNetBank;

	// Data host only — player chrome is CRmlUiBankForm.
	CForm* frmBank;
	CGoodsGrid* grdBank;
	CLabel* labCharName;

}; // end of class CBankMgr

} // end of namespace GUI

#endif // end of UI_BANK_FORM_H
