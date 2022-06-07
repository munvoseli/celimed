#include "gl2d.c"


int main(void) {
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
	float pixels[10000 * 3];
	int i = 0;
	for (int y = 0; y < 100; ++y)
	for (int x = 0; x < 100; ++x) {
		float v = ((float) x - (float) y + 100) / 200 + ((x ^ y) & 1 ? 0.05 : -0.05);
		pixels[i++] = v;
		pixels[i++] = v;
		pixels[i++] = v;
	}
	glTexImage2D(GL_TEXTURE_2D, 0,
		GL_RGB, 100, 100, 0, GL_RGB, GL_FLOAT, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glGenerateMipmap(GL_TEXTURE_2D);
	Paxet p = gl_shaderSetupTexture();
	for (;;) {
		while (SDL_PollEvent(&ev))
			if (ev.type == SDL_QUIT)
				goto end;
		int w, h;
		SDL_GetWindowSize(winp, &w, &h);
		glViewport(0, 0, w, h);
		glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float points[] = {
			1.0, 1.0, 1.0, 1.0,
			-1.0, 1.0, 0.0, 1.0,
			1.0, -1.0, 1.0, 0.0,
			-1.0, -1.0, 0.0, 0.0,
			-1.0, 1.0, 0.0, 1.0,
			1.0, -1.0, 1.0, 0.0,
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
