/* FxSound — Linux build: stand-in for Windows <direct.h> (directory ops). */
#ifndef FXSOUND_DSP_FAKE_DIRECT_H
#define FXSOUND_DSP_FAKE_DIRECT_H
#include <sys/stat.h>
#include <unistd.h>
#define _mkdir(path)        mkdir((path), 0755)
#define _getcwd(buf, size)  getcwd((buf), (size))
#endif
