#include "stddef.h"
#include "stdio.h"
#include "xpc.h"

main()
    {
    BUFLET b;
    INT c, i, x;
    TEXT buf[10];
    
    i = 0;
    do  {
	putchar('>');
	if (x = strlen(gets(buf)))
	    {
	    sscanf(buf, "%x", &c);
	    b.bufdata[i++] = c;
	    }
	} while (x);
    for (x = 0; x < i; ++x)
	printf("%02x  ", b.bufdata[x]);
    printf("=  %04x\n", get_crc(&b, 0, i));
    }