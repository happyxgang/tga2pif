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

#pragma pack (push)
#pragma pack (1)

	//////////////////////////////////////////////////////////////////////////
	//*.fsi
	struct fsi_frame_info
	{
		int16 offx;
		int16 offy;
		uint16 valid_width;
		uint16 valid_height; 
		uint32 len; 
		uint32 offset; 
		uint8 unused[4];
	};

	struct fsi_header
	{
		uint32 id; // FSI

		uint8  version; 
		uint8  num_frame;

		uint32 format;
		uint32 speed;

		uint16 kx;
		uint16 ky; 

		uint16 width;
		uint16 height; 

		int32 mask_scale;
		int32 mask_width;
		int32 mask_height;
		int32 mask_offset;
		int32 mask_len;

		int32 texture_offset;
		int32 texture_len;

		int8 scale;
		int8 unused[3];

		//fsi_frame_info frames[0];
		//void *mask_data;
		//void *texture_data;
	};
	

	struct fsi_texture{
		uint32 frame;
		struct fsi_frame_info info;
		//LPDIRECT3DTEXTURE9 texture;
	};

#pragma pack (pop)

struct {
	char *scale_str;
	char scale_num;
	bool save_mask;
} global_option;

ltga_t process_tga(const char *filename)
{
	ltga_t tga = ltga_load(filename);
	ltga_clip(tga);
	return tga;
}

int get_dir(const char *path)
{
	char p[255];
	char *str_dir = NULL;
	int state = 0;
	strcpy(p, path);

	for( int i=0; i<strlen(p); i++ ){
		switch( state ){
		case 0: 
			if( p[i] != '0' ){
				str_dir = p + i;
				state = 1;
			}
			break;
		case 1: 
			if( p[i] == '\\' || p[i] == '/' ){
				p[i] = 0;
				state = 2;
			}
			break;
		default:
			break;
		}
	}
	return atoi(str_dir);
}

void version()
{
	printf("============= puppy animation pack tool (v1.9) ===============\n");
}

void usage()
{
	printf("usage:tga2fsi.exe -d dir -o output.fsi -f [dxt5|argb8|png] -s 50%%\n");
	printf("   or:tga2fsi.exe -i input_file -o output.fsi -f [dxt5|argb8|png] -s 50%%\n");
	exit(-1);
}

bool get_files(char *dir, vector<string> &files)
{
	DIR * dp = opendir(dir);
	if (dp == NULL) {
		return false;	
	}

	struct dirent *dirp;
	while((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0 ||
				strcmp(dirp->d_name, "..") == 0) {
			continue;
		}
		files.push_back(string(dirp->d_name));
	}

	return true;
}

