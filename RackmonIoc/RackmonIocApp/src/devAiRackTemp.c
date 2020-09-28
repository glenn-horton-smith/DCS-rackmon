/*************************************************************************\
 Device driver for reading DS75 I2C thermometers on a Mu2e rack
 monitor device. Code runs on a BeagleBone black or equivalent linux
 computer with generic i2c-dev device support (/dev/i2c-2), and uses
 gpio pins to control a digital switch in the rack monitor which 
 selects one of six connected i2c bus stubs.

 Author: Glenn Horton-Smith, Kansas State University, 2013, 2020

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
#include "dev_gpio.h"

/* Create the dset for devAiRackTemp */
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
} devAiRackTemp = {
    6,
    NULL,
    NULL,
    init_record_ai,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAiRackTemp);


/************************************************************************
 * Ai Record                                
 *   INP = a number in the range 0 to 63 equal to istub + 8*iprobe
 * where istub sets the i2c bus stub in range 0 to 7,
 * and iprobe selects the DS75 address in range 0 to 7.
 *  (See DS75 datasheet)      
 *        append "S" to read thermostat overtemp setpoint
 *        append "Y" to read thermostat hysteresis
 *        append "C" to read config byte                            
 *        (default is to read current temperature)                  
 ************************************************************************/



/** structure for private info for each DS75 on bus.
    Just store address for now. */
#define MAX_ISTUB 7
#define NCH_RACKTEMP (MAX_ISTUB+1)*(MAX_DS75_ADDRESS+1)
static struct my_dpvt_s {
  int address;  // i2c address
  char what;    // what to read:  ' ' = temperature, H = high limit, L = low limit, C = config byte
} private_data[NCH_RACKTEMP*4];


/** global variable for the I2C bus file descriptor */
static int global_fd_i2c = -1;


/** internal function for selecting i2c bus stub for probe */
int select_probe(int istub)
{
  if (istub < 0 || istub > 7)
    return -1;
  if (gpio_write(26 /*"P8_14"*/, istub & 1) < 0)
    return -1;
  if (gpio_write(46 /*"P8_16"*/, istub & 2) < 0)
    return -1;
  if (gpio_write(65 /*"P8_18"*/, istub & 4) < 0)
    return -1;
  return 0;
}

/* EPICS interface routines */

/* helper for initialization task common to all records for this device */
static long init_record_common(dbCommon *prec, const char *inp)
{
    int inp_value = -1;
    char inp_char = ' ';
    int nconv = 0;

    /* check that fd is open to i2c device, open if necessary */
    if (global_fd_i2c < 0) {
      global_fd_i2c = open_i2c_bus(2);
      if (global_fd_i2c < 0) {
    recGblRecordError(S_dev_badBus, (void *)prec,
              "devRackTemp (init_record) Could not open I2C bus");
    return S_dev_badBus;
      }
    }

    /* INP field is text that we parse */

    nconv = sscanf(inp, "%d %c", &inp_value, &inp_char);
    if (nconv > 0) {
      if ( inp_value < 0 || inp_value >= NCH_RACKTEMP ) {
    recGblRecordError(S_db_badField, (void *)prec,
              "devRackTemp (init_record) Invalid INP value, out of range");
    return S_db_badField;
      }
      else {
    int i = inp_value;
    if (inp_char == 'H')
      i += NCH_RACKTEMP;
    else if (inp_char == 'L')
      i += 2*NCH_RACKTEMP;
    else if (inp_char == 'C')
      i += 3*NCH_RACKTEMP;
    private_data[i].address = inp_value;
    private_data[i].what = inp_char;
    prec->dpvt = &( private_data[i] );
    // -- delay setting prec->udf = FALSE until we have good data
      }
    }
    else {
      recGblRecordError(S_db_badField, (void *)prec,
            "devRackTemp (init_record) Invalid INP value, no number at start");
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
  int address = ((struct my_dpvt_s *)(prec->dpvt))->address;
  int addr = address/(MAX_ISTUB+1);
  int istub = address%(MAX_ISTUB+1);
  char what = ((struct my_dpvt_s *)(prec->dpvt))->what;

  if (select_probe(istub) < 0)
    ierr = -1;
  else if (what == ' ')
    ierr = read_temp_ds75( global_fd_i2c, addr,
                 &(prec->val) );
  else if (what == 'S')
    ierr = access_ds75_details( global_fd_i2c, addr,
                    &(prec->val), NULL, NULL, 'r');
  else if (what == 'Y')
    ierr = access_ds75_details( global_fd_i2c, addr,
                    NULL, &(prec->val), NULL, 'r');
  else if (what == 'C') {
    unsigned char cfg;
    ierr = access_ds75_details( global_fd_i2c, addr,
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

  
/* Create the dset for devAoRackTemp */
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
} devAoRackTemp = {
  6,
  NULL,
  NULL,
  init_record_ao,
  NULL,
  write_ao,
  NULL};
epicsExportAddress(dset,devAoRackTemp);


/* initialization for ao record */
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
     "devRackTemp (init_record_ao) Invalid INP value, trying to write to temperature");
    return S_db_badField;
  }
  else if (what == 'S')
    status = access_ds75_details( global_fd_i2c, addr,
                    &(prec->val), NULL, NULL, 'r');
  else if (what == 'Y')
    status = access_ds75_details( global_fd_i2c, addr,
                    NULL, &(prec->val), NULL, 'r');
  else if (what == 'C') {
    unsigned char cfg;
    status = access_ds75_details( global_fd_i2c, addr,
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
  int address = ((struct my_dpvt_s *)(prec->dpvt))->address;
  int addr = address/(MAX_ISTUB+1);
  int istub = address%(MAX_ISTUB+1);
  char what = ((struct my_dpvt_s *)(prec->dpvt))->what;

  if (select_probe(istub) < 0)
    ierr = -1;
  else if (what == 'H')
    ierr = access_ds75_details( global_fd_i2c, addr,
                    &(prec->val), NULL, NULL, 'w');
  else if (what == 'L')
    ierr = access_ds75_details( global_fd_i2c, addr,
                    NULL, &(prec->val), NULL, 'w');
  else if (what == 'C') {
    unsigned char cfg;
    if (prec->val < 0 || prec->val > 255)
      ierr = 1;
    else {
      cfg = (unsigned char)(int)(0.5+prec->val);
      ierr = access_ds75_details( global_fd_i2c, addr,
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
