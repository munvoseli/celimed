
int ofst_rgba8(VTFHEADER* vtfhp, int fNm) {
	
}

int ofst_dxt1(VTFHEADER* vtfhp, int fNm) {
	int size = 4;
	int x = 0x10;
	for (int i = 2; i < vtfhp->mpmCt - 1; ++i) {
		x += size * size / 2;
		size *= 2;
	}
	x *= vtfhp->frameCt;
	x += fNm * size * size / 2;
	return x;
}

int ofst_dxt5(VTFHEADER* vtfhp, int fNm) {
	int size = 4;
	int x = 0x10;
	for (int i = 2; i < vtfhp->mpmCt - 1; ++i) {
		x += size * size;
		size *= 2;
	}
	x *= vtfhp->frameCt;
	x += fNm * size * size + 16;
	return x;
}

// seeks to highest resolution in mipmap
void seekto_hri_frame(FILE* fp, VTFHEADER* vtfhp, int fNm) {
	int offset;
	switch (vtfhp->hriFmt) {
	case 0xd:
		offset = ofst_dxt1(vtfhp, fNm);
		break;
	case 0xf:
		offset = ofst_dxt5(vtfhp, fNm);
		break;
	default:
		printf("No support for format %d\n", vtfhp->hriFmt);
		return;
	}
}

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
	printf("%d mipmaps, %d frames\n", vtfhp->mpmCt, vtfhp->frameCt);
	printf("Depth: %d\n", vtfhp->depth);
	printf("Header size: %d 0x%x\n", vtfhp->headerBct, vtfhp->headerBct);
	printf("Version %x.%x\n", vtfhp->version[0], vtfhp->version[1]);
	if (vtfhp->version[1] >= 3)
		printf("%d resources\n", vtfhp->resCt);
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



void do_72(VTFHEADER* vtfhp) {
	
}

void do_73(VTFHEADER* vtfhp, FILE* fp, CColor* data, int offset) {
	for (int i = 0; i < vtfhp->resCt; ++i)
		fseek(fp, 8, SEEK_CUR);
	CColor* lri = decode_alloc_dxt1(fp, 16, 16);
	fseek(fp, offset, SEEK_SET);
	decode_dxt1(fp, 32, 32, data);
}

int get_hri_location(FILE* fp, VTFHEADER* vtfhp) {
	if (vtfhp->version[1] == 1)
		return 0x40 + byte_size_fmt(vtfhp->lriFmt, vtfhp->lriW, vtfhp->lriH);
	else if (vtfhp->version[1] == 2)
		return 0x50 + byte_size_fmt(vtfhp->lriFmt, vtfhp->lriW, vtfhp->lriH);
	else {
		fseek(fp, 0x50, SEEK_SET);
		for (int i = 0; i < 10; ++i) {
			int c = fgetc(fp);
			if (c == 0x30) {
				fseek(fp, 3, SEEK_CUR);
				return read_int(fp);
			}
			fseek(fp, 7, SEEK_CUR);
		}
		return 0xe8;
	}
}
