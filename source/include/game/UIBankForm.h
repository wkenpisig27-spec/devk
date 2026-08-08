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
	void ShowBank();								   // 显示道具栏物品
	CGoodsGrid* GetBankGoodsGrid() { return grdBank; } // 获取道具栏格子
	CForm* GetBankForm() { return frmBank; }
	bool IsBankOpen() const { return frmBank && frmBank->GetIsShow(); }

	bool PushToBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem);	// 将物品放入银行
	bool PopFromBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem); // 从银行中拖出物品
	bool BankToBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem);	// 将物品从一个银行移到另一个银行

	// Index-based transfer (no CDrag::GetParent); pile prompt unchanged.
	bool MoveBagToBank(int bagIndex, int bankSlot = -1);
	bool MoveBankToBag(int bankIndex, int bagSlot = -1);

protected:
	virtual bool Init();	  // 用户界面银行信息初始化
	virtual void CloseForm(); // 关闭表单

private:
	static void _MoveItemsEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey); // 移动多个物品
	static void _MoveAItemEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey); // 移动单个物品
	static void _evtBankToBank(CGuiData* pSender, int nFirst, int nSecond, bool& isSwap);	 // 用于用户银行表单中道具互换
	static void _evtOnClose(CForm* pForm, bool& IsClose);									 // 关闭银行表单
	static void _evtBankUseCommand(CCommandObj* pSender, bool& isUse);						 // dblclick withdraw → bag

private:
	stNumBox* m_pkNumberBox; // 询问个数
	stNetBank m_kNetBank;

	// 界面
	CForm* frmBank;		 // 银行表单
	CGoodsGrid* grdBank; // 格子表单
	CLabel* labCharName;

}; // end of class CBankMgr

} // end of namespace GUI

#endif // end of UI_BANK_FORM_H
