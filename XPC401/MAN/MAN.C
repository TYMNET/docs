/*	MAN.C - Create manual pages from 'C' sources
 */
#include "stddef.h"
#include <dos.h>
#include <ctype.h>
#include <stdio.h>

#define DIRNAME	20		/* Size of Module/Function Name */
#define SPACE	' '		/* The SPACE character */
#define EOS	'\0'		/* The End Of String character */

typedef struct
    {
    TEXT fname[DIRNAME + 2];	/* Module/Function Name */
    WORD pgno;			/* Page # */
    WORD pent;			/* Pointer to Source Name */
    } DIR;
LOCAL DIR  dir[400];			/* Maximum # of Modules/Functions */
LOCAL WORD nodir = 0;			/* Number of Modules/Functions */
LOCAL TEXT nam[150][14];		/* Maximum # of files */
LOCAL TEXT *default_name = "*.c";	/* This is mainly for *.c */
LOCAL WORD how_many;			/* How many files */
LOCAL TEXT page[500][80];		/* Large Page Buffer */
LOCAL WORD para[40];			/* Paragraph offsets */
LOCAL WORD pageno = 0;			/* Page Number */
LOCAL TEXT *file_name;			/* Current file name */
LOCAL UBYTE dta[80];			/* Disk Transfer Address */
LOCAL TEXT ff[2] = {0x0C, 0x0};		/* Form Feed */
LOCAL TEXT *header = "\n\t\t\t\t\t\t\t\tPage: %d\nSource Module Name: %s\n\n";
LOCAL TEXT *dirhdr =
 "\n\n\n\n\n\t\tDirectory Listings\n\n    Name               Module            Page\n\n";
LOCAL TEXT *stars = "**********************";

WORD main(ac, av)
    WORD ac;
    TEXT *av[];
    {
    union REGS sv, rv;
    TEXT line[60], tmp[10];
    BOOL more;
    FAST i, j, ln;

    bdos(0x1A, dta, 0);
    file_name = ac == 2 ? av[1] : default_name;
    sv.x.dx = (unsigned int)file_name;
    sv.x.cx = 0x10;
    how_many = 0;
    more = NO;
    FOREVER
	{
	sv.x.ax = more ? 0x4F00 : 0x4E00;
	int86(0x21, &sv, &rv);
	if (rv.x.ax & 0xFF)
	    break;
	more = YES;
	if (dta[21] == 0x10)
	    continue;
	strcpy(nam[how_many], dta + 30);
	++how_many;
	}
    if (how_many)
	{
	if (how_many > 1)
	    filesort();
	do_manpage();
	}
    else
	{
	printf("\nMAN: Cannot open or file does not exist %s\n", file_name);
	exit(1);
	}
    dirsort();
    printf(dirhdr);
    for (ln = 0, i = 0; i < nodir; ++i)
	{
	for (j = 0; j < 60; ++j)
	    line[j] = SPACE;
	strncpy(line, dir[i].fname, strlen(dir[i].fname));
	strncpy(line + 22, nam[dir[i].pent], strlen(nam[dir[i].pent]));
	itoa(dir[i].pgno, tmp, 10);
	strcpy(line + 42, tmp);
	printf("%s\n", line);
	if (ln++ > 45 && i < (nodir - 1))
	    {
	    ln = 0;
	    printf(ff);
	    printf(dirhdr);
	    }
	}
    printf(ff);
    exit(0);
    }

/*	Extract the manual pages for each module
 */
#define BUFLENGTH 128

