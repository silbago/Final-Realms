/**
 * @main
 * Interface between the vault room and the warehouse.
 * Holds a cache for the latest listings, to be able to retrive <#number> item
 * @author Silbago
 */

#include <warehouse.h>
#include <move_failures.h>

protected nosave mixed *list_cache;
private nosave string *vault_labels;
protected nosave string my_dir;
protected nosave object warehouse;
private nosave function deposit_filter = (:    $1->query_wand()
                                            || $1->query_scroll()
                                            || $1->query_tool()
                                            || $1->query_armour()
                                            || $1->query_weapon()
                                         :);

string *query_vault_labels() { return copy(vault_labels); }
function query_deposit_filter() { return copy(deposit_filter); }
protected object find_warehouse();

string query_group_description() {
    if ( !find_warehouse() )
        return "Oblivion";

    return warehouse->query_group_description();
}

void create() {
    mapping flags;
    string *syntax;
    string str;
    int i;

#   ifdef MASTER_LIST
    TO->create_masterlist();
#   endif
    list_cache   = ({ });
    vault_labels = ({ });
    my_dir       = file_dir(TO);
    warehouse    = 0;
    ::create();

    if ( !find_warehouse() )
        return;

    str = warehouse->query_group();
    if ( !stringp(str) || !group_exists(str) )
        return;

    set_group(str);
    i = warehouse->query_max_items();
    add_sign("It is gold rimmed and engraved.\n\n",
      "Inventory list for Vaultroom " + query_short() + "%^RESET%^\n"
#     ifdef MASTER_LIST
      "This is the %^GREEN%^Master Vault%^RESET%^ for the Warehouse owned by the " + query_group_description() + ".\n"
#     else
      "A part of the Warehouse owned by the " + query_group_description() + ".\n"
      "Items through this Vaultroom are labelled " + nice_list(vault_labels) + "\n"
#     endif
      "Total capacity in Warehouse is " + i + " items.\n"
      "\n"

      "See %^YELLOW%^help warehouse%^RESET%^ for indepth advanced usage.\n"
      "\n"

      "Commands available\n"
#     ifdef MASTER_LIST
      "    %^RED%^deposit <item>%^RESET%^  : You can not deposit in this room\n"
      "    list            : You can list ALL items in all your vaultrooms here.\n"
#     else
      "    deposit <items>\n"
      "    list\n"
#     endif
      "    retrieve <items>\n"
      "    browse <item>\n"
      "    logs\n"
      "    latest\n"
    );

    flags = ([
        ({ "p", "P" }) : 0,
        "t" : "<item_type>",
        "q" : "<quality_tier>",
        "c" : "<condition>",
        "u" : "<User>",
        "s" : 0,
    ]);
    syntax = ({ "", "<:pattern>" });
    add_cmd("list", syntax, flags);

    syntax = ({ "<items>" });
    flags = ([ ({ "a", "p", "P" }) : 0 ]);
    add_cmd("deposit", syntax, flags, "Deposit what ?\n");

    add_cmd("retrieve", ({"<:items>"}), (["s":0]), "Retrieve what ?\n");
    add_cmd("logs", "");
    add_cmd("browse", "<:item>");
    add_cmd("latest", "", 0, 0, "do_logs");
}

/**
 * When you deposit an item, the vault_labels to the room is added to the object.
 * That way you identify what vaultroom the given object was deposited from.
 * Vaults that list and retrieve must have same label in order to access the
 * items in the warehouse.
 *
 * Best practice for unique vaultrooms is to have some lame (though short)
 * label and not reuse it other vaultrooms that access the warehouse.
 * For example add_label("label1");
 *
 * Generally you can have vault_labels like "weapon" or "for silbago", and have
 * several vaults being able to access just that. Keep in mind that the
 * item filters filters objects by object type criteria, don't go overboard.
 *
 * You can for example have 1 vault, without vault_labels (for the patron only)
 * that has access to all objects in the warehouse. No vault_labels == full acccess.
 *
 * If you setup the narrative heavily on deposit filter, you can allow
 * this given vaultroom to RETRIEVE other items. Do not create a mess.
 */
void add_label(string label) {
    vault_labels += ({ label });
}

