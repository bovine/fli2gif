// []-----------------------------------------------------------------------[]
// |                                                                         |
// |                                                                         |
// |  This source and its associated executable are copyright by Jeff Lawson |
// |  of JL Enterprises, however some portions of this code may or may not   |
// |  have been originally inspired by other authors and credit is given if  |
// |  due.  You are encouraged to redistribute this package, but you must    |
// |  include all files originally distributed in this archive in their      |
// |  original state and format and without modifications of any kind.       |
// |  Under no circumstances may you make modifications to this product and  |
// |  redistribute the resulting code or executables.  If you believe you    |
// |  have made some useful or otherwise important enhancements and you      |
// |  would like to share these changes with others, please contact the      |
// |  author through one of the methods listed below.  Additionally, no      |
// |  fees may be charged for the usage of this product by anyone other than |
// |  the author of this file, except for fees (not to exceed US $10) by     |
// |  disk distributors to cover disk duplication and handling.  If there    |
// |  is any need to question this policy, please contact the author.        |
// |  This source code and its executable are distributed without any kind   |
// |  of warranty and the author may not be held accountable for damages of  |
// |  any kind.                                                              |
// |                                                                         |
// |                                                                         |
// |  I can reached via postal mail at:                                      |
// |          Jeff Lawson                                                    |
// |          1893 Kaweah Drive                                              |
// |          Pasadena, CA 91105-2174                                        |
// |          USA                                                            |
// |                                                                         |
// |  on the World Wide Web at:                                              |
// |          http://members.aol.com/JeffLawson/                             |
// |                                                                         |
// |  or through E-mail at:                                                  |
// |          jlawson@hmc.edu    or   JeffLawson@aol.com                     |
// |                                                                         |
// |  also via phone at:                                                     |
// |          (213) 258-5604     or   (213) 258-4264                         |
// |                                                                         |
// []-----------------------------------------------------------------------[]

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
