/*  Development notes
 *  --- Dimension    -> Not steady or rolled out properly, not tested or implemented properly
 *  --- NoSave       -> Same as with dimension
 *  --- Calibration  -> The entire mud requires a proper calibration, then twice a year
 *  --- Plain Coord  -> Work a bit more on this on specific areas (arena is done)
 */

/**
 * @main
 * Coordinate code for environment (rooms).
 * Coordinates move, bleed and migrate.
 *
 * When room loads, in create() in room.c we start by asking
 * for coordinates if it havent been done in setup().
 *
 * Standard room sizes, notice that 3rd dimension
 * only matters when it comes to up and down exits. We need to have it here somewhere.
 * ----Type of room     X     Y      Z -----
 *     room (indoor)    5 x   5  x   3 yards
 *     outside         15 x  15  x  15 yards
 *     underground      7 x   7  x   7 yards
 *     sea            100 x 100  x  50 yards
 *     forest          10 x  10  x  20 yards
 *
 * We have a secondary static coordinate system that takes care of distances
 * regarding amount of rooms and not size as the proper coordinate system does.
 *
 * Nosave coordinates
 *     Some areas has hardset coordinates, like arena.
 *     Arena should not be uploaded to DB at all, it is nowhere.
 *     The same goes for "chaos area".
 *     Other off-grid areas like Onyx Mirror saves however (real places)
 *
 * Area Dimensions
 *     Immortal heavens, Chaos area, Arena has no tellable location.
 *     Onyx Mirror (and a few others have - on top of XYZ mountain)
 *
 *     Just not easy to ascertain where actually.
 *     Used by shout/tell and other services that analyze distance-angles.
 *     Distance and angles make no sense from another dimension.
 *     Each Dimension has an owned ID number.
 *
 * @seealso See COORDINATE_HANDLER for detailed information.
 * @author Silbago
 */

#include <coordinates.h>

protected nosave int coordinates_set,                     // Ok it is set
                    *room_size = ({                       // Size values
                        STANDARD_ROOM_SIZE[0],
                        STANDARD_ROOM_SIZE[1],
                        STANDARD_ROOM_SIZE[2],
                      }),
                    *coordinates = DEFAULT_COORDINATE,    // Size based
                    *plain_coordinates = allocate(6);     // Same as with regulars

private nosave mapping exit_direction_references = ([ ]);

/**
 * Special areas (blink/onyx mirror/duel arena) are in other dimensions.
 * Very important for distance range effects !
 *
 * Therefore they can have the same coordinates but not be there.
 * Also goes for /w/ areas, each creator his dimension.
 */
int query_dimension() {
    return coordinates[COORD_ARRAY_DIMENSION];
}

/**
 * Coordinates to not upload to database (blink/arena).
 */
int query_nosave_coordinate() {
    return coordinates[COORD_ARRAY_NOSAVE];
}

/**
 * X;Y;Z size in feet
 */
int *query_room_size  () { return copy(room_size); }

/**
 * Ok you get x,y,z coordinates in return.
 * These are room-size based coordinates however, by feet.
 *
 * Return parameters,
 *   1->3   x,y,z (obviously)
 *   4      Quality level
 *   5      Dimension
 *   6      0 = save to, 1 = nosave (for cache database)
 */
int *query_coordinates() {
    return copy(coordinates);
}

/**
 * These are not size based, just room-per-room count.
 * Works mostly fine on short range, needed for spell/effect
 * ranges that cant rely on feet distance. Too unpredicteable.
 *
 * The return array is the same as normal coordinates.
 */
int *plain_coordinates() {
    return copy(plain_coordinates);
}

/**
 * North, east, up ... is alright. But what direction is hole or mist really ?
 */
string geographical_exit_direction(string dir) {
    if ( exit_direction_references[dir] )
        return exit_direction_references[dir];
    return dir;
}

/**
 * Basically no use for this, but used in Arena where local
 * coordinates is extremely important.
 *
 * Don't use this to stretch out global coordinate grid,
 * the 'regular' size-based coordinates deals with it.
 *
 * So therefore use this to deal with small-size local grids.
 *
 * Need to be able to hardlock these against migration.
 * Mostly needed for Arena, as coordinate migration gets very
 * wrong when non-loaded rooms in an area starts loading from
 * each direction.
 *
 * As such we also need dimension if needed.
 */
