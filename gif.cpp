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

//      "The Graphics Interchange Format(c) is the Copyright property of
//      CompuServe Incorporated. GIF(sm) is a Service Mark property of
//      CompuServe Incorporated."

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gif.hpp"

static int colorstobpp(int colors);
static int sqr(int x);
static int GIFNextPixel(struct ImageType image, int &curx, int &cury,
    long &CountDown, int &Pass);
static void Putword(short int w, FILE *fp);
static void compress(FILE *outfile, struct ImageType image);

//###########################################################################
//###########################################################################
//###########################################################################

static UBYTE reversebits(UBYTE value)
{
    // Borland C++ assigns bits from low to high.
    // GNU C++ assigns bits from high to low.
    union {
        struct {unsigned test:1, filler:7;} bitfield;
        UBYTE value;
    } test;
    test.bitfield.test = 1;
    test.bitfield.filler = 0;
    if (test.value & 0x80) {
        UBYTE newvalue = 0;
        int i;
        for (i = 0; i < 8; i++) {
            newvalue = (newvalue << 1) | (value & 1);
            value >>= 1;
        }
        value = newvalue;
    }
    return(value);
};

//###########################################################################
//###########################################################################
//###########################################################################
void WriteGif (struct GifType gif, FILE *fp)
{
    int i;

    // []------------------------------------[]
    // |  Begin the setup of the GIF headers  |
    // []------------------------------------[]
    /* Write the Magic header */
    fwrite("GIF89a", sizeof(char), 6, fp);
    
    /* Write out the screen width and height */
    Putword(gif.width, fp);
    Putword(gif.height, fp);
    
    /* Write out a bit-field */
    union {
        struct {unsigned GlobalSize:3, Sorted:1, ColorResolution:3,
               GlobalPalette:1;} field;
        UBYTE value;
    } infobyte;
    infobyte.field.GlobalSize = infobyte.field.ColorResolution = \
        colorstobpp(gif.numcolors) - 1;
    infobyte.field.GlobalPalette = (gif.palette != NULL);
    infobyte.field.Sorted = FALSE;
    fputc(reversebits(infobyte.value), fp);
    
    /* Write out the Background colour */
    fputc(gif.background, fp);
    
    /* Pixel Aspect Ratio */
    fputc(0, fp);               // no aspect ratio specified
    
    /* Write out the Global Colour Map, if necessary */
    if (infobyte.field.GlobalPalette) {
        int storedcolors = 1 << (infobyte.field.GlobalSize + 1);
        for (i = 0; i < storedcolors; i++) {
            fputc(gif.palette[i].red, fp);
            fputc(gif.palette[i].green, fp);
            fputc(gif.palette[i].blue, fp);
        }
    }


    // []-------------------------------------------[]
    // |  Write out the magical Netscape loop chunk  |
    // []-------------------------------------------[]
    if (gif.numimages) {
        fputc('!', fp);
        fputc(0xFF, fp);
        fputc(11, fp);
        fwrite("NETSCAPE2.0", sizeof(char), 11, fp);
        fputc(3, fp);
        fputc(1, fp);
        Putword(0, fp);         // loop count
        fputc(0, fp);
    }


    // []-----------------------------------------[]
    // |  Write out each of the individual images  |
    // []-----------------------------------------[]
    for (i = 0; i < (int) gif.numimages; i++) {
printf("%i..", i + 1);
        /* Write out the control extension chunk */
        union {
            struct {unsigned TransFlag:1, InputFlag:1,
                   Dispose:3, Reserved:3;} field;
            UBYTE value;
        } infobyte3;
        infobyte3.field.TransFlag = (gif.images[i].transparentflag != 0);
        infobyte3.field.InputFlag = 0;
        infobyte3.field.Dispose = 2;        // erase by background
        infobyte3.field.Reserved = 0;
        fputc('!', fp);
        fputc(0xF9, fp);
        fputc(4, fp);
        fputc(reversebits(infobyte3.value), fp);
        Putword(gif.images[i].waitdelay, fp);
        fputc(gif.images[i].transparentcolor, fp);
        fputc(0, fp);

        /* Write the Image header */
        fputc(',', fp);
        Putword(gif.images[i].leftcol, fp);
        Putword(gif.images[i].toprow, fp);
        Putword(gif.images[i].width, fp);
        Putword(gif.images[i].height, fp);

        /* Write out the local image bitfield */
        union {
            struct {unsigned LocalSize:3, reserved:2, Sorted:1,
                Interlaced:1, LocalPalette:1;} field;
            UBYTE value;
        } infobyte2;
        infobyte2.field.LocalPalette = (gif.images[i].numcolors != 0);
        infobyte2.field.Interlaced = (gif.images[i].interlaced != 0);
        infobyte2.field.Sorted = FALSE;
        if (infobyte2.field.LocalPalette) {
            infobyte2.field.LocalSize = 
                colorstobpp(gif.images[i].numcolors) - 1;
        } else {
            infobyte2.field.LocalSize = colorstobpp(gif.numcolors) - 1;
        }
        infobyte2.field.reserved = 0;
        fputc(reversebits(infobyte2.value), fp);

        /* Store the local palette, if one exists */
        if (gif.images[i].numcolors) {
            int storedcolors = 1 << (infobyte2.field.LocalSize + 1);
            for (int j = 0; j < storedcolors; j++) {
                fputc(gif.images[i].palette[j].red, fp);
                fputc(gif.images[i].palette[j].green, fp);
                fputc(gif.images[i].palette[j].blue, fp);
            }
        }

        /* Go and actually compress the data */
        compress(fp, gif.images[i]);

        /* Write out a Zero-length packet (to end the series) */
        fputc(0, fp);
    }


    // []------------------------------[]
    // |  We're done with the GIF file  |
    // []------------------------------[]
    /* Write the GIF file terminator */
    fputc(';', fp);
}

