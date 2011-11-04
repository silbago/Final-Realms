/**
 * @main
 * This is the standard vaultroom for Final Realms.
 *
 * 1) Create a subdirectory named warehouse/, -ALL- files goes in there.
 * 2) Create subdirectory logs/ in the warehouse/ directory
 * 3) Create a file named warehouse.c that inherits warehouse.c in this dir.
 *    Follow instructions in header of /std/rooms/warehouse/warehouse.c
 *
 * Then setup your vaultroom(s).
 *   1) inherit this file.
 *   2) You need to setup the vault_labels, for starters there are no vault_labels.
 *      Use whatever label that suits you, see info in add_label() for more info.
 *   3) The item type filters are defaulted, but you can alter them.
 *      See setup_item_filter() for more info.
 *   4) Setup access restrictions if you want others than set in warehouse.
 *
 * Create a file named warehouse.c
 *   1) inherit warehouse.c
 *   2) set_group(GROUP OWNER)
 *   3) If you need, set max items and access restrictions
 *
 * Authors: Original vaultcode was written by many contributors.
 *          This is just a new twist, new shape and colors.
 *          Vault code originally created by Radix.
 *          Radix, Driadan, Flode, Titan, Dyno and more.
 *
 * @author Silbago
 */

inherit "/std/room";

#include <warehouse.h>
#include <cmds.h>
#include <cmd.h>
#include "/std/rooms/warehouse/counter.i"
#include "/std/rooms/warehouse/access.i"
#include "/std/rooms/warehouse/help.i"

int query_vault_room() { return 1; }

/**
 * Depositing item into the vault, own_it parameter makes it add a
 * tag to the lists so that one knows given person wants to have it
 * as private. The str argument is mixed, as external objects can
 * send a deposit query by object argument directly
 * @author Silbago
 */
varargs int do_deposit(object me, mixed *argv, int match, mapping flags) {
    string str;
    int own_it;

#   ifdef MASTER_LIST
    me->msg("Sorry, depositing in Master Vaults are prohibited.\n");
    return 1;
#   endif

    TEST_ACCESS("You're denied access to depositing.\n")

    if ( !find_warehouse() ) {
        notify_fail("The vault is broken, the warehouse is burning. Contact an admin.\n");
        return 0;
    }

    flags = wipe_flags(flags);
    if ( flags["p"] )
        own_it = 1;
    else if ( flags["P"] )
        own_it = 2;

    str = "";
    foreach(object ob in ARG) {
        string ouch;
        int success;

        if ( flags["a"] )
            ob->add_static_property("anonymous deposit", 1);

        if ( ob->query_in_use() ) {
            str += "You can not deposit " + ob->query_short() +", it is in use.\n";
            continue;
        }

        if ( !test_deposit(ob, TP) ) {
            str += "You can not deposit " + ob->query_short() + "%^RESET%^ here.\n";
            continue;
        }

        success = warehouse->add_item(ob, me, own_it);
        ouch = use_fail_reason("deposit", ob->query_short(), TO->query_short(), success);
        if ( ouch ) {
            str += ouch + "\n";
            continue;
        }

        if ( success >= 1 && success < sizeof(FAIL_REASON) ) {
            str += FAIL_REASON[success] + ".\n";
            continue;
        }

        str += "The warehouse is burning, the clerk refuse to do this.\n";
    }

    me->more_string(str);
    return 1;
}

varargs int do_retrieve(object me, mixed *argv, int match, mapping flags) {
    int i;

    if ( !find_warehouse() ) {
        write("The warehouse is burning, you can not see anything.\n");
        return 1;
    }

    TEST_ACCESS("You're denied access to retrieving.\n")
    flags = wipe_flags(flags);

    // For external call, mostly from mastervault
    //
    if ( objectp(ARG) )
        list_cache = ({ ({ ARG, ARG2, 0, 0 }) });
    else if ( sscanf(ARG, "#%d", i) ) {
        if ( !sizeof(list_cache) )
            search_inventory(0, flags);
        if ( !sizeof(list_cache) ) {
            me->msg("No items has been listed yet, you do not know what item number " + i + " is.\n");
            return 1;
        }
        if ( i < 1 || i > sizeof(list_cache) ) {
            me->msg("Can't find an enlisted item with that number.\n");
            return 1;
        }

        list_cache = ({ list_cache[--i] });
    } else {
        if ( !search_inventory(ARG, flags) ) {
            me->msg("The warehouse is burning, you can not see anything.\n");
            return 1;
        }

        if ( !sizeof(list_cache) ) {
            me->msg("This vault is currently empty.\n");
            return 1;
        }
    }

    ARG = "";
    foreach(mixed thing in list_cache) {
        string ouch;
        int access;

        access = warehouse->remove_item(thing[0], thing[1], me, flags["s"] ? 1 : 0);
        ouch = use_fail_reason("retrieve", thing[0]->query_short(), TO->query_short(), access);
        if ( ouch ) {
            ARG += ouch + "\n";
            continue;
        }

        if ( access >= 1 && access < sizeof(FAIL_REASON) ) {
            ARG += FAIL_REASON[access] + ".\n";
            continue;
        }

        ARG += "The warehouse is burning, the clerk refuse to do this.\n";
    }

    search_inventory(0);
    me->more_string(ARG);
    return 1;
}

