 / See the file LICENSE for redistribution information.
 /
 / Copyright (c) 1997-2009 Oracle.  All rights reserved.
 /
 / $Id$
 /
 / int uts_lock ( int *p, int i );
 /             Update the lock word pointed to by p with the
 /             value i, using compare-and-swap.
 /             Returns 0 if update was successful.
 /             Returns 1 if update failed.
 /
         entry   uts_lock
 uts_lock:
         using   .,r15
         st      r2,8(sp)        / Save R2
         l       r2,64+0(sp)     / R2 -> word to update
         slr     r0, r0          / R0 = current lock value must be 0
         l       r1,64+4(sp)     / R1 = new lock value
         cs      r0,r1,0(r2)     / Try the update ...
         be      x               /  ... Success.  Return 0
         la      r0,1            /  ... Failure.  Return 1
 x:                              /
         l       r2,8(sp)        / Restore R2
         b       2(,r14)         / Return to caller
         drop    r15
