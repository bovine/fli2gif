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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fliplay.hpp"

#define MAGIC_FLI 0xAF11
#define MAGIC_FLC 0xAF12
#define MAGIC_FRAME 0xF1FA
#define MAGIC_PREFIX 0xF100

#define FLI_COLOR256 4
#define FLI_SS2 7
#define FLI_COLOR 11
#define FLI_LC 12
#define FLI_BLACK 13
#define FLI_BRUN 15
#define FLI_COPY 16
#define FLI_PSTAMP 18

struct fliheader {
    ULONG size;
    UWORD magic, frames;
    UWORD width, height, bits, flags;
    ULONG speed;        // only a word in FLIs
    UWORD filler[30];
    ULONG oframe1;      // only in FLCs
    UWORD filler2[22];
};

struct frameheader {
    ULONG size;
    UWORD magic, chunks;
    UBYTE expand[8];
};


//############################################################################
//############################################################################
//############################################################################

UWORD flipalettechanged;    // true if the palette needs updated
UWORD flitotalframes;       // number of frames in this FLIC
UWORD flicurframe;          // current frame (1 based; 0 = haven't started)
UWORD fliwidth, fliheight;  // dimensions of current FLIC
ULONG flispeed;             // playback speed of FLIC in milliseconds

//############################################################################
//############################################################################
//############################################################################

static FILE *flihandle;             // file handle of opened FLIC
static ULONG fliframe1ptr;          // pointer in FLIC data to loop back to
static ULONG flicurptr;             // current position in the FLIC data
PixelType HUGETYPE *fliscreenptr;   // pointer to screen buffer
PaletteEntry *flipaletteptr;        // pointer to 256 color palette buffer

//############################################################################
//############################################################################
//############################################################################

UWORD mygetw(FILE *fp)
{
    // reads an Intel 16bit int into this machine's native ordering
    UBYTE lo, hi;
    lo = getc(fp);
    hi = getc(fp);
    return(UWORD(lo) + (UWORD(hi) << 8));
}
ULONG mygetl(FILE *fp)
{
    // reads an Intel 32bit int into this machine's native ordering
    UWORD lo, hi;
    lo = mygetw(fp);
    hi = mygetw(fp);
    return(ULONG(lo) + (ULONG(hi) << 16));
}
#define getw(fp) mygetw(fp)
#define getl(fp) mygetl(fp)
BOOL bigendian(void)
{
    // return TRUE if this machine doesn't use Intel lo-hi ordering
    union {
        UWORD myword;
        struct {UBYTE first, second;} mybyte;
    } value;
    value.myword = 0x100;
    return(value.mybyte.first != 0); 
}
UWORD wordfix(UWORD value)
{
    // convert Intel lo-hi 16bit ints to this machine's native ordering
    if (bigendian()) value = ((value & 0xFF) << 8) + ((value & 0xFF00) >> 8);
    return(value);
}
ULONG longfix(ULONG value)
{
    // convert Intel lo-hi-LO-HI 32bit ints to this machine's native ordering
    if (bigendian()) {
        ULONG lo = (ULONG) wordfix(UWORD(value & 0xFFFF));
        ULONG hi = (ULONG) wordfix(UWORD(value >> 16));
        value = (lo << 16) + hi;
    }
    return(value);   
}

