#ifndef VTF_H
#define VTF_H

static const char* format_names[] = {
	"RGBA8888",
	"ABGR8888",
	"RGB888",
	"BGR888",
	"RGB565",
	"I8",
	"IA88",
	"P8",
	"A8",
	"RGB888_BLUESCREEN",
	"BGR888_BLUESCREEN",
	"ARGB8888",
	"BGRA8888",
	"DXT1", // 0x0d
	"DXT3",
	"DXT5",
	"BGRX8888",
	"BGR565",
	"BGRX5551",
	"BGRA4444",
	"DXT1_ONEBITALPHA",
	"BGRA5551",
	"UV88",
	"UVWQ8888",
	"RGBA16161616F",
	"RGBA16161616",
	"UVLX8888",
};

// https://developer.valvesoftware.com/wiki/Valve_Texture_Format
typedef struct tagVTFHEADER {
	char            sig[4];
	unsigned int    version[2];  
	unsigned int    headerBct;  
	unsigned short  w;       
	unsigned short  h;      
	unsigned int    flags;       
	unsigned short  frameCt;      
	unsigned short  firstFrame;  
	float           refl[3];
	float           bmpScl;
	unsigned int    hriFmt;
	unsigned char   mpmCt;
	unsigned int    lriFmt;
	unsigned char   lriW;
	unsigned char   lriH;
	// 7.2+
	unsigned short  depth;
	// 7.3+
	unsigned int    resCt;
} VTFHEADER;

#endif
