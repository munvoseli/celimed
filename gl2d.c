#define GL_INFO
#define GL_SHADER_DEBUG

// link with -lSDL2 and -lGL

// for 2d rendering with OpenGL in an SDL program
// you'll probably need to change the opengl shading version from 460
// i don't think it'll work for anything less than 330

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengles2.h>

/*
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
SDL_Window* winp = SDL_CreateWindow("h", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
SDL_GLContext* conp = gl_setup(winp);
GLuint shaders = {
SOLID_SHAD("0.0f, 0.0f, 0.0f")
};
int w, h;
SDL_GetWindowSize(winp, &w, &h);
glViewport(0, 0, w, h);
glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
SDL_GL_SwapWindow(winp);
SDL_GL_DeleteContext(conp);
SDL_Quit();
 */
typedef struct Camera {
	float x0;
	float y0;
	float x2;
	float y2;
} Camera;

typedef struct Pavec {
	GLuint prog;
	GLint ploc;
	GLint camloc;
	GLint clrloc;
} Pavec;

typedef struct Paxet {
	GLuint prog;
	GLint ploc;
	GLint txcloc;
	GLint texloc;
	GLint camloc;
} Paxet;

void drawWithTexture(
	int len, const float* points, GLenum mode,
	GLuint texture, Paxet* p,
	float x, float y, float sx, float sy
) {
	glUseProgram(p->prog);
	glUniform4f(p->camloc, x, y, sx, sy);
	glUniform1i(p->texloc, 0);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, len * sizeof(float),
		points, GL_STATIC_DRAW);
	glVertexAttribPointer(p->ploc, 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(p->ploc);
	glVertexAttribPointer(p->txcloc, 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(p->txcloc);
	glDrawArrays(mode, 0, len / (4 * sizeof(float))); // 4 floats / vert
	glDeleteBuffers(1, &vbo);
}

SDL_GLContext* gl_setup(SDL_Window* winp) {
	SDL_GLContext* conp = SDL_GL_CreateContext(winp);
#ifdef GL_INFO
	printf("OpenGL version %s\n"
	       "GLSL version %s\n"
	       "GL vendor %s\n"
	       "GL renderer %s\n",
	       glGetString(GL_VERSION),
	       glGetString(GL_SHADING_LANGUAGE_VERSION),
	       glGetString(GL_VENDOR),
	       glGetString(GL_RENDERER));
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("OpenGL version %d.%d\n", major, minor);
#endif
	glEnable(GL_DEBUG_OUTPUT);
	return conp;
}

void gl_shadlog(GLuint shad, int loglen) {
	if (loglen == 0) return;
	char* msg = malloc(loglen + 1);
	glGetShaderInfoLog(shad, loglen, &loglen, msg);
	printf("%s\n", msg);
	free(msg);
}

GLuint gl_shader_setup(const char* vsrc, const char* fsrc) {
//	char log[512];
	int res = 0;
	GLuint vert_num = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_num, 1, &vsrc, NULL);
	glCompileShader(vert_num);
	glGetShaderiv(vert_num, GL_INFO_LOG_LENGTH, &res);
#ifdef GL_SHADER_DEBUG
	printf("%d == 0  vertex shader compilation\n", res);
	gl_shadlog(vert_num, res);
#endif

	GLuint frag_num = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_num, 1, (const char**) &fsrc, NULL);
	glCompileShader(frag_num);
	glGetShaderiv(frag_num, GL_INFO_LOG_LENGTH, &res);
#ifdef GL_SHADER_DEBUG
	printf("%d == 0  fragment shader compilation\n", res);
	gl_shadlog(frag_num, res);
#endif
//	glGetShaderiv(vert_num, GL_COMPILE_STATUS, &res); res == 1
	GLuint shad = glCreateProgram();
	glAttachShader(shad, vert_num);
	glAttachShader(shad, frag_num);
	glLinkProgram(shad);
	glGetProgramiv(shad, GL_INFO_LOG_LENGTH, &res);
#ifdef GL_SHADER_DEBUG
	printf("%d == 0  shader program link\n", res);
#endif
//	glGetProgramiv(shad, GL_LINK_STATUS, &res);
	glDeleteShader(vert_num);
	glDeleteShader(frag_num);
	return shad;
}

Pavec gl_shaderSetupUniform() {
	const char* vert_src =
		"#version 150 core\n"
		"in vec2 aPos;"
		"uniform vec4 cam;"
		"void main() {"
		"gl_Position = vec4((aPos.x - cam.x) / cam.z,"
		"(aPos.y - cam.y) / cam.w, 0.0, 1.0);"
		"}";
	const char* frag_src =
		"#version 150 core\n"
		"out vec4 FragColor;"
		"uniform vec3 colorin;"
		"void main() {"
		"FragColor = vec4(colorin, 1.0f);"
		"}";
	Pavec pavec;
	pavec.prog = gl_shader_setup(vert_src, frag_src);
	pavec.ploc = glGetAttribLocation(pavec.prog, "aPos");
	pavec.camloc = glGetUniformLocation(pavec.prog, "cam");
	pavec.clrloc = glGetUniformLocation(pavec.prog, "colorin");
#ifdef GL_SHADER_DEBUG
	printf("Uni shader: %d %d %d\n", pavec.ploc,
	       pavec.camloc, pavec.clrloc);
	printf("    none of these should be -1\n");
#endif
	return pavec;
}

Paxet gl_shaderSetupTexture() {
	const char* vert_src =
		"#version 150 core\n"
		"in vec2 aPos;"
		"in vec2 aTexCoord;"
		"out vec2 TexCoord;"
		"uniform vec4 cam;"
		"void main() {"
		"gl_Position = vec4((aPos.x - cam.x) / cam.z,"
		"(aPos.y - cam.y) / cam.w, 0.0, 1.0);"
		//"gl_Position = vec4(aPos, 0.0, 1.0);"
		"TexCoord = aTexCoord;"
		"}";
	const char* frag_src =
		"#version 150 core\n"
		"out vec4 FragColor;"
		"uniform sampler2D gerald;"
		"in vec2 TexCoord;"
		"void main() {"
		"FragColor = texture(gerald, TexCoord);"
		"}";
	Paxet p;
	p.prog = gl_shader_setup(vert_src, frag_src);
	p.ploc = glGetAttribLocation(p.prog, "aPos");
	p.txcloc = glGetAttribLocation(p.prog, "aTexCoord");
	p.camloc = glGetUniformLocation(p.prog, "cam");
	p.texloc = glGetUniformLocation(p.prog, "gerald");
#ifdef GL_SHADER_DEBUG
	printf("Uni shader: %d %d %d %d\n", p.ploc,
	       p.camloc, p.txcloc, p.texloc);
	printf("    none of these should be -1\n");
#endif
	return p;
}
