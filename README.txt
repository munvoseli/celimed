This was originally meant to be a Celeste image file editor, but it turned into a VTF file viewer.

I still have hopes of making it some sort of editor.

This requires SDL2.  I am unsure whether it absolutely requires gcc, but gcc is what I used to compile it.

To compile:
gcc main.c -lSDL2 -lGL

To run:
./a.out [TF2]/tf/download/user_custom/*/*

It should be noted that it will fail on some files.  For example, it will not read a file if it does not have a VTF signature.  There were a few WAV files in that download folder.

If you're having trouble with the OpenGL version, I think you can just change it?  I tried to make it compatible with many things.


I almost forgot the controls:

You can use J and K to go through the list of files, looking at thumbnails of every mipmap and frame.  The background is color-coded with the version (7.0: black, 7.1: grey, 7.2: blue, 7.3: green, 7.4: yellow, 7.5: red).

Also, you can maximize and animate an image with the A key.  At that point, any key will put it back in thumbnail mode.
