#include "gl2d.c"
#include "vtf.c"

FILE* read_vtf(char* fname, VTFHEADER* vtfhp, int* offsetp) {
	printf("~~ Opening %s\n", fname);
	FILE* fp = fopen(fname, "rb");
	if (read_x50_header(vtfhp, fp) > 0) {
		printf("failed file read\n");
	}
	*offsetp = 0;
	seek_ghrimm_dxt(fp, vtfhp, 0);
	return fp;
}

void set_next_glvtftex(FILE* fp, VTFHEADER* vtfhp, int* offsetp) {
	if (vtfhp->version[1] < 3) return;
	if (*offsetp >= vtfhp->frameCt) {
		*offsetp = 0;
		seek_ghrimm_dxt(fp, vtfhp, 0);
	}
	int w = vtfhp->w;
	int h = vtfhp->h;
	float pixels[w * h * 3];
	CColor data[w * h];
	decode_dxt(fp, vtfhp, w, h, data);
	int i = 0;
	int j = 0;
	for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
		pixels[i++] = data[j].r / 31.0;
		pixels[i++] = data[j].g / 63.0;
		pixels[i++] = data[j].b / 31.0; ++j;
	}}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, pixels);
	*offsetp += 1;
}

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

void draw_frames_and_mipmaps(char* fname, GLuint texture, SDL_Window* winp, Paxet* pp) {
	printf("~~ Opening %s\n", fname);
	FILE* fp = fopen(fname, "rb");
	if (fp == NULL) {
		printf("Couldn't open file %s\n", fname);
		return;
	}
	VTFHEADER vtfh;
	if (read_x50_header(&vtfh, fp) > 0) {
		printf("failed file read\n");
		return;
	}
	if (vtfh.frameCt == 0) vtfh.frameCt = 1; // not in the spec
	int mipmapStart = get_hri_location(fp, &vtfh);
	fseek(fp, mipmapStart, SEEK_SET);
	float rd, gd, bd;
	if (vtfh.hriFmt >= 0xd && vtfh.hriFmt <= 0xf) {
		rd = 31; gd = 63; bd = 31;
	} else {
		rd = 255; gd = 255; bd = 255;
	}
	int ww, wh;
	SDL_GetWindowSize(winp, &ww, &wh);
	glViewport(0, 0, ww, wh);
	switch (vtfh.version[1]) {
	case 0: glClearColor(0.0f, 0.0f, 0.0f, 1.0f); break;
	case 1: glClearColor(0.5f, 0.5f, 0.5f, 1.0f); break;
	case 2: glClearColor(0.0f, 0.0f, 1.0f, 1.0f); break;
	case 3: glClearColor(0.0f, 0.8f, 0.0f, 1.0f); break;
	case 4: glClearColor(0.8f, 0.8f, 0.0f, 1.0f); break;
	case 5: glClearColor(1.0f, 0.0f, 0.0f, 1.0f); break;
	default: glClearColor(1.0f, 1.0f, 1.0f, 1.0f); break;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int k = 0;
	for (int mi = 0; mi < vtfh.mpmCt; ++mi) {
		int w = vtfh.w >> (vtfh.mpmCt - mi - 1);
		int h = vtfh.h >> (vtfh.mpmCt - mi - 1);
		if ((w & 3) || (h & 3)) {
			if (vtfh.hriFmt == 0xd) {
				fseek(fp, ruf(w) * ruf(h) / 2, SEEK_CUR);
				continue;
			} else if (vtfh.hriFmt == 0xe || vtfh.hriFmt == 0xf) {
				fseek(fp, ruf(w) * ruf(h), SEEK_CUR);
				continue;
			}
		}
		CColor* data = malloc(w * h * sizeof(CColor));
		float* pixels = malloc(w * h * 3 * sizeof(float));
		for (int fi = 0; fi < vtfh.frameCt; ++fi) {
			decode_hri_frame(fp, &vtfh, w, h, data);
			int i = 0;
			int j = 0;
			for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				pixels[i++] = data[j].r / rd;
				pixels[i++] = data[j].g / gd;
				pixels[i++] = data[j].b / bd; ++j;
			}}
			float points[] = {
				1.0, -1.0, 1.0, 1.0,
				-1.0, -1.0, 0.0, 1.0,
				1.0, 1.0, 1.0, 0.0,
				-1.0, 1.0, 0.0, 0.0,
				-1.0, -1.0, 0.0, 1.0,
				1.0, 1.0, 1.0, 0.0,
			};
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, pixels);
			float ox = (k % 9 - 4) * 2.1;
			float oy = (k / 9 - 2) * 2.1;
			++k;
			drawWithTexture(sizeof(points), points, GL_TRIANGLES,
				texture, pp, -ox, -oy, ww * 0.005, wh * 0.005);
		}
		free(data);
		free(pixels);
	}
	fclose(fp);
	SDL_GL_SwapWindow(winp);
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window* winp = SDL_CreateWindow("h", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
	SDL_GLContext* conp = gl_setup(winp);
	SDL_Event ev;

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
//	FILE* fp;
//	VTFHEADER vtfh;
//	int offset;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Paxet p = gl_shaderSetupTexture();
	int ti = 1;
	for (;;) {
		draw_frames_and_mipmaps(argv[ti], texture, winp, &p);
		while (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				goto end;
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_j) {
				++ti;
				if (argv[ti] == NULL) ti = 1;
				//fclose(fp);
				//fp = read_vtf(argv[ti], &vtfh, &offset);
				break;
			}
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_k) {
				--ti;
				if (ti == 0) ti = argc - 1;
				//fclose(fp);
				//fp = read_vtf(argv[ti], &vtfh, &offset);
				break;
			}
		}
//		int w, h;
//		SDL_GetWindowSize(winp, &w, &h);
//		glViewport(0, 0, w, h);
//		glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		set_next_glvtftex(fp, &vtfh, &offset);
//		float points[] = {
//			1.0, -1.0, 1.0, 1.0,
//			-1.0, -1.0, 0.0, 1.0,
//			1.0, 1.0, 1.0, 0.0,
//			-1.0, 1.0, 0.0, 0.0,
//			-1.0, -1.0, 0.0, 1.0,
//			1.0, 1.0, 1.0, 0.0,
//		};
//		drawWithTexture(sizeof(points), points, GL_TRIANGLES,
//			texture, &p, 0.0, 0.0, w * 0.01, h * 0.01);
//		SDL_GL_SwapWindow(winp);
		SDL_Delay(100);
	}
	end:
	SDL_GL_DeleteContext(conp);
	SDL_Quit();
	return 0;
}
