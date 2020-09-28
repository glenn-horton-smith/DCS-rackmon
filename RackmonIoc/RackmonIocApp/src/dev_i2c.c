/*************************************************************************\
 Support for reading DS1621 I2C thermometer, DS75 I2C, and
 MicroBooNE custom fan tachometer on I2C bus at address 0x0f, 
 on any linux machine with generic i2c-dev device support (/dev/i2c-n).

 Author: Glenn Horton-Smith, Kansas State University, 2013, 2020
\*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>  // for open
#include <sys/types.h> // for open
#include <sys/stat.h>  // for open
#include <fcntl.h>     // for open, O_RDWR
#include <sys/ioctl.h>      // for ioctl
#include <time.h>      // for nanosleep
#include <linux/i2c-dev.h>  // just for I2C_SLAVE
#include <pthread.h>   // for mutex locks

#ifndef I2C_SLAVE
// location of I2C_SLAVE was different in older kernels
#include <linux/i2c.h>
#endif

#include "dev_i2c.h"



/* Constants for the I2C devices */
const int
  I2C_DS1621_ADDR_BASE = 0x48,  /**< add 0-7 to get thermometers 0 through 7 */
  I2C_DS75_ADDR_BASE = 0x48,  /**< add 0-7 to get thermometers 0 through 7 */
  I2C_TACHOMETER_ADDR = 0x0F;   /**< the one tachometer address */


/* mutex for locking critical sections of i2c i/o code */
pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;


/** Open I2C bus file descriptor for read/write access.
    Returns file descriptor on success, -1 on error, same as open() */
int
open_i2c_bus(int bus /**< bus number, 0 for first (or only) bus */ )
{
  char filename[16];
  // open i2c bus in /dev filesystem
  snprintf(filename, 16, "/dev/i2c-%d", bus);
  return open(filename, O_RDWR);
}


/** read temperature from specified ds1621.
    Returns 0 on success, negative value on error. */
int
read_temp_ds1621( int fd,          /**< file descriptor for i2c bus device */
          int addr,        /**< 3-bit address of the ds1621 */
          double *T_degC   /**< destination for temperature, degC */
          )
{
  int status; // used for return value from i/o functions and this function
  char buf[4]; // I/O buffer

  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !! begin protected section.  No "returns" allowed in code below,
  // !! or else mutex might not get unlocked.  Beware!!!!!!!!!!!!!!!!
#define RETURN(s) { status=s; goto RETURN; }
  pthread_mutex_lock(&i2c_mutex);

  // tell bus who we want to talk to
  status = ioctl(fd, I2C_SLAVE, I2C_DS1621_ADDR_BASE+(addr&7) );
  if (status < 0)
    RETURN(status);

  // Do any write to register address 0xEE to get it started converting.
  // (This only needs to be done once at powerup if thermometer is in
  // continuous sample (not 1shot) mode, but do it each time so users
  // can plug and unplug thermometers during testing.)
  buf[0]= 0xee;
  buf[1]= 1;
  status = write(fd, buf, 2);
  if (status != 2)
    RETURN(-1);

  // now read temperature data from register 0xAA
  buf[0] = 0xaa;
  status = write(fd, buf, 1);
  if (status != 1)
    RETURN(-1);
  status = read(fd, buf, 2);
  if (status != 2)
    RETURN(-1);
  
  // store temperature and return
  *T_degC =  buf[0] + buf[1]/256.0;
  status = 0;

  // !! End of protected section
#undef RETURN
 RETURN:
  pthread_mutex_unlock(&i2c_mutex);
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  return status;
}


/** read or write thresholds TH and TL and config byte from specified ds1621.
    Any pointer argument may be null, in which case the corresponding
    quantity is not read/written to the ds1612.
    Returns 0 on success, negative value on error. */
