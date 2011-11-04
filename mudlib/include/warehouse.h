/**
 * @main
 * Global Vault settings for the warehouse system.
 * @author Silbago
 */

/** @ignore start */
#ifndef __INCLUDE_VAULT_H__
#define __INCLUDE_VAULT_H__
/** @ignore end */

/**
 * This is how many items we store on each shelf.
 * Not interesting -at all- regarding total stock size on warehouse
 * or in each vaultroom. Just to be perfectly clear. Do not change.
 *
 * A historical note is required about this, Silbago was here.
 * Over the course of 15 years Radix' vault had limit of 50, it was
 * perfectly stable. I tested it up to 60 to stress and to see what
 * happened, it crashed. Then I setup a memory cleaning method in
 * the auto_load program, and it became stable up to 90.
 *
 * Now with the usage of attributes memory usage increased ever more.
 * Recommend to keep this at 35, it is probably safe slightly above.
 */
#define MAX_PER_SHELF 35

/**
 * The default cost table is 1 vault = 50 items, so let's
 * deal with that. If the cost helpfile is changed change this ?
 */
#define MAX_ITEMS 35

#define WAREHOUSE_SHELF "/std/rooms/warehouse/shelf"

#define VAULTS_CLOSED ("/handlers/omiq/omiq.c"->flag_in_progress() || (find_object("/obj/shut") && find_object("/obj/shut")->query_time_to_crash() > - 10))

#define USE_OK         20
#define USE_FULL       21
#define USE_CLOSED     22
#define USE_CURSED     23
#define USE_SHIFTED    24
#define USE_NO_MOVE    25
#define USE_TOO_HEAVY  26
#define USE_SHELF_FULL 27
#define USE_NOT_HERE   28
#define USE_NO_LIVING  29

/**
 * Info on the deposited items.
 */
#define WAREHOUSE_TAGS "std:warehouse_tags"

/**
 * Deposited by person.
 */
#define WH_DEP_TAG_BY     0

/**
 * Person wants to keep it private.
 */
#define WH_DEP_TAG_OWN_IT 1

/**
 * Labels from the vaultroom it was deposited from.
 */
#define WH_DEP_TAG_LABELS 2

/**
 * We use this to identify vault swapping, 5 minutes.
 */
#define WAREHOUSE_RETRIEVED "std:warehouse_retrieved"

/**
 * Retrieved by this person.
 */
#define WH_RET_BY        0

/**
 * Retrieved from vault.
 */
#define WH_RET_FROM      1

/**
 * Originally deposited by.
 */
#define WH_RET_ORG_DEP   2

/**
 * Retrieved from what warehouse.
 */
#define WH_RET_WAREHOUSE 3

/**
 * When it was retrieved.
 */
#define WH_RET_TIME      4

#endif

