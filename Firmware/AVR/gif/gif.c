/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

/* There is a copy of the GIF89 specification, as defined by its
   pixels.r  inventor, Compuserve, in 1989, at http://members.aol.com/royalef/gif89a.txt

   This covers the high level format, but does not cover how the "data"
   contents of a GIF image represent the raster of color table indices.
   An appendix describes extensions to Lempel-Ziv that GIF makes (variable
   length compression codes and the clear and end codes), but does not
   describe the Lempel-Ziv base.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
//#include <assert.h>
#include "gif.h"
#include "../hardware.h"
#include "../led_matrix_ctrl/led_matrix_ctrl.h"
#include "../pff/pff.h"

//#define MAXCOLORTABLESIZE 256
#define MAXCOLORTABLESIZE 4

#define CM_RED 0
#define CM_GRN 1
#define CM_BLU 2

//#define MAX_LZW_BITS 12
#define MAX_LZW_BITS 6

#define COLORTABLE 0x80
#define INTERLACE 0x40

#define PLAINTEXT_EXTENSION 0x01
#define APPLICATION_EXTENSION 0xff
#define COMMENT_EXTENSION 0xfe
#define GRAPHICCONTROL_EXTENSION 0xf9

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))
#define LM_to_uint(a,b)  (((b)<<8)|(a))


extern volatile uint16_t frame[LMC_HEIGHT_COUNT];

/*----------------------------------------------------------------------------

  structure

  -----------------------------------------------------------------------------*/
typedef struct {
//	uint8_t r, g, b;
	uint8_t r;
} pixel;


/*----------------------------------------------------------------------------

  private function

  -----------------------------------------------------------------------------*/
#define exit(code) do{ \
	STATUS_LED0_ON(); \
	memset(frame, 0xFFFF, sizeof(frame)); \
} while (1);

//memset(frame, 0xFFFF, sizeof(frame));

static inline bool
ReadOK(FILE * const file, uint8_t * const buffer, size_t const len) {
	FRESULT res;
	WORD bytesRead;

	res = pf_read((WORD*)buffer, len, &bytesRead);
//	printf("%d %d\n", bytesRead, res);
	STATUS_LED1_TOGGLE();

	return (bytesRead != 0);
}


static inline void
mallocProduct(void ** const result, unsigned int const factor1, unsigned int const factor2) {
	/*----------------------------------------------------------------------------
	  malloc a space whose size in bytes is the product of 'factor1' and
	  'factor2'.  But if that size cannot be represented as an unsigned int,
	  return NULL without allocating anything.  Also return NULL if the malloc
	  fails.

	  If either factor is zero, malloc a single byte.

	  Note that malloc() actually takes a size_t size argument, so the
	  proper test would be whether the size can be represented by size_t,
	  not unsigned int.  But there is no reliable indication available to
	  us, like UINT_MAX, of what the limitations of size_t are.  We
	  assume size_t is at least as expressive as unsigned int and that
	  nobody really needs to allocate more than 4GB of memory.
	  -----------------------------------------------------------------------------*/

	if (factor1 == 0 || factor2 == 0) {
		*result = malloc(1);
	} else {
		if (UINT_MAX / factor2 < factor1) {
			*result = NULL;
		} else {
			*result = malloc(factor1 * factor2);
		}
	}
}
#define MALLOCARRAY(arrayName, nElements) do { \
	void * array; \
	mallocProduct(&array, nElements, sizeof(arrayName[0])); \
	arrayName = array; \
} while (0);


pixel**
img_allocarray(uint16_t const cols, uint16_t const rows) {
	/*----------------------------------------------------------------------------
	  Allocate an array of 'rows' rows of 'cols' columns each, with each
	  element 'size' bytes.

	  We use a special format where we tack on an extra element to the row
	  index to indicate the format of the array.

	  We have two ways of allocating the space: fragmented and
	  unfragmented.  In both, the row index (plus the extra element) is
	  in one block of memory.  In the fragmented format, each row is
	  also in an independent memory block, and the extra row pointer is
	  NULL.  In the unfragmented format, all the rows are in a single
	  block of memory called the row heap and the extra row pointer is
	  the address of that block.

	  We use unfragmented format if possible, but if the allocation of the
	  row heap fails, we fall back to fragmented.
	  -----------------------------------------------------------------------------*/

	char** rowIndex;
	char * rowheap;
	uint8_t size; 

	size = sizeof(pixel);

	MALLOCARRAY(rowIndex, rows + 1);
	if (rowIndex == NULL) {
//		printf("out of memory allocating row index (%u rows) for an array", rows);
		puts("E01");
		exit(1);
	}

	uint16_t row;

	rowheap = malloc(rows * cols * size);
	if (rowheap == NULL) {
		/* We couldn't get the whole heap in one block, so try fragmented
		   format.
		 */
		rowIndex[rows] = NULL;   /* Declare it fragmented format */

		for (row = 0; row < rows; ++row) {
			if (UINT_MAX / cols < size) {
//				printf("Arithmetic overflow multiplying %u by %u to get the "
//						"size of a row to allocate.", cols, size);
				puts("E02");
				exit(1);
			}

			rowIndex[row] = malloc(cols * size);

			if (rowIndex[row] == NULL) {
//				printf("out of memory allocating Row %u (%u columns, %u bytes per tuple) "
//						"of an array", row, cols, size);
				puts("E03");
				exit(1);
			}
		}
	} else {
		/* It's unfragmented format */
		rowIndex[rows] = rowheap;  /* Declare it unfragmented format */

		for (row = 0; row < rows; ++row) {
			rowIndex[row] = &(rowheap[row * cols * size]);
		}
	}
	return (pixel**)rowIndex;
}


