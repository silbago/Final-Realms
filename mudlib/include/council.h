/**
 * @main
 * A global configuration file for defaults regarding usergroups of
 * type guild, groups and race groups. For areas owned by these groups.
 * Access and denied access, other things regarding these ranking systems
 * @author Silbago
 */

/** @ignore start */
#ifndef __INCLUDE_COUNCIL_H__
#define __INCLUDE_COUNCIL_H__
/** @ignore end */

#include <handlers.h>
#define LORD_ACCESS_LEVEL      13
#define APRIOR_ACCESS_LEVEL    12
#define PATRON_ACCESS_LEVEL    11
#define LEADER_ACCESS_LEVEL    10
#define A_LEADER_ACCESS_LEVEL   9
#define TREASURY_ACCESS_LEVEL   8
#define COUNCIL_ACCESS_LEVEL    7
#define ELITE_ACCESS_LEVEL      6
#define MEMBER_ACCESS_LEVEL     5
#define INVITED_ACCESS_LEVEL    4
#define GUEST_ACCESS_LEVEL      3
#define BANISHED_ACCESS_LEVEL   2
#define WANTED_ACCESS_LEVEL     1
#define NO_ACCESS_LEVEL         0

/**
 * When a player joins, changes ranks or whatever the cache needs update.
 */
#define REFRESH_LEVELS(XXX) "/handlers/groups/council"->update_user_levels(XXX)

#endif
