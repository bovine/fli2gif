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


/***********************************/
/***  Custom defines and macros  ***/
/***********************************/
#define TRUE 1
#define FALSE 0
#define BITS    12
#define HSIZE   5003            /* 80% occupancy */
#define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)


/******************************************/
/***  Custom data types and structures  ***/
/******************************************/
#ifdef __TURBOC__
    #pragma warn -inl
#endif
#ifndef HUGETYPE
    #if !defined(_Windows) && (defined(__MSDOS__) || defined(_DOS))
        #define HUGETYPE huge
    #else
        #define HUGETYPE
    #endif
#endif
#ifndef __MYTYPES__
    typedef int BOOL;
    typedef unsigned char UBYTE;
    typedef unsigned short UWORD;
    typedef unsigned long ULONG;
    #define __MYTYPES__
#endif    
typedef int code_int;
typedef unsigned long int count_int;
#ifndef __PIXELTYPE__
    typedef unsigned char PixelType;
    #define __PIXELTYPE__
#endif
#ifndef __PALETTEENTRY__
    typedef struct {
        unsigned char red, green, blue;
    } HUGETYPE PaletteEntry;
    #define __PALETTEENTRY__
#endif
struct ImageType {
    unsigned waitdelay;             // playback speed in 1/100'ths of a second
    BOOL interlaced;                // 0 if no interlacing
    unsigned leftcol, toprow;       // position to display image

    BOOL transparentflag;           // 0 if no transparency
    PixelType transparentcolor;

    unsigned width, height;
    PixelType HUGETYPE *pixels;

    unsigned numcolors;             // 0 if no local palette
    PaletteEntry *palette;          // NULL if no local palette
};
struct GifType {
    PixelType background;           // background color
    unsigned width, height;         // dimensions of overall GIF

    unsigned numcolors;             // maximum number of colors ever used in
                                    // entire GIF (and size of global palette
                                    // if one is present)

    PaletteEntry *palette;          // NULL if no global palette

    unsigned numimages;
    ImageType *images;
};



/**************************************/
/***  External function prototypes  ***/
/**************************************/
extern void WriteGif (struct GifType gif, FILE *fp);


/******************************************/
/***  Custom program class definitions  ***/
/******************************************/
class BufferStream {
private:
    FILE *outfile;
    int count;
    char accum[256];
public:
    void out(unsigned int c) {accum[count++] = c; if (count >= 255) flush();};
    void flush(void) {
        if (count > 0) {
            int blocksize = (count > 255 ? 255 : count);
            fputc(blocksize, outfile);
            fwrite(accum, 1, blocksize, outfile);
            count -= blocksize;
        }
        fflush(outfile);
    };
    BufferStream(FILE *Outfile) {outfile = Outfile; count = 0;};
    ~BufferStream(void) {flush();};
};
//----------------------------------------------------------------------------
class PacketStream {
private:
    BufferStream *buffer;               /* output buffer stream */
    int cur_bits;                       /* number of bits used in our buffer */
    unsigned long cur_accum;            /* bit buffer of codes to output */
    void writeonce(void) {
        buffer->out((unsigned int)cur_accum & 0xff);
        cur_accum >>= 8;
        cur_bits -= 8;
    };
    
public:
    int n_bits;                 /* number of bits/code */
    int init_bits;              /* initial bit size */
    int maxbits;                /* user settable max # bits/code */
    code_int maxcode;           /* maximum code, given n_bits */
    code_int maxmaxcode;        /* should NEVER generate this code */
    code_int ClearCode;         /* clear hash table code */
    code_int EOFCode;           /* end-of-file code */
    code_int free_ent;          /* first unused entry */

    PacketStream(FILE *Outfile, int InitBits) {
        /* Initialize the bit buffer streams */
        buffer = new BufferStream(Outfile);
        cur_bits = 0;
        cur_accum = 0;

        /* Set up the dictionary size values */
        maxbits = BITS;
        maxmaxcode = 1 << BITS;
        n_bits = init_bits = InitBits;
        maxcode = MAXCODE(n_bits);
        ClearCode = 1 << (init_bits - 1);
        EOFCode = ClearCode + 1;
        free_ent = ClearCode + 2;
    };
    ~PacketStream(void) {
        delete buffer;
    };

    void output(code_int code) {
        static unsigned long masks[] = {0x0000, 0x0001, 0x0003,
            0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF,
            0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

        /* Add this code to our bit buffer */
        cur_accum &= masks[cur_bits];    
        if (cur_bits > 0) cur_accum |= ((long)code << cur_bits);
        else cur_accum = code;    
        cur_bits += n_bits;


        /* Empty out the buffer if we have a full byte */
        while (cur_bits >= 8) writeonce();

    
        /* If the next entry is going to be too big for   */
        /* the code size, then increase it, if possible.  */    
        if (code == ClearCode) {
            n_bits = init_bits;
            maxcode = MAXCODE(n_bits);
            free_ent = ClearCode + 2;
        } else if (free_ent > maxcode) {
            n_bits++;
            if (n_bits == maxbits) maxcode = maxmaxcode;
            else maxcode = MAXCODE(n_bits);
        }


        /* At EOF, write the rest of the buffer. */
        if (code == EOFCode) {            
            while (cur_bits > 0) writeonce();
            buffer->flush();    
        }
    };
};
//----------------------------------------------------------------------------