void
img_freearray(pixel ** const rowIndex, uint16_t const rows) {
	void * const rowheap = rowIndex[rows];

	if (rowheap != NULL)
		free(rowheap);
	else {
		uint16_t row;
		for (row = 0; row < rows; ++row) {
			free(rowIndex[row]);
		}
	}
	free(rowIndex);
}


static uint16_t const maxnum_lzwCode = (1 << MAX_LZW_BITS);


typedef uint8_t gifColorTable[3][MAXCOLORTABLESIZE];


typedef struct {
	uint16_t width;
	uint16_t height;
	uint8_t colorResolution;
	uint8_t background;
	float pixelAspectRatio;
	uint16_t globalColorTableSize;
	gifColorTable globalColorTable;
} gifScreen;


typedef struct {
	uint8_t disposal;
	bool userInput;
	uint16_t delayTime;
	bool useTransparent;
	uint8_t transparentIndex;
} gifCtrl;


typedef struct {
	uint16_t leftPosition;
	uint16_t topPosition;
	uint16_t width;
	uint16_t height;
	bool useInterlace;
	bool useLocalColorTable;
	gifColorTable localColorTable;
	uint16_t localColorTableSize;
} gifImageDesc;


static void
initGifCtrl(gifCtrl * const gifCtrl) {
	gifCtrl->disposal = 0;
	gifCtrl->userInput = false;
	gifCtrl->delayTime = 0;
	gifCtrl->useTransparent = false;
	gifCtrl->transparentIndex = 0;
}


static void
readColorTable(FILE * gifFile, gifColorTable colorTable, uint16_t const colorTableSize) {
	uint16_t i;
	uint8_t rgb[3];

//	assert(colorTableSize <= MAXCOLORTABLESIZE);

	for (i = 0; i < colorTableSize; ++i) {
		if (!ReadOK(gifFile, rgb, sizeof(rgb))) {
//			printf("Unable to read Color %d from colorTable\n", i);
			puts("E04");
			exit(1);
		}

		colorTable[CM_RED][i] = rgb[0];
		colorTable[CM_GRN][i] = rgb[1];
		colorTable[CM_BLU][i] = rgb[2];
	}
}


static bool zeroDataBlock = false;
/* the most recently read DataBlock was an EOD marker, i.e. had zero length */


static void
getDataBlock(FILE * const gifFile, uint8_t * const buf, bool * const eof, uint8_t * const length) {
	/*----------------------------------------------------------------------------
	  Read a DataBlock from file 'gifFile', return it at 'buf'.

	  The first byte of the datablock is the length, in pure binary, of the
	  rest of the datablock.  We return the data portion (not the length byte)
	  of the datablock at 'buf', and its length as *length.

	  Except that if we hit EOF or have an I/O error reading the first
	  byte (size field) of the DataBlock, we return *eof == true and
	 *length == 0.

	 We return *eof == false if we don't hit EOF or have an I/O error.
	 -----------------------------------------------------------------------------*/
	uint8_t count;

	if (!ReadOK(gifFile, &count, 1)) {
//		printf("EOF or error in reading DataBlock size from file\n" );
		puts("E05");
		*eof = true;
		*length = 0;
	} else {
		*eof = false;
		*length = count;

		if (count == 0) {
			zeroDataBlock = true;
		} else {
			zeroDataBlock = false;

			if (!ReadOK(gifFile, buf, count)) {
//				printf("EOF or error reading data portion of %d byte DataBlock from file\n", count);
				puts("E06");
				exit(1);
			}
		}
	}
}


static void
readThroughEod(FILE * const gifFile) {
	/*----------------------------------------------------------------------------
	  Read the file 'gifFile' through the next EOD marker.  An EOD marker is a
	  a zero length data block.

	  If there is no EOD marker between the present file position and EOF,
	  we read to EOF and issue warning message about a missing EOD marker.
	  -----------------------------------------------------------------------------*/
	uint8_t buf[256];
	uint8_t count;
	bool eod;

	eod = false;
	while (!eod) {
		bool eof;

		getDataBlock(gifFile, buf, &eof, &count);
		if (eof) {
//			printf("EOF encountered before EOD marker.  The GIF file is malformed, but we are proceeding "
//					"anyway as if an EOD marker were at the end of the file.\n");
			puts("E07");
		}
		if (eof || count == 0) {
			eod = true;
		}
	}
}



