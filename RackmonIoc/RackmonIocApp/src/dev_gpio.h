#ifndef __dev_gpio_h__
#define __dev_gpio_h__  1

/** read gpio pin state.
 *  Returns 0 for low, 1 for high, negative value on error. */
int
gpio_read(int gpio   /**< gpio id number for "/sys/class/gpio/gpio%d" */
    );

/** write gpio pin state.
 *  Returns non-negative number on success, negative value on error. */
int
gpio_write(int gpio,   /**< gpio id number for "/sys/class/gpio/gpio%d" */
           int state   /**< state to write, 0 or 1 */
    );

#endif  /* __dev_gpio_h__ */
