/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>

unsigned long currSP;   /* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:   Upon entry, currpid gives current process id.
 *      Proctab[currpid].pstate gives correct NEXT state for
 *          current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched() {
    int schr = getschedclass();
    if (schr == EXPDISTSCHED) {
        exponential_scheduling();
    } else if (schr == LINUXSCHED) {
        linux_like_scheduling();
    } else {
        default_xinu_scheduling();
    }
    return (OK);
}

/*
 * Function:  exponential_scheduling 
 * --------------------------------
 * To perform exponential scheduling 
 * Find the first process whose priority is greater than random
 */
int exponential_scheduling() {
    register struct pentry *current;        /* pointer to old process entry */
    register struct pentry *next;           /* pointer to new process entry */

    int random = (int) expdev(0.1);
    int nextpid = q[rdytail].qprev;			/* start traversal	*/
    int previous = q[nextpid].qprev;
    /* 
     * Find the first process whose priority is greater than random, 
     * by using using pointer to previous traversal 
     */
    while (q[previous].qkey > random) {
        if (q[nextpid].qkey != q[previous].qkey)
            nextpid = previous;
        previous = q[previous].qprev;
    }

    current = &proctab[currpid];
    next = &proctab[nextpid];
    preempt = QUANTUM;						/* reset preemption counter 	*/

    if ((nextpid == NULLPROC || toContinueCurrent(current, next, random) == TRUE)) {
        return (OK);
    }

    if (current -> pstate == PRCURR) {
        current -> pstate = PRREADY;
        insert(currpid, rdyhead, current -> pprio);
    }

    dequeue(nextpid);
    currpid = nextpid;
    next -> pstate = PRCURR; 				/* mark it currently running	*/

    /* perform context switch */

    ctxsw((int) & current -> pesp, (int) current -> pirmask, (int) & next -> pesp, (int) next -> pirmask);
    return OK;
}

/*
 * Function:  toContinueCurrent 
 * ----------------------------
 * Checks True to say that the current process can continue if 
 * (1) is still in PRCURR state
 * (2) lies in between random and chosen next
 */
