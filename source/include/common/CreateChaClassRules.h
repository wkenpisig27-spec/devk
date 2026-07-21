#pragma once

#include "JobType.h"

namespace create_cha {

enum eCreateChaIndex {
	CHA_LANCE = 0,
	CHA_CARSISE = 1,
	CHA_PHYLLIS = 2,
	CHA_AMI = 3,
	CHA_COUNT = 4,
};

enum { CREATE_CHA_CLASS_NONE = -1 };

int GetAllowedClassCount(int chaIndex);
short GetAllowedClassJobId(int chaIndex, int classIndex);
bool IsValidClassForCharacter(int chaIndex, short jobId);
int FindClassIndexForJob(int chaIndex, short jobId);

} // namespace create_cha
