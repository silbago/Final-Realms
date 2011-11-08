/**
 * @main
 * Contains the default settings on size on the most standard rooms.
 * These standard rooms should use these settings rather than internal
 * setup. That does not mean that you cant use custom settings, please do.
 *
 * Algorithms ascertain through these settings to get distances over
 * continents. As does long range commands, spells and whatnot.
 *
 * @author Silbago
 */

/** @ignore start */
#ifndef __INCLUDE_COORDINATES_H__
#define __INCLUDE_COORDINATES_H__
/** @ignore end */

/**
 * The dimensional pockets have their own owned slots.
 */
#define DIMENSIONAL_SLOTS ({ \
    0 : "Real World",        \
    1 : "Duel Arena",        \
    2 : "Blink Area",        \
})

#define COORD_ARRAY_X         0
#define COORD_ARRAY_Y         1
#define COORD_ARRAY_Z         2
#define COORD_ARRAY_LEVEL     3
#define COORD_ARRAY_DIMENSION 4
#define COORD_ARRAY_NOSAVE    5

#define COORD_QUAL_LOCKED        1
#define COORD_QUAL_LOCK_MIGRATED 2
#define COORD_QUAL_AREA_LOCK     3
#define COORD_QUAL_AREA_MIGRATED 4
#define COORD_QUAL_DEFAULT_ZONE  5

/**
 * Ancient FR is designed to room sizes 1-1-1
 * This makes large distance calculations offset by enlarge.
 * Systems should compensate this internally by room type, but
 * if it does not want to it can use this factor.
 * We assume this is the choice of lesser evil.
 * @author Silbago
 */
#define UNIVERSAL_COORDINATE_FACTOR 12

/**
 * Standard size for outside rooms (std/outside.c)
 */
#define OUTSIDE_ROOM_SIZE ({50, 50, 15})

/**
 * Standard size for sea rooms (std/sea.c)
 */
#define SEA_ROOM_SIZE ({500, 500, 10})

/**
 * Standard size for underground rooms (std/underground.c)
 */
#define UNDERGROUND_ROOM_SIZE ({15,15,5})

/**
 * Standard size for basic rooms rooms (std/room.c)
 * Inserted not in create() but by /std/rooms/object/coordinates.h
 *
 * Most common setting for indoor rooms.
 */
#define STANDARD_ROOM_SIZE ({5,5,3})

/**
 * Standard size for forest rooms (std/rooms/forest.c)
 */
#define FOREST_ROOM_SIZE ({30,30,20})

#endif