//static void
//doCommentExtension(FILE * const gifFile) {
//	/*----------------------------------------------------------------------------
//	  Read the rest of a comment extension from the input file 'gifFile' and handle
//	  it.
//
//	  We ought to deal with the possibility that the comment is not text.  I.e.
//	  it could have nonprintable characters or embedded nulls.  I don't know if
//	  the GIF spec requires regular text or not.
//	  -----------------------------------------------------------------------------*/
//	uint8_t buf[256];
//	uint8_t blocklen;
//	bool done;
//
//	done = false;
//	while (!done) {
//		bool eof;
//
//		getDataBlock(gifFile, buf, &eof, &blocklen); 
//
//		if (blocklen == 0 || eof) {
//			done = true;
//		} else {
//			buf[blocklen] = '\0';
//			printf("gif comment: %s\n", buf);
//		}
//	}
//}


static void 
doGraphicControlExtension(FILE * const gifFile, gifCtrl * const gifCtrl) {
	uint8_t buf[256];
	uint8_t length;
	bool eof;

	getDataBlock(gifFile, buf, &eof, &length);
	if (eof) {
//		printf("EOF/error encountered reading "
//				"1st DataBlock of Graphic Control Extension.\n");
		puts("E08");
		exit(1);
	} else if (length < 4) {
//		printf("graphic control extension 1st DataBlock too short.  "
//				"It must be at least 4 bytes; it is %d bytes.\n", length);
		puts("E09");
		exit(1);
	} else {
		gifCtrl->disposal = (buf[0] >> 2) & 0x07;
		gifCtrl->userInput = (buf[0] >> 1) & 0x01;
		gifCtrl->delayTime = LM_to_uint(buf[1],buf[2]);
		gifCtrl->useTransparent = buf[0] & 0x01;
		gifCtrl->transparentIndex = buf[3];

		readThroughEod(gifFile);
	}
}


static void
doExtension(FILE * const gifFile, uint8_t const label, gifCtrl * const gifCtrl) {
//	const char * str;

	switch (label) {
		case PLAINTEXT_EXTENSION:
//			str = "Plain Text";
			readThroughEod(gifFile);
			break;

		case APPLICATION_EXTENSION:
//			str = "Application";
			readThroughEod(gifFile);
			break;

		case COMMENT_EXTENSION:
//			str = "Comment";
//			doCommentExtension(gifFile);
			readThroughEod(gifFile);
			break;

		case GRAPHICCONTROL_EXTENSION:
//			str = "Graphic Control";
			doGraphicControlExtension(gifFile, gifCtrl);
			break;

		default:
			{
//				char buf[256];
//				str = buf;
//				sprintf(buf, "UNKNOWN (0x%02x)", label);
//				printf("Ignoring unrecognized extension (type 0x%02x)\n", label);
				readThroughEod(gifFile);
			}
			break;
	}
//	printf(" got a '%s' extension\n", str );
}


typedef struct {
//	uint8_t buf[280];
	uint8_t buf[225];
	/* This is the buffer through which we read the data from the 
	   stream.  We must buffer it because we have to read whole data
	   blocks at a time, but our client wants one code at a time.
	   The buffer typically contains the contents of one data block
	   plus two bytes from the previous data block.
	 */
	int bufCount;
	/* This is the number of bytes of contents in buf[]. */
	int curbit;
	/* The bit number (starting at 0) within buf[] of the next bit
	   to be returned.  If the next bit to be returned is not yet in
	   buf[] (we've already returned everything in there), this points
	   one beyond the end of the buffer contents.
	 */
	bool streamExhausted;
	/* The last time we read from the input stream, we got an EOD marker
	   or EOF
	 */
} getCodeState;


static void
initGetCode(getCodeState * const getCodeState) {
	/* Fake a previous data block */
	getCodeState->buf[0] = 0;
	getCodeState->buf[1] = 0;
	getCodeState->bufCount = 2;
	getCodeState->curbit = getCodeState->bufCount * 8;
	getCodeState->streamExhausted = false;
}


static void
getAnotherBlock(FILE * const gifFile, getCodeState * const getCodeState) {
	uint8_t count;
	uint16_t assumed_count;
	bool eof;

	/* Shift buffer down so last two bytes are now the
	   first two bytes.  Shift 'curbit' cursor, which must
	   be somewhere in or immediately after those two
	   bytes, accordingly.
	 */
	getCodeState->buf[0] = getCodeState->buf[getCodeState->bufCount - 2];
	getCodeState->buf[1] = getCodeState->buf[getCodeState->bufCount - 1];

	getCodeState->curbit -= (getCodeState->bufCount - 2) * 8;
	getCodeState->bufCount = 2;

	/* Add the next block to the buffer */
	getDataBlock(gifFile, &getCodeState->buf[getCodeState->bufCount], &eof, &count);
	if (eof) {
//		printf("EOF encountered in image before EOD marker.  The GIF file is malformed, but we are proceeding "
//				"anyway as if an EOD marker were at the end of the file.\n");
		puts("E10");
		assumed_count = 0;
	} else {
		assumed_count = count;
	}

	getCodeState->streamExhausted = (assumed_count == 0);
	getCodeState->bufCount += assumed_count;
}


