/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Routines to read/write pcx images.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "pcx.h"
#include "cfile.h"
#include "physfsx.h"

#ifdef OGL
#include "palette.h"
#endif

int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_file *fid);
int pcx_encode_line(ubyte *inBuff, int inLen, PHYSFS_file *fp);

/* PCX Header data type */
typedef struct {
	ubyte   Manufacturer;
	ubyte   Version;
	ubyte   Encoding;
	ubyte   BitsPerPixel;
	short   Xmin;
	short   Ymin;
	short   Xmax;
	short   Ymax;
	short   Hdpi;
	short   Vdpi;
	ubyte   ColorMap[16][3];
	ubyte   Reserved;
	ubyte   Nplanes;
	short   BytesPerLine;
	ubyte   filler[60];
} __pack__ PCXHeader;

#define PCXHEADER_SIZE 128

/*
 * reads n PCXHeader structs from a CFILE
 */
int PCXHeader_read_n(PCXHeader *ph, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ph->Manufacturer = cfile_read_byte(fp);
		ph->Version = cfile_read_byte(fp);
		ph->Encoding = cfile_read_byte(fp);
		ph->BitsPerPixel = cfile_read_byte(fp);
		ph->Xmin = cfile_read_short(fp);
		ph->Ymin = cfile_read_short(fp);
		ph->Xmax = cfile_read_short(fp);
		ph->Ymax = cfile_read_short(fp);
		ph->Hdpi = cfile_read_short(fp);
		ph->Vdpi = cfile_read_short(fp);
		cfread(&ph->ColorMap, 16*3, 1, fp);
		ph->Reserved = cfile_read_byte(fp);
		ph->Nplanes = cfile_read_byte(fp);
		ph->BytesPerLine = cfile_read_short(fp);
		cfread(&ph->filler, 60, 1, fp);
	}
	return i;
}

int pcx_read_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette )
{
	PCXHeader header;
	CFILE * PCXfile;
	int i, row, col, count, xsize, ysize;
	ubyte data, *pixdata;

	PCXfile = cfopen( filename , "rb" );
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	if (PCXHeader_read_n( &header, 1, PCXfile )!=1) {
		cfclose( PCXfile );
		return PCX_ERROR_NO_HEADER;
	}

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		cfclose( PCXfile );
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;

	if ( bmp->bm_data == NULL )
		gr_init_bitmap_alloc (bmp, 0, 0, xsize, ysize, xsize);

	for (row=0; row< ysize ; row++)      {
		pixdata = &bmp->bm_data[bmp->bm_rowsize*row];
		for (col=0; col< xsize ; )      {
			if (cfread( &data, 1, 1, PCXfile )!=1 )	{
				cfclose( PCXfile );
				return PCX_ERROR_READING;
			}
			if ((data & 0xC0) == 0xC0)     {
				count =  data & 0x3F;
				if (cfread( &data, 1, 1, PCXfile )!=1 )	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				memset( pixdata, data, count );
				pixdata += count;
				col += count;
			} else {
				*pixdata++ = data;
				col++;
			}
		}
	}

	// Read the extended palette at the end of PCX file
	if ( palette != NULL )	{
		// Read in a character which should be 12 to be extended palette file
		if (cfread( &data, 1, 1, PCXfile )==1)	{
			if ( data == 12 )	{
				if (cfread(palette,768, 1, PCXfile)!=1)	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				for (i=0; i<768; i++ )
					palette[i] >>= 2;
			}
		} else {
			cfclose( PCXfile );
			return PCX_ERROR_NO_PALETTE;
		}
	}
	cfclose(PCXfile);
	return PCX_ERROR_NONE;
}

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(ubyte *inBuff, int inLen, PHYSFS_file *fp)
{
	ubyte this, last;
	int srcIndex, i;
	register int total;
	register ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		this = *(++inBuff);
		if (this == last)	{
			runCount++;			// it encodes
			if (runCount == 63)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		} else {   	// this != last
			if (runCount)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = this;
			runCount = 1;
		}
	}

	if (runCount)	{		// finish up
		if (!(i=pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_file *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == PHYSFSX_putc(fid, (int)byt))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == PHYSFSX_putc(fid, (int)0xC0 | cnt))
				return 0; 	// disk write error
			if(EOF == PHYSFSX_putc(fid, (int)byt))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//text for error messges
static const char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Couldn't read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Couldn't find palette information.\0"
	"Error writing data.\0"
};


//function to return pointer to error message
const char *pcx_errormsg(int error_number)
{
	const char *p = pcx_error_messages;

	while (error_number--) {

		if (!p) return NULL;

		p += strlen(p)+1;

	}

	return p;
}