LOCAL VOID do_manpage()
    {
    FILE  *fopen(), *infile;
    TEXT  *fgets();
    TEXT  line[BUFLENGTH + 2];
    TEXT  c, *s;
    WORD  k, m, nopara, parasize;
    WORD  module, noline;
    BOOL  ontitle;
    FAST  i, j;
    
    module = 0;
    while (how_many--)			/* For all the Source Modules */
	{
	if ((infile = fopen(nam[module],"r")) == NULL)
	    {
	    printf("\nMAN: Error - cannot open %s\n",nam[module]);
	    ++module;
	    continue;
	    }
	ontitle = NO;
	nopara = noline = 0;
	while (fgets(line, BUFLENGTH, infile))	/* Get all the lines */
	    {
	    if ((i = strlen(line)) < 80)
		while (i < 80)		/* Make sure we have   */
		    line[i++] = EOS;	/* an EOS filled line  */
	    line[78] = '\n';		/* and no more than 80 */
	    line[79] = EOS;			/*  chars. per line    */
	    if (ontitle)
		{

		/* Get all the lines of a paragraph
		* until finding an all STARS line
		*/
		if (strncmp(line + 2, stars, 20))
		    {
		    line[0] = line[1] = SPACE;
		    strcpy(page[noline++], line);
		    }
		else
		    ontitle = NO;
		}
	    else

		/* Search file for begining of paragraph 
		* indicated by all STARS line
		*/
		if (strncmp(line + 2, stars, 20) == 0)
		    {
		    ontitle = YES;		/* Begin of New Paragraph */
		    para[nopara++] = noline;
		    }
	    }
	fclose(infile);
	printf(header, ++pageno, nam[module]);
	para[nopara++] = noline;		/* End of Last Paragraph */
	noline = 2;
	ontitle = YES;
	for (j = 0; j < nopara - 1; ++j)	/* For All the Paragraphs */
	    {
	    parasize = para[j + 1] - para[j];	/* get paragraph size */

	    /* skip page if paragraph greater then
	     * remaininig lines in page and more then 30 lines already
	     * have been writen
	     */
	    if (noline > 30 && parasize > (60 - noline))
		{
		printf(ff);
		printf(header, ++pageno, nam[module]);
		noline = 2;
		}
	    
	    /* Extract The Module / Function Name
	     */
	    s = page[para[j]];			/* First Line of Para. */
	    m = strlen(s) - 2;
	    for (i = 0; i < DIRNAME; ++i)
		dir[nodir].fname[i] = SPACE;
	    if (ontitle)
		{
		ontitle = NO;
		
		/* For the Module name - extract the first word
		 * sorrounding by SPACE's
		 */
		for (i = 0; i <= m; ++i)
		    s[i] = toupper(s[i]);	/* Upper Case Module Name */
		for (i = 0; i < m; ++i)
		    if (!isspace(s[i]))		/* Skip the First    */
			break;			/* SPACE characters  */
		}
	    else
		{
		
		/* For Function name - search for word which
		 * has a '('
		 */
		for (i = 0; i < m; ++i)
		    if (s[i] == '(')		/* Search for '('   */
			break;
		m = i; --i;
		while (isalnum(s[i]) || s[i] == '_')
		    --i;			/* Backspace to begining */
		++i;				/* of the word		 */
		}
	    if (i && i < m)
		{				/* Extract word assuming */
		for (k = 0; i < m; ++i)		/* all alphnumeric and _ */
		    {
		    c = s[i];
		    if ((isalnum(c) || c == '_') && k <= DIRNAME)
			dir[nodir].fname[k++] = c;
		    else
			break;
		    }

		/* Set the Directory Entry For this Module / Function
		 */
		dir[nodir].fname[DIRNAME] = EOS;
		dir[nodir].pgno = pageno;
		dir[nodir].pent = module;
		++nodir;
		}

	    /* Print The Paragraph
	     */
	    for (i = para[j]; i < para[j + 1]; ++i)
		{
		printf("%s", page[i]);
		if (++noline > 60)	/* paragraph greater then */
		    {			/* remaininig lines in page */
		    printf(ff);
		    printf(header, ++pageno, nam[module]);
		    noline = 2;
		    }
		}
	    printf("\n\n");
	    noline += 2;
	    }
	printf(ff);
	++module;
	}
    }

/*	Shell Sort Modules/Functions Names
 */
LOCAL dirsort()
    {
    FAST j, i, gap;
    DIR temp;

    for (gap = nodir / 2; gap > 0; gap /= 2)
	for (i = gap; i < nodir; i++)
	    for (j = i - gap; j >= 0; j -= gap)
		{
		if (strcmp(dir[j].fname, dir[j+gap].fname) <= 0)
		    break;

		/* swap members (the hard way)
		 */
		temp.pgno = dir[j].pgno;
		temp.pent = dir[j].pent;
		strcpy(temp.fname, dir[j].fname);

		dir[j].pgno = dir[j + gap].pgno;
		dir[j].pent = dir[j + gap].pent;
		strcpy(dir[j].fname, dir[j + gap].fname);

		dir[j + gap].pgno = temp.pgno;
		dir[j + gap].pent = temp.pent;
		strcpy(dir[j + gap].fname, temp.fname);
		}
    }

/*	Shell Sort File Names
 */
LOCAL filesort()
    {
    FAST j, i, gap;
    TEXT temp[14];

    for (gap = how_many / 2; gap > 0; gap /= 2)
	for (i = gap; i < how_many; i++)
	    for (j = i - gap; j >= 0; j -= gap)
		{
		if (strcmp(nam[j], nam[j+gap]) <= 0)
		    break;

		/* swap members
		 */
		strcpy(temp, nam[j]);
		strcpy(nam[j], nam[j + gap]);
		strcpy(nam[j + gap], temp);
		}
    }
