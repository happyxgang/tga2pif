#pragma once

/************************************************************************
** 文件名: linecompress.h
** 描  述: 目前用于点选信息的压缩
** 时  间: 5/8/2010
*************************************************************************/
//#include "fbase/ftype.h"
#include "stdafx.h"

// 压缩buff
bool line_compress(char *buff, int32 line_size, int32 number_line, char *compress_buff, int32 &len);
// 从压缩的内存中取坐标(x,y)的数据
char line_compress_get(char *compress_buff, int32 x, int32 y);

// 点选信息压缩
bool mask_compress(char *buff, int32 line_size, int32 number_line,int32 mask_scale, char *compress_buff, int32 &len);
char mask_compress_get(char *compress_buff,int32 mask_scale, int32 x, int32 y);

