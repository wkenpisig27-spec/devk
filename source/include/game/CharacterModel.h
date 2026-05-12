#pragma once

// CharacterModel Ϊֱ����MPCharacter�򽻵�����
// ������Ҫ���ð���ģ�͵Ĵ���, �������йص����в����ӿ�
// �Լ�������ݵı���, ͬʱҲ�ж����Ĳ��ź͹���
// Character���̳�CharacterModel
// Character�����ִ���Ϸ�߼��������
// ��̳й�ϵΪ:

// CCharacter: public CCharacterModel, public CActor
// �������Ҫ������ΪJack Li

// �������ṩ�Ľӿڰ���

// Load(����ID)

// ChangePart(PartInfo)

// PlayPose(PoseID, LoopFlag)

// AttachItem(����ID, ��λ���)

// AttachEffect(EffectID, ��λ���)

/*
	dummy_0		�Ż���
	dummy_1		ͷ��
	dummy_2		�ز�
	dummy_3		����
	dummy_4		���
	dummy_5		���
	dummy_6		����
	dummy_7		�Ҽ�
	dummy_8		�ұ�
	dummy_9		����
	dummy_10	����
	dummy_11	��ϥ
	dummy_12	���
	dummy_13	����
	dummy_14	��ϥ
	dummy_15	�ҽ�
*/

enum {
	LINK_ID_BASE = 0,
	LINK_ID_HEAD = 1,
	LINK_ID_CHEST = 2,
	LINK_ID_PELVIS = 3,
	LINK_ID_LEFTCLAVICLE = 4,
	LINK_ID_LEFTARM = 5,
	LINK_ID_LEFTHAND = 6,
	LINK_ID_RIGHTCLAVICLE = 7,
	LINK_ID_RIGHTARM = 8,
	LINK_ID_RIGHTHAND = 9,
	LINK_ID_LEFTTHIGH = 10,
	LINK_ID_LEFTCALF = 11,
	LINK_ID_LEFTFOOT = 12,
	LINK_ID_RIGHTTHIGH = 13,
	LINK_ID_RIGHTCALF = 14,
	LINK_ID_RIGHTFOOT = 15,

	LINK_ID_NUM = 24,
};

class CSceneItem;
struct ItemLinkInfo {
	CSceneItem* obj;
	DWORD link_item_id;
	DWORD link_parent_id;
	DWORD data;
};

class MPDrunkCtrl {
private:
	MPSceneItem* dummy_obj;
	MPCharacter* cha_obj;
	lwMatrix44 cha_mat_save;
	lwMatrix44 cha_mat;

public:
	MPDrunkCtrl()
		: dummy_obj(0), cha_obj(0) {}
	~MPDrunkCtrl() {
		LW_SAFE_DELETE(dummy_obj);
	}
	BOOL CreateDrunkDummy(const char* file);
	void Attach(MPCharacter* cha) {
		if (cha) {
			cha_obj = cha;
			cha_mat_save = *cha_obj->GetMatrix();
			cha_mat = *cha_obj->GetMatrix();
			dummy_obj->PlayObjImpPose(ANIM_CTRL_TYPE_MAT, 0, PLAY_LOOP);
		}
	}
	MPCharacter* Detach() {
		if (cha_obj == 0)
			return 0;

		// cha_obj->SetMatrix(&cha_mat_save);
		MPCharacter* cha = cha_obj;
		cha_obj = 0;
		return cha;
	}
	void UpdateChaMatrix() {
		cha_mat = *cha_obj->GetMatrix();
	}
	void Update() {
		if (cha_obj) {
			lwMatrix44 mat;
			dummy_obj->FrameMove();
			lwMatrix44* rot_mat = dummy_obj->GetObject()->GetPrimitive()->GetRenderCtrlAgent()->GetGlobalMatrix();
			// lwMatrix44Multiply(&mat, rot_mat, &cha_mat_save);
			cha_obj->UpdateYawPitchRoll();
			lwMatrix44Multiply(&mat, rot_mat, cha_obj->GetMatrix());
			cha_obj->SetMatrix(&mat);
		}
	}
};

class CCharacterModel : public MPCharacter {
public:
	enum {
		MODEL_INVALID = 0,
		MODEL_CHARACTER = 1,
		MODEL_WARSHIP = 2,
		MODEL_TOWER = 3,
	};

protected:
	DWORD _ModelType;
	CSceneItem* _LinkItem[LINK_ID_NUM];

	float _UIScaleDis;
	int _UIYaw;
	int _UIPitch;
	BYTE _UIColor[3]; // r, g, b

	DWORD _TypeID; // �ˣ��������������ж�
	DWORD _BoneID;
	DWORD _PartID[8];
	const char* _PartFile[8];
	DWORD _ShipType; // �����������䴬�����ж�
	DWORD _SmallPoseID;
	DWORD _PoseType;
	DWORD _PoseTabID;
	D3DMATERIALX _mtl;
	lwPoseKeyFrameProc _KeyFrameProc;
	void* _ProcParam;
	BOOL _EmiColor;

	BOOL _DrunkState;
	MPDrunkCtrl* _DrunkCtrl;

	BYTE _color[3];

private:
	int GetBoatPart(int dummy);
	DWORD _BigPose2SmallPose(DWORD pose_id);
	int cha_type_id;

public:
	CCharacterModel();
	~CCharacterModel() {
		LW_SAFE_DELETE(_DrunkCtrl);
	}

	void FrameMove();
	void Render();
	void RenderForUI(int x, int y, bool bShowLingItem = true);
	void RenderUI(int x, int y);

