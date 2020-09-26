#include <stdio.h>
#include <string.h>
#include "dev_gpio.h"

const char GPIO_FILENAME_PREFIX[] = "/sys/class/gpio/gpio";
#define GPIO_PREFIX_LEN sizeof(GPIO_FILENAME_PREFIX)


/** get filename for accessing given gpio id and parameter
 * filename is written into user-supplied buffer
 * Returns 0 for ok, -1 for error
 */
int
gpio_get_path(int gpio, /**< gpio id number for /sys/class/gpio/gpio%d */
              const char* value, /**< e.g., "value", "direction", etc. */
              char* fnout, /**< pointer to user-supplied buffer */
              size_t buffsize /**< length of buffer */)
{
  char *cp;
  if (!fnout)
    return -1;
  *fnout = '\0';
  if (gpio < 0 || gpio > 999) 
    return -1;
  if (buffsize < GPIO_PREFIX_LEN + strlen(value) + 5)
    return -1;
  strcpy(fnout, GPIO_FILENAME_PREFIX);
  cp = fnout + sizeof(GPIO_FILENAME_PREFIX) - 1;
  if (gpio < 10)
    *(cp++) = '0' + gpio;
  else if (gpio < 100) {
    *(cp++) = '0' + gpio/10;
    *(cp++) = '0' + gpio%10;
  }
  else {
    *(cp++) = '0' + gpio/100;
    *(cp++) = '0' + (gpio/10)%10;
    *(cp++) = '0' + gpio%10;
  }
  *(cp++) = '/';
  strcpy(cp, value);
  return 0;
}

/** read gpio pin state.
 *  Returns 0 for low, 1 for high, negative value on error. */
int
gpio_read(int gpio /**< gpio id number for "/sys/class/gpio/gpio%d" */)
{
  char fn[FILENAME_MAX];
  int istat;
  istat = gpio_get_path(gpio, "value", fn, sizeof(fn));
  if (istat < 0) 
    return -1;
  FILE *f = fopen(fn, "r");
  if (!f) 
    return -1;
  istat = fgetc(f);
  fclose(f);
  if (istat == '0')
    return 0;
  else if (istat == '1')
    return 1;
  return -1;
}    

/** write gpio pin state.
 *  Returns non-negative number on success, negative value on error. */
int
gpio_write(int gpio,   /**< gpio id number for "/sys/class/gpio/gpio%d" */
           int state   /**< state to write, any non-zero treated as 1 */ )
{
  char fn[FILENAME_MAX];
  int istat;
  istat = gpio_get_path(gpio, "value", fn, sizeof(fn));
  if (istat < 0) 
    return -1;
  FILE *f = fopen(fn, "w");
  if (!f) 
    return -1;
  fputc((state ? '1' : '0'), f);
  istat = fputc('\n', f);
  fclose(f);
  if (istat != '\n')
    return -1;
  return 0;
}