int do_browse(object me, mixed *argv, int match, mapping flags) {
    int i;
    mixed tags;
    object ob;
    string ret;

    if ( !ARG ) {
        notify_fail("Browse what item ?\n");
        return 0;
    }

    TEST_ACCESS("You're denied access to browsing items.\n")

    if ( sscanf(ARG, "#%d", i) ) {
        if ( !sizeof(list_cache) ) {
            write("No items has been listed yet, you do not know what item number " + i + " is.\n");
            return 1;
        }
        if ( i < 1 || i > sizeof(list_cache) ) {
            write("Can't find an enlisted item with that number.\n");
            return 1;
        }

        list_cache = ({ list_cache[--i] });
    } else {
        if ( !search_inventory(ARG) ) {
            write("The warehouse is burning, you can not see anything.\n");
            return 1;
        }

        if ( !sizeof(list_cache) ) {
            write("This vault is currently empty.\n");
            return 1;
        }
    }

    ret = "";
    if ( sizeof(list_cache) > 1 )
        ret += "Trying to browse more than one item, choosing first.\n\n";

    ob = list_cache[0][0];
    tags = ob->query_property(WAREHOUSE_TAGS);
    ret += "Deposited by " + CAP(tags[WH_DEP_TAG_BY]) + ".\n";
    if ( tags[WH_DEP_TAG_OWN_IT] )
        ret += "    %^RED%^Warning%^RESET%^: Personal item. Do NOT withdraw.\n";
    ret += "\n";

    ret += VIEW_CMD->item_details(ob, me);
    me->more_string(ret);
    return 1;
}

int do_logs(object me, mixed *argv, int match, mapping flags) {
    if ( !find_warehouse() ) {
        notify_fail("The vault is broken, the warehouse is burning. Contact an admin.\n");
        return 0;
    }

    TEST_ACCESS("You're denied access to reading logs.\n")

    TP->more_string(warehouse->read_log(TP, VERB == "latest"));
    return 1;
}

int do_list(object me, mixed *argv, int match, mapping flags) {
    string str;
    string ret;
    int i, ii, go_mxp;

    TEST_ACCESS("You're denied access to listing inventory.\n")

    if ( match )
        str = ARG;

    flags = wipe_flags(flags);
    if ( !search_inventory(str, flags) ) {
        write("The warehouse is burning, you can not see anything.\n");
        return 1;
    }

    if ( !sizeof(list_cache) ) {
        write("This vault is currently empty.\n");
        return 1;
    }

    i = warehouse->query_stock_size();
    ii = warehouse->query_max_items();

    ret = sprintf("[%d/%d/%d/%d]",
        sizeof(list_cache),
        ii - i, i, ii);
    ret = "%^CYAN%^Num  Item Name " + sprintf("%-23s", ret) +
          "     Condition       Contributor%^RESET%^\n";

    i = 0;
    go_mxp = TP && TP->query_mx_setting("vaults") ? 1 : 0;
    foreach(mixed thing in list_cache) {
        string deposited, shortish, cond, ownertag;

#       ifndef MASTER_LIST
        TP->add_view_object(thing[0]);
#       endif

        // colors in sprintf is an abomination, therefore this ugliness
        //
        i++;
        {
            int too_short;
            too_short = 35 - strlen(strip_colours(thing[0]->query_short()));
            shortish  = thing[0]->query_short() + repeat_string(" ", too_short);
        }

        cond = thing[0]->cond_word();
        if ( !cond )
            cond = "wreck";

        if ( thing[2] == me->query_name() )
            deposited = sprintf("%%^YELLOW%%^%-11s%%^RESET%%^", "You");
        else
            deposited = sprintf("%%^ORANGE%%^%-11s%%^RESET%%^", CAP(thing[2]));

        if ( thing[3] ) {
            ownertag = "*%^RESET%^";
            if ( thing[3] & 2 )
                ownertag = "%^RED%^"+ ownertag;
            else
                ownertag = "%^GREEN%^"+ ownertag;
        } else
            ownertag = " ";

        ret += sprintf("%%^GREEN%%^%s%%^RESET%%^  %-35s   %-13s  %s%s",
            go_mxp ? mxp_vault(sprintf("%3d", i)) : sprintf("%3d", i),
            shortish, CAP(cond), ownertag, deposited);
#       ifdef MASTER_LIST
        ret += print_labels(thing[4]);
#       endif
        ret += "\n";
    }

    TP->more_string(ret);
    return 1;
}

int do_evaluate(string str) {
    if ( !search_inventory(str) ) {
        write("The warehouse is burning, you can not see anything.\n");
        return 1;
    }

    if ( !sizeof(list_cache) ) {
        write("This vault is currently empty.\n");
        return 1;
    }

    TEST_ACCESS("You're denied access to listing inventory.\n")

    TP->more_string("This is not created yet, notification will be given when released.\n"
                    "It will be a list of items with same method as evaluate in smith.\n");
    return 1;
}
