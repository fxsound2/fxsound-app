/* FxSound — Linux build: stand-in for Windows <io.h> (low-level file access),
   mapping the handful of helpers the shared util layer uses to POSIX. */
#ifndef FXSOUND_DSP_FAKE_IO_H
#define FXSOUND_DSP_FAKE_IO_H
#include <unistd.h>
#include <stdio.h>
#ifndef _access
#define _access(path, mode) access((path), (mode))
#endif
#ifndef _unlink
#define _unlink(path) unlink((path))
#endif
#endif
