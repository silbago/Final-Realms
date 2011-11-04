/**
 * @main
 *
 * This is the warehouse, where all the shelves are located that holds
 * all the stored items. This is not a room or some object to move.
 *
 * PROCUREMENT room, standing in front of a customer service COUNTER,  where
 * you speak to the employees on the other side of the COUNTER.
 * Inside the WAREHOUSE they move items on the CART between the COUNTER and
 * on the numerous SHELVES in the warehouse.
 * 
 * PROCUREMENT - The room the player stands in (known as vaults).
 * COUNTER     - Interface reception -> warehouse.
 * CSM         - Customer Service Manager (NPC), maybe we can have that ?
 * WAREHOUSE   - The building where things are stored.
 * CART        - Interface between warehouse and the SHELVES.
 * SHELF       - Where the objects are stacked and stored.
 *
 * All files in the warehouse should (REALLY) be in a seperate directory
 * for this purpose only (ONLY). It might work if you dont do it though.
 *
 * You should only have -1- warehouse per usergroup, do as you please.
 *
 * Make yourself a warehouse.c file and inherit the warehouse file. 
 *     set_group("<group>");    That owns the warehouse (domains unrestricted)
 *     set_max_items(450);      Pick a number
 *
 * Then create reception rooms by inherit reception.c and build the rooms as
 * any other rooms. All setup needed is in the warehouse. Notice the warehouse
 * should never be walked into or exits attached. It is imaginary.
 *
 * We have a mastervault.c that is a bit abstract, see documentation in that
 * one to figure out what it is. After studying reception a bit further.
 *
 * The warehouse is designed to have unlimited amount of items stored for the
 * players accessible through multiple connection points.  It is a good idea
 * to stress test if you wanna go haywire with size and amount.
 *
 * @author Silbago
 */

inherit "/std/object";
#include <warehouse.h>
#include <council.h>
#include <move_failures.h>

private nosave mapping log_dump;
private nosave string group_name,
                      group_short,
                      group_object;
private nosave int max_items,
                   warehouse_general_access;

#include "/std/rooms/warehouse/cart.i"

/**
 * Use this in setup of the warehouse for the group to determine the
 * maximum amount of items the group is allowed to deposit.
 * Defaults is low !
 */
protected void set_max_items(int i) {
    max_items = i;
}

string query_group       () { return group_name;   }
string query_group_name  () { return group_name;   }
string query_group_short () { return group_short;  }
string query_group_object() { return group_object; }
int    query_max_items   () { return max_items;    }

/**
 * A bit more descriptive than just query_short()
 */
string query_group_description() {
    if ( !group_name )
        return "Unknown owner group";

    switch(group_type(group_name)) {
    case "sig":
        return "Special Interest Group " + group_short;
    case "race_group":
        return "Race Group " + group_short;
    case "domain":
        return "Domain of " + group_short;
    case "guild":
        return "Guild " + group_short;
    case "race_leader":
        return "City of " + group_short;
    case "area_leader":
        return "Faction of " + group_short;
    case "tower":
        return "Tower of " + group_short;
    }

    return group_short;
}

/**
 * Wether the user has access to do this or not.
 * All command access is restricted the same way.
 * You can set general access restriction in the
 * warehouse, and override per-room in each room.
 * @return 0 for NOPE and 1 for YES
 */
int query_access(object me) {
    if ( undefinedp(warehouse_general_access) )
        return 1;

    if ( warehouse_general_access == -1 )
        return 1;

    return query_access_level(me, group_name, warehouse_general_access);
}

/**
 * Setting specific access level for the warehouse.
 */
void set_access_level(int i) {
    warehouse_general_access = i;
}

/**
 * This is a setting that makes no restrictions for the warehouse.
 */
void unrestricted_warehouse() {
    warehouse_general_access = -1;
}

/**
 * Attempts to deposit given objects with vault_labels set from the vaultroom.
 * @return USE_OK success, else oh dear.
 */
int add_item(object ob, object me, int dep_flag) {
    // Added by Timion, 06 NOV 97
    // To prevent deposits in vault during CTF
    // Taniwha 1999 blocked during reboots.
    //
    if ( VAULTS_CLOSED )
        return USE_CLOSED;

    if ( query_stock_size() >= max_items )
        return USE_FULL;

    // Wiii, dests the shift_object(gp drain) Titan
    //
    if ( ob->query_auto_load() )
        return USE_SHIFTED;

    if ( ob->query_property("cursed") )
        return USE_CURSED;

    if ( ENV(ob) != me && ENV(ob) != ENV(me) )
        return USE_NOT_HERE;

    if ( living(ob) )
        return USE_NO_LIVING;

    // Need to find appropriate shelf for each item we attempt to deposit.
    //
    shelf = load_free_shelf();
    return shelf->add_item(ob, previous_object(), TO, me, dep_flag);
}

int remove_item(object item, object shelf, object me, int sticky) {
    if ( VAULTS_CLOSED )
        return USE_CLOSED;

    if ( (me->query_contents_weight() + item->query_weight()) > me->query_max_weight() )
        return USE_TOO_HEAVY;

    return shelf->remove_item(item, previous_object(), TO, me, sticky);
}

/**
 * Scans through all the shelves and finds objects according to requirements.
 * The array returned looks like described in counter.i
 */
mixed *search_inventory(string arg, string *vault_labels, mapping flags) {
    mixed *ret = ({ });

    load_shelves();
#   ifdef DEVELOPMENT
    if ( TP && geteuid(TP) == "silbago" )
        printf("Scanning shelves %O\n", shelves);
#   endif
    foreach(savefile, shelf in shelves)
        foreach(object ob in shelf->filter_objects(arg, vault_labels, flags)) {
            mixed tags = ob->query_property(WAREHOUSE_TAGS);
#           ifdef DEVELOPMENT
            if ( TP && geteuid(TP) == "silbago" )
                printf("Search Match on %O for %s, result %O\n", shelf, arg, ob);
#           endif
            ret += ({ ({ ob, shelf, tags[WH_DEP_TAG_BY],
                         tags[WH_DEP_TAG_OWN_IT], tags[WH_DEP_TAG_LABELS] }), });
        }

    return ret;
}

protected void set_group(string grp) {
    if ( !group_exists(grp) ) {
        warning("set_group(\""+grp+"\"), usergroup does not exist.");
        return;
    }

    group_name = grp;
    group_short = group_property(grp, "short");
    group_object = group_property(grp, "object");
}