void get_texture_list(char *input_dir, fsi_header &header, std::vector<struct fsi_texture> &tex_list, vector<ltga_t> &tgas)
{
	vector<std::string> files;
	vector<std::string> tgafiles;

	if (!get_files(input_dir, files)) {
		printf(">>> input directory error!");
		exit(1);
	}

	size_t i;
	for( i =0; i< files.size(); i++ ){
		if( files[i].find(".tga") == files[i].length() - 4 ){
			tgafiles.push_back(files[i]);
		}
	}

	uint32 total_len = 0;
	header.speed = 4;// 
	header.id = FS_FSI_FILE_ID;
	header.version = FS_FSI_VERSION;
	header.num_frame = (uint8)tgafiles.size();

	for( i=0; i<tgafiles.size(); i++ ){
		int32 len = tgafiles[i].size();
		if( len > 40 ) len -= 40;
		printf(">>> processing tga file %s... \n", tgafiles[i].c_str());
		struct fsi_texture tex;
		ltga_t tga = process_tga((string(input_dir) + "/" + tgafiles[i]).c_str());
		tga->valid_width = (tga->valid_width + 3)/4*4;
		tga->valid_height = (tga->valid_height + 3)/4*4;
		tgas.push_back(tga);

		tex.frame = (uint8)i;
		tex.info.offx = tga->offsetx;
		tex.info.offy = tga->offsety;
		tex.info.valid_height = tga->valid_height;
		tex.info.valid_width = tga->valid_width;

		printf("tex info: offx=%d, offy=%d, h=%d, w=%d\n", 
				tex.info.offx, tex.info.offy, tex.info.valid_height, tex.info.valid_width);

		tex_list.push_back(tex);
		header.width = tga->width;
		header.height = tga->height;
	}

	if(header.kx == 0 && header.ky == 0){
		if( header.height == 320 ){
			header.kx = 160;
			header.ky = 220;
		}else if ( header.width == 500 ){
			header.kx = 250;
			header.ky = 320;
		}else if ( header.width == 800){
			header.kx = 400;
			header.ky = 460;
		}
	}

	printf("\n");
	printf(">>> tga file process done\n");
}
void print_info(fsi_frame_info* info ){
		printf("offx%u\t\n", info->offx);
		printf("offy%u\t\n", info->offy);
		printf("valid_width%u\t\n",info->valid_width);
		printf("valid_height%u\t\n", info->valid_height);
		printf("len%u\t\n", info->len);
		printf("offset%u\t\n", info->offset);
}
#define MASK_SCALE 2
FILE *save_mask(const char *output_file,
					struct fsi_header &header,
					std::vector<struct fsi_texture> &tex_list,
					vector<ltga_t> tga_list) {	
	//------------------------------------------------------------------------------
	/**
	* 
	* fsi header:
	*
	* header:
	* 4 block_number
	*
	* blocks:
	* 4 data_len
	* 4 width
	* 4 height
	* data_len data
	*
	* ...
	*/

	FILE *f;
	DWORD flen;
	int i;

	char filename[1024];
	snprintf(filename, sizeof(filename), "%s", output_file);
	f = fopen(filename, "wb");

	//fwrite(&header, sizeof(struct fsi_header), 1, f);

	for (i=0; i<tex_list.size(); i++) {
		struct fsi_texture *tex = &tex_list[i];
		fwrite(&tex->info, sizeof(struct fsi_frame_info), 1, f);
		print_info(&tex->info);
	}

	header.mask_scale = MASK_SCALE;

	ltga_t tga = tga_list[0];

	unsigned char *alpha_buff = (unsigned char *)malloc(tga->valid_width * tga->valid_height);
	for (uint32 ij=0; ij<tga->valid_height; ij += 1){
		for (uint32 ii=0; ii<tga->valid_width; ii += 1) {
			unsigned char *alpha = alpha_buff + ij*tga->valid_width + ii;
			if (ij>=tga->height || ii>=tga->width) {
				*alpha=0;
			} else {
				if((uint8)*(tga->alpha_buf + (ij + tga->offsety)*tga->width + ii + tga->offsetx) > 50){
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
		printf("saving mask\n" );
		header.mask_width = tga->valid_width;
		header.mask_height = tga->valid_height;
		header.mask_len = len;
		header.mask_offset = ftell(f);

		//fseek(f, 0, SEEK_SET);
		//fwrite(&header, sizeof(struct fsi_header), 1, f);
		//fseek(f, header.mask_offset, SEEK_SET);
		fwrite(buff2, len, 1, f);
		header.texture_offset = ftell(f);
		delete buff2;
	} else {
		header.mask_width = 0;
		header.mask_height = 0;
		header.mask_len = 0;
		header.mask_offset = 0;
		header.texture_offset = ftell(f);
	}
	return f;
}


void save_texture(FILE *f, 
						struct fsi_header &header, 
						std::vector<struct fsi_texture> &tex_list,
						vector<ltga_t> tga_list, 
						int32 format)
{

	DWORD flen;
	int i;

	char filename[1024];

	fseek(f, header.texture_offset, SEEK_SET);
	header.texture_len = 0;
	for( i=0; i<header.num_frame; i++ ) {
		ltga_t tga = tga_list[i];
		ltga_save(tga, "1.tga");

		if (format == FMT_DXT5) {
			printf(">>> dds format not supported!");
			exit(1);
		} else if (format == FMT_PNG) {
			if (global_option.scale_str) {
				char command[255];
				sprintf(command, "convert -scale %s 1.tga 1.png", global_option.scale_str);
				printf("execute:%s\n", command);
				system(command);
				ltga_scale(tga, global_option.scale_str);
			} else {
				system("convert 1.tga 1.png\n");
			}
			system("png8 -O 1.png\n");
			
			FILE *file = fopen("1.png", "rb");
			fseek(file, 0, SEEK_END);
			int32 file_len = ftell(file);
			fseek(file, 0, SEEK_SET);
			char *image_buffer = (char *)malloc(file_len);
			fread(image_buffer, 1, file_len, file);
			fclose(file);
			system("rm 1.tga 1.png");


			tex_list[i].info.offx = tga->offsetx;
			tex_list[i].info.offy = tga->offsety;
			tex_list[i].info.valid_width = tga->valid_width;
			tex_list[i].info.valid_height = tga->valid_height;
			tex_list[i].info.len = file_len;
			tex_list[i].info.offset = ftell(f);
			header.texture_len += file_len;

			fwrite(image_buffer, file_len, 1, f);
			free(image_buffer);
		}

	}

	fseek(f, 0, SEEK_SET);

	fwrite(&header, sizeof(struct fsi_header), 1, f);

	for( i=0; i<tex_list.size(); i++ ){
		struct fsi_texture *tex = &tex_list[i];
		fwrite(&tex->info, sizeof(struct fsi_frame_info), 1, f);
	}

	fclose(f);
}

void get_keypoint(const char *output_file, int32 &kx, int32 &ky)
{
	FILE *file = fopen(output_file, "r");
	if( file != INVALID_HANDLE_VALUE){
		struct fsi_header header;
		fread(&header, sizeof(struct fsi_header), 1, file);
		fclose(file);
		kx = header.kx;
		ky = header.ky;

		char command[1024];
		sprintf(command, "rm %s", output_file);
		system(command);
	}else{
		kx = 0;
		ky = 0;
	}
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

	global_option.scale_str = "100%";
	global_option.scale_num = 100;
	global_option.save_mask = true;

	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "d:i:o:f:ms:")) != -1){
		switch(c){
		case 'd':
			input_dir = optarg;
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'f':
			format_string = optarg;
			break;
		case 'm':
			global_option.save_mask = true;
			break;
		case 's':
			global_option.scale_str = optarg;
			break;
		case '?':
			usage();
		default:
			break;
		}
	}

	char s = 0;
	for (int i=0; i<strlen(global_option.scale_str) -1; i ++) {
		s = s*10 + global_option.scale_str[i] - '0';
	}
	global_option.scale_num = s;

	if (!output_file || (!input_file && !input_dir) || (input_dir && input_file)) {
		usage();
	}

	get_keypoint(output_file, kx, ky);
	printf("file keypoint:%d,%d\n", kx, ky);

	if ( strcmp(format_string, "dxt5") == 0 ){
		format = FMT_DXT5;
	} else if (strcmp(format_string, "argb8") == 0){
		format = FMT_A8R8G8B8;
	} else if (strcmp(format_string, "png") == 0) {
		format = (D3DFORMAT)FMT_PNG;
	} else {
		format = FMT_DXT5;
	}

	fsi_header header;
	header.format = format;
	header.kx = kx;
	header.ky = ky;
	header.scale = global_option.scale_num;

	std::vector<struct fsi_texture> tex_list;

	vector<ltga_t> tga_list;

	if( input_dir ){ 
		get_texture_list(input_dir, header, tex_list, tga_list);
	}else if(input_file){
		ltga_t tga = process_tga(input_file);
		tga->valid_width = (tga->width + 3)/4*4;
		tga->valid_height = (tga->height + 3)/4*4;
		tga->offsetx = 0;
		tga->offsety = 0;

		header.format = format;
		header.speed = 4;
		header.kx = 0;
		header.ky = 0;
		header.id = FS_FSI_FILE_ID;
		header.version = FS_FSI_VERSION;
		header.num_frame = 1;
		header.width = (tga->width + 3)/4*4;
		header.height = (tga->height + 3)/4*4;

		struct fsi_texture tex;
		tex.frame = 0;
		tex.info.offx = 0;
		tex.info.offy = 0;
		tex.info.valid_height = tga->valid_height;
		tex.info.valid_width = tga->valid_width;

		printf("tex info: offx=%d, offy=%d, h=%d, w=%d\n", 
				tex.info.offx, tex.info.offy, tex.info.valid_height, tex.info.valid_width);

		tex_list.push_back(tex);

		tga_list.push_back(tga);
	}
	
	FILE *f = save_mask(output_file, header, tex_list,tga_list);
	//save_texture(f, header, tex_list,tga_list, format);

	printf("input:%s\nframes:%d\noutput:%s\n",input_dir?input_dir:input_file, header.num_frame, output_file);
	return 0;
}
