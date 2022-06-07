#include <stdio.h>

static const char* format_names[] = {
	"RGBA8888",
	"ABGR8888",
	"RGB888",
	"BGR888",
	"RGB565",
	"I8",
	"IA88",
	"P8",
	"A8",
	"RGB888_BLUESCREEN",
	"BGR888_BLUESCREEN",
	"ARGB8888",
	"BGRA8888",
	"DXT1", // 0x0d
	"DXT3",
	"DXT5",
	"BGRX8888",
	"BGR565",
	"BGRX5551",
	"BGRA4444",
	"DXT1_ONEBITALPHA",
	"BGRA5551",
	"UV88",
	"UVWQ8888",
	"RGBA16161616F",
	"RGBA16161616",
	"UVLX8888",
};

// https://developer.valvesoftware.com/wiki/Valve_Texture_Format
typedef struct tagVTFHEADER {
	char            sig[4];
	unsigned int    version[2];  
	unsigned int    headerBct;  
	unsigned short  w;       
	unsigned short  h;      
	unsigned int    flags;       
	unsigned short  frameCt;      
	unsigned short  firstFrame;  
	float           refl[3];
	float           bmpScl;
	unsigned int    hriFmt;
	unsigned char   mpmCt;
	unsigned int    lriFmt;
	unsigned char   lriW;
	unsigned char   lriH;
	// 7.2+
	unsigned short  depth;
	// 7.3+
	unsigned int    resCt;
} VTFHEADER;

char read_x50_header(VTFHEADER* vtfhp, FILE* fp) {
	fread(&vtfhp->sig, 4, 1, fp);
	if (vtfhp->sig[0] != 'V'
	 || vtfhp->sig[1] != 'T'
	 || vtfhp->sig[2] != 'F'
	 || vtfhp->sig[3] != 0) {
		printf("Signature does not match\n");
		return 1;
	}
	fread(&vtfhp->version,   8, 1, fp);
	fread(&vtfhp->headerBct, 4, 1, fp);

	fread(&vtfhp->w,          2, 1, fp);
	fread(&vtfhp->h,          2, 1, fp);
	fread(&vtfhp->flags,      4, 1, fp);
	fread(&vtfhp->frameCt,    2, 1, fp);
	fread(&vtfhp->firstFrame, 2, 1, fp);
	fseek(fp, 4, SEEK_CUR);

	fread(&vtfhp->refl, 12, 1, fp);
	fseek(fp, 4, SEEK_CUR);

	fread(&vtfhp->bmpScl,  4, 1, fp);
	fread(&vtfhp->hriFmt,  4, 1, fp);
	fread(&vtfhp->mpmCt,   1, 1, fp);
	fread(&vtfhp->lriFmt,  4, 1, fp);
	fread(&vtfhp->lriH,    1, 1, fp);
	fread(&vtfhp->lriW,    1, 1, fp);
	fread(&vtfhp->depth,   2, 1, fp); // cut in middle
	fseek(fp, 3, SEEK_CUR);
	fread(&vtfhp->resCt,   4, 1, fp);
	fseek(fp, 8, SEEK_CUR);
	printf("Largest mipmap: %d x %d\n", vtfhp->w, vtfhp->h);
	printf("Low res: %d x %d\n", vtfhp->lriW, vtfhp->lriH);
	printf("%d resources, %d mipmaps, %d frames\n",
		vtfhp->resCt, vtfhp->mpmCt, vtfhp->frameCt);
	printf("Depth: %d\n", vtfhp->depth);
	printf("Header size: %d 0x%x\n", vtfhp->headerBct, vtfhp->headerBct);
	printf("Version %x.%x\n", vtfhp->version[0], vtfhp->version[1]);
	// will NOT work on some endiannesses (BIG)
	if (vtfhp->hriFmt < 27)
		printf("High-res image format: %s\n",
		format_names[vtfhp->hriFmt]);
	else
		printf("High-res image format code: %x\n",
		vtfhp->hriFmt);
	if (vtfhp->lriFmt < 27)
		printf("Low-res image format: %s\n",
		format_names[vtfhp->lriFmt]);
	else
		printf("Low-res image format code: %x\n",
		vtfhp->lriFmt);
	return 0;
}

typedef unsigned char u8;

typedef struct {
	u8 r; u8 g; u8 b; u8 a;
} CColor;

void color565_to_bytes(u8* c565, u8* c) {
	c[0] = c565[0] >> 3;
	c[1] = ((c565[0] & 7) << 3) | (c565[1] >> 5);
	c[2] = c565[1] & 31;
	c[3] = 255;
}

