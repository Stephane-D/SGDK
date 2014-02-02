/****************************************************************
*			lz77 packer v1.3			*
*****************************************************************
* Source adapted and optimized to/for Atari 16/32bit machines	*
* by ray//.tSCc. 2003. Final bugfixes 01.04.05		*
*								*
* Base framework: Haruhiko Okumura				*
*****************************************************************
* eml:	 ray@tscc.de						*
* http://ray.tscc.de						*
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define N		4096	/* Size of dictonary */
#define F		17	/* Upper limit for match length */
#define THRESHOLD	2	/* Encode strings longer than this */

#define NIL		N	/* Index for root of binary search trees */

unsigned long int textsize = 0,	/* Text size counter */
		  codesize = 0;	/* Code size counter */

unsigned char text_buf[N+F-1];	/* Ring buffer for string comparison */

int match_position, match_length;	/* Set by the InsertNode() procedure. */
int lson[N+1],rson[N+257], dad[N+1];	/* Left & right children & parents (binary search trees) */
int a = 0;
FILE *infile, *outfile;



/*******************************************
*
*  void InitTree(void)
*
* Initialise binary search trees for fast
* string comparison
*
*******************************************/

void InitTree(void)
{
	int  i;

	for (i=N+1;i<=N+256;i++) rson[i] = NIL;
	for (i=0;i<N;i++) dad[i] = NIL;
}


/*******************************************
*
*  void InsertNode(int r)
*
* Insert text_buf[r..r+F-1], into one of the trees and return
* the longest-match position and length.
*
* If match_length = F, then removes the old node.
*
*******************************************/

void InsertNode(int r)
{
	int  i, p, cmp;
	unsigned char	*key;

	if (r > ftell(infile)) { return; }

	cmp = 1;
	key = &text_buf[r];
	p = N+1+key[0];

	rson[r] = lson[r] = NIL;
	match_length = 0;
	for (;;) {
		if (cmp >= 0) {
			if (rson[p] != NIL)
				p = rson[p];
			else {
				rson[p] = r;
				dad[r] = p;
				return;
			}
		}
		else {
			if (lson[p] != NIL)
				p = lson[p];
			else {
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)
				break;
		if (i > match_length) {
			match_position = r-p;
			if ((match_length = i) >= F)
				break;
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];

	dad[lson[p]] = r;
	dad[rson[p]] = r;
	if (rson[dad[p]] == p)
		rson[dad[p]] = r;
	else
		lson[dad[p]] = r;
	dad[p] = NIL;	/* remove p */
}


/*******************************************
*
*  void DeleteNode(int p)
*
* InsertNode's counterpart
*
*******************************************/

void DeleteNode(int p)
{
	int  q;

	if (dad[p] == NIL)
		return;  /* not in tree */
	if (rson[p] == NIL)
		q = lson[p];
	else if (lson[p] == NIL)
		q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {
				q = rson[q];
			} while (rson[q] != NIL);
			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}
		rson[q] = rson[p];
		dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p)
		rson[dad[p]] = q;
	else
		lson[dad[p]] = q;

	dad[p] = NIL;
}



/*******************************************
*
*  void Progress(void)
*
* Report progress of the en/decoding procedure
*
*******************************************/

void Progress(void)
{
	const char progress[4] = {'|','/','-','\\'};

	//printf("%c\b",progress[a++]);
	a &= 3;

	return;
}


/*******************************************
*
*  void Encode(void)
*
* Encoding procedure.
*
*******************************************/

