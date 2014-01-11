#ifndef _TGA_H
#define _TGA_H

#include <stdio.h>


typedef struct ltga_s {
	int width;
	int height;
	int offsetx;
	int offsety;
	int valid_width;
	int valid_height;
	char *rgb_buf;
	char *alpha_buf;
} *ltga_t;

ltga_t ltga_load(const char *filename);
void ltga_save(ltga_t tga,const char *filename);

ltga_t ltga_load_buffer(const char *buff);
void ltga_clip(ltga_t tga);
void ltga_free(ltga_t tga);
void ltga_scale(ltga_t tga, const char *scale);

#endif