varargs void set_plain_coordinates(int x, int y, int z, int dimension) {
    if ( undefinedp(x) || undefinedp(y) || undefinedp(z) )
        return;

    // Ok coordinates is hard set here so we dont override.
    //
    if ( plain_coordinates[COORD_ARRAY_LEVEL] == 1 )
        return;

    // well it is set alright, so it must be 1
    // Not sure how much dimension is used, but we need it
    // for arena and other places.
    //
    // Nosave is irrelevant, we never use nosave on these.
    // Keeping it to have exact same array format as regular coord.
    //
    plain_coordinates = ({ x, y, z, 1, dimension, 0 });
}

/**
 * This method to ONLY be used by authorized personell.
 * Used for HARD LOCKED coordinates, set up by an authorized
 * administrator during process of calibration of the globe.
 *
 * Fourth parameter: Dimension
 *   Ok we need dimensions and we need nosave flag.
 *   Dimensions as Chaos area, Arena etc.
 *
 *   The Onyx Mirror place, probably needs a physical place
 *   though -> Somewhere in a mountain on FR ?
 *
 * Fifth parameter: Nodump
 *   If you don't want the coordinate to be uploaded to the
 *   COORDINATE_HANDLER cache of locked coordinates. Nosave ...
 */
varargs void set_coordinates(int x, int y, int z, int dimension, int nodump) {
#   ifdef DEVELOPMENT
    if ( TP )
        TP->msg("%^YELLOW%^BING%^RESET%^ $# $# $#\n", x, y, z);
#   endif

    if ( undefinedp(x) || undefinedp(y) || undefinedp(z) )
        return;

    coordinates       = ({ x, y, z, 1, dimension, nodump });

    if ( !nodump )
        COORDINATES_HANDLER->update_coordinate_reference(TO, ({ x, y, z, 1, dimension, 0 }));
    coordinates_set = 1;
    if ( !nodump )
        TO->dump_coordinate_xml_data();
}

/**
 * Strange exits, we need to know where they go. Really.
 * Like exit "crack", does it lead e,w,s,n,se,sw,ne,nw,up or down.
 *
 * This behaviour is supported in add_exit() so can be removed.
 */
void set_exit_direction(string exit, string dir) {
    exit_direction_references[exit] = dir;
}

/**
 * Stretching out the room, this does two things
 * Determines how many objects that can be in this room or
 * living to be more accurate.
 * Secondly it widens out the walking distances.
 *
 * For the record x is east-west.
 *                y is north-south.
 *                z is up-down.
 *
 * Standard sizes (actually read /include/coordinates.h)
 *     room (indoor)    6 x   6  x   3 yards
 *     outside         50 x  50  x  15 yards
 *     underground     15 x  15  x   5 yards
 *     sea            500 x 500  x  10 yards
 *     forest          30 x  30  x  20 yards
 *
 * Parameters: X, Y, Z. Or varargs sizeof X == 3
 */
varargs void set_room_size(mixed x, int y, int z) {
    if ( arrayp(x) ) {
        z = x[COORD_ARRAY_Z];
        y = x[COORD_ARRAY_Y];
        x = x[COORD_ARRAY_X];
    }

    room_size = ({ x, y, z });
}

void adjust_room_size_x(int i) {
    if ( (room_size[0] + i) < 1 )
        warning("adjust_room_size_x(" + i + "), result is less than 1. Aborted", 1);
    else
        room_size[0] += i;
}

void adjust_room_size_y(int i) {
    if ( (room_size[1] + i) < 1 )
        warning("adjust_room_size_y(" + i + "), result is less than 1. Aborted", 1);
    else
        room_size[1] += i;
}

void adjust_room_size_z(int i) {
    if ( (room_size[2] + i) < 1 )
        warning("adjust_room_size_z(" + i + "), result is less than 1. Aborted", 1);
    else
        room_size[2] += i;
}

/**
 * Coordinates inserted from neighbour room by walking.
 * If direction is not geographical we downgrade strength.
 *   east, north, south, west, ne, se, sw, nw, up and down.
 *
 * Tier   Where it comes from                  Downgraded ?
 *   1    Hard setting in setup() of room      Yes to 2 by walking
 *   2    Migrated by walk from 1              No, origins from 1
 *   3    Directory setting grabbed from 1     Yes to 3 by walking
 *   4    Migrated by walk from 3              No, origins from 3
 *   5    Default zero coordinates
 *
 * Adjusting a coordinate on roomsizes means we have to use half of
 * both rooms to get a median value. Else going back will not give same.
 */