//############################################################################
//############################################################################
//############################################################################
//
// returns:   0 = success
//            1 = error (corrupted file / unknown format)
//            2 = error (incorrect bits per pixel)
//
int fli_init(PixelType HUGETYPE *scr_buffer, PaletteEntry *palette_ptr, \
    FILE *handle)
{
    if (fseek(handle, 0, SEEK_SET)) return(1);

    // read in the header
    struct fliheader ourheader;
    fread(&ourheader, sizeof(struct fliheader), 1, handle);

    // correct for endian-ness, if necessary
    ourheader.size = longfix(ourheader.size);
    ourheader.speed = longfix(ourheader.speed);
    ourheader.oframe1 = longfix(ourheader.oframe1);
    ourheader.magic = wordfix(ourheader.magic);
    ourheader.frames = wordfix(ourheader.frames);
    ourheader.width = wordfix(ourheader.width);
    ourheader.height = wordfix(ourheader.height);
    ourheader.bits = wordfix(ourheader.bits);
    ourheader.flags = wordfix(ourheader.flags);


    // validate the header
    if (ourheader.magic == MAGIC_FLI) {
        flicurptr = sizeof(struct fliheader);
        ourheader.speed = (ourheader.speed * 1000) / 70;
    } else if (ourheader.magic == MAGIC_FLC) {
        flicurptr = ourheader.oframe1;
    } else return(1);
    if (ourheader.bits != 8) return(2);
    flitotalframes = ourheader.frames;
    flicurframe = 0;
    fliwidth = ourheader.width;
    fliheight = ourheader.height;
    flispeed = ourheader.speed;
    flihandle = handle;
    fliscreenptr = scr_buffer;
    flipaletteptr = palette_ptr;

    if (flipaletteptr) memset(flipaletteptr, 0, 256 * 3);
    flipalettechanged = 1;
    return(0);
}
//############################################################################
//############################################################################
//############################################################################
static void RLE_decomp(BOOL partial)
{
    ULONG index = 0;
    UWORD len = fliheight;
    if (partial) {
        index = getw(flihandle) * fliwidth;
        len = getw(flihandle);
    }
    while (len--) {
        UBYTE packets = getc(flihandle);
        UWORD col = 0;
        while (packets--) {
            if (partial) col += UWORD(getc(flihandle));
            short int count = (signed char) getc(flihandle);
            if (partial) count = -count;
            if (count >= 0) { // duplicate character
                if (count == 0) count = 256;
                if (col + count > fliwidth) {
                    count = fliwidth - col;
                    len = packets = 0;
                }
                memset(fliscreenptr + index + col, getc(flihandle), count);
            } else { // copy straight over
                count = -count;
                if (col + count > fliwidth) {
                    count = fliwidth - col;
                    len = packets = 0;
                }
                fread(fliscreenptr + index + col, 1, count, flihandle);
            }
            col += count;
        }
        index += fliwidth;
    }
}
//############################################################################
//############################################################################
//############################################################################
//
// returns:   0 = frame decoded okay
//           else error occured (unknown chunk type?)
//
int fli_nextframe(void)
{
    flipalettechanged = 0;

    // read in the frame header
    struct frameheader ourheader;
    if (fseek(flihandle, flicurptr, SEEK_SET)) return(1);
    fread(&ourheader, sizeof(struct frameheader), 1, flihandle);

    // correct for endian-ness, if necessary
    ourheader.size = longfix(ourheader.size);
    ourheader.magic = wordfix(ourheader.magic);
    ourheader.chunks = wordfix(ourheader.chunks);

    // validate the frame header
    if (ourheader.magic == MAGIC_PREFIX) {
        flicurptr += ourheader.size;
        return(fli_nextframe());
    }
    if (ourheader.magic != MAGIC_FRAME) return(1);

    // frame decoding loop
    while (ourheader.chunks--) {
        ULONG chunkend = ftell(flihandle);
        chunkend += getl(flihandle);
        UWORD chunktype = getw(flihandle);

        switch (chunktype) {
            // ----------------------------------------------------------
            case FLI_COLOR: // 64-level palette change
            case FLI_COLOR256: // 256-level palette change
            {
                flipalettechanged = 1;
                UWORD packets = getw(flihandle), index = 0;
                do {
                    index += UBYTE(getc(flihandle));
                    UWORD len = getc(flihandle);
                    if (len == 0) len = 256;
                    if (index + len > 256) {
                        len = 256 - index;
                        packets = 1;
                    }

                    // read in the palette chunk
                    PaletteEntry *ptr = flipaletteptr + 3 * index;
                    fread(ptr, sizeof(PaletteEntry), len, flihandle);
                    if (chunktype == FLI_COLOR) {
                        for (int i = len; i--; ptr++) {
                            ptr->red <<= 2;
                            ptr->green <<= 2;
                            ptr->blue <<= 2;
                        }
                    }
                    index += len;
                } while (--packets);
                break;
            }
            // ----------------------------------------------------------
            case FLI_LC: // RLE partial screen
                RLE_decomp(1);
                break;
            // ----------------------------------------------------------
            case FLI_BRUN: // RLE entire screen
                RLE_decomp(0);
                break;
            // ----------------------------------------------------------
            case FLI_SS2: // RLE partial screen in words
            {
                PixelType HUGETYPE *ptr = fliscreenptr;
                UWORD lines = getw(flihandle);
                while (lines--) {
                    // process the prefix commands
                    short int cmd;
                    while (1) {
                        cmd = getw(flihandle);
                        if (cmd & 0x8000) {
                            if (cmd & 0x4000) {
                                // skip down some lines
                                ptr += fliwidth * -cmd;
                            } else {
                                // set last pixel value
                                *(ptr + fliwidth - 1) = cmd & 0xFF;
                            }
                        } else break;
                    }

                    // start decoding the line
                    UWORD col = 0;
                    while (cmd--) {
                        col += UWORD(getc(flihandle));
                        short int cmd2 = (signed char) getc(flihandle);
                        if (cmd2 > 0) { // copy straight
                            fread(ptr + col, 2, cmd2, flihandle);
                            col += 2 * cmd2;
                        } else { // repeat
                            UWORD val = getw(flihandle);
                            cmd2 = -cmd2;
                            do {
                                *(UWORD *)(ptr + col) = val;
                                col += 2;
                            } while (--cmd2);
                        }
                    }
                    ptr += fliwidth;
                }
                break;
            }
            // ----------------------------------------------------------
            case FLI_BLACK: // completely black screen
            {
                PixelType HUGETYPE *ptr = fliscreenptr;
                for (UWORD i = 0; i < fliheight; i++, ptr += fliwidth) {
                    memset(ptr, 0, fliwidth * sizeof(PixelType));
                }
                break;
            }
            // ----------------------------------------------------------
            case FLI_COPY: // uncompressed entire screen
            {
                PixelType HUGETYPE *ptr = fliscreenptr;
                for (UWORD i = 0; i < fliheight; i++, ptr += fliwidth) {
                    fread(ptr, sizeof(PixelType), fliwidth, flihandle);
                }
                break;
            }
        }
        fseek(flihandle, chunkend, SEEK_SET);
    }
    flicurptr += ourheader.size;
    flicurframe++;

    if (flicurframe == 1) fliframe1ptr = flicurptr;
    if (flicurframe > flitotalframes) {
        flicurptr = fliframe1ptr;
        flicurframe = 1;
    }
    return(0);
}
//############################################################################
//############################################################################
//############################################################################


