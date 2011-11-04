/*    We need to consider to have each shelf locked to the labels.
      This means labels not stored on the items and we have unique shelves.
      In turn this means we might lose items in the warehouse. hahahaha.
      Initial intention: save memory
*/
/**
 * @main
 * This is the shelf for the warehouse, the warehouse controls all aspects of it.
 * You do not need to worry about this object at all.
 * Replica code with small alterations from the original vault_obj code.
 *
 * The shelf stays loaded until the cleaner kills it, or the warehouse times it out.
 * This is an object to be cloned, not inherited. Residing in /std/ with friends.
 * We do not allow saving in /std/, will be messy.
 *
 * Beta: Cleaner         : Ensure objects doesnt just disappear.
 *       Volothamp's idea: by only keeping the auto load array in memory. Very good
 *                         Need a nosave array for this, and to update by add/remove.
 *                         This will probably ruin the list cache in vaultroom.
 *
 * @author Silbago
 */

inherit "/std/container.c";
inherit "/global/auto_load";

#include <council.h>
#include <warehouse.h>
#include <move_failures.h>

private nosave string savefile;
private nosave object *stockpile;

mixed *auto_load;

string query_savefile  () { return savefile;                  }
object *query_stockpile() { return all_inventory(TO);         }
void   clean_up        () { return 0;                         }
int    query_stock_size() { return sizeof(all_inventory(TO)); }

/**
 * Loading the stock in the shelf.
 * If the loading fails we assume this shelf is to be created so we save
 * it instantly for the warehouse to be able to list it properly.
 */
void load_shelf(string path) {
    if ( savefile ) {
        warning("Attempt to reload warehouse shelf \"" + path + "\" "
                "while savefile \"" + savefile + "\" is deined.", 1);
        return;
    }

    savefile  = path;
    auto_load = ({ });
    seteuid("Root");

    // Loading failed, we file, we save it to register as new shelf.
    //
    if ( !restore_object(savefile) && savefile[0..4] != "/std/")
        save_object(savefile);

#   ifdef DEVELOPMENT
    printf("Attempting to load shelf %O\n", path);
#   endif
    seteuid("PLAYER");

    if ( sizeof(auto_load) )
        stockpile = load_auto_load(auto_load, TO);
    else
        stockpile = ({ });
}

/**
 * After each change we save, redundancy is priority.
 */
private void save_shelf() {
    stockpile = all_inventory(TO);
    if ( !savefile )
        return;

    if ( sizeof(stockpile) )
        auto_load = create_auto_load(stockpile);
    else
        auto_load = ({ });

    seteuid("Root");
    if ( savefile[0..4] != "/std/" )
        save_object(savefile);
    seteuid("PLAYER");
}

/**
 * Listing and finding objects for retrieval.
 * By criterias of the vaultROOM vault_labels AND freetext argument.
 *
 * Get item #number, how that work out for us ?
 * That is translated by the warehouse into argument ?
 */
object *filter_objects(string arg, string *vault_labels, mapping flags) {
    object *obs, *ret;
    stockpile = all_inventory(TO);

    if ( !vault_labels ) {
        if ( arg )
            return find_items(TP?TP:TO, arg, 0, 0, 0, stockpile);

        return stockpile;
    }

    if ( !flags )
        flags = ([ ]);

    if ( arg )
        obs = find_items(TP?TP:TO, arg, 0, 0, 0, stockpile);
    else
        obs = stockpile;

#   ifdef DEVELOPMENT
    if ( arg && sizeof(obs) && TP && geteuid(TP) == "silbago" )
        printf("Shelf %O has found content %O\n\nAll content %O\n", TO, obs, stockpile);
#   endif

    ret = ({ });
    foreach(object ob in obs) {
        mixed tags;
        if ( member_array(ob, ret) != -1 )
            continue;

        tags = ob->query_property(WAREHOUSE_TAGS);
        if ( sizeof(tags) != 3 )
            tags = allocate(3);

        if ( flags["u"] && flags["u"] != tags[WH_DEP_TAG_BY] )
            continue;

        if ( flags["t"] && flags["t"] != ob->query_quality_tier_string() )
            continue;

        if ( flags["c"] && flags["c"] != ob->cond_word() )
            continue;

        if ( flags["p"] && !tags[WH_DEP_TAG_OWN_IT] )
            continue;

        if ( flags["P"] && !(tags[WH_DEP_TAG_OWN_IT] & 2) )
            continue;

        // The vault doing the query wants to see all labels
        //
        if ( !sizeof(vault_labels) ) {
            ret += ({ ob });
            continue;
        }

/*      Bad incidents with bad setup vaults cause this.
        // There are no tags on the deposited item, should not happen but can !
        //
        if ( !sizeof(tags[WH_DEP_TAG_LABELS]) ) {
            ret += ({ ob });
            continue;
        }
*/

        // The argument matches, but we need to scan for matching labels.
        //
        foreach(string lbl in vault_labels)
            if ( member_array(lbl, tags[WH_DEP_TAG_LABELS]) != -1 ) {
                if ( member_array(ob, ret) == -1 )
                    ret += ({ ob });
                continue;
            }
    }

