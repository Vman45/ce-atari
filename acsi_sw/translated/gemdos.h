#ifndef GEMDOS_H
#define GEMDOS_H

// path functions
#define GEMDOS_Dsetdrv    0x0e
#define GEMDOS_Dgetdrv    0x19
#define GEMDOS_Dsetpath   0x3b
#define GEMDOS_Dgetpath   0x47

// directory & file search
#define GEMDOS_Fsetdta    0x1a
#define GEMDOS_Fgetdta    0x2f
#define GEMDOS_Fsfirst    0x4e
#define GEMDOS_Fsnext     0x4f

// file and directory manipulation
#define GEMDOS_Dfree      0x36
#define GEMDOS_Dcreate    0x39
#define GEMDOS_Ddelete    0x3a
#define GEMDOS_Frename    0x56
#define GEMDOS_Fdatime    0x57
#define GEMDOS_Fdelete    0x41
#define GEMDOS_Fattrib    0x43

// file content functions
#define GEMDOS_Fcreate    0x3c
#define GEMDOS_Fopen      0x3d
#define GEMDOS_Fclose     0x3e
#define GEMDOS_Fread      0x3f
#define GEMDOS_Fwrite     0x40
#define GEMDOS_Fseek      0x42

// date and time function
#define GEMDOS_Tgetdate   0x2A
#define GEMDOS_Tsetdate   0x2B
#define GEMDOS_Tgettime   0x2C
#define GEMDOS_Tsettime   0x2D

#endif // GEMDOS_H