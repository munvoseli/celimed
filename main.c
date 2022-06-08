#include "gl2d.c"
#include "vtf.c"

#define IMW 128

void set_pixels(FILE* fp, int offset, float* pixels) {
	CColor data[IMW * IMW];
	//fseek(fp, offset, SEEK_SET);
	decode_dxt1(fp, IMW, IMW, data);
	int i = 0;
	int j = 0;
	for (int y = 0; y < IMW; ++y) {
	for (int x = 0; x < IMW; ++x) {
		pixels[i++] = data[j].r / 31.0;
		pixels[i++] = data[j].g / 63.0;
		pixels[i++] = data[j].b / 31.0; ++j;
	}}
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
	float pixels[IMW * IMW * 3];
	FILE* fp = fopen(argv[1], "rb");
	VTFHEADER vtfh;
	if (read_x50_header(&vtfh, fp) > 0) printf("failed file read\n");
	int offset = 0x18b0;//0x2ba0;
	// 16 (lri): 0x68
	// 4: 0xf8 = o + 0x10
	// 8: 0x100 = o + 0x18
	// 16: 0x120 = o + 0x38
	// 32: 0x1a0 = o + 0xb8
	// 64: 0x3a0 = o + 0x2b8
	// 128: 0x3a0 + 64 * 32 = o + 0xab8
	// 256: 0x2ba0 = o + 0x2ab8
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glGenerateMipmap(GL_TEXTURE_2D);
	Paxet p = gl_shaderSetupTexture();
	seek_ghrimm_dxt1(fp, &vtfh, 0);
	for (;;) {
		while (SDL_PollEvent(&ev))
			if (ev.type == SDL_QUIT)
				goto end;
		int w, h;
		SDL_GetWindowSize(winp, &w, &h);
		glViewport(0, 0, w, h);
		set_pixels(fp, offset, pixels);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGB, IMW, IMW, 0, GL_RGB, GL_FLOAT, pixels);
		glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		offset += 1;
		if (offset >= vtfh.frameCt) { offset = 0; seek_ghrimm_dxt1(fp, &vtfh, 0); }
		SDL_Delay(100);
	}
	end:
	SDL_GL_DeleteContext(conp);
	SDL_Quit();
	return 0;
}
