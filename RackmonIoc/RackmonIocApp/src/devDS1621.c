/*************************************************************************\
 Device driver for reading DS1621 I2C thermometer on a linux machine
 with generic i2c-dev device support (/dev/i2c-0).

 Author: Glenn Horton-Smith, Kansas State University, 2013

 The EPICS interface is the copied from the devAiSoft device driver
 found in EPICS BASE. The devAiSoft.c file contains the following
 attribution:

  "Original Authors: Bob Dalesio and Marty Kraimer / Date: 3/6/91"

 This code is meant to work with EPICS BASE, which is distributed
 subject to a Software License Agreement found in file LICENSE that is
 included with the EPICS distribution.  EPICS BASE is (c) 2008 by
 UChicago Argonne LLC, as Operator of Argonne National Laboratory, and
 (c) 2002 The Regents of the University of California, as Operator of
 Los Alamos National Laboratory.

 \*************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>  // for open
#include <sys/types.h> // for open
#include <sys/stat.h>  // for open
#include <fcntl.h>     // for open, O_RDWR
#include <sys/ioctl.h>      // for ioctl
#include <linux/i2c-dev.h>  // just for I2C_SLAVE

#ifndef I2C_SLAVE
// location of I2C_SLAVE was different in older kernels
#include <linux/i2c.h>
#endif

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "devSup.h"
#include "aiRecord.h"
#include "aoRecord.h"
#include "epicsExport.h"

#include "dev_i2c.h"


/* Create the dset for devAiDS1621 */
static long init_record_ai(aiRecord *prec);
static long read_ai(aiRecord *prec);

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_ai;
    DEVSUPFUN special_linconv;
} devAiDS1621 = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAiDS1621);


/************************************************************************/
/* Ai Record								*/
/*  INP = address of DS1621 in range 0 to 7 (See DS1621 datasheet)      */
/*        append "H" or "L" to read high or low thermostat threshold    */
/*        append "C" to read config byte                                */
/*        (default is to read current temperature)                      */
/************************************************************************/




/** structure for private info for each DS1621 on bus.
    Just store address for now. Only one bus supported for now. */
#define NCH_DS1621 (MAX_DS1621_ADDRESS+1)
static struct my_dpvt_s {
  int address;  // i2c address
  char what;    // what to read:  ' ' = temperature, H = high limit, L = low limit, C = config byte
} private_data[NCH_DS1621*4];


/** global variable for the I2C bus file descriptor */
static int global_fd_i2c = -1;



/* EPICS interface routines */

/* helpr for initialization task common to all records for this device */
static long init_record_common(dbCommon *prec, const char *inp)
{
    int inp_value = -1;
    char inp_char = ' ';
    int nconv = 0;

    /* check that fd is open to i2c device, open if necessary */
    if (global_fd_i2c < 0) {
      global_fd_i2c = open_i2c_bus(0);
      if (global_fd_i2c < 0) {
	recGblRecordError(S_dev_badBus, (void *)prec,
			  "devDS1621 (init_record) Could not open I2C bus");
	return S_dev_badBus;
      }
    }

    /* INP field is text that we parse */

    nconv = sscanf(inp, "%d %c", &inp_value, &inp_char);
    if (nconv > 0) {
      if ( inp_value < 0 || inp_value > MAX_DS1621_ADDRESS ) {
	recGblRecordError(S_db_badField, (void *)prec,
			  "devDS1621 (init_record) Invalid INP value, out of range");
	return S_db_badField;
      }
      else {
	int i = inp_value;
	if (inp_char == 'H')
	  i += NCH_DS1621;
	else if (inp_char == 'L')
	  i += 2*NCH_DS1621;
	else if (inp_char == 'C')
	  i += 3*NCH_DS1621;
	private_data[i].address = inp_value;
	private_data[i].what = inp_char;
	prec->dpvt = &( private_data[i] );
	// -- delay setting prec->udf = FALSE until we have good data
      }
    }
    else {
      recGblRecordError(S_db_badField, (void *)prec,
			"devDS1621 (init_record) Invalid INP value, no number at start");
      return S_db_badField;
    }

    return 0;
}


/* initialization for ai record */
static long init_record_ai(aiRecord *prec)
{
  return init_record_common((struct dbCommon*)prec, prec->inp.text);
}

