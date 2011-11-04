/**
 * @main
 * Global and important settings and tables for the mounts.
 * @author Silbago
 */

/** @ignore start */
#ifndef __INCLUDE_MOUNT_H__
#define __INCLUDE_MOUNT_H__
/** @ignore end */

/**
 * This is what they get when getting mount artifact reward.
 * It turns into real horse when they release the bridle.
 */
#define REWARD_BRIDLE "/obj/mounts/bridle_reward"

/**
 * All mount objects MUST be located here.
 */
#define MOUNT_DIR "/obj/mounts/"

/**
 * Translated seconds life of mount to year.
 * 12 hours online equals 1 year.
 */
#define MOUNT_YEAR(XXX) (XXX/(12*3600))

/**
 * Standard for how long a year is for a mount in seconds
 */
#define MOUNT_YEAR_VALUE (3600*12)

/**
 * What speed each animal has for each age.
 * If age for some reason is < 1 || > 20 we give it -20.
 */
#define SPEED_RANGE ({ \
  -3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-3, \
  -4,-4,-4,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-4,-4,-4, \
  -4,-4,-4,-4,-3,-3,-3,-3,-2,-2,-2,-2,-3,-3,-3,-4,-4,-5,-5,-5, \
  -6,-5,-5,-5,-5,-4,-4,-4,-4,-4,-4,-4,-4,-5,-5,-5,-6,-6,-7,-7, \
})

/**
 * Prices for the beasts are set for age 10.
 * Changes dynamically by percent value.
 * As you can see prime age they fluctuate for prime-age.
 */
#define PRICE_RANGE ({ \
   50, 65, 80, 110, 120, 123, 125, 120, 110, 100, \
   90, 85, 80,  70,  60,  50,  35,  20,  15,  15, \
})

#endif
