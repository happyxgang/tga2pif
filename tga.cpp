#include "stdafx.h"
#include "tga.h"
#include "bmpfile.h"
#include "ltga.h"
#include <iostream>
#include <string.h>

typedef unsigned char byte;
typedef unsigned long dword;


#define TGA_NULL           0
#define TGA_COLORMAP       1
#define TGA_RGB            2
#define TGA_BW             3
#define TGA_RLE_COLORMAP   9
#define TGA_RLE_RGB       10
#define TGA_RLE_BW        11
#define TGA_COMPRESSED_COLORMAP_1
#define TGA_COMPRESSED_COLORMAP_2
#define INVALID_COLOR     0xFF00FF

#pragma pack (push)
#pragma pack (1)

struct PIXEL24 {
	byte red;							// 8bit 红色
	byte green;							// 8bit 绿色
	byte blue;							// 8bit 兰色
};

struct TGAFILEHEADER{
	byte bIDLength;						//附加信息长度
	byte bPalType;						//调色板信息(不支持)
	byte bImageType;					//图象类型(只支持 3,10)

	unsigned short wPalIndex;			//调色板第一个索引值
	unsigned short wPalLength;			//调色板索引数
	byte bPalBits;

	unsigned short wLeft;				//图象左端坐标(基本不用)
	unsigned short wBottom;				//图象底端坐标(基本不用)
	unsigned short wWidth;				//图象宽度
	unsigned short wHeight;				//图象高度
	byte bBits;							//一个象素位数
	byte bAlphaBits:4;					//Alpha位数
	byte bReserved:1;					//保留,必须为0
	byte bTopDown:1;					//为1表示从上到下(只支持从下到上)
	byte bInterleaving:2;				//隔行(一般为0)
};
#pragma pack (pop)


void zoom_rgb(PIXEL24* rgb, int &width,int &height, float zoom);
void zoom_alpha(byte* alpha, int &width,int &height, float zoom);
static bool load_tga_buffer(byte* tgadata, byte **p_rgb_buf, byte **p_alpha_buf,int &width,int &height,int &kx,int &ky);

ltga_t ltga_load(const char *path)
{
	FILE* tga = fopen(path, "rb");
	if (!tga) {
		return NULL;
	}
	fseek(tga, 0, SEEK_END);
	int len = ftell(tga);
	byte* tgadata = new byte[len];
	if (!tgadata || len <= 0) {
		return NULL;
	}
	fseek(tga, 0, SEEK_SET);
	if (fread(tgadata, 1, len, tga) != len) {
		delete tgadata;
		return NULL;
	}
	ltga_t tga_ = ltga_load_buffer((char *)tgadata);
	delete tgadata;
	return tga_;
}

ltga_t ltga_load_buffer(const char *buff)
{
	ltga_t tga = (ltga_t)malloc(sizeof *tga);
	load_tga_buffer((byte *)buff, (byte **)&tga->rgb_buf, (byte **)&tga->alpha_buf, tga->width, tga->height, tga->offsetx, tga->offsety);
	tga->valid_height=tga->height;
	tga->valid_width=tga->width;
	return tga;
}

void ltga_clip(ltga_t tga)
{	
	// 对空白边界进行裁减
	int csw, cew, csh, ceh;
	csw = 0;
	csh = 0;
	cew = tga->width-1;
	ceh = tga->height-1;
	int i, j;
	unsigned char *alpha_buf = (unsigned char *) tga->alpha_buf;
	//csw -- 左边
	for (i=0; i<tga->width; ++i) {
		for (j=0; j<tga->height; ++j) {
			if (alpha_buf[i+j*tga->width] > 0) {
				csw = i;
				i = tga->width;
				j = tga->height;
			}
		}
	}
	//csh -- 上方
	for (j=0; j<tga->height; ++j) {
		for (i=0; i<tga->width; ++i) {
			if (alpha_buf[i+j*tga->width] > 0) {
				csh = j;
				i = tga->width;
				j = tga->height;
			}
		}
	}
	//cew -- 右边
	for (i=tga->width-1; i>=0; --i) {
		for (j=0; j<tga->height; ++j) {
			if (alpha_buf[i+j*tga->width] > 0) {
				cew = i;
				i = -1;
				j = tga->height;
			}
		}
	}
	//ceh -- 下面
	for (j=tga->height-1; j>=0; --j) {
		for (i=0; i<tga->width; ++i) {
			if (alpha_buf[i+j*tga->width] > 0) {
				ceh = j;
				i = tga->width;
				j = -1;
			}
		}
	}
	int nw = cew - csw + 1;
	int nh = ceh - csh + 1;
	tga->valid_width = (nw + 3)/4*4;
	tga->valid_height = (nh + 3)/4*4;
	tga->offsetx = csw;
	tga->offsety = csh;
}

