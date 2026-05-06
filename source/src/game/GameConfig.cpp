#include "stdafx.h"
#include "GameConfig.h"
#include "GameApp.h"

using namespace std;

CGameConfig g_Config;

CGameConfig::CGameConfig() {
	g_bBinaryTable = TRUE; // �˱��������˱���ʹ���Ƿ������
	SetDefault();
	Load("scripts/kop.cfg"); // �������ļ�
}

void CGameConfig::SetDefault() // Ĭ������
{
	memset(m_ChaList, 0, sizeof(SPlaceCha) * 20);

	m_bAutoLogin = FALSE;

	m_bFullScreen = FALSE;
	m_bEnableMusic = TRUE;

	m_nChaCnt = 0; // ��ɫ����

	m_LightColor[0] = 1.0f;
	m_LightColor[1] = 1.0f;
	m_LightColor[2] = 1.0f;
	m_LightDir[0] = 1.0f;
	m_LightDir[1] = 1.0f;
	m_LightDir[2] = -1.0f;

	m_bNoObj = FALSE;
	m_bEditor = FALSE; //  ��Ϸ�༭��
	m_btScreenMode = 0;

	m_nMaxChaType = 350;
	m_nMaxSceneObjType = 3000;
	m_nMaxEffectType = 14000; // �������Ӿ������ߵ���Ч��	modify by Michael 2005-11-9
	m_nMaxItemType = 32768;
	m_nMaxResourceNum = 3000;

	m_fCameraVel = 0;
	m_fCameraAccl = 0;

	m_nCreateScene = 1; // ��ʼ����

	nLeftHand = 0; // ������������������
	nRightHand = 0;

	m_bCheckOvermax = TRUE;
	m_nSendHeartbeat = 30;
	m_nConnectTimeOut = 0; // �������ӳ�ʱ

	m_bEnableLGMsg = TRUE; //   ��������LG-Box
	m_bEnableLG = TRUE;	   //   �������LG��Ϣ

	m_bRenderSceneObj = TRUE; // ��������ʵʩ
	m_bRenderCha = TRUE;	  // ��ɫʵʩ
	m_bRenderEffect = TRUE;
	m_bRenderTerrain = TRUE;
	m_bRenderUI = TRUE;
	m_bRenderMinimap = TRUE;
	m_bRender = TRUE;

	m_bMThreadRes = TRUE; // �߳���Դ����

	m_dwFullScreenAntialias = 0;

	m_fLgtFactor = 0.0f;
	m_dwLgtBkColor = 0xffc0c0c0;
	// ���������л���ͼʱ�Ĳ���
	m_dwMaxCha = 300;
	m_dwMaxEff = 500;
	m_dwMaxItem = 400;
	m_dwMaxObj = 800;

	strcpy(m_szMD5Pass, ""); // ��д��MD5����
	memset(m_szVerErrorHTTP, 0, sizeof(m_szVerErrorHTTP));

	m_IsShowConsole = false; // �Ƿ���Բ����ؼ�̨
	m_IsMoveClient = true;
	m_IsBill = false; //  �Ƿ��ֵ

	// Add by lark.li 20080429 for res
	memcpy(m_szFontName1, "", sizeof(m_szFontName1));
	memcpy(m_szFontName2, "", sizeof(m_szFontName1));
	// Res

	// Add by lark.li 20080825 begin
	m_nMovieW = -1;
	m_nMovieH = -1;
	// End

	// Shadow mapping defaults
	m_bEnableShadowMap = TRUE;
	m_nShadowMapQuality = 1;  // 1 = Medium (1024)
}