    return ret;
}

/**
 * Depositing query from the warehouse.
 * @return USE_OK at success, else the other failures
 */
int add_item(object ob, object vault_ob, object warehouse_ob, object me, int own_it) {
    int i;
    mixed tmp, tags = ({ 0, 0, 0, });

    stockpile = all_inventory(TO);
    if ( sizeof(stockpile) >= MAX_ITEMS )
        return USE_SHELF_FULL;

    if ( !ob->gettable() )
        return MOVE_NOT_DROPPABLE;

    if ( !ob->droppable() )
        return MOVE_NOT_GETTABLE;

    if ( (i=ob->move(TO)) != MOVE_OK )
        return i;

    tmp = vault_ob->query_vault_labels();
    if ( !tmp )
        tmp = ({ });

    previous_object()->log_deposit(ob, me, vault_ob, warehouse_ob);

    if ( ob->query_static_property("anonymous deposit") )
        tags[WH_DEP_TAG_BY] = "anonymous";
    else
        tags[WH_DEP_TAG_BY] = me->query_name();
    tags[WH_DEP_TAG_OWN_IT] = own_it;
    tags[WH_DEP_TAG_OWN_IT] = own_it;
    tags[WH_DEP_TAG_LABELS] = tmp;

    // For handling transfer of owner tags when moving to warehouse
    //
    if ( (tmp=ob->query_static_property("sticky vault transaction")) ) {
        tags[WH_DEP_TAG_BY]     = tmp[0];
        tags[WH_DEP_TAG_OWN_IT] = tmp[1];
    }

    ob->add_property(WAREHOUSE_TAGS, tags);
    ob->remove_property(WAREHOUSE_RETRIEVED);

    // Cleanup of past fun, keep until late 2010
    //
    ob->remove_property();
    ob->remove_timed();
// Investigate cleaning of nulled attributes (not timed out)

    save_shelf();
    return USE_OK;
}

/**
 * Retrieve query from the warehouse.
 * @return USE_OK at success, else the rest of the failures
 */
int remove_item(object ob, object vault_ob, object warehouse_ob, object me, int sticky) {
    int ret;
    mixed tags, retag;

    // We do not "retrieve" items from other places !
    //
    if ( member_array(ob, all_inventory(TO)) == -1 )
        return USE_NOT_HERE;

    tags = ob->query_property(WAREHOUSE_TAGS);
    if ( tags[WH_DEP_TAG_OWN_IT] && tags[WH_DEP_TAG_BY] != geteuid(me) ) {
        string own = warehouse_ob->query_group();
        int yes = query_access_level(me, own, COUNCIL_ACCESS_LEVEL);
        me->msg("$O is marked as owned by $O, ", ob, tags[WH_DEP_TAG_BY]);

        if ( tags[WH_DEP_TAG_OWN_IT] == 1 || yes )
            me->msg("please do not steal it.\n");
        else {
            me->msg("you're denied taking it.\n");
            return USE_NO_MOVE;
        }
    }

    if ( (ret=ob->move(me)) != MOVE_OK ) {
        if ( (ret=ob->move(ENV(me))) != MOVE_OK )
            return ret;
    }

    previous_object()->log_retrieve(ob, me, vault_ob, warehouse_ob);

    tags = copy(ob->query_property(WAREHOUSE_TAGS));
    if ( sizeof(tags) != 3 )
        tags = allocate(3);

    // For transfering purposes to keep the original owners
    //
    if ( sticky ) {
        ob->add_static_property("sticky vault transaction", ({
           tags[WH_DEP_TAG_BY],
           tags[WH_DEP_TAG_OWN_IT],
        }));
    }
    ob->remove_property(WAREHOUSE_TAGS);

    // Need some data on the retrieved item. Keep in mind we delete at logoff.
    //
    retag = ({ 0, 0, 0, 0, 0 });
    retag[WH_RET_BY]        = me->query_name();
    retag[WH_RET_FROM]      = vault_ob->query_short();
    retag[WH_RET_ORG_DEP]   = tags[WH_DEP_TAG_BY];
    retag[WH_RET_WAREHOUSE] = base_name(warehouse_ob);
    retag[WH_RET_TIME]      = time();

    ob->add_timed_property(WAREHOUSE_RETRIEVED, retag, 120);
    save_shelf();
    return USE_OK;
}
