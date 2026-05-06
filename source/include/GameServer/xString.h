#ifndef XSTRING_H
#define XSTRING_H

#include <tchar.h>

// 从给定字符窜(_TCHAR *in)的给定位置(int *in_from)获取字符窜，
// 直到遇到字符窜列表(_TCHAR *end_list)中的任一字符时结束
int StringGet(_TCHAR* out, int out_max, _TCHAR* in, int* in_from, _TCHAR* end_list, int end_len);

// 从给定字符窜(_TCHAR *in)的给定位置(int *in_from)剔除字符窜列表(_TCHAR *end_list)中的任一字符
void StringSkipCompartment(_TCHAR* in, int* in_from, _TCHAR* skip_list, int skip_len);

#endif
