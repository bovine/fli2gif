// Fli2gif
// Copyright (c) 1996, Jeff Lawson
// All rights reserved.
// See LICENSE for distribution information.

/******************************************/
/***  Custom data types and structures  ***/
/******************************************/
#ifndef __MYTYPES__
    typedef int BOOL;
    typedef unsigned char UBYTE;
    typedef unsigned short UWORD;
    typedef unsigned long ULONG;
    #define __MYTYPES__
#endif    
#ifndef __PIXELTYPE__
    typedef unsigned char PixelType;
    #define __PIXELTYPE__
#endif
#ifndef __PALETTEENTRY__
    typedef struct {
        unsigned char red, green, blue;
    } PaletteEntry;
    #define __PALETTEENTRY__
#endif
#ifndef HUGETYPE
    #if !defined(_Windows) && (defined(__MSDOS__) || defined(_DOS))
        #define HUGETYPE huge
    #else
        #define HUGETYPE
    #endif
#endif


/*************************************/
/***  External data and variables  ***/
/*************************************/
extern UWORD flipalettechanged;     // true if the palette needs updating
extern UWORD flitotalframes;        // number of frames in this FLIC
extern UWORD flicurframe;           // current frame (1 based; 0 = haven't started)
extern UWORD fliwidth, fliheight;   // dimensions of current FLIC
extern ULONG flispeed;              // playback speed in 1/1000'ths of a sec.
extern PixelType HUGETYPE *fliscreenptr;    // pointer to screen buffer
extern PaletteEntry *flipaletteptr;     // pointer to 256 color palette


/**************************************/
/***  External function prototypes  ***/
/**************************************/
extern int fli_init(PixelType HUGETYPE *scr_buffer, PaletteEntry *palette_ptr, FILE *handle);
extern int fli_nextframe(void);