static void
doGetCode(FILE * const gifFile, int const codeSize, getCodeState * const getCodeState, int * const retval) {

	while (getCodeState->curbit + codeSize > getCodeState->bufCount * 8 && !getCodeState->streamExhausted) 
		/* Not enough left in buffer to satisfy request.  Get the next
		   data block into the buffer.

		   Note that a data block may be as small as one byte, so we may need
		   to do this multiple times to get the full code.  (This probably
		   never happens in practice).
		 */
		getAnotherBlock(gifFile, getCodeState);

	if ((getCodeState->curbit+codeSize) > getCodeState->bufCount * 8) {
		/* If the buffer still doesn't have enough bits in it, that means
		   there were no data blocks left to read.
		 */
		*retval = -1;  /* EOF */

		int const bitsUnused = getCodeState->bufCount * 8 - getCodeState->curbit;
		if (bitsUnused > 0) {
//			printf("Stream ends with a partial code (%d bits left in file; "
//					"expected a %d bit code).  Ignoring.\n", bitsUnused, codeSize);
			puts("E11");
		}
	} else {
		int i, j;
		int code;
		uint8_t * const buf = getCodeState->buf;

		code = 0;  /* initial value */
		for (i = getCodeState->curbit, j = 0; j < codeSize; ++i, ++j) {
			code |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;
		}
		getCodeState->curbit += codeSize;
		*retval = code;
	}
}


static int
getCode(FILE * const gifFile, int const codeSize, bool const init) {
	/*----------------------------------------------------------------------------
	  If 'init', initialize the code getter.

	  Otherwise, read and return the next lzw code from the file *gifFile.

	  'codeSize' is the number of bits in the code we are to get.

	  Return -1 instead of a code if we encounter the end of the file.
	  -----------------------------------------------------------------------------*/
	static getCodeState getCodeState;

	int retval;

	if (init) {
		initGetCode(&getCodeState);
		retval = 0;
	} else {
		doGetCode(gifFile, codeSize, &getCodeState, &retval);
	}
	return retval;
}


typedef struct {
	int * stack;  // malloc'ed array
	int * sp;  // stack pointer
	int * top;  // next word above top of stack
} stack;


static void 
initStack(stack * const stackP, uint16_t const size) {
	MALLOCARRAY(stackP->stack, size);
	if (stackP->stack == NULL) {
//		printf("Unable to allocate %d-word stack.\n", size);
		puts("E12");
		exit(1);
	}
	stackP->sp = stackP->stack;
	stackP->top = stackP->stack + size;
}


static void
pushStack(stack * const stack, int const value) {
	if (stack->sp >= stack->top) {
//		printf("stack overflow\n");
		puts("E13");
		exit(1);
	}
	*(stack->sp++) = value;
}


static bool
stackIsEmpty(const stack * const stack) {
	return stack->sp == stack->stack;
}


static int
popStack(stack * const stack) {
	if (stack->sp <= stack->stack) {
//		printf("stack underflow\n");
		puts("E14");
		exit(1);
	}
	return *(--stack->sp);
}


static void
termStack(stack * const stack) {
	free(stack->stack);
	stack->stack = NULL;
}



/*----------------------------------------------------------------------------
  Some notes on LZW.

  LZW is an extension of Limpel-Ziv.  The two extensions are:

  1) in Limpel-Ziv, codes are all the same number of bits.  In
  LZW, they start out small and increase as the stream progresses.

  2) LZW has a clear code that resets the string table and code
  size.

  The LZW code space is allocated as follows:

  The true data elements are dataWidth bits wide, so the maximum
  value of a true data element is 2**dataWidth-1.  We call that
  max_dataVal.  The first byte in the stream tells you what dataWidth
  is.

  LZW codes 0 - max_dataVal are direct codes.  Each on represents
  the true data element whose value is that of the LZW code itself.
  No decompression is required.

  max_dataVal + 1 and up are compression codes.  They encode
  true data elements:

  max_dataVal + 1 is the clear code.

  max_dataVal + 2 is the end code.

  max_dataVal + 3 and up are string codes.  Each string code 
  represents a string of true data elements.  The translation from a
  string code to the string of true data elements varies as the stream
  progresses.  In the beginning and after every clear code, the
  translation table is empty, so no string codes are valid.  As the
  stream progresses, the table gets filled and more string codes 
  become valid.

  -----------------------------------------------------------------------------*/


typedef struct {
	stack stack;
	int fresh;
	/* The stream is right after a clear code or at the very beginning */
	int codeSize;
	/* The current code size -- each LZW code in this part of the image
	   is this many bits.  Ergo, we read this many bits at a time from
	   the stream.
	 */
	int maxnum_code;
	/* The maximum number of LZW codes that can be represented with the 
	   current code size.  (1 << codeSize)
	 */
	int next_tableSlot;
	/* Index in the code translation table of the next free entry */
	int firstcode;
	/* This is always a true data element code */
	int prevcode;
	/* The code just before, in the image, the one we're processing now */
	int table[2][(1 << MAX_LZW_BITS)];

	/* The following are constant for the life of the decompressor */
	FILE * gifFile;
	uint8_t init_codeSize;
	int max_dataVal;
	int clear_code;
	int end_code; 
} decompressor;