int
access_ds1621_details( int fd,         /**< file descriptor for i2c bus device */
             int addr,       /**< 3-bit address of the ds1261 */
             double *TH,     /**< src/dest for TH, degC */
             double *TL,     /**< src/dest for TH, degC */
             unsigned char *cfg,  /**< src/dest for config byte */
             char mode       /**< 'w' for write, otherwise read */
             )
{
  int status; // used for return value from i/o functions
  int t256;    // temperature in (1/256) degC units
  char buf[4]; // I/O buffer
  struct timespec ts = {0,11000000};  // 10 ms sleep required after write
 
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !! begin protected section.  No "returns" allowed in code below,
  // !! or else mutex might not get unlocked.  Beware!!!!!!!!!!!!!!!!
#define RETURN(s) { status=s; goto RETURN; }
  pthread_mutex_lock(&i2c_mutex);

  // tell bus who we want to talk to
  status = ioctl(fd, I2C_SLAVE, I2C_DS1621_ADDR_BASE+addr);
  if (status < 0)
    RETURN(status);

  if (mode == 'w') {
    // write TH to 0xA1
    if (TH) {
      t256 = (int)( (*TH)*256 + 0.5 );
      buf[0] = 0xa1;
      buf[1] = t256>>8;
      buf[2] = t256&(0xff);
      status = write(fd, buf, 3);
      if (status != 3)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
    // write TL to 0xA2
    if (TL) {
      t256 = (int)( (*TL)*256 + 0.5 );
      buf[0] = 0xa2;
      buf[1] = t256>>8;
      buf[2] = t256&(0xff);
      status = write(fd, buf, 3);
      if (status != 3)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
    // write cfg to 0xAC
    if (cfg) {
      buf[0] = 0xaC;
      buf[1] = *cfg;
      status = write(fd, buf, 2);
      if (status != 2)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
  }
  else {
    // read TH from 0xA1
    if (TH) {
      buf[0] = 0xa1;
      status = write(fd, buf, 1);
      status += read(fd, buf, 2);
      if (status != 3)
        RETURN(-1);
      *TH =  buf[0] + buf[1]/256.0;
    }
    // read TL from 0xA2
    if (TL) {
      buf[0] = 0xa2;
      status = write(fd, buf, 1);
      status += read(fd, buf, 2);
      if (status != 3)
        RETURN(-1);
      *TL =  buf[0] + buf[1]/256.0;
    }
    // read cfg from 0xAC
    if (cfg) {
      buf[0] = 0xaC;
      status = write(fd, buf, 1);
      status += read(fd, buf, 1);
      if (status != 2)
        RETURN(-1);
      *cfg = buf[0];
    }
  }

  status = 0;

  // !! End of protected section
#undef RETURN
 RETURN:
  pthread_mutex_unlock(&i2c_mutex);
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!

  return status;
}


/*--- code for DS75 temperature probe ---*/

/** read temperature from specified DS75.
    Returns 0 on success, negative value on error. */
int
read_temp_ds75( int fd,          /**< file descriptor for i2c bus device */
          int addr,        /**< 3-bit address of the ds75 */
          double *T_degC   /**< destination for temperature, degC */
          )
{
  int status; // used for return value from i/o functions and this function
  char buf[4]; // I/O buffer

  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !! begin protected section.  No "returns" allowed in code below,
  // !! or else mutex might not get unlocked.  Beware!!!!!!!!!!!!!!!!
#define RETURN(s) { status=s; goto RETURN; }
  pthread_mutex_lock(&i2c_mutex);

  // tell bus who we want to talk to
  status = ioctl(fd, I2C_SLAVE, I2C_DS75_ADDR_BASE+(addr&7) );
  if (status < 0)
    RETURN(status);

  // read temperature data from register 0x00
  buf[0] = 0x00;
  status = write(fd, buf, 1);
  if (status != 1)
    RETURN(-1);
  status = read(fd, buf, 2);
  if (status != 2)
    RETURN(-1);
  
  // store temperature and return
  *T_degC =  buf[0] + buf[1]/256.0;
  status = 0;

  // !! End of protected section
#undef RETURN
 RETURN:
  pthread_mutex_unlock(&i2c_mutex);
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  return status;
}


/** read or write thresholds T_OS and T_HYST and config byte from 
    specified DS75.  (T_OS is the overtemp trip threshold, and T_HYST 
    is the overtemp reset threshold. See DS75 datasheet.)
    Any pointer argument may be null, in which case the corresponding
    quantity is not read from or written to the ds75.
    Returns 0 on success, negative value on error. */
int
access_ds75_details( int fd,         /**< file descriptor for i2c bus device */
             int addr,         /**< 3-bit address of the ds1261 */
             double *T_OS,     /**< src/dest for T_OS, degC */
             double *T_HYST,   /**< src/dest for T_HYST, degC */
             unsigned char *cfg,  /**< src/dest for config byte */
             char mode         /**< 'w' for write, otherwise read */
             )
{
  int status; // used for return value from i/o functions
  int t256;    // temperature in (1/256) degC units
  char buf[4]; // I/O buffer
  struct timespec ts = {0,11000000};  // 10 ms sleep required after write
 
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !! begin protected section.  No "returns" allowed in code below,
  // !! or else mutex might not get unlocked.  Beware!!!!!!!!!!!!!!!!
#define RETURN(s) { status=s; goto RETURN; }
  pthread_mutex_lock(&i2c_mutex);

  // tell bus who we want to talk to
  status = ioctl(fd, I2C_SLAVE, I2C_DS75_ADDR_BASE+addr);
  if (status < 0)
    RETURN(status);

  if (mode == 'w') {
    // write T_OS to register 3
    if (T_OS) {
      t256 = (int)( (*T_OS)*256 + 0.5 );
      buf[0] = 3;
      buf[1] = t256>>8;
      buf[2] = t256&(0xff);
      status = write(fd, buf, 3);
      if (status != 3)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
    // write TL to register 2
    if (T_HYST) {
      t256 = (int)( (*T_HYST)*256 + 0.5 );
      buf[0] = 2;
      buf[1] = t256>>8;
      buf[2] = t256&(0xff);
      status = write(fd, buf, 3);
      if (status != 3)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
    // write cfg to register 1
    if (cfg) {
      buf[0] = 1;
      buf[1] = *cfg;
      status = write(fd, buf, 2);
      if (status != 2)
        RETURN(-1);
      nanosleep(&ts, 0);
    }
  }
  else {
    // read T_OS from register 3
    if (T_OS) {
      buf[0] = 3;
      status = write(fd, buf, 1);
      status += read(fd, buf, 2);
      if (status != 3)
        RETURN(-1);
      *T_OS =  buf[0] + buf[1]/256.0;
    }
    // read T_HYST from register 2
    if (T_HYST) {
      buf[0] = 2;
      status = write(fd, buf, 1);
      status += read(fd, buf, 2);
      if (status != 3)
        RETURN(-1);
      *T_HYST =  buf[0] + buf[1]/256.0;
    }
    // read cfg from register 1
    if (cfg) {
      buf[0] = 1;
      status = write(fd, buf, 1);
      status += read(fd, buf, 1);
      if (status != 2)
        RETURN(-1);
      *cfg = buf[0];
    }
  }

  status = 0;

  // !! End of protected section
#undef RETURN
 RETURN:
  pthread_mutex_unlock(&i2c_mutex);
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!

  return status;
}


/*--- code for Dave Huffman's PIC-based tachometer for MicroBooNE ---*/

/** read speed of 12 fans from the PIC-based tachometer (Dave Huffman version) 
    Input array for rpm should be initialized.
    If fan speeds are seen for most but not all fans, old values are left
    in place for the fans without speeds, and no error is returned.
    Returns 0 on success, negative value on error. 

Details:
To read out the Dave Huffman PIC tachometer,
write n to I2C device 0xf, where n is a byte between 0 and 11,
then read from device 0xf repeatedly, receiving a string of the form

fnn=ssss

where nn is a two-digit decimal number equal to n+1,
and sss is a 4-digit decimal number equal to the fan speed.

If the PIC does not have an RPM for the channel (e.g., updating), it may
return a space in the first byte and a null in the second byte.

The string received should always be 8 chars long.
If it is not, it indcates a transmission error.

Examples (fans on):
f01=3016
f02=3011
f03=2994
f4=3033
f05=3039
f06=3016
f07=3005
f08=3068
f09=3062
f10=3068
f11=3074
f12=3068

Examples (fans off):
f01=0000
f02=0000
f03=0000
f04=0000
f05=0000
f06=0000
f07=0000
f08=0000
f09=0000
 0=0000
f11=0000
f12=0000
*/
int
read_fan_speeds( int fd,          /**< file descriptor for i2c bus device */
         double *rpm,     /**< destination for speed values, rpm */
         int n_fan        /**< number of speed values to save */
        )
{
  int status;      // used for return value from i/o functions
  char buf[16];    // I/O buffer for short exchanges
  int ich;         // channel index
  int ich_check;   // channel index seen in response, for check
  int ird;         // loop variable for reading 1 byte at a time
  int rpm_read;    // rpm seen in response
  int n_success;   // number of fans read successfully
  struct timespec ts = {0,11000000};  // ~10 ms sleep required after write

  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !! begin protected section.  No "returns" allowed in code below,
  // !! or else mutex might not get unlocked.  Beware!!!!!!!!!!!!!!!!
#define RETURN(s) { status=s; goto RETURN; }
  pthread_mutex_lock(&i2c_mutex);

  // tell bus who we want to talk to
  status = ioctl(fd, I2C_SLAVE, I2C_TACHOMETER_ADDR);
  if (status < 0)
    RETURN(status);

  // now read starting at 0 from tachometer
  n_success = 0;
  for (ich = 0; ich < n_fan; ich++) {
    buf[0] = I2C_TACHOMETER_ADDR;
    buf[1] = ich;
    status = write(fd, buf, 2);
    if (status != 2)
      RETURN(-2);
    memset(buf, 0, sizeof(buf));
    nanosleep(&ts, 0);
/*
    status = read(fd, buf, 9);  // magic string length = 9
    if (status != 9)
      RETURN(-3);
*/
    for (ird=0; ird<8; ird++) {
      status = read(fd, buf+ird, 1);
      if (status != 1)
        RETURN(-3);
    }
    if (buf[0] == ' ' && buf[1] == '\0') {
       // this is a normal occurence when fan data not ready
       continue;
    }
    if (buf[0] != 'f' || buf[3] != '=') {
      printf("Bad tachometer data for ich=%d, buffer `%s'   ", ich, buf);
      for (ird=0; ird<8; ird++) {
        printf(" %02x ", buf[ird]);
      }
      printf("\n");
      continue;
    }
    status = sscanf(buf, "f%d=%d", &ich_check, &rpm_read);
    if (status != 2 || ich_check != ich+1) {
      printf("Unparseable tachometer data for ich=%d, buffer `%s'\n", ich, buf);
      continue;
    }
    rpm[ich] = rpm_read;
    n_success += 1;
  } 

  // did we see a low success rate?  If so, count it as all failure.
  if (n_success <= n_fan/2)
    RETURN(-4);

  // success
  status = 0;

  // !! End of protected section
#undef RETURN
 RETURN:
  pthread_mutex_unlock(&i2c_mutex);
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!

  return status;
}

