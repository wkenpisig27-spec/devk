//======================================================================================================================
// FileName: CharacterRecord.h
// Creater: ZhangXuedong
// Date: 2004.09.01
// Comment: CChaRecordSet class
//======================================================================================================================

#ifndef CHARACTERRECORD_H
#define CHARACTERRECORD_H

#include <tchar.h>
#include "util.h"
#include "TableData.h"

// �����Ϣ-ģ������
enum EChaModalType {
	enumMODAL_MAIN_CHA = 1,
	enumMODAL_BOAT,
	enumMODAL_EMPL,
	enumMODAL_OTHER,
};

// ��������
enum EChaCtrlType {
	enumCHACTRL_NONE = 0, // δ�����

	enumCHACTRL_PLAYER = 1, // ���

	enumCHACTRL_NPC = 2,	   // ��ͨNPC
	enumCHACTRL_NPC_EVENT = 3, // �¼�NPC

	enumCHACTRL_MONS = 5,		// ��ͨ����
	enumCHACTRL_MONS_TREE = 6,	// ������
	enumCHACTRL_MONS_MINE = 7,	// ��ʯ����
	enumCHACTRL_MONS_FISH = 8,	// �����
	enumCHACTRL_MONS_DBOAT = 9, // ��������

	enumCHACTRL_PLAYER_PET = 10, // ��ҳ���

	enumCHACTRL_MONS_REPAIRABLE = 17, // ���޲�����
};

#define defCHA_AI_NONE 0		   // ����
#define defCHA_AI_ATTACK_PASSIVE 1 // ��������
#define defCHA_AI_ATTACK_ACTIVE 2  // ��������

const char cchChaRecordKeyValue = (char)0xff;

#define defCHA_NAME_LEN 32
#define defCHA_ICON_NAME_LEN 17

#define defCHA_SKIN_NUM 8
#define defCHA_INIT_SKILL_NUM 11 // 0����,1-10Ϊ��������
#define defCHA_INIT_ITEM_NUM 20 // Increased from 15
#define defCHA_GUILD_NAME_LEN 33
#define defCHA_TITLE_NAME_LEN 33
#define defCHA_JOB_NAME_LEN 17
#define defCHA_ITEM_KIND_NUM 20
#define defCHA_DIE_EFFECT_NUM 3	  // �ͻ��������������
#define defCHA_BIRTH_EFFECT_NUM 3 // �ͻ�������������
#define defCHA_HP_EFFECT_NUM 3

class CChaRecord : public CRawDataInfo {
public:
	// CChaRecord();

