/**
 * @main
 * Coordinate database, stores hard-set coordinates mudwide.
 *
 * Coordinates are set in room setup, by a mud administrator.
 * These are sent to this database for lookups, external queries
 * are then used to assess coordinates.
 *
 * See the included codebase used by room.c
 *
 * Data for each file in lookup is as follows ({
 *     (int)   time of input,
 *     (int *) ({ x, y, z, });
 * })
 *
 * When throwing coordinates back, and stored in roomfile and object
 * in environment we have fourth data that is STRENGTH.
 * Would look like ({ -250, 33, 4, 1, 0, 0 });
 *
 * Coordinates has strengths depending how much we trust them.
 *   1)  Coordinate set in setup, this is hard locked and per defintion 100%
 *   2)  Coordinate derived from this by living walking into this room
 *   3)  Same coordinate only ASKED BY a file in same directory.
 *       Per definition a close match.
 *   4)  Logged on to area of no reference, or teleported.
 *       This coordinate is per definition useless, but still it exists.
 *
 * When you walk into a room you propagate your coordinate into the new room.
 * If the new room has a higher strength you do not overwrite.
 *
 * Ok here's the stranger behaviour
 *     If you walk from a strength 2 coordinate you have 2
 *     If you walk from 4 you still have 2.
 *     If you walk into a higher quality you do not override, simply.
 *
 * The center of the mud is here:
 *     /d/ss/wilderness/cross1.c
 *
 * @author Silbago
 */

#include <room.h>
#include <handlers.h>
#include <coordinates.h>

#define SAVEFILE "/save/Handler/coordinates"

private nosave mapping dir_refs;
private mapping lookups;

int clean_up() { return 0; }
int dest_me () { return 0; }
int dwep    () { return 0; }

mapping query_dir_refs() { return copy(dir_refs);       }
mapping query_lookups () { return copy(lookups);        }
private void load_me  () { restore_object(SAVEFILE, 1); }
private void save_me  () { save_object(SAVEFILE, 1);    }

void create() {
    SETEUID;

    lookups  = ([ ]);
    dir_refs = ([ ]);
    load_me();
    call_out("rebuild_refs", 0);
}

void rebuild_refs() {
    dir_refs = ([ ]);
    foreach(string file, mixed data in copy(lookups)) {
        string dir;
        int i = file_size(file +".c");

        if ( i < 1 ) {
            lookups = m_delete(lookups, file);
            continue;
        }

        dir = "/" + implode(explode(file, "/")[0..<2], "/") + "/";
        dir_refs[dir] = file;
    }

    save_me();
}

void purge_database() {
    dir_refs = ([ ]);
    lookups  = ([ ]);
    save_me();
}

void purge_room(string roomfile) {
    lookups = m_delete(lookups, roomfile);
    save_me();
    call_out("rebuild_refs", 0);
}

/**
 * Finds references to this environment.
 * If you want to get coordinates of a directory, then say so.
 * That will downgrade the STRENGTH from 1 to 3 regardless.
 */
int *query_coordinates(string roomfile, int is_directory) {
    // If we have a directory reference for 'roomfile' that is a dir
    // we return that coordinate. Ensure that Strength is not stronger
    // than level 3. As that is not possible
    //
    if ( is_directory ) {
        if ( dir_refs[roomfile] ) {
            int *ret = copy(lookups[dir_refs[roomfile]][1]);
            ret[COORD_ARRAY_LEVEL] = COORD_QUAL_AREA_LOCK;
            return ret;
        }

        return DEFAULT_COORDINATE;
    }

    if ( roomfile[<2..] == ".c" )
        roomfile = roomfile[0..<3];
    if ( lookups[roomfile] )
        return lookups[roomfile][1];

    roomfile = "/" + implode(explode(roomfile, "/")[0..<2], "/") + "/";
    return query_coordinates(roomfile, 1);
}

/**
 * Sent by set_coordinate() in setup of the room that has this.
 * We set it in the lookup and then we also set a reference for the directory.
 */
void update_coordinate_reference(object room, int *coordinates) {
    string dir,
           file = base_name(room);
  
    if ( lookups[file] ) {
        if (    lookups[file][1][COORD_ARRAY_X] == coordinates[COORD_ARRAY_X]
             && lookups[file][1][COORD_ARRAY_Y] == coordinates[COORD_ARRAY_Y]
             && lookups[file][1][COORD_ARRAY_Z] == coordinates[COORD_ARRAY_Z] )
        {
            return;
        }

        warning("Coordinate reference changed in setup of room.", 2);
    }

    coordinates[COORD_ARRAY_LEVEL] = 1;
    lookups[file] = ({ last_touched(file + ".c"), coordinates });

    dir = "/" + implode(explode(file, "/")[0..<2], "/") + "/";
    dir_refs[dir] = file;

    save_me();
}

/**
 * Finding changes and dealing with it dynamically.
 * UHM: Not implemented yet.
 * Remember to upgrade help library with rename and copy bugs ...
 */
void event_change_file(string file, string uid, string funct, string renamed) {
    switch(funct) {
    case "cp":
    case "rename":
//      reload_file(renamed);
    case "rm":
    case "write_file":
    case "write_buffer":
//      reload_file(file);
        return;
    }

    if ( funct != "rename" )
        return;

// Recurse tree's
}