void ltga_save(ltga_t tga,const char *filename)
{
	//tga->offsetx=0;
	//tga->offsety=0;
	//tga->valid_height=tga->height;
	//tga->valid_width=tga->width;
	unsigned char *buff = (unsigned char *)malloc(tga->valid_height * tga->valid_width * 4);
	memset(buff,0,tga->valid_height * tga->valid_width * 4);
	int h = tga->valid_height;
	int w = tga->valid_width;
	
	int k = 0;
	for(int j = 0; j < h && j + tga->offsety < tga->height; j++){
		int k = 0;
		unsigned char *tmp = buff + w * (h-j-1)*4;
		for(int i = 0; i < w && i + tga->offsetx < tga->width; i++ ){
			int index = tga->width*(j + tga->offsety) + i + tga->offsetx;
			unsigned char a = tga->alpha_buf[index];
			unsigned char r = tga->rgb_buf[index * 3 + 2];
			unsigned char g = tga->rgb_buf[index * 3 + 1];
			unsigned char b = tga->rgb_buf[index * 3 + 0];
			tmp[k++] = b;
			tmp[k++] = g;
			tmp[k++] = r;
			tmp[k++] = a;
		}
	}
	TgaImage img;
	img.depth = 32;
	img.width = w;
	img.height = h;
	img.pixels = buff;

	saveTGA(filename, img);
}

void ltga_scale(ltga_t tga, const char *scale)
{
	float s = 0;
	for (int i=0; i<strlen(scale) -1; i ++) {
		s = s*10 + scale[i] - '0';
	}
	s /= 100;
	tga->width *= s;
	tga->height *= s;
	tga->offsetx *= s;
	tga->offsety *= s;
	tga->valid_width *= s;
	tga->valid_height *= s;
}

bool load_tga_buffer(byte* tgadata, byte **p_rgb_buf, byte **p_alpha_buf,int &width,int &height,int &kx,int &ky)
{
	dword dataptr;
	PIXEL24 *line;
	byte *alpha_line,*rgb_buf,*channel;
	int i,j;
	TGAFILEHEADER *head=(TGAFILEHEADER *)tgadata;
	////////////////////////////////////////////////////////
	if ((head->bImageType!=2 && head->bImageType!=10)
		|| head->bBits!=32 || head->bInterleaving!=0 ) {
			//delete tgadata;
			std::cout <<" tag does not match \n";
			return false;
	} 
	std::cout <<"bImageType: " << head->bImageType << std::endl;
	width=(int)head->wWidth,height=(int)head->wHeight;
	rgb_buf=new byte[width*height*3];
	channel=new byte[width*height];
	dataptr=(dword)tgadata+sizeof(TGAFILEHEADER)+head->bIDLength;
	if (head->bTopDown == 0){
		if (head->bImageType==2) {
			// 不压缩
			for (i=height-1;i>=0;i--) {
				line=(PIXEL24*)(rgb_buf+3*width*i);
				alpha_line=channel+width*i;
				for (j=0;j<width;j++) {
					byte red,green,blue,alpha;
					red=*(byte*)dataptr++;
					green=*(byte*)dataptr++;
					blue=*(byte*)dataptr++;
					alpha=*(byte*)dataptr++;
					//alpha=MAX(MAX(MAX(red,green),blue),alpha);
					alpha_line[j]=alpha;
					//line[j].red=red&0xf8;
					//line[j].green=green&0xfc;
					//line[j].blue=blue&0xf8;

					line[j].red=red;
					line[j].green=green;
					line[j].blue=blue;
				}
			}
		}
		else {
			// RLE 压缩
			int run=0,raw=0;
			PIXEL24 runpixel;
			byte runalpha;
			byte red,green,blue,alpha;
			for (i=height-1;i>=0;i--) {
				line=(PIXEL24*)(rgb_buf+3*width*i);
				alpha_line=channel+width*i;
				for (j=0;j<width;) {
					if (run>0) {
						line[j]=runpixel,alpha_line[j]=runalpha,--run;
						++j;
					}
					else if (raw>0) {
						red=*(byte*)dataptr++;
						green=*(byte*)dataptr++;
						blue=*(byte*)dataptr++;
						alpha=*(byte*)dataptr++;
						//					alpha=MAX(MAX(MAX(red,green),blue),alpha);
						alpha_line[j]=alpha;
						//line[j].red=red&0xf8;
						//line[j].green=green&0xfc;
						//line[j].blue=blue&0xf8;
						line[j].red=red;
						line[j].green=green;
						line[j].blue=blue;
						--raw;
						++j;
					}
					else {
						byte packhead=*(byte*)dataptr++;
						if (packhead&0x80) {
							run=(packhead&0x7f)+1;
							red=*(byte*)dataptr++;
							green=*(byte*)dataptr++;
							blue=*(byte*)dataptr++;
							runalpha=*(byte*)dataptr++;
							//						runalpha=MAX(MAX(MAX(red,green),blue),runalpha);
							runpixel.red=red&0xf8;
							runpixel.green=green&0xfc;
							runpixel.blue=blue&0xf8;
						}
						else {
							raw=packhead+1;
						}
					}
				}
			}
		}
	}else{

		if (head->bImageType==2) {
			// 不压缩
			for (i=0;i<height;i++) {
				line=(PIXEL24*)(rgb_buf+3*width*i);
				alpha_line=channel+width*i;
				for (j=0;j<width;j++) {
					byte red,green,blue,alpha;
					red=*(byte*)dataptr++;
					green=*(byte*)dataptr++;
					blue=*(byte*)dataptr++;
					alpha=*(byte*)dataptr++;
					//alpha=MAX(MAX(MAX(red,green),blue),alpha);
					alpha_line[j]=alpha;
					//line[j].red=red&0xf8;
					//line[j].green=green&0xfc;
					//line[j].blue=blue&0xf8;

					line[j].red=red;
					line[j].green=green;
					line[j].blue=blue;
				}
			}
		}
		else {
			// RLE 压缩
			int run=0,raw=0;
			PIXEL24 runpixel;
			byte runalpha;
			byte red,green,blue,alpha;
			for (i=0;i<height;i++) {
				line=(PIXEL24*)(rgb_buf+3*width*i);
				alpha_line=channel+width*i;
				for (j=0;j<width;) {
					if (run>0) {
						line[j]=runpixel,alpha_line[j]=runalpha,--run;
						++j;
					}
					else if (raw>0) {
						red=*(byte*)dataptr++;
						green=*(byte*)dataptr++;
						blue=*(byte*)dataptr++;
						alpha=*(byte*)dataptr++;
						//					alpha=MAX(MAX(MAX(red,green),blue),alpha);
						alpha_line[j]=alpha;
						//line[j].red=red&0xf8;
						//line[j].green=green&0xfc;
						//line[j].blue=blue&0xf8;
						line[j].red=red;
						line[j].green=green;
						line[j].blue=blue;
						--raw;
						++j;
					}
					else {
						byte packhead=*(byte*)dataptr++;
						if (packhead&0x80) {
							run=(packhead&0x7f)+1;
							red=*(byte*)dataptr++;
							green=*(byte*)dataptr++;
							blue=*(byte*)dataptr++;
							runalpha=*(byte*)dataptr++;
							//						runalpha=MAX(MAX(MAX(red,green),blue),runalpha);
							runpixel.red=red&0xf8;
							runpixel.green=green&0xfc;
							runpixel.blue=blue&0xf8;
						}
						else {
							raw=packhead+1;
						}
					}
				}
			}
		}
	}
	kx = 0;
	ky = 0;
	*p_rgb_buf = rgb_buf;
	*p_alpha_buf = channel;
	//delete tgadata;
	return true;
}