void decode_dxt1_block(FILE* fp, int width, CColor* data) {
	// 64 bits = 8 bytes
	u8 color0[2];
	u8 color1[2];
	CColor cl[4];
	u8 c[8];
	fread(&color0, 2, 1, fp);
	fread(&color1, 2, 1, fp);
	color565_to_bytes(color0, c);
	color565_to_bytes(color1, c + 4);
	cl[0].r = c[0];
	cl[0].g = c[1];
	cl[0].b = c[2];
	cl[0].a = 255;
	cl[1].r = c[4];
	cl[1].g = c[5];
	cl[1].b = c[6];
	cl[1].a = 255;
//	printf("%d %d %d , %d %d %d\n", c[0], c[1], c[2], c[4], c[5], c[6]);
	// may NOT work on some endiannesses (BIG)
	if ((*(short*)color0 >= *(short*)color1) || 1) {
		cl[2].r = (c[0] * 2 + c[4]) / 3;
		cl[2].g = (c[1] * 2 + c[5]) / 3;
		cl[2].b = (c[2] * 2 + c[6]) / 3;
		cl[2].a = 255; 
		cl[3].r = (c[0] + c[4] * 2) / 3;
		cl[3].g = (c[1] + c[5] * 2) / 3;
		cl[3].b = (c[2] + c[6] * 2) / 3;
		cl[3].a = 255;
	} else {
		cl[2].r = (c[0] + c[4]) >> 1;
		cl[2].g = (c[1] + c[5]) >> 1;
		cl[2].b = (c[2] + c[6]) >> 1;
		cl[2].a = 255; 
		cl[3].r = 255;
		cl[3].g = 255;
		cl[3].b = 255;
		cl[3].a = 0;
	}
	for (int y = 0; y < 4; ++y) {
		CColor* row = &data[y * width];
		int rd = fgetc(fp); if (rd == EOF) printf("no no bad file\n");
		row[3] = cl[rd >> 6];
		row[2] = cl[(rd >> 4) & 3];
		row[1] = cl[(rd >> 2) & 3];
		row[0] = cl[rd & 3];
//		printf("%x %d %d %d %d\n", rd, row[0].r, row[1].r, row[2].r, row[3].r);
	}
}

void decode_dxt1(FILE* fp, int width, int height, CColor* data) {
	for (int y = 0; y < height; y += 4) {
	for (int x = 0; x < width; x += 4) {
		int offset = x + y * width;
		decode_dxt1_block(fp, width, &data[offset]);
	}}
}

void skip_dxt1(FILE* fp, int width, int height) {
	fseek(fp, width * height / 2, SEEK_CUR);
}

void do_72(VTFHEADER* vtfhp) {
	
}

void do_73(VTFHEADER* vtfhp, FILE* fp, CColor* data, int offset) {
//	for (int i = 0; i < vtfhp->resCt; ++i)
//		fseek(fp, 8, SEEK_CUR);
	//decode_dxt1(fp, 16, 16, data);
//	skip_dxt1(fp, 16, 16);
	fseek(fp, offset, SEEK_SET);
	decode_dxt1(fp, 32, 32, data);
	printf("%d %d %d %d\n", data[0].r, data[1].r, data[2].r, data[3].r);
}

void grab_data(FILE* fp, CColor* data, int offset, int w) {
	fseek(fp, offset, SEEK_SET);
	decode_dxt1(fp, w, w, data);
}

void fread_dxt1(FILE* fp) {
}

//int main(int argc, char** argv) {
//	for (int i = 1; i < argc; ++i) {
//		FILE* fp = fopen(argv[i], "rb");
//		if (fp == NULL) continue;
//		printf("\n~~ Opened %s\n", argv[i]);
//		VTFHEADER vtfh;
//		if (read_x50_header(&vtfh, fp) > 0) continue;
////		int n = fread(&vtfh, 1, sizeof(VTFHEADER), fp);
////		if (n < sizeof(VTFHEADER)) {
////			printf("File %s not big enough (%d < %d)\n", argv[i],
////			n, sizeof(VTFHEADER));
////			printf("(%x < %x)\n", argv[i], n, sizeof(VTFHEADER));
////		}
//		if (vtfh.version[0] == 7) {
//			switch (vtfh.version[1]) {
//			case 2:
//				do_72(&vtfh);
//				break;
//			case 3:
//			case 4:
//				do_73(&vtfh, fp);
//				break;
//			}
//		}
//	}
//}