static void
resetDecompressor(decompressor * const decomp) {
	decomp->codeSize = decomp->init_codeSize+1;
	decomp->maxnum_code = 1 << decomp->codeSize;
	decomp->next_tableSlot = decomp->max_dataVal + 3;
	decomp->fresh = 1;
}


static void
lzwInit(decompressor * const decomp, FILE * const gifFile, uint8_t const init_codeSize) {

//	printf("Image says the initial compression code size is " "%d bits\n", init_codeSize);

	decomp->gifFile = gifFile;
	decomp->init_codeSize = init_codeSize;

//	assert(decomp->init_codeSize < sizeof(decomp->max_dataVal) * 8);

	decomp->max_dataVal = (1 << init_codeSize) - 1;
	decomp->clear_code = decomp->max_dataVal + 1;
	decomp->end_code = decomp->max_dataVal + 2;

//	printf("Initial code size is %u bits; clear code = 0x%02x, end code = 0x%02x\n",
//			decomp->init_codeSize, decomp->clear_code, decomp->end_code);

	/* The entries in the translation table for true data codes are
	   constant throughout the stream.  We set them now and they never
	   change.
	 */
	uint16_t i;
	for (i = 0; i <= decomp->max_dataVal; ++i) {
		decomp->table[0][i] = 0;
		decomp->table[1][i] = i;
	}
	resetDecompressor(decomp);

	getCode(decomp->gifFile, 0, true);

	decomp->fresh = true;

	initStack(&decomp->stack, maxnum_lzwCode * 2);
}


static void
lzwTerm(decompressor * const decomp) {
	termStack(&decomp->stack);
}


static void
expandCodeOntoStack(decompressor * const decomp, int const incode, bool * const error) {
	/*----------------------------------------------------------------------------
	  'incode' is an LZW string code.  It represents a string of true data
	  elements, as defined by the string translation table in *decompP.

	  Expand the code to a string of LZW direct codes and push them onto the
	  stack such that the leftmost code is on top.

	  Also add to the translation table where appropriate.

	  Iff the translation table contains a cycle (which means the LZW stream
	  from which it was built is invalid), return *errorP == true.
	  -----------------------------------------------------------------------------*/
	int code;

	*error = false;

	if (incode < decomp->next_tableSlot) {
		code = incode;
	} else {
		/* It's a code that isn't in our translation table yet */
		pushStack(&decomp->stack, decomp->firstcode);
		code = decomp->prevcode;
	}

	/* Get the whole string that this compression code
	   represents and push it onto the code stack so the
	   leftmost code is on top.  Set decompP->firstcode to the
	   first (leftmost) code in that string.
	 */

	unsigned int stringCount;
	stringCount = 0;

	while (code > decomp->max_dataVal && !(*error)) {
		if (stringCount > maxnum_lzwCode) {
//			printf("Error in GIF image: contains LZW string loop\n");
			puts("E15");
			*error = true;
		} else {
			++stringCount;
			pushStack(&decomp->stack, decomp->table[1][code]);
			code = decomp->table[0][code];
		}
	}
	decomp->firstcode = decomp->table[1][code];
	pushStack(&decomp->stack, decomp->firstcode);

	if (decomp->next_tableSlot < maxnum_lzwCode) {
		decomp->table[0][decomp->next_tableSlot] = decomp->prevcode;
		decomp->table[1][decomp->next_tableSlot] = decomp->firstcode;
		++decomp->next_tableSlot;
		if (decomp->next_tableSlot >= decomp->maxnum_code) {
			/* We've used up all the codes of the current code size.
			   Future codes in the stream will have codes one bit longer.
			   But there's an exception if we're already at the LZW
			   maximum, in which case the codes will simply continue
			   the same size.
			 */
			if (decomp->codeSize < MAX_LZW_BITS) {
				++decomp->codeSize;
				decomp->maxnum_code = 1 << decomp->codeSize;
			}
		}
	}

	decomp->prevcode = incode;
}


