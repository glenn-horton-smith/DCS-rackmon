/*************************************************************************\
 Device driver for reading from GPIO pin
\*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>    // for UINT_MAX

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "devSup.h"
#include "biRecord.h"
#include "epicsExport.h"


/* Create the dset for devBiRackProt */
static long init_record(biRecord *prec);
static long read_bi(biRecord *prec);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_bi;
} devBiRackProt = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_bi
};
epicsExportAddress(dset, devBiRackProt);


/*****************************************************************
 * bi record for rack protection status bit in "rackmon box".
 *****************************************************************/


/** global variable for the heartbeat */
unsigned int hb_sequence_counter = 0;


/* EPICS interface routines */

static long init_record(biRecord *prec)
{
  /* INP field ignored, nothing else to do here 
   (for initial demo, GPIO bit to read is hard-coded) */

  return 0;
}


static long read_bi(biRecord *prec)
{
  int istat=-1;
  unsigned int state = 0;
  /* hard-coded to gpio1[28] for now (pin 12 on P9 of BeagleBone Black) */
  FILE *f = fopen("/sys/class/gpio/gpio60/value","r");
  if (f) {
    istat = fscanf(f,"%d",&state);
    fclose(f);
  }
  if (istat == 1) {
    // set the value of the variable
    if (state == 0 && prec->val != 0)
      errlogPrintf("RackProt bit is zero for %s\n", prec->name);
    prec->val = state;
    prec->udf = FALSE;
    return 2;  /* 2: succesful read, do not convert */    
  }
  else {
    // indicate undefined value
    prec->udf = TRUE;
    return -1; /* error */
  }
}
