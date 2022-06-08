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

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	FILE* fp;
	VTFHEADER vtfh;
	int offset;
	fp = read_vtf(argv[1], &vtfh, &offset);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glGenerateMipmap(GL_TEXTURE_2D);
	Paxet p = gl_shaderSetupTexture();
	int ti = 1;
	for (;;) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				goto end;
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_f) {
				++ti;
				if (argv[ti] == NULL) ti = 1;
				fclose(fp);
				fp = read_vtf(argv[ti], &vtfh, &offset);
			}
		}
		int w, h;
		SDL_GetWindowSize(winp, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		set_next_glvtftex(fp, &vtfh, &offset);
		float points[] = {
			1.0, -1.0, 1.0, 1.0,
			-1.0, -1.0, 0.0, 1.0,
			1.0, 1.0, 1.0, 0.0,
			-1.0, 1.0, 0.0, 0.0,
			-1.0, -1.0, 0.0, 1.0,
			1.0, 1.0, 1.0, 0.0,
		};
		drawWithTexture(sizeof(points), points, GL_TRIANGLES,
			texture, &p, 0.0, 0.0, w * 0.001, h * 0.001);
		SDL_GL_SwapWindow(winp);
		SDL_Delay(100);
	}
	end:
	SDL_GL_DeleteContext(conp);
	SDL_Quit();
	return 0;
}