void CGameConfig::Load(const char* pszFileName) //  ��kop.cfg
{
	LG("init", "Load Game Config File(Text Mode) [%s]\n", pszFileName);

	ifstream in(pszFileName);
	if (in.is_open() == 0) // �Ƿ��ܴ�
	{
		return;
	}
	string strPair[2];
	string strComment;
	string strLine;
	char szLine[255];
	while (!in.eof()) // �Ƿ񵽴��ļ�β
	{
		in.getline(szLine, 255);
		strLine = szLine;
		auto p = strLine.find("//"); // ��ȡע�ͺʹ���
		if (p != std::string::npos) {
			string strLeft = strLine.substr(0, p);
			strComment = strLine.substr(p + 2, strLine.size() - p - 2);
			strLine = strLeft;
		} else {
			strComment = "";
		}
		Util_TrimString(strLine); //  ��ʧ�ո�����
		if (strLine.size() == 0)
			continue;
		if (strLine[0] == '[') {
			Log("\n%s\n", strLine.c_str());
			continue;
		}

		int n = Util_ResolveTextLine(strLine.c_str(), strPair, 2, '=');
		if (n < 2)
			continue;
		string strKey = strPair[0];
		string strValue = strPair[1];

		if (strKey == "autologin") // �Զ���¼
		{
			m_bAutoLogin = Str2Int(strValue);
		} else if (strKey == "cha") // �̶�λ�ðڷŵĽ�ɫ, For Demo
		{
			string strCha[5];
			int nCnt = Util_ResolveTextLine(strValue.c_str(), strCha, 3, ',');
			if (nCnt == 3 && m_nChaCnt < 20) {
				m_ChaList[m_nChaCnt].nTypeID = Str2Int(strCha[0]);
				m_ChaList[m_nChaCnt].nX = Str2Int(strCha[1]);
				m_ChaList[m_nChaCnt].nY = Str2Int(strCha[2]);
				m_nChaCnt++;
			}
		} else if (strKey == "fullscreen") // ȫ��
		{
			m_bFullScreen = Str2Int(strValue);
		} else if (strKey == "screenmode") //  ��Ļģʽ
		{
			m_btScreenMode = (BYTE)(Str2Int(strValue));
		} else if (strKey == "light_dir") {
			string strLight[3];
			int nCnt = Util_ResolveTextLine(strValue.c_str(), strLight, 3, ',');
			if (nCnt == 3) {
				m_LightDir[0] = Str2Float(strLight[0]);
				m_LightDir[1] = Str2Float(strLight[1]);
				m_LightDir[2] = Str2Float(strLight[2]);
			}
		} else if (strKey == "light_color") {
			string strColor[3];
			int nCnt = Util_ResolveTextLine(strValue.c_str(), strColor, 3, ',');
			if (nCnt == 3) {
				m_LightColor[0] = Str2Float(strColor[0]);
				m_LightColor[1] = Str2Float(strColor[1]);
				m_LightColor[2] = Str2Float(strColor[2]);
			}
		} else if (strKey == "fogcolorR") // ����ɫ
		{
			m_iFogR = Str2Int(strValue);
		} else if (strKey == "fogcolorG") {
			m_iFogG = Str2Int(strValue);
		} else if (strKey == "fogcolorB") {
			m_iFogB = Str2Int(strValue);
		} else if (strKey == "fogexp2") // ���ܶ�
		{
			m_fExp2 = Str2Float(strValue);
		} else if (strKey == "noobj") {
			m_bNoObj = Str2Int(strValue);
		} else if (strKey == "eyeX") {
			eyeX = Str2Float(strValue);
		} else if (strKey == "eyeY") {
			eyeY = Str2Float(strValue);
		} else if (strKey == "eyeZ") {
			eyeZ = Str2Float(strValue);
		} else if (strKey == "refX") {
			refX = Str2Float(strValue);
		} else if (strKey == "refY") {
			refY = Str2Float(strValue);
		} else if (strKey == "refZ") {
			refZ = Str2Float(strValue);
		} else if (strKey == "fov") {
			fov = Str2Float(strValue); //  ��Ұ
		} else if (strKey == "Aspect") {
			fAspect = Str2Float(strValue); // ���
		} else if (strKey == "near") {
			fnear = Str2Float(strValue);
		} else if (strKey == "far") {
			ffar = Str2Float(strValue);
		} //
		else if (strKey == "CreateScene") {
			m_nCreateScene = Str2Int(strValue);
		} else if (strKey == "music") {
			m_bEnableMusic = Str2Int(strValue);
		} else if (strKey == "cameraVel") {
			m_fCameraVel = Str2Float(strValue);
		} else if (strKey == "cameraAccl") {
			m_fCameraAccl = Str2Float(strValue);
		} else if (strKey == "left_hand") // ��������
		{
			nLeftHand = Str2Int(strValue);
		} else if (strKey == "right_hand") // ��������
		{
			nRightHand = Str2Int(strValue);
		} else if (strKey == "check_overmax") {
			m_bCheckOvermax = Str2Int(strValue);
		} else if (strKey == "send_heartbeat") {
			m_nSendHeartbeat = Str2Int(strValue);
		} else if (strKey == "enable_lg_msg") {
			m_bEnableLGMsg = Str2Int(strValue); // �Ƿ���������LG-Box
		} else if (strKey == "enable_lg") {
			m_bEnableLG = Str2Int(strValue); // �Ƿ��������LG��Ϣ
		} else if (strKey == "sceneobj_render") {
			m_bRenderSceneObj = Str2Int(strValue);
		} else if (strKey == "cha_render") {
			m_bRenderCha = Str2Int(strValue);
		} else if (strKey == "effect_render") {
			m_bRenderEffect = Str2Int(strValue);
		} else if (strKey == "terrain_render") {
			m_bRenderTerrain = Str2Int(strValue);
		} else if (strKey == "ui_render") {
			m_bRenderUI = Str2Int(strValue);
		} else if (strKey == "minimap_render") {
			m_bRenderMinimap = Str2Int(strValue); // �Ƿ�ʵʩС��ͼ
		} else if (strKey == "render") {
			m_bRender = Str2Int(strValue);
		} else if (strKey == "multithreadres") {
			m_bMThreadRes = Str2Int(strValue);
		} else if (strKey == "fullscreen_antialias") {
			m_dwFullScreenAntialias = Str2Int(strValue);
		} else if (strKey == "connect_time_out") {
			m_nConnectTimeOut = 1000 * Str2Int(strValue);
		} else if (strKey == "lgt_factor") {
			m_fLgtFactor = Str2Float(strValue);
		} else if (strKey == "lgt_bkcolor") {
			string strColor[3];
			int nCnt = Util_ResolveTextLine(strValue.c_str(), strColor, 3, ',');
			if (nCnt == 3) {
				lwDwordByte4 c;
				c.b[3] = 0xff;
				c.b[2] = Str2Int(strColor[0]);
				c.b[1] = Str2Int(strColor[1]);
				c.b[0] = Str2Int(strColor[2]);
				m_dwLgtBkColor = c.d;
			}
		} else if (strKey == "MaxChaNum") {
			m_dwMaxCha = Str2Int(strValue);
		} else if (strKey == "MaxEffNum") {
			m_dwMaxEff = Str2Int(strValue);
		} else if (strKey == "MaxItemNum") {
			m_dwMaxItem = Str2Int(strValue);
		} else if (strKey == "MaxObjNum") {
			m_dwMaxObj = Str2Int(strValue);
		} else if (strKey == "md5pass") // ��д��MD5����
		{
			strcpy(m_szMD5Pass, strValue.c_str());
		} else if (strKey == "console") {
			m_IsShowConsole = Str2Int(strValue) != 0 ? true : false;
		} else if (strKey == "HTTP") // �汾��ƥ��ʱ���õ���ҳ
		{
			strcpy(m_szVerErrorHTTP, strValue.c_str());
		} else if (strKey == "client_move") {
			// m_IsMoveClient = Str2Int(strValue)!=0 ? true : false;
		} else if (strKey == "bill") {
			// m_IsBill = Str2Int(strValue)!=0 ? true : false;
			//  �ĳɺ궨��
#ifdef USED_BILL
			m_IsBill = true;
#else
			m_IsBill = false;

#endif
		}
		// Add by lark.li 20080201
		else if (strKey == "locale") {
			strcpy(m_szLocale, strValue.c_str());
		} else if (strKey == "path") {
			strcpy(m_szResPath, strValue.c_str());
		} else if (strKey == "fontname1") {
			strcpy(m_szFontName1, strValue.c_str());
		} else if (strKey == "fontname2") {
			strcpy(m_szFontName2, strValue.c_str());
		}
		// End
		// Add by lark.li 20080825 begin
		else if (strKey == "movie_w") {
			m_nMovieW = Str2Int(strValue);
		} else if (strKey == "movie_h") {
			m_nMovieH = Str2Int(strValue);
		}
		// End
		// Shadow mapping config
		else if (strKey == "shadow_map") {
			m_bEnableShadowMap = Str2Int(strValue);
		} else if (strKey == "shadow_quality") {
			m_nShadowMapQuality = Str2Int(strValue);
		}
	}
	in.close(); // �ر��ļ�
}

void CGameConfig::SetMoveClient(bool v) // �����Ƿ���ͻ���ͬ��
{
	m_IsMoveClient = v;
	// g_pGameApp->SysInfo( g_Config.m_IsMoveClient ? RES_STRING(CL_LANGUAGE_MATCH_142) : RES_STRING(CL_LANGUAGE_MATCH_141) );
}