int toContinueCurrent(struct pentry * current, struct pentry * next, int random) {
    if (current -> pstate == PRCURR) {
        if ((next -> pprio > current -> pprio) && (current -> pprio > random))
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}

/*
 * Function:  default_xinu_scheduling 
 * ----------------------------------
 * Moved existing definition in a function
 * This is default method in xinu
 */
int default_xinu_scheduling() {
    register struct pentry * optr; /* pointer to old process entry */
    register struct pentry * nptr; /* pointer to new process entry */

    /* no switch needed if current process priority higher than next*/

    if (((optr = &proctab[currpid]) -> pstate == PRCURR) &&
        (lastkey(rdytail) < optr -> pprio)) {
        return (OK);
    }
    /* force context switch */

    if (optr -> pstate == PRCURR) {
        optr -> pstate = PRREADY;
        insert(currpid, rdyhead, optr -> pprio);
    }

    /* remove highest priority process at end of ready list */

    nptr = &proctab[(currpid = getlast(rdytail))];
    nptr -> pstate = PRCURR;                /* mark it currently running */ 
    #ifdef RTCLOCK
        preempt = QUANTUM;                  /* reset preemption counter	 */ 
    #endif

    ctxsw((int) &optr -> pesp, (int) optr -> pirmask, (int) &nptr -> pesp, (int) nptr -> pirmask);

    /* The OLD process returns here when resumed. */
    return OK;
}

/*
 * Function:  linux_like_scheduling 
 * --------------------------------
 * To perform linux like scheduling
 */
int linux_like_scheduling() {
    
    if (get_epochstate() == 0) {            /* if new epoch to be started */
        init_processes_for_epoch();
        set_epochstate(1);
        start_first_process();
    } else {                                /* ongoing epoch */
        struct pentry *current;
        struct pentry *next;

        current = &proctab[currpid];
        update_current_counters(current);
        int maxgoodnesspid = get_max_goodness_process();

        if (maxgoodnesspid == -1) {         /* end of curent epoch */
            set_epochstate(0);
            linux_like_scheduling();
        } else if (currpid == maxgoodnesspid && current -> pstate == PRCURR) {
            /*current process has highest goodness, no ctxsw needed*/
            preempt = current -> tq;
        } else {                            /* force context switch */

            if (current -> pstate == PRCURR) {
                current -> pstate = PRREADY;
                insert(currpid, rdyhead, current -> pprio);
            }

            next = &proctab[maxgoodnesspid];
            next -> pstate = PRCURR;
            preempt = next -> tq;
            currpid = maxgoodnesspid;
            dequeue(currpid);

            ctxsw((int) &current -> pesp, (int) current -> pirmask, (int) &next -> pesp, (int) next -> pirmask);        
        }
    }
    return (OK);
}

/*
 * Function:  update_current_counters 
 * ----------------------------------
 * To update timequantum and goodness values of the current process
 * based on the preemption, in the middle of the epoch
 */
void update_current_counters(struct pentry * current) {
    current -> tq = preempt;
    if (current -> tq <= 0 || currpid == NULLPROC) {    /* end of time quantum*/
        current -> tq = 0;
        current -> goodness = 0;
    } else {
        current -> goodness = current -> epoch_priority + preempt;
    }
}

/*
 * Function:  init_processes_for_epoch 
 * -----------------------------------
 * To set timequantum and goodness values of the current process
 * at the begining of the epoch 
 */
void init_processes_for_epoch() {
    int pid = 0;
    struct pentry *pptr;
    for (pid = 0; pid < NPROC; pid++) {
        if ((pptr = &proctab[pid]) -> pstate != PRFREE) {
            pptr -> isinepoch = TRUE;
            pptr -> epoch_priority = pptr -> pprio;     /* priority changes shouldn't take affect in epoch */
            if (pptr -> tq == 0)                        /* new process or time quantum elapsed in previous epoch */
                pptr -> tq = pptr -> pprio;
            else
                pptr -> tq = pptr -> tq / 2 + pptr -> pprio;
            pptr -> goodness = pptr -> pprio + pptr -> tq;
        }
    }
}

/*
 * Function:  start_first_process 
 * ------------------------------
 * Insert old process in ready queue
 * Get highest goodness process, pid
 * If pid is -1, start NULLPROC with default QUANTUM
 * Else dequeue process, update currpid, preempt & start pid process
 */
void start_first_process() {
    register struct pentry *optr;                       /* pointer to old process entry */
    register struct pentry *nptr;                       /* pointer to new process entry */

    optr = &proctab[currpid];
    if (optr -> pstate == PRCURR) {
        optr -> pstate = PRREADY;                       /* mark old as ready */
        insert(currpid, rdyhead, optr -> pprio);
    }

    int pid = get_max_goodness_process();

    /* set process with highest goodness as the new ptr
     * if maxgoodness is -1, it means no ready process availble
     * so start NULLPROC (set premempt as QUANTUM)
     * else start process with pid returned 
     */

    nptr = &proctab[(pid == -1) ? NULLPROC : pid];
    nptr -> pstate = PRCURR;                            /* mark it currently running */ 
    #ifdef RTCLOCK 
        preempt = (pid == -1) ? QUANTUM : nptr -> tq;   /* reset preemption counter */
    #endif

    currpid = pid;
    dequeue(currpid);
    
    /* perform context switch */
    ctxsw((int) &optr -> pesp, (int) optr -> pirmask, (int) &nptr -> pesp, (int) nptr -> pirmask);
}

/*
 * Function:  get_max_goodness_process 
 * --------------------------------
 * To get the process id of the process having the highest goodness value 
 * from the ready queue (select process whose isinepoch is TRUE)
 * Returns:
 * -1 if no process (or no process available in the epoch)
 * pid of the process having highest goodness
 */
int get_max_goodness_process() {
    register struct pentry *nptr; /* pointer to new process entry */
    int pid = 0;
    int maxgoodness = 0;
    int maxgoodnesspid = -1;
    if (q[rdytail].qprev == q[rdyhead].qnext)
        return -1;
    for (pid = q[rdyhead].qnext; pid != rdytail; pid = q[pid].qnext) {
        nptr = &proctab[pid];
        /* first maximum goodness selected, implicit RR as the same process will have lower goodness next time */
        if ((proctab[pid].isinepoch) && (proctab[pid].goodness > maxgoodness))
            maxgoodnesspid = pid;
    }
    return maxgoodnesspid;
}