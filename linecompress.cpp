#include "stdafx.h"
#include "linecompress.h"

// 压缩buff
bool line_compress(char *buff, int32 line_size, int32 number_line, char *compress_buff, int32 &len)
{
	int32 length=0;
	char *src=NULL;
	char *dest=compress_buff;
	for (int32 i=0; i<number_line; i++){
		int32 j = 0;
		uint8 num = 0;
		src = buff + i*line_size;
		// 取第一个
		char last = *src++;j++;num++;
		while(j<line_size){
			char cur = *src++;
			if (cur == last || num > 255){
				num ++;
			}else{
				// 写入
				*dest ++ = last;
				*dest ++ = num;
				last = cur;
				num = 1;
			}
			j++;
		}
		// 行结束符
		*dest ++= 0;
		*dest ++= 0;
	}
	length = dest - compress_buff;
	if (len < length){
		return false;
	}
	len = length;
	return true;
}
// 从压缩的内存中取坐标(x,y)的数据
char line_compress_get(char *compress_buff, int32 x, int32 y)
{
	char *src = compress_buff;
	int32 line = 0;

	char val, num;
	// 找到第y行
	while (true){
		if (line == y){
			break;
		}
		val = *src++;
		num = *src++; 
		if (val == 0 && num ==0){ // 行结束符
			line ++;
		}
	}
	int32 row = 0;
	while(true){
		val = *src++;
		num = *src++;
		if (row + num >= x){
			return val; // 正确的返回路径
		}else{
			row += num;
		}
	}
	return 0;
}


// 压缩buff
//------------------------------------------------------------------------------
/** | 7 | 6 ~ 0 |
*    值    数量
* 类似于上面的算法， 把val和num压缩为一个字节
* header:
* 4 width;
* 4 height;
* 1 line1 offset;
* 1 line2 offset;
* 1 line3 offset;
* ...(height lines);
*/
struct mask_buffer {
	uint32 width;
	uint32 height;
	uint32 line_offset[0];
};
bool mask_compress(char *buff, int32 line_size, int32 number_line, int32 mask_scale,  char *compress_buff, int32 &len)
{
//#define MASK_FILE
	int32 length=0;
	char *src=NULL;
	uint8 *dest=(uint8 *)compress_buff;
	mask_buffer *mask_buff = (mask_buffer *)compress_buff;
	mask_buff->width = line_size;
	mask_buff->height = number_line;

#ifdef MASK_FILE
	FILE *f = fopen(FS_TMP_DIR"mask.txt", "wb");
#endif
	dest += sizeof(mask_buffer) + number_line * sizeof(uint32); 
	for (int32 i=0; i<number_line; i += mask_scale){
		int32 j = 0;
		uint8 num = 0;
		src = buff + i*line_size;

		// 如果整行都是0的话， 填入0表示这一行
		bool all_zero = true;
		while (j<line_size){
			if(*src ++ != 0){
				all_zero = false;
				break;
			}
			j++;
		}
		if (all_zero){
			*dest ++ = 0;
			// 填入偏移量
			mask_buff->line_offset[i] = dest - (uint8 *)compress_buff;
			continue;
		}

		j=0; num = 0; src = buff + i*line_size;
		// 取第一个
		uint8 last = *src++;j++;num++;
		while(j<line_size){
			uint8 cur = *src++;
			if (cur == last && num < 0x80-1){
				num ++;
			}else{
				// 写入
				*dest ++ = last<<7 | num;
#ifdef MASK_FILE
				fprintf(f, "(%d,%d)",last, num);
#endif
				last = cur;
				num = 1;
			}
			j++;
		}
		// 填入偏移量
		mask_buff->line_offset[i] = dest - (uint8 *)compress_buff;
#ifdef MASK_FILE
		fprintf(f, "\n");
#endif

	}
	length = dest - (uint8 *)compress_buff;
	if (len < length){
		return false;
	}
	len = length;

#ifdef MASK_FILE
	fprintf(f, "len=%d", len);
#endif
#ifdef MASK_FILE
	fclose(f);
#endif
#undef MASK_FILE
	return true;
}
// 从压缩的内存中取坐标(x,y)的数据
char mask_compress_get(char *compress_buff,int32 mask_scale, int32 x, int32 y)
{
	char *src = compress_buff;
	mask_buffer *buff = (mask_buffer *)compress_buff;
	int32 line = 0;

	uint8 val, num;

	src += buff->line_offset[ (y/mask_scale) * mask_scale];
	int32 row = 0;
	while(true){
		val = (*src&0x80) >> 7;
		num = (*src&0x7f);
		src ++;
		if (row + num >= x){
			return val; // 正确的返回路径
		}else if(num == 0){
			return val; // 0表示整行都是0
		}else{
			row += num;
		}
	}
	return 0;
}
