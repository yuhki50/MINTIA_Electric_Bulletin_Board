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

/*----------------------------------------------------------------------------

  prototype

  -----------------------------------------------------------------------------*/
void gif_open(const char * const name);
void convertImages();
