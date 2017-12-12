// Fli2gif
// Copyright (c) 1996, Jeff Lawson
// All rights reserved.
// See LICENSE for distribution information.


// ******************
// ***  Includes  ***
// ******************
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#if defined(__TURBOC__) || defined(_MSC_VER)
    #include <conio.h>
#else
    #define _MAX_PATH 250
    #define stricmp(x,y) strcasecmp(x,y)
    #define strnicmp(x,y,z) strncasecmp(x,y,z)
#endif
#include "fliplay.hpp"
#include "gif.hpp"


// ********************************
// ***  Defined error messages  ***
// ********************************
#define NO_MEMORY 1
#define BAD_FLIC 2
#define BAD_SYNTAX 4
#define FILE_ERROR 5
#define WRITE_ERROR 7
#define WRONG_BITS 8
#define MULTIPLE_PALETTE 9
#define NO_PALETTE 10
#define SAMENAME_ERROR 11


// ***********************
// ***  Other Defines  ***
// ***********************
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))


// *****************************
// ***  Function prototypes  ***
// *****************************
void handle_error(int errorcode);
char *add_extension(char *filename, char *extension, BOOL enforce);


// **************************
// ***  Global variables  ***
// **************************
char flicname[_MAX_PATH], gifname[_MAX_PATH];
int interlaceall = FALSE, maketransparent = TRUE, bestcrop = FALSE;
int screenpause = TRUE, gotpalette = FALSE;
PixelType backgroundcolor = 0;