/**
 * This is for functional filtering at depositing, code you insert here
 * will be actual testing.
 * Arguments to the evaluation are in order:
 *      $1 = item, $2 = this_player(), $3 = this room
 *
 * Again, this filter will determine requirements of object fun calls
 * to deposit. Listing and retrieaval is not a part of this.
 */
void set_deposit_filter(function fun) {
    deposit_filter = fun;
}

/**
 * External function to dest legal deposit item, the requirements
 * For the evaluation of the function, we have these extra params
 *   $1   object ob : being deposited
 *   $2   object me : this_player() prefereably
 *   $3   object this_object(), this room
 * @return 1 if the deposit attempt of item is legal
 * @author Silbago
 */
int test_deposit(object ob, object me) {
    return evaluate(deposit_filter, ob, me, TO);
}

protected object find_warehouse() {
    if ( warehouse )
        return warehouse;

    catch(load_object(my_dir + "warehouse"));
    if ( !find_object(my_dir + "warehouse") )
        return 0;

    warehouse = find_object(my_dir + "warehouse");
    return warehouse;
}

/**
 * Call through this function to acquire the list of items.
 * The list_cache array will be built, if empty no matches found.
 * If this function returns 0 the warehouse did not load.
 *
 * array = ({ ({(object) item, (object shelf), (string) deposit_by, (int) deposit_tag}), ... })
 */
varargs protected int search_inventory(string arg, mapping flags) {
    list_cache = ({ });

    if ( !find_warehouse() )
        return 0;

    list_cache = warehouse->search_inventory(arg,
#            ifdef MASTER_LIST
                 0,
#            else
                 vault_labels,
#            endif
                 flags);
    list_cache = sort_array(list_cache, "sort_names");
    return 1;
}

/**
 * Driadan, Jul 2003 - Introducing sorted arrays
 * Silbago, Dec 2008 - extra sorting by condition
 */
int sort_names (mixed thing1, mixed thing2) {
    int cond1, cond2;
    string name1, name2;

    name1 = CAP(strip_colours(thing1[0]->query_short()));
    name2 = CAP(strip_colours(thing2[0]->query_short()));

    if ( name1 > name2 )
        return 1;

    if ( name1 < name2 )
        return -1;

    cond1 = thing1[0]->query_cond();
    cond2 = thing2[0]->query_cond();
    if ( cond1 > cond2 )
        return 1;

    if ( cond1 < cond2 )
        return -1;

    return 0;
}

/**
 * Plain english message as of why deposit or retrieve was denied.
 */
private string use_fail_reason(string WHAT, string ITEM, string ROOM, int use_fail) {
    switch(use_fail) {
    case USE_OK:
        return "You successfully " + WHAT + " " + ITEM + "%^RESET%^.";
    case USE_FULL:
        return "The warehouse is full.";
    case USE_CLOSED:
        return "Sorry, you may not " + WHAT + " items during a Reboot or Flag Game.";
    case USE_CURSED:
        return "The " + ROOM + " refuses to accept your accursed " + ITEM + "%^RESET%^.";
    case USE_SHIFTED:
        return "You cannot "+ WHAT + " " + ITEM + "%^RESET%^ into the " + ROOM + "%^RESET%^.";
    case USE_TOO_HEAVY:
        return "You can not lift the " + ITEM + "%^RESET%^.";
    case USE_NOT_HERE:
        return ITEM + "%^RESET%^ is nowhere to be seen.";
    case USE_NO_LIVING:
        return ITEM + " violently refuses to be shelved.";
    case USE_SHELF_FULL:
        return "The shelf is full, no available shelves were found. Contact an immortal.";
    }

    return 0;
}

mapping wipe_flags(mapping flags) {
    if ( !flags )
        flags = ([ ]);
    else {
        if ( flags["u"] )
            flags["u"] = LC(flags["u"]);
        if ( flags["t"] )
            flags["t"] = LC(flags["t"]);
        if ( flags["q"] )
            flags["q"] = LC(flags["q"]);
        if ( flags["c"] ) {
            flags["c"] = LC(flags["c"]);
            flags["c"] = replace_string(flags["c"], "_", " ");
        }
    }

    return flags;
}
