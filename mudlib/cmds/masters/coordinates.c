/**
 * @main
 * Command to calibrate coordinates.
 * @author Silbago
 */

#include <cmd.h>
#include <handlers.h>
inherit CMD_BASE;

string *query_syntax() {
    return ({ "" });
}

string query_short_help() {
    return
        "%^BOLD%^Syntax%^RESET%^\n"
        "    coordinates\n"
        "\n"

        "%^BOLD%^Description%^RESET%^\n"
        "    Shows coordinate settings in the domain(s) you have access to.\n"
        "    Governed by the DemiGods, do not change them!\n"
        "\n"

        "%^BOLD%^See also%^RESET%^\n"
        "    Help multi lacing (incomplete document to implementation to Final Realms).\n"
        "    http://en.wikipedia.org/wiki/Cartesian_coordinate_system\n"
        "    Full documentation and cohersion will be made later. Silbago Mar 2010.\n"
        "\n";
}

protected int pcmd(mixed *argv, object me, int match, mapping flags) {
    int *i;
    string ret, file, *bing;
    mapping coords, dir_refs;

    object *obs = filter(users(), (: user_exists(geteuid($1)) :));

    ret = "";

    if ( group_member(me, "demis") ) {
        ret = "%^CYAN%^" + sprintf("%-11s  %|18s  %4s  %5s  %-9s  %s",
              "User", "Coordinate", "Dist", "Angle", "Direction", "Room") + "%^RESET%^\n";

        obs = sort_array(obs, (: query_coordinate_distance($(me), $1) - query_coordinate_distance($(me), $2) :));
        foreach(object ob in obs) {
            string roomshort = ENV(ob)->query_short();
            i = ENV(ob)->query_coordinates();
            if ( sizeof(i) != 4 )
                i = ({ 0, 0, 0, 4 });

            roomshort = replace_string(roomshort, "\n", "");

            ret += sprintf("%-11s  %-18s  %4d  %5d  %-9s  %s\n",
                       ob->query_cap_name(),
                       sprintf("%5d %5d %4d %d", i[0], i[1], i[2], i[3]),
                       query_coordinate_distance(me, ob),
                       query_coordinate_angle(me, ob),
                       query_direction_string(me, ob),
                       roomshort + "%^RESET%^");
        }
    }

    ret += "\n%^CYAN%^" + sprintf("%-10s  %|18s  %s\n",
           "Input time", "Coordinates", "Roomfile (yellow is directory setting)") + "%^RESET%^";
    coords = COORDINATES_HANDLER->query_lookups();
    dir_refs = COORDINATES_HANDLER->query_dir_refs();
    dir_refs = implode(values(map(dir_refs, (:([$2:$1]):))), (:$1+$2:));

    bing = sort_array(keys(coords), 1);
    for ( int a=0; a < sizeof(bing); a++ ) {
        string when, *bang;
        mixed data;

        file = bing[a];
        if ( !master()->valid_read(file, me, "read_file") )
            continue;

        data = coords[bing[a]];
        i = data[1];
        if ( dir_refs[file] ) {
            string thestart = dir_refs[file];
            string theend = file[strlen(thestart)..];

            file = "%^YELLOW%^" + thestart + "%^RESET%^" + theend;
        }

        bang = explode(ctime(data[0], 15), "/");
        when = sprintf("%4s/%s/%2s", bang[2], bang[0], bang[1]);

        ret += sprintf("%-10s  %-18s  %s\n",
                   when,
                   sprintf("%5d %5d %5d", i[0], i[1], i[2]),
                   file);
    }

    me->more_string(ret);
    return 1;
}
