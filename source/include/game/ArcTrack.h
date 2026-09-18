#pragma once

class CArcTrack {
public:
	CArcTrack() : _isEnd(true) {}

	void Start(const XMVECTOR3& vStart, const XMVECTOR3& vEnd, float fVel, float fHei, DWORD dwDurationMs = 2000);
	void FrameMove();

	XMVECTOR3 GetPos() { return _vPos; }
	bool IsEnd() { return _isEnd; }

private:
	XMVECTOR3 _vStart, _vEnd, _vOrg, _vPos;
	float _fHei, _fVel, _fCurAngle;
	bool _isEnd;
	DWORD _dwLastTime;
	DWORD _dwOverTime;
};