	int lID;								 // ���
	_TCHAR szName[defCHA_NAME_LEN];			 // ����
	_TCHAR szIconName[defCHA_ICON_NAME_LEN]; // ͷ��ͼ������
	char chModalType;						 // ģ������
	char chCtrlType;						 // ��������
	short sModel;							 // �Ǽܺ�
	short sSuitID;							 // �׺�
	short sSuitNum;							 // ��װ����
	short sSkinInfo[defCHA_SKIN_NUM];		 // Ƥ��
	short sFeffID[4];						 // FeffID
	short sEeffID;							 // EeffID
	short sEffectActionID[3];				 // ��Ч�������,0-�������,1-��Ч,2-dummy
	short sShadow;							 // Ӱ��
	short sActionID;						 // �������
	char chDiaphaneity;						 // ͸����
	short sFootfall;						 // ������Ч
	short sWhoop;							 // ��Ϣ��Ч
	short sDirge;							 // ������Ч
	char chControlAble;						 // �Ƿ�ɿ�
	// char	chMoveAble;							// �ɷ��ƶ�
	char chTerritory;							// ��������
	short sSeaHeight;							// �߶�ƫ��
	short sItemType[defCHA_ITEM_KIND_NUM];		// ��װ������
	float fLengh;								// �����ף�
	float fWidth;								// �����ף�
	float fHeight;								// �ߣ��ף�
	short sRadii;								// �뾶
	char nBirthBehave[defCHA_BIRTH_EFFECT_NUM]; // ��������
	char nDiedBehave[defCHA_DIE_EFFECT_NUM];	// ��������
	short sBornEff;								// ������Ч
	short sDieEff;								// ������Ч
	short sDormancy;							// ���߶���
	char chDieAction;							// ����ʱ�Ķ���
	int _nHPEffect[defCHA_HP_EFFECT_NUM];		// ʣ��hp��Ч����
	bool _IsFace;								// �Ƿ����ת
	bool _IsCyclone;							// �Ƿ�ɱ�쫷紵��
	int lScript;								// �ű��ļ�ID
	int lWeapon;								// ���߱��е�����ID
	int lSkill[defCHA_INIT_SKILL_NUM][2];		// ���ܼ�ʹ��Ƶ��
	int lItem[defCHA_INIT_ITEM_NUM][2];		// ��Ʒ�����ϼ���
	int lTaskItem[defCHA_INIT_ITEM_NUM][2];	// ������Ʒ�����ϼ���
	int lMaxShowItem;							// ÿ����౬����Ʒ����
	float fAllShow;								// �򱬼���
	int lPrefix;								// ǰ׺�ȼ�
	int lAiNo;									// AI���
	char chCanTurn;								// ����ʱ�Ƿ�ת��
	int lVision;								// ��Ұ��Χ�����ף�
	int lNoise;								// ���з�Χ�����ף�
	int lGetEXP;								// ֱ�ӻ�õ�EXP
	bool bLight;								// �Ƿ��ܹ���Ӱ��

	int lMobexp;						   // ��ͨexp
	int lLv;							   // ��ɫ�ȼ�
	int lMxHp;							   // ���HP
	int lHp;							   // ��ǰHP
	int lMxSp;							   // ���SP
	int lSp;							   // ��ǰSP
	int lMnAtk;						   // ��С������
	int lMxAtk;						   // ��󹥻���
	int lPDef;							   // �����ֿ�
	int lDef;							   // ������
	int lHit;							   // ������
	int lFlee;							   // ������
	int lCrt;							   // ������
	int lMf;							   // Ѱ����
	int lHRec;							   // hp�ָ��ٶ�
	int lSRec;							   // sp�ָ��ٶ�
	int lASpd;							   // �������
	int lADis;							   // ��������
	int lCDis;							   // ׷����Χ
	int lMSpd;							   // �ƶ��ٶ�
	int lCol;							   // ��Դ�ɼ��ٶ�
	int lStr;							   // ������strength��		Ӱ�칥�����������ٶ�
	int lAgi;							   // ���ݣ�agility��		Ӱ��������
	int lDex;							   // רע��dexterity��	Ӱ��������
	int lCon;							   // ���ʣ�constitution��	Ӱ���������hp
	int lSta;							   // ������stamina��		Ӱ��sp��Ӱ�켼�ܹ�����
	int lLuk;							   // ���ˣ�luck��			Ӱ�����һ�����ʡ���Ʒ���伸��
	int lLHandVal;						   // ������������
	_TCHAR szGuild[defCHA_GUILD_NAME_LEN]; // �л�����
	_TCHAR szTitle[defCHA_TITLE_NAME_LEN]; // ��ɫ�ƺ�
	_TCHAR szJob[defCHA_JOB_NAME_LEN];	   // ��ɫְҵ
	LONG64 lCExp;						   // ��ǰ����
	LONG64 lNExp;						   // ��һ�����辭��
	int lFame;							   // ����
	int lAp;							   // ���Ե�
	int lTp;							   // ���ܵ㣬ÿ��������ã����ܵ㣬���ɵ�technique point��
	LONG64 lGd;							   // ��Ǯ (64-bit for 100B gold cap)
	int lSpri;							   // �ڵ������ٶ�
	int lStor;							   // ����(����)����
	int lMxSail;						   // ��Ա����
	int lSail;							   // ���д�Ա������
	int lStasa;						   // ��׼��Ա��������
	int lScsm;							   // ��������

	int lTStr;	 // ��ʱ����				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTAgi;	 // ��ʱ����				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTDex;	 // ��ʱרע				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTCon;	 // ��ʱ����				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTSta;	 // ��ʱ����				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTLuk;	 // ��ʱ����				�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTMxHp; // ��ʱ���Ѫ��			�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTMxSp; // ��ʱ�����ֵ		�ɵ��߻��߼��ܼӳɵģ��õ����߻���ʱ�䵽���Զ�����
	int lTAtk;	 // ��ʱ������			��ʱ���ӵĹ�����
	int lTDef;	 // ��ʱ������			��ʱ���ӵķ�����
	int lTHit;	 // ��ʱ������			��ʱ���ӵ�������
	int lTFlee; // ��ʱ������			��ʱ���ӵ�������
	int lTMf;	 // ��ʱѰ����			��ʱ���ӵ�Ѱ����
	int lTCrt;	 // ��ʱ������			��ʱ���ӵı�����
	int lTHRec; // ��ʱhp�ָ��ٶ�		��ʱ���ӵ�ÿ���ӻָ�hp����ֵ
	int lTSRec; // ��ʱsp�ָ��ٶ�		��ʱ���ӵ�ÿ���ӻָ�sp����ֵ
	int lTASpd; // ��ʱ�������			��ʱ������ҹ������
	int lTADis; // ��ʱ��������			��ʱ������ҵĹ�������
	int lTSpd;	 // ��ʱ�ƶ��ٶ�			��ʱ���ӵ��ƶ��ٶ�
	int lTSpri; // ��ʱ�����ٶ�			��ʱ�ܵ�ÿ���ӷ��еľ���
	int lTScsm; // ��ʱ��������			��ʱ�ı�ĵ�λʱ���ڵ�����

	// added by clp ������Ϣ
	float scaling[3];

public:
	// ����������������Ѫ���ʱ,Ҫ��ʾ����Ч
	bool HaveEffectFog() { return _HaveEffectFog; }
	int GetEffectFog(int i) { return _nHPEffect[i]; }

	bool IsFace() { return _IsFace; }		// �Ƿ����ת
	bool IsCyclone() { return _IsCyclone; } // �Ƿ�ɱ�쫷�ȴ�����

	void RefreshPrivateData(); // ˢ���ڲ�����

private:
	bool _HaveEffectFog;
};

class CChaRecordSet : public CRawDataSet {
public:
	static CChaRecordSet* I() { return _Instance; }

	CChaRecordSet(int nIDStart, int nIDCnt, int nCol = 128)
		: CRawDataSet(nIDStart, nIDCnt, nCol) {
		_Instance = this;
		_Init();
	}

protected:
	static CChaRecordSet* _Instance; // �൱�ڵ���, ���Լ���ס

	virtual CRawDataInfo* _CreateRawDataArray(int nCnt) {
		return new CChaRecord[nCnt];
	}

	virtual void _DeleteRawDataArray() {
		delete[] (CChaRecord*)_RawDataArray;
	}

	virtual int _GetRawDataInfoSize() {
		return sizeof(CChaRecord);
	}

	virtual void* _CreateNewRawData(CRawDataInfo* pInfo) {
		return nullptr;
	}

	virtual void _DeleteRawData(CRawDataInfo* pInfo) {
		SAFE_DELETE(pInfo->pData);
	}

	virtual BOOL _ReadRawDataInfo(CRawDataInfo* pRawDataInfo, std::vector<std::string>& ParamList);
	virtual void _ProcessRawDataInfo(CRawDataInfo* pInfo);
};

inline CChaRecord* GetChaRecordInfo(int nTypeID) {
	return (CChaRecord*)CChaRecordSet::I()->GetRawDataInfo(nTypeID);
}

#endif // CHARACTERRECORD_H