static int
lzwReadByte(decompressor * const decomp) {
	/*----------------------------------------------------------------------------
	  Return the next data element of the decompressed image.  In the context
	  of a GIF, a data element is the color table index of one pixel.

	  We read and return the next byte of the decompressed image, or:

	  Return -1 if we hit EOF prematurely (i.e. before an "end" code.  We
	  forgive the case that the "end" code is followed by EOF instead of
	  an EOD marker (zero length DataBlock)).

	  Return -2 if there are no more bytes in the image.  In that case,
	  make sure the file is positioned immediately after the image (i.e.
	  after the EOD marker that marks the end of the image or EOF).

	  Return -3 if we encounter errors in the LZW stream.
	  -----------------------------------------------------------------------------*/
	int retval;

	if (!stackIsEmpty(&decomp->stack)) {
		retval = popStack(&decomp->stack);
	} else if (decomp->fresh) {
		decomp->fresh = false;
		/* Read off all initial clear codes, read the first non-clear code,
		   and return it.  There are no strings in the table yet, so the next
		   code must be a direct true data code.
		 */
		do {
			decomp->firstcode = getCode(decomp->gifFile, decomp->codeSize, false);
			decomp->prevcode = decomp->firstcode;
		} while (decomp->firstcode == decomp->clear_code);
		if (decomp->firstcode == decomp->end_code) {
			if (!zeroDataBlock) {
				readThroughEod(decomp->gifFile);
			}
			retval = -2;
		} else {
			retval = decomp->firstcode;
		}
	} else {
		int code;
		code = getCode(decomp->gifFile, decomp->codeSize, false);
		if (code == -1) {
			retval = -1;
		} else {
//			assert(code >= 0);  /* -1 is only possible error return */
			if (code == decomp->clear_code) {
				resetDecompressor(decomp);
				retval = lzwReadByte(decomp);
			} else {
				if (code == decomp->end_code) {
					if (!zeroDataBlock) {
						readThroughEod(decomp->gifFile);
					}
					retval = -2;
				} else {
					bool error;
					expandCodeOntoStack(decomp, code, &error);
					if (error) {
						retval = -3;
					} else {
						retval = popStack(&decomp->stack);
					}
				}
			}
		}
	}
	return retval;
}


enum pass {MULT8PLUS0, MULT8PLUS4, MULT4PLUS2, MULT2PLUS1};


static void
bumpRowInterlace(uint16_t * const row, uint16_t const rows, enum pass * const pass) {
	/*----------------------------------------------------------------------------
	  Move *pixelCursor to the next row in the interlace pattern.
	  -----------------------------------------------------------------------------*/
	/* There are 4 passes:
MULT8PLUS0: Rows 8, 16, 24, 32, etc.
MULT8PLUS4: Rows 4, 12, 20, 28, etc.
MULT4PLUS2: Rows 2, 6, 10, 14, etc.
MULT2PLUS1: Rows 1, 3, 5, 7, 9, etc.
	 */

	switch (*pass) {
		case MULT8PLUS0:
			*row += 8;
			break;

		case MULT8PLUS4:
			*row += 8;
			break;

		case MULT4PLUS2:
			*row += 4;
			break;

		case MULT2PLUS1:
			*row += 2;
			break;
	}
	/* Set the proper pass for the next read.  Note that if there are
	   more than 4 rows, the sequence of passes is sequential, but
	   when there are fewer than 4, we may skip e.g. from MULT8PLUS0
	   to MULT4PLUS2.
	 */
	while (*row >= rows && *pass != MULT2PLUS1) {
		switch (*pass) {
			case MULT8PLUS0:
				*pass = MULT8PLUS4;
				*row = 4;
				break;

			case MULT8PLUS4:
				*pass = MULT4PLUS2;
				*row = 2;
				break;

			case MULT4PLUS2:
				*pass = MULT2PLUS1;
				*row = 1;
				break;

			case MULT2PLUS1:
				/* We've read the entire image */
				break;
		}
	}
}


typedef struct {
	pixel ** pixels;
	uint16_t col;
	uint16_t row;
} imageBuffer;


static void
addPixelToRaster(uint8_t const colorTableIndex,
		imageBuffer * const imageBuffer,
		uint16_t const cols,
		uint16_t const rows,
		gifColorTable colorTable, 
		uint16_t const colorTableSize,
		bool const useInterlace,
		enum pass * const pass) {

	if (colorTableIndex >= colorTableSize) {
//		printf("Invalid color index %u in an image that has only "
//				"%u colors in the color table.\n", colorTableIndex, colorTableSize);
		puts("E16");
		exit(1);
	}

	imageBuffer->pixels[imageBuffer->row][imageBuffer->col].r = colorTable[CM_RED][colorTableIndex];
//	imageBuffer->pixels[imageBuffer->row][imageBuffer->col].g = colorTable[CM_GRN][colorTableIndex];
//	imageBuffer->pixels[imageBuffer->row][imageBuffer->col].b = colorTable[CM_BLU][colorTableIndex];

	++imageBuffer->col;
	if (imageBuffer->col == cols) {
		imageBuffer->col = 0;
		if (useInterlace) {
			bumpRowInterlace(&imageBuffer->row, rows, pass);
		} else {
			++imageBuffer->row;
		}
	}
}


