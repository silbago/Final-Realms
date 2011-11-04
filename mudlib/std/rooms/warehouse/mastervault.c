/**
 * @main
 * This is the master vault, it is a bit larger.
 * You do not want everyone to use this, only those who knows how.
 *
 * @author Silbago
 */

string print_labels(string *labels);
#define MASTER_LIST
#include "room.c"

private nosave object *vaultrooms;
private nosave string long_extension, main_long, *active_labels;
private nosave mapping vault_info;

int query_master_vault_room() { return 1; }

/**
 * Finds the objects to all vaultrooms.
 */
private void load_vaultrooms() {
    object ob;
    mixed *bing;

    if ( !vaultrooms )
        vaultrooms = ({ });
    else
        vaultrooms -= ({ 0 });

    if ( !vault_info )
        vault_info = ([ ]);
    else
        vault_info = m_delete(vault_info, 0);

    active_labels = ({ });

    bing = get_dir(my_dir + "*.c", -1);
    foreach(mixed file in bing) {
        if ( file[1] < 20 ) // Trying to not catch very small objects that are bugs to begin with
            continue;

        file = file[0];
        catch(load_object(my_dir+file));
        if ( (ob=find_object(my_dir+file)) )
            if ( member_array(ob, vaultrooms) == -1 )
                if ( ob->query_vault_room() && !ob->query_master_vault_room() ) {
                    if (    strsrch(file, "base") != -1
                         || ob->query_master_vault_room()
                         || !ob->query_short() )
                        continue;
                    vaultrooms += ({ ob });
                    active_labels |= ob->query_vault_labels();
                }
    }

    long_extension = "List of vaultrooms:\n";
    foreach(ob in vaultrooms) {
        if ( !vault_info[ob] )
            vault_info[ob] = ({ ob->query_vault_labels(), ob->query_deposit_filter() });

        long_extension += sprintf("    %-50s  Labels: %s\n",
            strip_colours(ob->query_short()),
            nice_list(copy(vault_info[ob][0])));
    }

    vault_info = m_delete(vault_info, 0);
    vaultrooms -= ({ 0 });
    set_long(main_long + long_extension);
}

void create_masterlist() {
    if ( base_name(TO) == "/std/rooms/warehouse/mastervault" )
        return;

    my_dir = file_dir(TO);
    load_vaultrooms();
}

void set_main_long(string str) {
    main_long = str;
}

/**
 * To show labels, but most to find the ghost items.
 */
string print_labels(string *labels) {
    foreach(string label in labels)
        if ( member_array(label, active_labels) != -1 )
            return nice_list(labels);

    return "%^B_RED%^GHOST%^RESET%^" + nice_list(labels);
}
