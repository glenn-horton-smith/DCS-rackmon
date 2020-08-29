#ifndef __dev_i2c_h__
#define __dev_i2c_h__  1


/* Constants for the I2C devices */

/** number of tachometer channels */
#define N_TACHOMETER_CH 12 

/** maximum value of DS1621 address */
#define MAX_DS1621_ADDRESS 7


/** Open I2C bus file descriptor for read/write access.
    Returns file descriptor on success, -1 on error, same as open() */
int
open_i2c_bus(int bus /**< bus number, 0 for first (or only) bus */ );


/** read temperature from specified ds1621.
    Returns 0 on success, negative value on error. */
int
read_temp_ds1621( int fd,          /**< file descriptor for i2c bus device */
		  int addr,        /**< 3-bit address of ds1621 */
		  double *T_degC   /**< destination for temperature, degC */
		  );


/** read or write thresholds TH and TL and config byte from specified ds1621.
    Any pointer argument may be null, in which case the corresponding
    quantity is not read/written to the ds1612.
    Returns 0 on success, negative value on error. */
int
access_ds1621_details( int fd,         /**< file descriptor for i2c bus device */
		     int addr,       /**< i2c address of the device */
		     double *TH,     /**< src/dest for TH, degC */
		     double *TL,     /**< src/dest for TH, degC */
		     unsigned char *cfg,  /**< src/dest for config byte */
		     char mode       /**< 'w' for write, otherwise read */
		       );


/** read speed of 12 fans from the PIC-based tachometer (Dave Huffman version) 
    Input array for rpm should be initialized.
    If fan speeds are seen for most but not all fans, old values are left
    in place for the fans without speeds, and no error is returned.
    Returns 0 on success, negative value on error. 
*/
int
read_fan_speeds( int fd,          /**< file descriptor for i2c bus device */
		 double *rpm,     /**< destination for speed values, rpm */
		 int n_fan        /**< number of speed values to save */
		 );

#endif  /* __dev_i2c_h__ */