void ltga_free(ltga_t tga)
{
	free(tga->rgb_buf);
	free(tga->alpha_buf);
	free(tga);
	tga = 0;
}

void adjust_color(PIXEL24 *buf, byte *channel,int width,int height)
{
	int i,j;
	byte *old_chan = channel;
	PIXEL24 back;
	PIXEL24 *old_buf = buf;
	for (i=0;i<height;i++) 
		for (j=0;j<width;j++,++buf,++channel) 
			if (*channel==0) goto _break;
_break:
	channel = old_chan;
	if (*channel!=0) {
		back.red=0;
		back.green=0;
		back.blue=0;
	}
	else back=*buf;
	buf = old_buf;
	for (i=0;i<height;i++) {
		for (j=0;j<width;j++,++buf,++channel) {
			int alpha=*channel;
			if (alpha==0) {
				buf->red=buf->green=buf->blue=0;
			}
			else if (alpha>0 && alpha<255) {
				int t;
				t=((int)buf->red*255-back.red*(255-alpha)+alpha/2)/alpha;
				buf->red=MAX(MIN(t,255),0);
				t=((int)buf->green*255-back.green*(255-alpha)+alpha/2)/alpha;
				buf->green=MAX(MIN(t,255),0);
				t=((int)buf->blue*255-back.blue*(255-alpha)+alpha/2)/alpha;
				buf->blue = MAX(MIN(t,255),0);

			}
			*channel>>=3;
		}
	}
}


// 缩放24位位图
void zoom_rgb(PIXEL24* rgb, int &width,int &height, float zoom)
{
	int new_w = width * zoom;
	int new_h = height * zoom;
	PIXEL24* tmp_buf = new PIXEL24[new_w*new_h];
	for (int j=0; j<new_h; ++j) {
		for (int i=0; i<new_w; ++i) {
			tmp_buf[i + j * new_w] = rgb[static_cast<int>(i/zoom) + static_cast<int>(j/zoom)*width];
		}
	}
	memcpy(rgb, tmp_buf, new_w*new_h*sizeof(PIXEL24));
	width = new_w;
	height = new_h;
	delete tmp_buf;
}

void zoom_alpha(byte* alpha, int &width,int &height, float zoom)
{
	int new_w = width * zoom;
	int new_h = height * zoom;
	byte* tmp_buf = new byte[new_w*new_h];
	for (int j=0; j<new_h; ++j) {
		for (int i=0; i<new_w; ++i) {
			tmp_buf[i + j * new_w] = alpha[static_cast<int>(i/zoom) + static_cast<int>(j/zoom)*width];
		}
	}
	memcpy(alpha, tmp_buf, new_w*new_h);
	width = new_w;
	height = new_h;
	delete tmp_buf;
}
