#include "vtf.h"

void color565_to_bytes(u8* c565, u8* c) {
	c[0] = c565[1] >> 3;
	c[1] = ((c565[1] & 7) << 3) | (c565[0] >> 5);
	c[2] = c565[0] & 31;
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
		int rd = fgetc(fp);// if (rd == EOF) printf("no no bad file\n");
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

void decode_dxt5(FILE* fp, int width, int height, CColor* data) {
	for (int y = 0; y < height; y += 4) {
	for (int x = 0; x < width; x += 4) {
		int offset = x + y * width;
		fseek(fp, 8, SEEK_CUR);
		decode_dxt1_block(fp, width, &data[offset]);
	}}
}

void decode_dxt(FILE* fp, VTFHEADER* vtfhp, int width, int height, CColor* data) {
	switch (vtfhp->hriFmt) {
	case 0x0d:
		decode_dxt1(fp, width, height, data);
		break;
	case 0x0f:
		decode_dxt5(fp, width, height, data);
		break;
	}
}

void skip_dxt1(FILE* fp, int width, int height) {
	fseek(fp, width * height / 2, SEEK_CUR);
}

CColor* decode_alloc_dxt1(FILE* fp, int width, int height) {
	CColor* data = malloc(sizeof(CColor) * width * height);
	decode_dxt1(fp, width, height, data);
	return data;
}

CColor* decode_alloc_dxt5(FILE* fp, int width, int height) {
	CColor* data = malloc(sizeof(CColor) * width * height);
	decode_dxt5(fp, width, height, data);
	return data;
}

void seek_ghrimm_dxt1(FILE* fp, VTFHEADER* vtfhp, int frameNm) {
	// skip to 4x4
	int size = 4;
	int start = 0x10;
	for (int i = 2; i < vtfhp->mpmCt - 1; ++i) {
		start += size * size / 2;
		size *= 2;
	}
	start *= vtfhp->frameCt;
	start += 0xe8 + frameNm * size * size / 2;
	fseek(fp, start, SEEK_SET);
}

void seek_ghrimm_dxt5(FILE* fp, VTFHEADER* vtfhp, int frameNm) {
	// skip to 4x4
	int size = 4;
	int start = 0x10;
	for (int i = 2; i < vtfhp->mpmCt - 1; ++i) {
		start += size * size;
		size *= 2;
	}
	start *= vtfhp->frameCt;
	start += 0xe8 + frameNm * size * size + 16;
	fseek(fp, start, SEEK_SET);
}

void seek_ghrimm_dxt(FILE* fp, VTFHEADER* vtfhp, int frameNm) {
	switch (vtfhp->hriFmt) {
	case 0xd:
		seek_ghrimm_dxt1(fp, vtfhp, frameNm);
		break;
	case 0xf:
		seek_ghrimm_dxt5(fp, vtfhp, frameNm);
		break;
	}
}