static void
readImageData(FILE * const gifFile, 
		pixel ** const pixels, 
		uint16_t const cols,
		uint16_t const rows,
		gifColorTable colorTable,
		uint16_t const colorTableSize,
		bool const useInterlace,
		uint8_t const transparentIndex) {

	uint8_t lzwMinCodeSize;
	enum pass pass;
	decompressor decomp;
	imageBuffer imageBuffer;
	bool gotMinCodeSize;

	pass = MULT8PLUS0;

	imageBuffer.pixels = pixels;
	imageBuffer.col = 0;
	imageBuffer.row = 0;

	gotMinCodeSize = ReadOK(gifFile, &lzwMinCodeSize, 1);
	if (!gotMinCodeSize) {
//		printf("GIF stream ends (or read error) "
//				"right after an image separator; no "
//				"image data follows.\n");
		puts("E17");
		exit(1);
	}

	if (lzwMinCodeSize > MAX_LZW_BITS) {
//		printf("Invalid minimum code size value in image data: %u.  "
//				"Maximum allowable code size in GIF is %u\n", 
//				lzwMinCodeSize, MAX_LZW_BITS);
		puts("E18");
		exit(1);
	}

	lzwInit(&decomp, gifFile, lzwMinCodeSize);


	while (imageBuffer.row < rows) {
		int const rc = lzwReadByte(&decomp);

		switch (rc) {
			case -3:
//				printf("Error in GIF input stream\n");
				puts("E19");
				exit(1);
				break;

			case -2:
//				printf("Error in GIF image: Not enough raster data to fill "
//						"%u x %u dimensions.  Ran out of raster data in "
//						"row %u\n", cols, rows, imageBuffer.row);
				puts("E20");
				exit(1);
				break;

			case -1:
//				printf("Premature end of file; no proper GIF closing\n");
				puts("E21");
				exit(1);
				break;

			default:
				addPixelToRaster(rc, &imageBuffer, cols, rows, colorTable, colorTableSize,
						useInterlace, &pass);
				break;
		}
	}

	if (lzwReadByte(&decomp) >= 0) {
//		printf("Extraneous data at end of image.  Skipped to end of image\n");
		puts("E22");
	}

	lzwTerm(&decomp);
}


static void
writeData(FILE * const gifFile, pixel ** const pixels, 
		const uint16_t width, const uint16_t height) {
	/*----------------------------------------------------------------------------
	  Write a PNM image to the current position of file 'gifFile' with
	  dimensions 'width' x 'height' and raster 'pixels'.
	  -----------------------------------------------------------------------------*/
//	printf("writing a file\n");

	uint16_t x, y;
//	uint8_t r, g, b, value;
	uint8_t value;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			value = pixels[y][x].r;

//			r = pixels[y][x].r;
//			g = pixels[y][x].g;
//			b = pixels[y][x].b;

//			value = (r * 3 + g * 6 + b * 1) / 10;

			if (value > 128 + 64) {
//				printf("XX");
				frame[y] |= (0x0001 << x);
			} else {
//				printf("--");
				frame[y] &= ~(0x0001 << x);
			}
		}
//		puts("");
	}
}


static void
readGifHeader(FILE * const gifFile, gifScreen * const gifScreen) {
	/*----------------------------------------------------------------------------
	  Read the GIF stream header off the file gifFile, which is present
	  positioned to the beginning of a GIF stream.  Return the info from it
	  as *gifScreen.
	  -----------------------------------------------------------------------------*/
	uint8_t buf[16];
	char version[4];


	if (!ReadOK(gifFile, buf, 6)) {
//		printf("error reading magic number\n" );
		puts("E23");
		exit(1);
	}

	// check file signature //
	if (strncmp((char *)buf, "GIF", 3) != 0) {
//		printf("File does not contain a GIF stream.  It does not start with 'GIF'.\n");
		puts("E24");
		exit(1);
	}

	// check version //
	strncpy(version, (char *)buf + 3, 3);
	version[3] = '\0';

	if (strcmp(version, "87a") != 0 && strcmp(version, "89a")) {
//		printf("bad version number, not '87a' or '89a'\n" );
		puts("E25");
		exit(1);
	}

//	printf("GIF format version is '%s'\n", version);


	if (!ReadOK(gifFile, buf, 7)) {
//		printf("failed to read screen descriptor\n" );
		puts("E26");
		exit(1);
	}

	gifScreen->width = LM_to_uint(buf[0], buf[1]);
	gifScreen->height = LM_to_uint(buf[2], buf[3]);
	gifScreen->colorResolution = (buf[4] & 0x70 >> 4) + 1;
	gifScreen->background = buf[5];
	gifScreen->pixelAspectRatio = (buf[6] == 0) ?  1.0 : (buf[6] + 15.0) / 64.0;
	gifScreen->globalColorTableSize = 1 << ((buf[4] & 0x07) + 1);

//	printf("GIF Width = %d GIF Height = %d " "Pixel aspect ratio = %f\n",
//			gifScreen->width, gifScreen->height, gifScreen->pixelAspectRatio);
//	printf("Colors = %d   Color Resolution = %d\n",
//			gifScreen->globalColorTableSize, gifScreen->colorResolution);

	// read global color table //
	if (BitSet(buf[4], COLORTABLE)) {
		readColorTable(gifFile, gifScreen->globalColorTable, gifScreen->globalColorTableSize);
	}

	// check aspect ration //
	/* if you need un-square pixel, it comment out */
	if (gifScreen->pixelAspectRatio != 1.0) {
//		printf("warning - input pixels are not square!");
		puts("E27");
	}
}


