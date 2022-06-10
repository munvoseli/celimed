This was originally meant to be a Celeste image file editor, but it turned into a VTF file viewer.

I still have hopes of making it some sort of editor.

This requires SDL2.  I am unsure whether it absolutely requires gcc, but gcc is what I used to compile it.

To compile:
gcc main.c -lSDL2 -lGL

To run:
./a.out [TF2]/tf/download/user_custom/*/*

It should be noted that it will fail on some files.  For example, it will not read a file if it does not have a VTF signature.  There were a few WAV files in that download folder.

If you're having trouble with the OpenGL version, I think you can just change it?  I tried to make it compatible with many things.
