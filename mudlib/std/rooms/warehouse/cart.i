/**
 * @main
 * This is the interface that connects the Warehouse to the Shelves.
 * Functions and methods to communicate between with high stability.
 *
 * The namespace rules for shelf savefile filenames is
 *     shelf_XXX.o as in XXX is the number -> shelf_1.o or shelf_43.o
 *
 * @author Silbago
 */

private nosave mapping shelves;
private nosave object shelf;
private nosave string savefile, my_dir;
private nosave int savefiles_scanned;
#include "/std/rooms/warehouse/logs.i"

private void load_shelves();

void create() {
    SETEUID;
    log_dump          = ([ ]);
    max_items         = MAX_ITEMS;
    my_dir            = file_dir(TO); // "shelves/" is wished but NONO
    savefiles_scanned = 0;

    ::create();
    if ( !clonep(TO) )
        return;

    load_shelves();
    call_out("clean_logs", 600);
}

/**
 * Tells how many items deposited totally in the warehouse.
 */
int query_stock_size() {
    int ret;

    ret = 0;
    load_shelves();
    foreach(savefile, shelf in shelves)
        ret += shelf->query_stock_size();

    return ret;
}

/**
 * Returns all objects in all the shelves, as mapping by shelf object as key.
 */
mapping query_stockpile() {
    mapping ret;

    ret = ([ ]);
    load_shelves();
    foreach(savefile, shelf in shelves)
        ret[shelf] = shelf->query_stockpile();

    return ret;
}

/**
 * Find files with namespace "shelf_<NUMBER>.o.mud_name()"
 * Above namespace for domains, in user homedirs it is different. Just .o endings.
 * Returning the full filepaths without the save extension.
 *
 * Secondary we clone the shelves, can only run this once.
 */
private string *find_savefiles() {
    string *ret, home;

    if ( savefiles_scanned )
       return ({ });

    savefiles_scanned = 1;

    // Need to have things in order for standard files
    //
    if ( my_dir == "/std/rooms/warehouse/" )
        return ({ my_dir + "shelf_1" });

    if ( explode(my_dir, "/")[<1] == "housing" )
        home = "";
    else
        home = "." + mud_name();

    ret = ({ });
    foreach(mixed file in get_dir(my_dir + "shelf_*.o" + home, -1)) {
        int num;
        string str;

        // Not a file really, bypass
        //
        file = file[0];
        if ( file[1] < 1 )
            continue;

        // Here we check if the namespace is correct
        //
        if ( !strlen(home) ) {
            if ( !sscanf(file, "shelf_%d.o", num) )
                continue;
        } else if ( sscanf(file, "shelf_%d.o.%s", num, str) != 2 )

        // Doubble sanity checks
        //
        if ( strlen(home) && str != mud_name() )
            continue;

#ifdef DEVELOPMENT
        printf("Found num %d\n", num);
#endif
        ret += ({ my_dir + "shelf_" + num });
    }

#   ifdef DEVELOPMENT
    printf("Found savefiles for loading %O\n", ret);
#   endif
    return ret;
}

/**
 * This method finds all the shelf savefiles (namespace 'shelf_<#number>').
 * Then creates the shelves mapping and clones the shelf objects.
 * Redundancy and stability enhanced, refinds what is lost and so forth.
 */
private void load_shelves() {
    object shelf;
    string savefile;

    if ( !shelves )
        shelves = ([ ]);

    // Searching for savefiles, initiate them
    //
    foreach(savefile in find_savefiles())
        if ( !shelves[savefile] )
            shelves[savefile] = 0;

    // Lost savefiles, look for children.
    // This should not occur so often, so we scan children each time.
    //
    foreach(savefile, shelf in shelves) {
        object *obs, *obs2;

        if ( shelf )
            continue;

        obs = children(WAREHOUSE_SHELF);
        obs2 = filter(obs, (: $1->query_savefile() == $(savefile) :));

#       ifdef DEVELOPMENT
        if ( TP && geteuid(TP) == "silbago" )
            printf("Shelf %O   with object  %O\n", savefile, obs2);
#       endif

        if ( sizeof(obs2) ) {
            if ( sizeof(obs2) > 1 )
                warning("Memory Leak: Warehouse Shelf duplication (%d) for savefile %s.\n",
                        sizeof(obs2), savefile);
            shelves[savefile] = obs2[0];
            continue;
        }

        shelf = clone_object(WAREHOUSE_SHELF);
        shelf->load_shelf(savefile);
        shelves[savefile] = shelf;
    }

    // No shelves loaded, so we create our first one.
    //
    if ( !m_sizeof(shelves) ) {
        shelf    = clone_object(WAREHOUSE_SHELF);
        savefile = my_dir + "shelf_1";

        shelf->load_shelf(savefile);
        shelves[savefile] = shelf;
    }

#   ifdef DEVELOPMENT
    write("%^RED%^Scanning children%^RESET%^\n");
    foreach(shelf in children(WAREHOUSE_SHELF))
        printf("    Child %O savefile %O\n", shelf, shelf->query_savefile());
#   endif
}

/**
 * Finds a shelf with available space to deposit in.
 * If you need to deposit multiple items, you need to redo it for each item.
 *
 * If all the shelves are full and the total limit is not exceeded we create a new.
 * @return shelf object if available space, else 0 if the warehouse is full.
 * @author Silbago
 */
private object load_free_shelf() {
    int shelf_count;

    load_shelves();
    foreach(string savef, shelf in shelves) {
        int i = shelf->query_stock_size();
        if ( i >= MAX_PER_SHELF )
            continue;

        return shelf;
    }

    // If maximum amount of items are deposited for the group, we say no.
    //
    if ( query_stock_size() > max_items )
        return 0;

    shelf_count = m_sizeof(shelves) + 1;
    while(shelves[my_dir + "shelf_" + shelf_count])
        shelf_count++;

    savefile = my_dir + "shelf_" + shelf_count;
    shelf = clone_object(WAREHOUSE_SHELF);
// Need to export_uid(shelf);
    shelf->load_shelf(savefile);
    shelves[savefile] = shelf;

#   ifdef DEVELOPMENT
    printf("===---- CLONED NEW SHELF for storage %O with savefile %O\n", shelf, savefile);
#   endif
    return shelf;
}
