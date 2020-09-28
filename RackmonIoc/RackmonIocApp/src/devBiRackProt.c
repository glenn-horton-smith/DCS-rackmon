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
#include "errlog.h"

#include "dev_gpio.h"

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
  /* hard-coded gpio 44 for now (pin 12 on P8 of BeagleBone Black) */
  istat = gpio_read(44);
  if (istat >= 0) {
    // set the value of the variable
    if (istat == 0 && prec->val != 0)
      errlogPrintf("RackProt bit is zero for %s\n", prec->name);
    prec->val = istat;
    prec->udf = FALSE;
    /* flash heartbeat normally. heartbeat gpio hard-coded for now */
    hb_sequence_counter++;
    gpio_write(45, hb_sequence_counter%2);
    return 2;  /* 2: succesful read, do not convert */    
  }
  else {
    // indicate undefined value
    prec->udf = TRUE;
    /* flash heartbeat irregularly. heartbeat gpio hard-coded for now */
    hb_sequence_counter++;
    gpio_write(45, (308404&(1<<(20-hb_sequence_counter%21)))!=0 );
    return -1; /* error */
  }
}