//###########################################################################
//###########################################################################
//###########################################################################

static int colorstobpp(int colors)
{
    if (colors <= 2) return 1;
    else if (colors <= 4) return 2;
    else if (colors <= 8) return 3;
    else if (colors <= 16) return 4;
    else if (colors <= 32) return 5;
    else if (colors <= 64) return 6;
    else if (colors <= 128) return 7;
    else return 8;
}

//###########################################################################
//###########################################################################
//###########################################################################

static int sqr(int x)
{
    return x*x;
}

//###########################################################################
//###########################################################################
//###########################################################################

/* Return the next pixel from the image */
static int GIFNextPixel(struct ImageType image, int &curx, int &cury,
    long &CountDown, int &Pass)
{
    if (CountDown == 0) return EOF;
    CountDown--;


    /*  Read in the actual pixel  */
    int r = image.pixels[cury * image.width + curx];


    /*
     * If we are at the end of a scan line, set curx back to the beginning
     * If we are interlaced, bump the cury to the appropriate spot,
     * otherwise, just increment it.
     */
    if (++curx == (int) image.width) {
        curx = 0;

        if (!image.interlaced) cury++;
        else if (Pass == 0) {
            cury += 8;
            if (cury >= (int) image.height) {Pass++; cury = 4;}
        } else if (Pass == 1) {
            cury += 8;
            if (cury >= (int) image.height) {Pass++; cury = 2;}
        } else if (Pass == 2) {
            cury += 4;
            if (cury >= (int) image.height) {Pass++; cury = 1;}
        } else if (Pass == 3) {
            cury += 2;
        }
    }

    return(r);
}

//###########################################################################
//###########################################################################
//###########################################################################

/*
 * Write out a 16bit word to the GIF file in Intel lo-hi ordering.
 */
static void Putword(short int w, FILE *fp)
{
    fputc( w & 0xff, fp) ;
    fputc( (w & 0xff00) >> 8, fp) ;
}

//###########################################################################
//###########################################################################
//###########################################################################

/*
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  
 */

static void compress(FILE *outfile, struct ImageType image)
{
    /* Set up the image related variables */
    int curx = 0, cury = 0, Pass = 0;
    long CountDown = (long)image.width * (long)image.height;
    int Resolution;
//Resolution = colorstobpp(image.numcolors);
Resolution = 9;
    int init_bits = (Resolution <= 1 ? 2 : Resolution);
    PacketStream gifstream(outfile, init_bits);


    /* Write out the initial code size */
    fputc(init_bits - 1, outfile);


    /* Set hash code range bound */
    register int hshift = 0;
    long fcode;
    for (fcode = HSIZE; fcode < 65536L; fcode *= 2L) hshift++;
    hshift = 8 - hshift;
    code_int hsize_reg = HSIZE;


    /* clear hash table */
    static count_int htab[HSIZE];
    static unsigned short codetab[HSIZE];
    memset(htab, -1, HSIZE * sizeof(count_int));
    gifstream.output(gifstream.ClearCode);


    /* Main compression loop */
    code_int ent = GIFNextPixel(image, curx, cury, CountDown, Pass);

    while (TRUE) {
        /* read in the next pixel to process */
        int c = GIFNextPixel(image, curx, cury, CountDown, Pass);
        if (c == EOF) break;


        /* do the primary XOR hash */
        fcode = ((long) c << gifstream.maxbits) + ent;
        code_int i = ((code_int) c << hshift) ^ ent;
        if ((long) htab[i] == fcode) {
            ent = codetab[i];
            continue;
        } else if ((long)htab[i] >= 0) {            /* not empty slot */
            /* secondary hash */
            int breakout = FALSE;
            code_int disp = hsize_reg - i;
            if (i == 0) disp = 1;

            do {
                if ((i -= disp) < 0) i += hsize_reg;
                if ((long)htab[i] == fcode) {
                    ent = codetab[i];
                    breakout = TRUE;
                    break;
                }
            } while ((long)htab[i] > 0);
            if (breakout) continue;
        }

        gifstream.output(ent);
        ent = c;
        if ((unsigned) gifstream.free_ent < (unsigned) gifstream.maxmaxcode) {
            /* code -> hashtable */
            codetab[i] = gifstream.free_ent++;
            htab[i] = fcode;
        } else {
            /* table clear for block compress */
            memset(htab, -1, HSIZE * sizeof(count_int));
            gifstream.output(gifstream.ClearCode);
        }
    }


    /* Put out the final code. */
    gifstream.output(ent);
    gifstream.output(gifstream.EOFCode);
}

//###########################################################################
//###########################################################################
//###########################################################################
