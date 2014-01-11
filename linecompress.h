#pragma once

/************************************************************************
** �ļ���: linecompress.h
** ��  ��: Ŀǰ���ڵ�ѡ��Ϣ��ѹ��
** ʱ  ��: 5/8/2010
*************************************************************************/
//#include "fbase/ftype.h"
#include "stdafx.h"

// ѹ��buff
bool line_compress(char *buff, int32 line_size, int32 number_line, char *compress_buff, int32 &len);
// ��ѹ�����ڴ���ȡ����(x,y)������
char line_compress_get(char *compress_buff, int32 x, int32 y);

// ��ѡ��Ϣѹ��
bool mask_compress(char *buff, int32 line_size, int32 number_line,int32 mask_scale, char *compress_buff, int32 &len);
char mask_compress_get(char *compress_buff,int32 mask_scale, int32 x, int32 y);