//############################################################################
//############################################################################
//############################################################################
void main(int argc, char *argv[])
{
    FILE *fp;
    int i, j;


    // []----------------[]
    // |  Initialization  |
    // []----------------[]
    printf("FLI2GIF.EXE -- Automatic Flic to Animated GIF convertor, v1.1\n");
    printf("   by Jeff Lawson, 1996.  (Compiled with "
        #if defined(__TURBOC__)
            "Borland"
        #elif defined(_MSC_VER)
            "Microsoft"
        #else
            "Unknown"
        #endif
        #if defined(_Windows)
            " for Windows"
        #elif defined(__MSDOS__) || defined(_DOS)
            " for MS-DOS"
        #endif    
        ")\n\n");
    {
        int gotflic = FALSE, gotgif = FALSE;
        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '/') {
                if (!gotflic) {
                    strcpy(flicname, add_extension(argv[i], ".flc", FALSE));
                    gotflic = TRUE;
                } else if (!gotgif) {
                    strcpy(gifname, add_extension(argv[i], ".gif", FALSE));
                    gotgif = TRUE;
                } else handle_error(BAD_SYNTAX);
            } else if (!stricmp(argv[i], "/i")) {
                interlaceall = TRUE;
            } else if (!stricmp(argv[i], "/t")) {
                maketransparent = FALSE;
            } else if (!stricmp(argv[i], "/c")) {
                bestcrop = TRUE;
            } else if (!stricmp(argv[i], "/np")) {
                screenpause = FALSE;
            } else if (!strnicmp(argv[i], "/b", 2)) {
                backgroundcolor = atoi(argv[i] + 2);
            } else handle_error(BAD_SYNTAX);
        }
        if (!gotflic) handle_error(BAD_SYNTAX);
        if (!gotgif) {
            strcpy(gifname, add_extension(flicname, ".gif", TRUE));
            gotgif = TRUE;
        }
    }
    if (strcmp(flicname, gifname) == 0) handle_error(SAMENAME_ERROR);
    printf("Input filename:  %s\n"
           "Output filename:  %s\n"
           "Background color=%u, Interlaced=%s, "
           "Transparent=%s, BestCrop=%s\n\n",
           flicname, gifname, (unsigned) backgroundcolor,
           (interlaceall ? "on" : "off"),
           (maketransparent ? "on" : "off"),
           (bestcrop ? "on" : "off")
           );

    
    // []---------------------------------[]
    // |  Open up the specified flic file  |
    // []---------------------------------[]
    if ((fp = fopen(flicname, "rb")) == NULL) handle_error(FILE_ERROR);
    flipaletteptr = new PaletteEntry[256];
    if (flipaletteptr == NULL) handle_error(NO_MEMORY);
    if ((i = fli_init(NULL, flipaletteptr, fp)) != 0) {
        if (i == 1) handle_error(BAD_FLIC);
        else if (i == 2) handle_error(WRONG_BITS);
    }
    fliscreenptr = new PixelType HUGETYPE[ULONG(fliwidth) * ULONG(fliheight)];
    if (fliscreenptr == NULL) handle_error(NO_MEMORY);


    // []-------------------------------------[]
    // |  Initialize our GIF output structure  |
    // []-------------------------------------[]
    GifType mygif;
    mygif.images = new ImageType[flitotalframes];
    if (!mygif.images) handle_error(NO_MEMORY);
    mygif.background = 0;
    mygif.numcolors = 256;
    mygif.numimages = flitotalframes;
    mygif.palette = new PaletteEntry[256];
    if (mygif.palette == NULL) handle_error(NO_MEMORY);
    gotpalette = FALSE;


    // []--------------------------------------------------[]
    // |  Start scanning each frame for the cropping range  |
    // []--------------------------------------------------[]
    UWORD gminx = UWORD(-1L), gminy = UWORD(-1L), gmaxx = 0, gmaxy = 0;
    for (i = 0; i < (int) flitotalframes; i++) {
        UWORD minx = UWORD(-1L), miny = UWORD(-1L), maxx = 0, maxy = 0;

        if (fli_nextframe() || (int) flicurframe != i + 1)
            handle_error(BAD_FLIC);
        printf("Scanning frame #%i...", flicurframe);

        // determine the limits of the non-zero pixels
        UWORD x, y;
        for (y = 0; y < fliheight; y++) {
            PixelType HUGETYPE *p = &fliscreenptr[ULONG(y) * ULONG(fliwidth)];
            for (x = 0; x < fliwidth; x++) {
                if (*p++ != backgroundcolor) {
                    gminx = MIN(gminx, x);
                    gmaxx = MAX(gmaxx, x);
                    gminy = MIN(gminy, y);
                    gmaxy = MAX(gmaxy, y);
                    minx = MIN(minx, x);
                    maxx = MAX(maxx, x);
                    miny = MIN(miny, y);
                    maxy = MAX(maxy, y);
                }
            }
        }
        if (minx == UWORD(-1L) && maxx == 0) minx = maxx = 0;
        if (miny == UWORD(-1L) && maxy == 0) miny = maxy = 0;
        UWORD width = maxx - minx + 1;
        UWORD height = maxy - miny + 1;
        printf("(%i,%i)-(%i,%i); %ix%i\n",
            minx, miny, maxx, maxy, width, height);

        // advise the user on possible background colors
        if (i == 0 && width == fliwidth && height == fliheight) {
            // scan along top and left edges for most freqently used colors
            ULONG *colorusage = new ULONG[256];
            if (colorusage == NULL) handle_error(NO_MEMORY);
            memset(colorusage, 0, sizeof(ULONG)*256);
            for (x = 0; x < fliwidth; x++) colorusage[fliscreenptr[x]]++;
            for (y = 0; y < fliheight; y++) \
                colorusage[fliscreenptr[(long)y * (long)fliwidth]]++;

            // determine color with highest usage
            ULONG bestusage = colorusage[0];   int bestcolor = 0;
            for (x = 1; x < 256; x++) {
                if (colorusage[x] > bestusage) {
                    bestusage = colorusage[x];
                    bestcolor = x;
                }
            }
            if (bestcolor != backgroundcolor) printf("  *** Found possible "
                "background color.  Try the \"/b%u\" option.\n", 
                (unsigned) bestcolor);
            delete colorusage;
        }

        // fill in some of the GIF info
        mygif.images[i].width = width;
        mygif.images[i].height = height;
        mygif.images[i].waitdelay = (int) flispeed / 10;
        mygif.images[i].interlaced = (interlaceall != FALSE);
        mygif.images[i].leftcol = minx;
        mygif.images[i].toprow = miny;
        mygif.images[i].transparentflag = (maketransparent != FALSE);
        mygif.images[i].transparentcolor = backgroundcolor;

        // handle local color chunks
        mygif.images[i].numcolors = 0;
        if (flipalettechanged) {
            if (gotpalette) handle_error(MULTIPLE_PALETTE);
            memmove(mygif.palette, flipaletteptr, 256 * sizeof(PaletteEntry));
            gotpalette = TRUE;
        }

        // pause here if needed
        #if defined(__TURBOC__) || defined(_MSC_VER)
            #if !defined(_QWINVER)
                if (screenpause) {
                    if ((i + 8) % 24 == 0 ||
                        (i == (int) flitotalframes - 1 && ((i + 8) % 24) + 5 > 24)) {
                        printf("-- Press any key to continue --");
                        if (getch() == 0) getch();
                        printf("\r                               \r");
                    }
                }
            #endif
        #endif
    }
    if (!gotpalette) handle_error(NO_PALETTE);
    printf("\nOverall animation dimensions: (%u,%u)-(%u,%u), %ux%u, "
        "%u frames\n", gminx, gminy, gmaxx, gmaxy, gmaxx - gminx + 1,
        gmaxy - gminy + 1, flitotalframes);
    mygif.width = gmaxx - gminx + 1;
    mygif.height = gmaxy - gminy + 1;


    // []--------------------------------------------------------------[]
    // |  Go back and compensate for the (now known) global dimensions  |
    // []--------------------------------------------------------------[]
    printf("Processing frame...");
    for (i = 0; i < (int) flitotalframes; i++) {
        if (fli_nextframe() || (int) flicurframe != i + 1)
            handle_error(BAD_FLIC);
        printf("%i..", flicurframe);
        
        // crop the image to the region we found
        int miny, minx, width, height;
        if (bestcrop) {
            // **********************
            // *** Do a best crop ***
            // **********************
            miny = mygif.images[i].toprow;
            minx = mygif.images[i].leftcol;
            width = mygif.images[i].width;
            height = mygif.images[i].height;
            mygif.images[i].leftcol -= gminx;
            mygif.images[i].toprow -= gminy;
        } else {
            // *************************
            // *** Do a generic crop ***
            // *************************
            minx = gminx;
            miny = gminy;
            width = mygif.images[i].width = mygif.width;
            height = mygif.images[i].height = mygif.height;
            mygif.images[i].leftcol = 0;
            mygif.images[i].toprow = 0;
        }


        // **********************************
        // ***  Actually do the cropping  ***
        // **********************************
        PixelType HUGETYPE *s = &fliscreenptr[ULONG(miny) * \
            ULONG(fliwidth) + minx];
        PixelType HUGETYPE *t = new PixelType[long(width) * long(height)];
        if ((mygif.images[i].pixels = t) == NULL) handle_error(NO_MEMORY);
        for (j = 0; j < height; j++) {
            memmove(t, s, sizeof(PixelType) * width);
            t += width;
            s += fliwidth;
        }
    }
    fclose(fp);
    printf("\n");


    // []-------------------------------------------[]
    // |  Finally go and write out the animated GIF  |
    // []-------------------------------------------[]
    printf("Now writing out final GIF...");
    if ((fp = fopen(gifname, "wb")) == NULL) handle_error(WRITE_ERROR);
    WriteGif(mygif, fp);
    fclose(fp);
    printf("\nOutput file write completed.\n");

    
    exit(0);
}
//############################################################################
//############################################################################
//############################################################################
void handle_error(int errorcode)
{
    printf("\n");
    switch (errorcode) {
        case NO_MEMORY: printf("Insufficient conventional memory available.\n"); break;
        case BAD_FLIC: printf("Flic error.  Possibly corrupted file?\n"); break;
        case BAD_SYNTAX:
            printf("Syntax:  FLI2GIF [options] ®flic.FL?¯ [output.GIF]\n"
                   "Options:  /I  = interlace all images\n"
                   "          /T  = do NOT make color-0 transparent\n"
                   "          /Bn = specify background color (n=color)\n"
                   "          /C  = best crop each frame and use positional offsets\n"
#if defined(__TURBOC__) || defined(_MSC_VER)
                   "          /NP = do not pause between screenfuls\n"
#endif                   
                   "\n"
                   "The latest version of this utility can be found on my home page at\n"
                   "http://www.bovine.net/ or you can e-mail me directly at\n"
                   "jeff@bovine.net\n"
            ); break;
        case SAMENAME_ERROR: printf("Input and output filenames are the same!\n"); break;
        case FILE_ERROR: printf("Error opening the specified flic file.\n"); break;
        case WRITE_ERROR: printf("Error attempting to write out resulting GIF.\n"); break;
        case WRONG_BITS: printf("Wrong number of bits in flic.  Only 256-color (8-bit) flics are handled.\n"); break;
        case MULTIPLE_PALETTE: printf("Multiple palettes encountered in flic.  Please quantize it to use only one.\n"); break;
        case NO_PALETTE: printf("No palette chunk encountered in flic.  Cannot continue.\n"); break;
        default: break;
    }
    exit(1);
}
//############################################################################
//############################################################################
//############################################################################
// Takes a user-entered filename and adds the specified extension if an
// entension is not explicitly stated in the passed filename.  A pointer
// to a static buffer is returned.
char *add_extension(char *filename, char *extension, BOOL enforce)
{
    static char Path[_MAX_PATH];

#if defined(__TURBOC__) || defined(__MSC)
    char Drive[_MAX_DRIVE], Dir[_MAX_DIR], Name[_MAX_FNAME], Ext[_MAX_EXT];
    
    _splitpath(filename, Drive, Dir, Name, Ext);
    if (enforce || strlen(Ext) == 0) strcpy(Ext, extension);
    _makepath(Path, Drive, Dir, Name, Ext);
    strupr(Path);
#else
    strcpy(Path, filename);
    char *f = strrchr(Path, '/');
    if (f == NULL) f = Path;
    char *e = strrchr(f, '.');
    if (e == NULL) {
        strcat(Path, extension);
    } else if (enforce) {
        // enforce the specified extension, but don't enfore
        // the case of it, if it's already been specified by the user
        if (stricmp(e, extension) != 0) strcpy(e, extension);
    }    
#endif

    return(Path);
}
//############################################################################
//############################################################################
//############################################################################