	void Destroy();
	virtual int LoadCha(DWORD type_id, DWORD bone_id, const DWORD* part_buf);
	virtual int LoadCha(const MPChaLoadInfo* info);
	virtual int ChangePart(DWORD id, const char* file);
	virtual int ChangePart(DWORD part_id, DWORD tab_id);
	virtual int ChangePart(DWORD tab_id);
	virtual int ReCreate(DWORD type_id);
	virtual int LoadPart(DWORD part_id, const char* file);
	virtual int LoadPart(DWORD part_id, DWORD id);
	int LoadPose();
	virtual int DestroyPart(DWORD id);
	virtual int LoadShip(DWORD type_id, DWORD ship_type, DWORD* part_buf);
	virtual int LoadShip(DWORD type_id, DWORD* part_buf);
	virtual int LoadTower(DWORD type_id, DWORD* part_buf);

	int LoadPose(int cha_type);
	void PlayPose(DWORD pose, DWORD type = PLAY_ONCE, int time = -1, int fps = 32, DWORD blend_flag = 1, DWORD blend_src_num = 5, bool IsGlitched = false); // LoopSpeed,���ٺ���Poseѭ��һ��
	void PlayPose(const MPPlayPoseInfo* info);
	void PlayPosePause();
	void PlayPoseContinue();
	DWORD GetCurPoseID();	// ʵ�ʵ�pose �� Сpose
	DWORD GetCurPoseType(); // pose������, �ο� enum EActionNumber

	int IsPosePlaying(DWORD pose);
	int IsCurPosePlaying();

	// link_id:�ο������LINK_ID_XXX; obj:���߶�����Ҫ��ͨ��Scene->AddSceneItem��
	// part_type������Ϊ-1����ֻΪ��λ����
	int AttachItem(DWORD link_id, CSceneItem* obj, DWORD part_type, DWORD item_link_id = 0);
	int AttachItem(DWORD link_id, const ItemLinkInfo* info);
	CSceneItem* DetachItem(CSceneItem* obj);
	CSceneItem* DetachItem(DWORD link_id);
	CSceneItem* GetLinkItem(DWORD link_id) { return link_id < 0 || link_id >= LINK_ID_NUM ? 0 : _LinkItem[link_id]; }

	void SetModelType(DWORD type) { _ModelType = type; }
	DWORD GetModelType() const { return _ModelType; }

	void SetUIScaleDis(float dis) { _UIScaleDis = dis; }
	void SetUIYaw(int angle) { _UIYaw = angle; }
	void SetUIPitch(int angle) { _UIPitch = angle; }
	void SetUIColor(BYTE r, BYTE g, BYTE b) {
		_UIColor[0] = r;
		_UIColor[1] = g;
		_UIColor[2] = b;
	}

	DWORD GetTypeID() const { return _TypeID; }
	DWORD GetPartID(DWORD id) const { return _PartID[id]; }
	DWORD GetShipType() const { return _ShipType; }
	void SetShipType(DWORD type) { _ShipType = type; }

	HRESULT GetObjDummyRunTimeMatrix(MPMatrix44* mat, DWORD dummy_id, int type = -1);

	void SetAmbColor(BYTE r, BYTE g, BYTE b);
	void SetColor(BYTE r, BYTE g, BYTE b);
	void SetOpaque(float opaque);
	float GetOpaque() const { return _mtl.Ambient.a; }
	void GetColor(BYTE* buf3) {
		buf3[0] = _color[0];
		buf3[1] = _color[1];
		buf3[2] = _color[2];
	}

	void SetPoseKeyFrameProc(lwPoseKeyFrameProc proc, void* proc_param);
	DWORD GetCurPoseKeyFrameNum();
	MPIPoseCtrl* GetPoseCtrl();
	MPPoseInfo* GetPoseInfo(DWORD pose_id);
	MPPoseInfo* GetPoseInfoBig(DWORD big_pose_id);
	float GetPoseVelocity(); // ��ֵԽ��Խ��, default: 1.0f
	void SetEmiColorFLag(BOOL flag) { _EmiColor = flag; }
	BOOL GetEmiColorFlag() { return _EmiColor; }

	int Unlit(DWORD part_id);
	int Lit(DWORD part_id, DWORD lit_id);
	BOOL Cull();
	BOOL SetDrunkState(BOOL flag);
	BOOL GetDrunkState() { return _DrunkState; }
	void UpdateChaMatrix() { _DrunkCtrl->UpdateChaMatrix(); }

	void SetPoseVelocity(float velocity);
};

inline int CCharacterModel::GetBoatPart(int dummy) {
	switch (dummy) {
	case 0:
	case 1:
		return 1;
		break;
	case 2:
	case 3:
	case 7:
		return 0;
		break;
	case 4:
	case 5:
	case 6:
		return 2;
	default:
		return 0;
	}
}
inline HRESULT CCharacterModel::GetObjDummyRunTimeMatrix(MPMatrix44* mat, DWORD dummy_id, int type) {
	if (_ModelType == MODEL_CHARACTER) {
		return MPCharacter::GetObjDummyRunTimeMatrix(mat, dummy_id);
	} else if (_ModelType == MODEL_WARSHIP) {
		if (type < 0)
			return MPCharacter::GetObjDummyRunTimeMatrix(mat, GetBoatPart(dummy_id), dummy_id);
		else
			return MPCharacter::GetObjDummyRunTimeMatrix(mat, 0, dummy_id);
	} else if (_ModelType == MODEL_TOWER) {
		const int id[8] =
			{
				1, 2, 1, 0, 0, 0, 0, 0};

		if (dummy_id >= 8) {
			// LG("error", "msgInvalid dummy id called GetObjDummyRunTimeMatrix");
			return -1;
		}

		return MPCharacter::GetObjDummyRunTimeMatrix(mat, id[dummy_id], dummy_id);
	} else {
		return -1;
	}
}

inline DWORD CCharacterModel::GetCurPoseType() {
	return _PoseType;
}
