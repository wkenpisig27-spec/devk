//------------------------------------------------------------------------
//	2005.4.27	Arcol	create this file
//------------------------------------------------------------------------

#include "stdafx.h"
#include "uiguildapply.h"
#include "netguild.h"
#include "UIFormMgr.h"
#include "UITextButton.h"
#include "UIBoxForm.h"
#include "UIEdit.h"
#include "GuildData.h"
#include "commfunc.h"
#include "rmlui/RmlUiManager.h"
#include "rmlui/RmlUiGuildApplyForm.h"

using namespace std;

namespace {

bool UseRmlGuildApply() {
	return CRmlUiManager::Instance().IsReady() && CRmlUiGuildApplyForm::Instance().LoadOk();
}

} // namespace

CForm* CUIGuildApply::m_pGuildNameInputForm = nullptr;
CEdit* CUIGuildApply::m_pGuildNameEdit = nullptr;
CEdit* CUIGuildApply::m_pGuildPasswordEdit = nullptr;
CEdit* CUIGuildApply::m_pGuildConfirmEdit = nullptr;

CUIGuildApply::CUIGuildApply(void) {
}

CUIGuildApply::~CUIGuildApply(void) {
}

bool CUIGuildApply::Init() {
	FORM_LOADING_CHECK(m_pGuildNameInputForm, "npc.clu", "frmName");
	FORM_CONTROL_LOADING_CHECK(m_pGuildNameEdit, m_pGuildNameInputForm, CEdit, "npc.clu", "edtName");
	FORM_CONTROL_LOADING_CHECK(m_pGuildPasswordEdit, m_pGuildNameInputForm, CEdit, "npc.clu", "edtPCode");
	FORM_CONTROL_LOADING_CHECK(m_pGuildConfirmEdit, m_pGuildNameInputForm, CEdit, "npc.clu", "edtPCode2");
	m_pGuildPasswordEdit->SetIsPassWord(true);
	m_pGuildConfirmEdit->SetIsPassWord(true);
	m_pGuildNameInputForm->evtEntrustMouseEvent = OnConfirm;
	m_pGuildNameInputForm->evtEscClose = OnEscClose;

	return true;
}

void CUIGuildApply::ShowForm() {
	if (m_pGuildPasswordEdit)
		m_pGuildPasswordEdit->SetCaption("");
	if (m_pGuildConfirmEdit)
		m_pGuildConfirmEdit->SetCaption("");
	if (m_pGuildNameEdit) {
		m_pGuildNameEdit->SetCaption("");
		m_pGuildNameEdit->SetIsEnabled(true);
	}

	if (UseRmlGuildApply()) {
		if (m_pGuildNameInputForm)
			m_pGuildNameInputForm->SetIsShow(false);
		CRmlUiGuildApplyForm::Instance().Show(true);
		return;
	}

	m_pGuildNameInputForm->ShowModal();
	m_pGuildNameEdit->SetActive(m_pGuildNameEdit);
}

// ?????????????GBK?????????????????????????
// name???????????????(??????)???true;
// len??????name???=strlen(name),?????NULL???
inline bool IsValidGuildName(const char* name, unsigned short len, bool bEng) {
	const unsigned char* l_name = reinterpret_cast<const unsigned char*>(name);
	bool l_ishan = false;
	for (unsigned short i = 0; i < len; i++) {
		if (!l_name[i]) {
			return false;
		} else if (l_ishan) {
			if (l_name[i - 1] == 0xA1 && l_name[i] == 0xA1) // ??????
			{
				return false;
			}
			if (l_name[i] > 0x3F && l_name[i] < 0xFF && l_name[i] != 0x7F) {
				l_ishan = false;
			} else {
				return false;
			}
		} else if (l_name[i] > 0x80 && l_name[i] < 0xFF) {
			l_ishan = true;
		} else if ((l_name[i] >= 'A' && l_name[i] <= 'Z') || (l_name[i] >= 'a' && l_name[i] <= 'z') || (l_name[i] >= '0' && l_name[i] <= '9')) {

		} else if (bEng && l_name[i] == ' ') {
		} else {
			return false;
		}
	}
	return !l_ishan;
}

void CUIGuildApply::OnEscClose(CForm* pForm) {
	if (m_pGuildNameInputForm == pForm)
		CancelApply();
}