static void
readExtensions(FILE * const gifFile, gifCtrl * const gifCtrl, bool * const eod) {
	/*----------------------------------------------------------------------------
	  Read extension blocks from the GIF stream to which the file *gifFile is
	  positioned.  Read up through the image separator that begins the
	  next image or GIF stream terminator.

	  If we encounter EOD (end of GIF stream) before we find an image 
	  separator, we return *eod == true.  Else *eod == false.

	  If we hit end of file before an EOD marker, we abort the program with
	  an error message.
	  -----------------------------------------------------------------------------*/
	bool imageStart;

	imageStart = false;
	*eod = false;

	// read the image descriptor //
	while (!imageStart && !(*eod)) {
		uint8_t c;

		if (!ReadOK(gifFile, &c, 1)) {
//			printf("EOF / read error on image data\n" );
			puts("E28");
			exit(1);
		}

		if (c == ';') {  // GIF terminator
			*eod = true;
		} else if (c == '!') {  // Extension
			if (!ReadOK(gifFile, &c, 1)) {
//				printf("EOF / read error on extension function code\n");
				puts("E29");
				exit(1);
			}
			doExtension(gifFile, c, gifCtrl);
		} else if (c == ',') {
			imageStart = true;
		} else {
//			printf("bogus character 0x%02x, ignoring\n", c);
			puts("E30");
		}
	}
}


static void
convertImage(FILE * const gifFile,
		gifScreen * const gifScreen,
		gifCtrl * const gifCtrl,
		gifImageDesc * const gifImageDesc) {
	/*----------------------------------------------------------------------------
	  Read a single GIF image from the current position of file.
	  -----------------------------------------------------------------------------*/
	uint8_t buf[16];
	pixel **pixels;

	if (!ReadOK(gifFile, buf, 9)) {
//		printf("couldn't read left/top/width/height\n");
		puts("E31");
		exit(1);
	}

	gifImageDesc->leftPosition = LM_to_uint(buf[0], buf[1]);
	gifImageDesc->topPosition = LM_to_uint(buf[2], buf[3]);
	gifImageDesc->width = LM_to_uint(buf[4], buf[5]);
	gifImageDesc->height = LM_to_uint(buf[6], buf[7]);
	gifImageDesc->useInterlace = BitSet(buf[8], INTERLACE);
	gifImageDesc->useLocalColorTable = BitSet(buf[8], COLORTABLE);
	gifImageDesc->localColorTableSize = 1 << ((buf[8] & 0x07) + 1);


//	printf("reading %u by %u%s GIF image\n", gifImageDesc->width, gifImageDesc->height,
//			gifImageDesc->useInterlace ? "interlaced" : "");

//	if (gifImageDesc->useLocalColorTable) {
//		printf("  Uses local color table of %u colors\n", gifImageDesc->localColorTableSize);
//	} else {	
//		printf("  Uses global color table of %u colors\n", gifScreen->globalColorTableSize);
//	}


	pixels = img_allocarray(gifImageDesc->width, gifImageDesc->height);

	if (!pixels) {
//		printf("couldn't alloc space for image\n" );
		puts("E32");
		exit(1);
	}

	if (gifImageDesc->useLocalColorTable) {
		readColorTable(gifFile, gifImageDesc->localColorTable, gifImageDesc->localColorTableSize);

		readImageData(gifFile, pixels, gifImageDesc->width, gifImageDesc->height,
				gifImageDesc->localColorTable, gifImageDesc->localColorTableSize,
				gifImageDesc->useInterlace, gifCtrl->transparentIndex);

		writeData(gifFile, pixels, gifImageDesc->width, gifImageDesc->height);

		_delay_ms(gifCtrl->delayTime * 5);  // display hold time
	} else {
		readImageData(gifFile, pixels, gifImageDesc->width, gifImageDesc->height, 
				gifScreen->globalColorTable, gifScreen->globalColorTableSize,
				gifImageDesc->useInterlace, gifCtrl->transparentIndex);

		writeData(gifFile, pixels, gifImageDesc->width, gifImageDesc->height);

		_delay_ms(gifCtrl->delayTime * 5);  // display hold time
	}

	img_freearray(pixels, gifImageDesc->height);
}



/*----------------------------------------------------------------------------

  public function

  -----------------------------------------------------------------------------*/
FATFS fs;

void
gif_open(const char * const name) {
	FRESULT res;

	res = pf_mount(&fs);
//	printf("res: %d\n", res);

	res = pf_open(name);
//	printf("res: %d\n", res);
}


void
convertImages(FILE * const gifFile) {
	gifScreen gifScreen;
	gifCtrl gifCtrl;
	gifImageDesc gifImageDesc;
	bool eod = false;

	readGifHeader(gifFile, &gifScreen);

	if (gifScreen.width != LMC_WIDTH_COUNT || gifScreen.height != LMC_HEIGHT_COUNT) {
		STATUS_LED0_ON();
		return;
	}

	initGifCtrl(&gifCtrl);

	while (!eod) {
		readExtensions(gifFile, &gifCtrl, &eod);

		if (!eod) {
//			puts("Reading Image Sequence");

			convertImage(gifFile, &gifScreen, &gifCtrl, &gifImageDesc);
		}
	}
}
