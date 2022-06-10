#include <stdio.h>
#include "vtf.h"
#include "gl2d.c"
#include "dxt.c"
#include "fmt.c"
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

char vtfopen(char* fname, FILE** fpp, VTFHEADER* vtfhp) {
	printf("~~ Opening %s\n", fname);
	*fpp = fopen(fname, "rb");
	if (fpp == NULL) {
		printf("Couldn't open file %s\n", fname);
		return 1;
	}
	if (read_x50_header(vtfhp, *fpp) > 0) {
		printf("failed file read\n");
		return 2;
	}
	if (vtfhp->frameCt == 0) vtfhp->frameCt = 1; // not in the spec
	return 0;
}

void play_anim(char* fname, GLuint texture, SDL_Window* winp, Paxet* pp) {
	VTFHEADER vtfh; FILE* fp;
	if (vtfopen(fname, &fp, &vtfh)) return;
	int fstart = get_hri_location(fp, &vtfh) + fmt_get_hoffset(vtfh.hriFmt,
		vtfh.w, vtfh.h, vtfh.mpmCt, vtfh.frameCt);
	fseek(fp, fstart, SEEK_SET);
	int ww, wh;
	SDL_GetWindowSize(winp, &ww, &wh);
	glViewport(0, 0, ww, wh);
	float rd, gd, bd;
	if (vtfh.hriFmt >= 0xd && vtfh.hriFmt <= 0xf) {
		rd = 31; gd = 63; bd = 31;
	} else {
		rd = 255; gd = 255; bd = 255;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	SDL_Event ev;
	CColor* data = malloc(vtfh.w * vtfh.h * sizeof(CColor));
	float* pixels = malloc(vtfh.w * vtfh.h * 3 * sizeof(float));
	float points[] = {
		1.0, -1.0, 1.0, 1.0,
		-1.0, -1.0, 0.0, 1.0,
		1.0, 1.0, 1.0, 0.0,
		-1.0, 1.0, 0.0, 0.0,
		-1.0, -1.0, 0.0, 1.0,
		1.0, 1.0, 1.0, 0.0,
	};
	for (int fi = 0;;) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_KEYDOWN) {
				free(data);
				free(pixels);
				return;
			}
		}
		++fi;
		if (fi >= vtfh.frameCt) {
			fseek(fp, fstart, SEEK_SET);
			fi = 0;
		}
		
		decode_hri_frame(fp, &vtfh, vtfh.w, vtfh.h, data);
		int i = 0;
		int j = 0;
		for (int k = 0; k < vtfh.w * vtfh.h; ++k) {
			pixels[i++] = data[j].r / rd;
			pixels[i++] = data[j].g / gd;
			pixels[i++] = data[j].b / bd; ++j;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vtfh.w, vtfh.h,
			0, GL_RGB, GL_FLOAT, pixels);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawWithTexture(sizeof(points), points, GL_TRIANGLES,
			texture, pp, 0, 0, ww * 0.001, wh * 0.001);
		SDL_GL_SwapWindow(winp);
		SDL_Delay(100);
	}
}

void draw_frames_and_mipmaps(char* fname, GLuint texture, SDL_Window* winp, Paxet* pp) {
	VTFHEADER vtfh; FILE* fp;
	if (vtfopen(fname, &fp, &vtfh)) return;
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
	for (int mi = vtfh.mpmCt - 1; mi >= 0; --mi) {
		int w = vtfh.w >> mi;
		int h = vtfh.h >> mi;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Paxet p = gl_shaderSetupTexture();
	int ti = 1;
	for (;;) {
		draw_frames_and_mipmaps(argv[ti], texture, winp, &p);
		while (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				goto end;
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_a) {
				play_anim(argv[ti], texture, winp, &p);
				draw_frames_and_mipmaps(argv[ti], texture, winp, &p);
			}
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_j) {
				++ti;
				if (ti >= argc) ti = 1;
				break;
			}
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_k) {
				--ti;
				if (ti == 0) ti = argc - 1;
				break;
			}
		}
		SDL_Delay(100);
	}
	end:
	SDL_GL_DeleteContext(conp);
	SDL_Quit();
	return 0;
}
