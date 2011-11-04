/**
 * @main
 * Access settings for the rooms specificly, similar settings
 * in the warehouse.c for the warehouse generally.
 *
 * For default everything falls to no restriction, for group
 * owned warehouses you migth want to do this in warehouse.c
 *   set_access_level(MEMBER_ACCESS_LEVEL);
 * Some rooms for guests, do this in given roomfile
 *   set_access_level(GUEST_ACCESS_LEVEL);
 *
 * Room specific settings override warehouse general.
 * Default settings are no restrictions - so be aware.
 * Notice no access settings in warehouse results in yes as
 * it does if you do unrestricted_warehouse()
 *
 * @author Silbago
 */

#include <council.h>

#define TEST_ACCESS(XXX)       \
    if ( !query_access(TP) ) { \
        notify_fail(XXX);      \
        return 0;              \
    }

private nosave int room_specific_access;

/**
 * Test if this room gives access, if not the warehouse.
 */
int query_access(object me) {
    if ( !undefinedp(room_specific_access) ) {
        if ( room_specific_access == -1 )
            return 1;

        return query_access_level(me, group_name, room_specific_access);
    }

    // This is a bug btw, just to be redundant.
    //
    if ( !find_warehouse() ) {
        warning("Testing access in vault room, did not find warehouse object.");
        return 1;
    }

    return warehouse->query_access(me);
}

/**
 * This is a setting that makes no restrictions for the warehouse.
 */
void unrestricted_room() {
    room_specific_access = -1;
}

/**
 * Setting room specific access level according to COUNCIL levels.
 */
void set_access_level(int i) {
    room_specific_access = i;
}