/* read for ai */
static long read_ai(aiRecord *prec)
{
  int ierr;
  int addr = ((struct my_dpvt_s *)(prec->dpvt))->address;
  char what = ((struct my_dpvt_s *)(prec->dpvt))->what;

  if (what == ' ')
    ierr = read_temp_ds1621( global_fd_i2c, addr,
			     &(prec->val) );
  else if (what == 'H')
    ierr = access_ds1621_details( global_fd_i2c, addr,
				    &(prec->val), NULL, NULL, 'r');
  else if (what == 'L')
    ierr = access_ds1621_details( global_fd_i2c, addr,
				    NULL, &(prec->val), NULL, 'r');
  else if (what == 'C') {
    unsigned char cfg;
    ierr = access_ds1621_details( global_fd_i2c, addr,
				    NULL, NULL, &cfg, 'r');
    prec->val = (double)cfg;
  }
  else {
    ierr = 1;
  }

  if ( ierr == 0 ) {
    prec->udf = FALSE;
    return 2;  /* 2: succesful read, do not convert */
  }
  else {
    prec->udf = TRUE;
    return -1;  /* error */
  }    
}


/* ================================================================ */

  
/* Create the dset for devAoDS1621 */
static long init_record_ao(aoRecord *prec);
static long write_ao(aoRecord *prec);

struct {
  long            number;
  DEVSUPFUN       report;
  DEVSUPFUN       init;
  DEVSUPFUN       init_record;
  DEVSUPFUN       get_ioint_info;
  DEVSUPFUN       write_ao;
  DEVSUPFUN       special_linconv;
} devAoDS1621 = {
  6,
  NULL,
  NULL,
  init_record_ao,
  NULL,
  write_ao,
  NULL};
epicsExportAddress(dset,devAoDS1621);


/* initialization for ai record */
static long init_record_ao(aoRecord *prec)
{
  long status;
  int addr;
  char what;
  /* call the common init function */
  status = init_record_common((struct dbCommon*)prec, prec->out.text);
  
  /* if that failed, return that error status */
  if (status != 0)
    return status;
  /* we are an output, but initialize current value from current setting
     -- we can do that since device has persistant memory
     -- also check for weirdness like trying to make a channel
        to write to the temperature readback
  */	
  addr = ((struct my_dpvt_s *)(prec->dpvt))->address;
  what = ((struct my_dpvt_s *)(prec->dpvt))->what;
  if (what == ' ') {
    recGblRecordError(S_db_badField, (void *)prec,
	 "devDS1621 (init_record_ao) Invalid INP value, trying to write to temperature");
    return S_db_badField;
  }
  else if (what == 'H')
    status = access_ds1621_details( global_fd_i2c, addr,
				    &(prec->val), NULL, NULL, 'r');
  else if (what == 'L')
    status = access_ds1621_details( global_fd_i2c, addr,
				    NULL, &(prec->val), NULL, 'r');
  else if (what == 'C') {
    unsigned char cfg;
    status = access_ds1621_details( global_fd_i2c, addr,
				    NULL, NULL, &cfg, 'r');
    prec->val = (double)cfg;
  }
  else {
    status =  1;
  }
  if ( status == 0 ) {
    /* read succeeded, clear undefined status */
    prec->udf = FALSE;
    prec->sevr = NO_ALARM;
    prec->stat = NO_ALARM;
  }
  else {
    /* read failed, set undefined status  */
    prec->udf = TRUE;
  }
  /* return 2 to indicate successful initialization, and
     EPICS should do no conversion on output values */
  return 2;
}


/* write for ao */
static long write_ao(aoRecord *prec)
{
  int ierr;
  int addr = ((struct my_dpvt_s *)(prec->dpvt))->address;
  char what = ((struct my_dpvt_s *)(prec->dpvt))->what;

  if (what == 'H')
    ierr = access_ds1621_details( global_fd_i2c, addr,
				    &(prec->val), NULL, NULL, 'w');
  else if (what == 'L')
    ierr = access_ds1621_details( global_fd_i2c, addr,
				    NULL, &(prec->val), NULL, 'w');
  else if (what == 'C') {
    unsigned char cfg;
    if (prec->val < 0 || prec->val > 255)
      ierr = 1;
    else {
      cfg = (unsigned char)(int)(0.5+prec->val);
      ierr = access_ds1621_details( global_fd_i2c, addr,
				    NULL, NULL, &cfg, 'w');
    }
  }
  else {
    ierr = 1;
  }

  if ( ierr == 0 ) {
    prec->udf = FALSE;
    return 0;  /* 0: succesful write */
  }
  else {
    prec->udf = TRUE;
    return -1;  /* error. unsure of output status, so set UDF */
  }    
}
