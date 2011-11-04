/**
 * @main
 * Communication filter, against players who are in the sabotage mode.
 * It sure wont make them happier, but it will make them silent.
 *
 * The intention is to not let them communicate with new players, and
 * otherwise cause sadness among the old players who enjoy this.
 *
 * In brief, this filters communications on those who are hellbent on sabotage.
 * @author Silbago
 */

#include <handlers.h>

/**
 * Players in these accounts are set as BAD.
 * They can't no nothin'
 */
#define BAD_ACCOUNTS ({ \
    "dugu", \
})

/**
 * Members of this list cant shout.
 */
#define DONT_SHOUT ({ \
    "", \
})

int no_shout(mixed usr) {
    if ( objectp(usr) )
        usr = geteuid(usr);
    else if ( !stringp(usr) )
        return 0;

    usr = ACCOUNT_HANDLER->query_account_owner(usr);
    if ( member_array(usr, BAD_ACCOUNTS) != -1 )
        return 1;
    if ( member_array(usr, DONT_SHOUT) != -1 )
        return 1;
    return 0;
}
