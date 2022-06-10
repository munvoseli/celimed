
// assumes that fp is seeked

#define DECODEFOUR(A, B, C, D) for (int i = 0; i < w * h; ++i) {\
	data[i].A = fgetc(fp); data[i].B = fgetc(fp);\
	data[i].C = fgetc(fp); data[i].D = fgetc(fp); }
#define DECODETHREE(A, B, C) for (int i = 0; i < w * h; ++i) {\
	data[i].A = fgetc(fp); data[i].B = fgetc(fp);\
	data[i].C = fgetc(fp); data[i].a = 255; }

char decode_hri_frame(FILE* fp, VTFHEADER* vtfhp, int w, int h, CColor* data) {
	switch (vtfhp->hriFmt) {
	case 0: // rgba8
		DECODEFOUR(r, g, b, a)
		break;
	case 1: // abgr8
		DECODEFOUR(a, b, g, r)
		break;
	case 2: // rgb8
		DECODETHREE(r, g, b)
		break;
	case 3: // bgr8
		DECODETHREE(b, g, r)
		break;
	case 0x0b: // argb8
		DECODEFOUR(a, r, g, b)
		break;
	case 0x0c: // bgra8
		DECODEFOUR(b, g, r, a)
		break;
	case 0x0d:
		decode_dxt1(fp, w, h, data);
		break;
	case 0x0f:
		decode_dxt5(fp, w, h, data);
		break;
	default:
		printf("Couldn't find support for %s (0x%x)\n",
			format_names[vtfhp->hriFmt], vtfhp->hriFmt);
		return 1;
	}
	return 0;
}

int read_int(FILE* fp) {
	return fgetc(fp) | (fgetc(fp) << 8) | (fgetc(fp) << 16) | (fgetc(fp) << 24);
}

int ruf(int x) {
	//return (x & 3) ? (x | 3) + 1 : x;
	return x + ((4 - (x & 3)) & 3);
}

int byte_size_fmt(unsigned int fmt, int w, int h) {
	switch (fmt) {
	case 0x0:
	case 0x1:
	case 0xb:
	case 0xc: return w * h * 4;
	case 0x2:
	case 0x3: return w * h * 3;
	case 0xd: return ruf(w) * ruf(h) / 2;
	case 0xe:
	case 0xf: return ruf(w) * ruf(h);
	}
	return 0;
}

int fmt_get_hoffset(unsigned int fmt, int w, int h, int mpmCt, int fCt) {
	int bs = 0;
	for (int i = 1; i <= mpmCt; ++i) {
		bs += byte_size_fmt(fmt, w >> i, h >> i);
	}
	return bs * fCt;
}
