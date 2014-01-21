#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <zlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#include "tga.h"
#include "linecompress.h"

#include <snappy.h>

using namespace std;

/*
#define FMT_DXT5 D3DFMT_DXT5
#define FMT_A8R8G8B8 D3DFMT_A8R8G8B8
#define FMT_PNG 100
*/

#define FMT_DXT5 fmt_dxt5
#define FMT_A8R8G8B8 fmt_a8r8g8b8
#define FMT_PNG fmt_a8r8g8b8

#define FS_FSI_FILE_ID 'FSI'
#define FS_MASK_FILE_ID 'MASK'
#define FS_FSI_VERSION 18

struct {
	char *scale_str;
	char scale_num;
	bool save_mask;
} global_option;

ltga_t process_tga(const char *filename)
{
	ltga_t tga = ltga_load(filename);
	return tga;
}

void version()
{
	//printf("============= puppy animation pack tool (v1.9) ===============\n");
}

void usage()
{
	printf("   or:tga2fsi.exe -i input_file -o output.fsi -s 50%%\n");
	exit(-1);
}

#define MASK_SCALE 1 
FILE *save_mask(const char *output_file,
					std::vector<ltga_t> tga_list) {	

	FILE *f;
	DWORD flen;
	int i;

	char filename[1024];
	snprintf(filename, sizeof(filename), "%s", output_file);
	f = fopen(filename, "wb");

	ltga_t tga = tga_list[0];
	unsigned char *alpha_buff = (unsigned char *)malloc(tga->valid_width * tga->valid_height);
	for (uint32 ij=0; ij<tga->valid_height; ij += 1){
		for (uint32 ii=0; ii<tga->valid_width; ii += 1) {
			unsigned char *alpha = alpha_buff + ij*tga->valid_width + ii;
			if (ij>=tga->height || ii>=tga->width) {
				*alpha=0;
			} else {
				if((uint8)*(tga->alpha_buf + (ij + tga->offsety)*tga->width + ii + tga->offsetx) > 1){
					*alpha = 1;
				} else {
					*alpha=0;
				}
			}
		}
	}
	
	char *buff2 = new char[1024*1024*10];
	int32 len = tga->valid_width * tga->valid_height;

	mask_compress((char *)alpha_buff, tga->valid_width, tga->valid_height, MASK_SCALE, buff2, len);

	if (global_option.save_mask) {
		printf("saving mask:%s\n",output_file );
		fwrite(buff2, len, 1, f);
        printf("save ended\n");
		delete buff2;
	} 
	return f;
}

int main(int argc, char *argv[])
{
	bool need_save = false; //true;
	char *path;
	char output[255];
	int kx,ky;
	D3DFORMAT format;

	int t_process=0, t_mask = 0, t_texture = 0, t_total = 0;
	int t;
	t = time(NULL);

	version();

	const char *def_format = "dxt5";

	char *input_dir = 0;
	char *input_file = 0;
	char *output_file = 0;
	char *format_string = (char *)def_format;

	global_option.save_mask = true;

	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "d:i:o:f:ms:")) != -1){
		switch(c){
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case '?':
			usage();
		default:
			break;
		}
	}

	if (!output_file || (!input_file && !input_dir) || (input_dir && input_file)) {
		usage();
	}

	vector<ltga_t> tga_list;

	if(input_file){
		ltga_t tga = process_tga(input_file);
		tga_list.push_back(tga);
	}
	
	FILE *f = save_mask(output_file, tga_list);
         
	return 0;
}
