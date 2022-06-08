#include <stdio.h>
#include "vtf.h"


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
		printf("High-res image format: %s (0x%x)\n",
		format_names[vtfhp->hriFmt], vtfhp->hriFmt);
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

#include "dxt.c"

void do_72(VTFHEADER* vtfhp) {
	
}

void do_73(VTFHEADER* vtfhp, FILE* fp, CColor* data, int offset) {
	for (int i = 0; i < vtfhp->resCt; ++i)
		fseek(fp, 8, SEEK_CUR);
	CColor* lri = decode_alloc_dxt1(fp, 16, 16);
	fseek(fp, offset, SEEK_SET);
	decode_dxt1(fp, 32, 32, data);
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
