#include "stddef.h"
#include "ai.h"
#include "dos.h"

IMPORT INT int86x();			/* generates software interrupt */

IMPORT INT vector;			/* interrupt vector */

/* callxpc - call xpc driver
 */
VOID callxpc(pr)
    REQ *pr;				/* pointer to request block */
    {
    struct SREGS sregs;			/* see dos.h */
    union REGS regs;			/* see dos.h */

    /* initialize "registers" and call X.PX Driver through vector
     */
    segread(&sregs);
    pr->ds = sregs.es = sregs.ds;
    regs.x.bx = (UWORD)pr;
    (VOID)int86x(vector, &regs, &regs, &sregs);
    }