void CUIGuildApply::CancelApply() {
	CM_GUILD_PUTNAME(false, "", "");
	if (m_pGuildNameInputForm)
		m_pGuildNameInputForm->SetIsShow(false);
	if (UseRmlGuildApply())
		CRmlUiGuildApplyForm::Instance().Hide();
}

void CUIGuildApply::TrySubmit(const char* nameRaw, const char* passwordRaw, const char* confirmRaw) {
	const char* nameIn = nameRaw ? nameRaw : "";
	const char* passIn = passwordRaw ? passwordRaw : "";
	const char* confirmIn = confirmRaw ? confirmRaw : "";

	auto hideRmlForMsgBox = []() {
		if (UseRmlGuildApply())
			CRmlUiGuildApplyForm::Instance().Hide();
	};

	if (strlen(nameIn) > 0) {
		string name = nameIn;
		bool bEnglishName = true;
		if (!CTextFilter::IsLegalText(CTextFilter::DIALOG_TABLE, name)) {
			if (m_pGuildNameEdit)
				m_pGuildNameEdit->SetCaption("");
			if (UseRmlGuildApply())
				CRmlUiGuildApplyForm::Instance().ClearName();
			hideRmlForMsgBox();
			CBoxMgr::ShowMsgBox(OnShowForm, "??????!", true);
			return;
		}

		int nLen = (int)name.size();
		for (int i = 0; i < nLen; ++i) {
			if (name[i] & 0x80) {
				bEnglishName = false;
				break;
			}
		}

		if (!IsValidGuildName(name.c_str(), (unsigned short)name.length(), bEnglishName)) {
			hideRmlForMsgBox();
			CBoxMgr::ShowMsgBox(OnShowForm, RES_STRING(CL_LANGUAGE_MATCH_51), true);
		} else {
			string strPass = passIn;
			if (strPass.length() > 0) {
				if (strPass == confirmIn) {
					CM_GUILD_PUTNAME(true, nameIn, passIn);
					if (m_pGuildNameInputForm)
						m_pGuildNameInputForm->SetIsShow(false);
					if (UseRmlGuildApply())
						CRmlUiGuildApplyForm::Instance().Hide();
				} else {
					if (m_pGuildPasswordEdit)
						m_pGuildPasswordEdit->SetCaption("");
					if (m_pGuildConfirmEdit)
						m_pGuildConfirmEdit->SetCaption("");
					if (UseRmlGuildApply())
						CRmlUiGuildApplyForm::Instance().ClearPasswords();
					hideRmlForMsgBox();
					CBoxMgr::ShowMsgBox(OnShowForm, RES_STRING(CL_LANGUAGE_MATCH_580), true);
				}
			} else {
				hideRmlForMsgBox();
				CBoxMgr::ShowMsgBox(OnShowForm, RES_STRING(CL_LANGUAGE_MATCH_581), true);
			}
		}
	} else {
		hideRmlForMsgBox();
		CBoxMgr::ShowMsgBox(OnShowForm, RES_STRING(CL_LANGUAGE_MATCH_582), true);
	}
}

void CUIGuildApply::OnConfirm(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes) {
		CancelApply();
		return;
	}

	TrySubmit(m_pGuildNameEdit ? m_pGuildNameEdit->GetCaption() : "",
			  m_pGuildPasswordEdit ? m_pGuildPasswordEdit->GetCaption() : "",
			  m_pGuildConfirmEdit ? m_pGuildConfirmEdit->GetCaption() : "");
}

void CUIGuildApply::OnShowForm(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (UseRmlGuildApply()) {
		if (m_pGuildNameInputForm)
			m_pGuildNameInputForm->SetIsShow(false);
		CRmlUiGuildApplyForm::Instance().Show(false);
		return;
	}

	m_pGuildNameInputForm->ShowModal();
	if ((strlen(m_pGuildNameEdit->GetCaption()) > 0)) {
		m_pGuildPasswordEdit->SetActive(m_pGuildPasswordEdit);
	}
}

void RmlGuildApply_OnConfirm(const char* name, const char* password, const char* confirm) {
	CUIGuildApply::TrySubmit(name, password, confirm);
}

void RmlGuildApply_OnCancel() {
	CUIGuildApply::CancelApply();
}