void Encode(void)
{
	int  i,c,len,r,s,last_match_length,code_buf_ptr;
	unsigned char	code_buf[17], mask;

	/* Determine inputfile length, there must me more elegant ways, though */
	unsigned long fl=0;
	while (getc(infile) != EOF) fl++;
	rewind(infile);

	//fwrite(&fl,4,1,outfile);
	unsigned char *ss=(unsigned char*) &fl;
	fwrite(&ss[3],1,1,outfile);
	fwrite(&ss[2],1,1,outfile);
	fwrite(&ss[1],1,1,outfile);
	fwrite(&ss[0],1,1,outfile);

	InitTree();		/* Initialize trees */
	code_buf[0] = 0xff;	/* Initialise header flags for the first time */

	code_buf_ptr = 1;
	mask = 0x80;	/* Headerbit to be masked out in case of a literal string */

	s = 0;
	r = N - F;
	for (i = s; (i < r) && (c = getc(infile)) != EOF; i++) text_buf[i] = c^0xff; /* Clear the buffer */

	/* Read F bytes into the last F bytes of the buffer */
	rewind(infile);
	for (len = 0; (len < F) && (c = getc(infile)) != EOF; len++) text_buf[r + len] = c;

	if ((textsize = len) == 0)
		return;

	for (i = 1; i <= F; i++)
		InsertNode(r - i);	/* Insert the F strings into the tree */
		InsertNode(r);		/* Finally, insert the whole string. */

	/* Encoding loop */
	do {
		if (match_length > len)
			match_length = len;

		/* match is too short or exceeds file entry */
		if ((match_length <= THRESHOLD) ||((ftell(outfile)&0xfff) <= match_position)) {
			match_length = 1;
			code_buf[0] -= mask;  			/* Mask out headerbit */
			code_buf[code_buf_ptr++] = text_buf[r]; /* Send literally */

		/* otherwise we found a match, so encode it */
		} else {

			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
				| (match_length - THRESHOLD));  /* Send index/length pair */
   			code_buf[code_buf_ptr++] = (unsigned char) match_position;
		}

		if ((mask >>= 1) == 0) {  /* Shift mask right one bit. */

			for (i = 0; i < code_buf_ptr; i++)	/* Send at most 8 units */
				putc(code_buf[i], outfile);

			codesize += code_buf_ptr;		/* Reset encoding variables */
			code_buf[0] = 0xff;
			code_buf_ptr = 1;
			mask = 0x80;
		}

		last_match_length = match_length;

		for (i = 0; i < last_match_length && ((c = getc(infile)) != EOF); i++) {

			DeleteNode(s);			/* Delete old strings and */
			text_buf[s] = c;		/* read on */

			if (s < F - 1) text_buf[s + N] = c;	/* Speedup for strings near the
								end of the input */

			s = (s+1)&(N-1);	/* Keep the ringbuffer's range */
			r = (r+1)&(N-1);

			InsertNode(r);		/* Register the string in text_buf[r..r+F-1] */
		}

		textsize += i;
		Progress();

		while (i++ < last_match_length) {	/* Handle segments after the end of */
			DeleteNode(s);			/* the input */
			s = (s+1)&(N-1);
			r = (r+1)&(N-1);
			if (--len) InsertNode(r);
		}

	} while (len > 0);

	if (code_buf_ptr > 1) {		/* Send remaining code. */
   		for (i = 0; i < code_buf_ptr; i++) putc(code_buf[i], outfile);
		codesize += code_buf_ptr;
    }

	if (mask == 0x80) {		/* Write the EOF flag *bugfix 2004*/
		c = 0x80; putc(c,outfile);
	}
	c = 0x00; putc(c,outfile);

	printf("AChIeVeD rAtIo  : %.2f%%\n", (float)codesize*100 / textsize);
}



/*******************************************
*
*  void Decode(void)
*
* Encode's counterpart
*
*******************************************/

void Decode(void)
{
	int  i, j, r, c, loop1;
	unsigned int tag;

	fseek(infile,4,0); /* Skip original size */

 	r = N - F;

	for (;;) {
	tag = getc(infile);		/* Load command header */

	for (loop1=0;loop1<8;loop1++) {

	/* Decode packed string segment */
	if (tag & 0x80) {

		 /* End of input stream? */
		if ((j = getc(infile)) == 0) {
			printf("ok.\n");
			return;
		}

		i = getc(infile);

		i |= (j<<4)&0xf00;	/* Compute position & length of string */
		j &= 0x0f;

		i = (r-i)&(N-1);

		while(j!=-2) {
		   	c = text_buf[(i++)&(N-1)]; /* Copy from dictonary */
	       		putc(c, outfile);
		    	text_buf[r++]=c;r&=(N-1);
			j--;
			Progress();
		}

	/* Restore literal units */
	} else {
		c=getc(infile);
		putc(c, outfile);
		text_buf[r++]=c;r&=(N-1);
		Progress();
	}

	tag <<= 1;	/* Next entry */
	}
    }
}


int LZ77_Encode(char *src,char *dst)
{
	char *s;

	//printf("LZ77 packer  v1.3\n\n");

	if ((s = src, (infile  = fopen(s, "rb")) == NULL)||
	    (s = dst, (outfile = fopen(s, "wb")) == NULL)) {
		return 0;
	}

	//printf("PaCkIng...");
	Encode();

	fclose(infile);fclose(outfile);
	return 1;
}
