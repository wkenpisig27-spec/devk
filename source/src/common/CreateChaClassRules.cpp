#include "CreateChaClassRules.h"

namespace create_cha {

namespace {

struct CharacterClassRule {
	short jobs[5];
};

// Extend a character entry by appending job IDs and terminating with CREATE_CHA_CLASS_NONE.
const CharacterClassRule kCharacterClassRules[CHA_COUNT] = {
	// Lance: Swordsman, Explorer, Hunter
	{{JOB_TYPE_JIANSHI, JOB_TYPE_MAOXIANZHE, JOB_TYPE_LIEREN, CREATE_CHA_CLASS_NONE}},
	// Carsise: Swordsman only
	{{JOB_TYPE_JIANSHI, CREATE_CHA_CLASS_NONE}},
	// Phyllis: Swordsman, Hunter, Herbalist, Explorer
	{{JOB_TYPE_JIANSHI, JOB_TYPE_LIEREN, JOB_TYPE_QIYUANSHI, JOB_TYPE_MAOXIANZHE, CREATE_CHA_CLASS_NONE}},
	// Ami: Herbalist, Explorer
	{{JOB_TYPE_QIYUANSHI, JOB_TYPE_MAOXIANZHE, CREATE_CHA_CLASS_NONE}},
};

const CharacterClassRule* GetRule(int chaIndex) {
	if (chaIndex < 0 || chaIndex >= CHA_COUNT)
		return nullptr;
	return &kCharacterClassRules[chaIndex];
}

} // namespace

int GetAllowedClassCount(int chaIndex) {
	const CharacterClassRule* rule = GetRule(chaIndex);
	if (!rule)
		return 0;

	int count = 0;
	for (; count < static_cast<int>(sizeof(rule->jobs) / sizeof(rule->jobs[0])); ++count) {
		if (rule->jobs[count] == CREATE_CHA_CLASS_NONE)
			break;
	}
	return count;
}

short GetAllowedClassJobId(int chaIndex, int classIndex) {
	const CharacterClassRule* rule = GetRule(chaIndex);
	if (!rule || classIndex < 0)
		return CREATE_CHA_CLASS_NONE;

	for (int i = 0; i < static_cast<int>(sizeof(rule->jobs) / sizeof(rule->jobs[0])); ++i) {
		if (rule->jobs[i] == CREATE_CHA_CLASS_NONE)
			break;
		if (i == classIndex)
			return rule->jobs[i];
	}

	return CREATE_CHA_CLASS_NONE;
}

bool IsValidClassForCharacter(int chaIndex, short jobId) {
	return FindClassIndexForJob(chaIndex, jobId) >= 0;
}

int FindClassIndexForJob(int chaIndex, short jobId) {
	const CharacterClassRule* rule = GetRule(chaIndex);
	if (!rule)
		return -1;

	for (int i = 0; i < static_cast<int>(sizeof(rule->jobs) / sizeof(rule->jobs[0])); ++i) {
		if (rule->jobs[i] == CREATE_CHA_CLASS_NONE)
			break;
		if (rule->jobs[i] == jobId)
			return i;
	}

	return -1;
}

} // namespace create_cha