void migrate_coordinates(string dir, int x, int y, int z, int strength, object from_room) {
    int *from_size, *from_plain_coordinates, from_dimension, from_nosave_coords;

    if ( objectp(from_room) ) {
        from_size              = from_room->query_room_size();
        from_dimension         = from_room->query_dimension();
        from_nosave_coords     = from_room->query_nosave_coordinate();
        from_plain_coordinates = from_room->plain_coordinates();
    }

    if ( !from_size )
        from_size = ({
            STANDARD_ROOM_SIZE[0],
            STANDARD_ROOM_SIZE[1],
            STANDARD_ROOM_SIZE[2],
        });

    if ( !from_plain_coordinates )
        from_plain_coordinates = DEFAULT_COORDINATE;

// This is for legacy code that must be removed, if you see this, just do it.
// Seriously.
// Then gupdate /std/rooms, update /std/outside.c  wait a while and see for bugs
// It should not happen though. Silbago Nov 2011
    else while(sizeof(from_plain_coordinates) < 6 )
        from_plain_coordinates += ({ 0 });

    // Event login setting, he logged off with better coordinates
    // than coordinates static database could find.
    //
    dir = LC(dir);
    if ( exit_direction_references[dir] )
        dir = exit_direction_references[dir];
    if ( dir == "logon" ) {
        if ( strength < coordinates[COORD_ARRAY_LEVEL] ) {
            x = coordinates[COORD_ARRAY_X];
            y = coordinates[COORD_ARRAY_Y];
            z = coordinates[COORD_ARRAY_Z];
        }
    } else

    // Exit has a real direction. Notice that 'logon' is nulling.
    // So should teleport do as well
    //
    if ( MIGRATE_COORDINATES[dir] ) {
        int *merged_size;
        if ( sizeof(from_size) == 3 ) {
            merged_size = copy(room_size);
            merged_size[0] = (merged_size[0] + from_size[0]) / 2;
            merged_size[1] = (merged_size[1] + from_size[1]) / 2;
            merged_size[2] = (merged_size[2] + from_size[2]) / 2;
        } else
            merged_size = room_size;

        if ( from_plain_coordinates[COORD_ARRAY_LEVEL] <= plain_coordinates[COORD_ARRAY_LEVEL] ) {
            plain_coordinates[0] = from_plain_coordinates[0] + MIGRATE_COORDINATES[dir][0];
            plain_coordinates[1] = from_plain_coordinates[1] + MIGRATE_COORDINATES[dir][1];
            plain_coordinates[2] = from_plain_coordinates[2] + MIGRATE_COORDINATES[dir][2];
        }

        x += MIGRATE_COORDINATES[dir][0] * merged_size[0];
        y += MIGRATE_COORDINATES[dir][1] * merged_size[1];
        z += MIGRATE_COORDINATES[dir][2] * merged_size[2];
        // Ok there's a good reason for this. Dont mess up.
        if ( strength == 1 || strength == 3 )
            strength++;

    // Not sure what I intended with this one really.
    // MAP-directions not fully implemented on Final Realms yet
    //
    } else {
#       ifdef 0
        Uhm just not yet please ... :) Too much spamm
        warning("migrate_coordinates(\"" + dir + "\"), we dont know what "
                "geographical direction this is.  Use set_exit_direction() "
                "in conjunction with add_exit().", 1);
#       endif

        if ( strength < 3 )
            strength = 3;
    }

#   ifdef DEVELOPMENT
    if ( TP )
        TP->msg("Moved direction $O\n"
                "Attempt to set : $# $# $# ($# $#)\n"
                "Has got setting: $# $# $# ($# $#)\n",
                dir,
                x, y, z, strength, from_dimension,
                coordinates[0], coordinates[1], coordinates[2],
                coordinates[3], from_dimension);
#   endif

    // Coordinates here are better anyways, no budge !
    //
    if ( coordinates_set || coordinates[COORD_ARRAY_LEVEL] < strength )
        return;

    coordinates = ({ x, y, z, strength, from_dimension, 0 });
    TO->dump_coordinate_xml_data();

    if ( strength == 1 )
        coordinates_set = 1;
}
