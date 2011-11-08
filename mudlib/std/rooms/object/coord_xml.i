/*
    pbmplus application to convert pbm files
    http://en.wikipedia.org/wiki/Portable_anymap

    http://www.imagemagick.org/script/binary-releases.php#windows
    http://www.gimp.org/windows/
    http://www.thesitewizard.com/php/create-image.shtml
*/

/**
 * @main
 * ---------------------------------------------------------------------------
 *             Automatic map creation system: Suggestion for future
 * ---------------------------------------------------------------------------
 * Flash/java or something map graphics write engine based on generated
 * data on the mud. What do I know, it should be possible
 *
 * Here is a xml suggestion for dump of coordinates.
 * Writing a dot somewhere on the picture based on the coordinate.
 * The size and style of the dot (can) be based on the size of the room.
 * Each time coordinate is set on the room it will write to the logfile
 * so we should find the latest dump.
 * There are tens of thousands of rooms. It might be an idea to sort
 * it by domains ? Zones can also be used for this purpose.
 * A mud-wide map, domain map and zone maps could be preferred.
 *
 * XML input for each room format, we need to have it in seperate
 * files for the creation to make it sane on the mud end.
 *
 *
 * <key>/d/um/areas/fd/tavern</key>
 * <dict>
 *     <key>File</key><string>/d/um/areas/fd/tavern</string>
 *     <key>Domain</key><string>Unicorne Mountains</string>
 *     <key>Zone</key><string>Facedeux</string>
 *     <key>Room</key><string>East Gate</string>
 *     <key>Time</key><integer>1266197990</integer>
 *     <key>Coordinate X</key><integer>4490</integer>
 *     <key>Coordinate Y</key><integer>-3510</integer>
 *     <key>Coordinate Z</key><integer>50</integer>
 *     <key>Size X</key><integer>30</integer>
 *     <key>Size Y</key><integer>30</integer>
 *     <key>Size Z</key><integer>30</integer>
 *     <key>east</key><string>/d/um/areas/fd/city13</string>
 *  </dict>
 *
 *
 * Some websites that deals with maps
 *
 *      http://www.gpsvisualizer.com/
 *      http://www.manifold.net/doc/coordinates_in_projected_maps.htm
 *      http://www.elated.com/articles/creating-image-maps/
 */

#define IGNORE_DOMAINS ({ \
    "heaven",             \
    "fp",                 \
    "dl",                 \
    "pharonae",           \
    "radish",             \
})

#define INCLUDE_OUTSIDE_DOMAIN ({ \
    "/room/sacrifice",     \
})

/**
 * At room loading we write the xml file for the room setting.
 * Written to /log/globamap/<ROOM.FILE.path>.xml
 */
void dump_coordinate_xml_data() {
    mixed doh;
    string xml, shrt, str, xml_file, dom, Zone;

// We can't be doing this blindly.
// First dont overwrite if existing .xml has higher quality coordinate
// Secondly if same quality overwrite
//
// To do this we need a database that caches what coord settings we
// have for all rooms. Extremely heavy and should only be used
// for periods where we recalibrate the mud.
//
// Then let it work against the cache database for 3 months,
// Hopefully 3 months is long enough to write everything
// Check for filesize of dumped .xml file to override db test ?
//
return;

    if ( coordinates[3] > 2 )
        return;

    xml_file = base_name(TO);
    str = xml_file;
    if ( xml_file[0..2] != "/d/" ) {
        int gottit = 0;
        foreach(str in INCLUDE_OUTSIDE_DOMAIN) {
            int i = strlen(str) - 1;
            if ( str[0..i] == str )
                gottit = 1;
        }

        if ( !gottit )
            return;

        dom = "NONE";
        xml_file = implode(explode(xml_file, "/"), "/");
        xml_file = replace_string(xml_file, "/", ".");
        xml_file = "/other/" + xml_file;
    } else {
        dom = explode(xml_file, "/")[1];
        if ( member_array(dom, IGNORE_DOMAINS) != -1 )
            return;

        xml_file = implode(explode(xml_file, "/")[2..], "/");
        xml_file = replace_string(xml_file, "/", ".");
        xml_file = dom + "/" + xml_file;
    }

    doh = TO->query_zone();
    if ( stringp(doh) )
        Zone = doh;
    else if ( arrayp(doh) )
        Zone = implode(doh, ",");
    else
        Zone = "NONE";

    shrt = TO->query_short();
    if ( !shrt )
        shrt = "SHORTLESS";
    shrt = strip_colours(shrt);
    if ( !shrt )
        shrt = "SHORTLESS";
    shrt = replace_string(shrt, "\n", "");
    if ( !shrt )
        shrt = "SHORTLESS";

    xml =
      "\t<key>" + str + "</key>\n"
      "\t<dict>\n"
      "\t\t<key>File</key><string>"   + str + "</string>\n"
      "\t\t<key>Domain</key><string>" + dom + "</string>\n"
      "\t\t<key>Zone</key><string>"   + Zone + "</string>\n"
      "\t\t<key>Room</key><string>"   + shrt + "</string>\n"
      "\t\t<key>Time</key><integer>"  + time() + "</integer>\n"
      "\t\t<key>Coordinate X</key><integer>" + coordinates[0] + "</integer>\n"
      "\t\t<key>Coordinate Y</key><integer>" + coordinates[1] + "</integer>\n"
      "\t\t<key>Coordinate Z</key><integer>" + coordinates[2] + "</integer>\n"
      "\t\t<key>Size X</key><integer>" + room_size[0] + "</integer>\n"
      "\t\t<key>Size Y</key><integer>" + room_size[1] + "</integer>\n"
      "\t\t<key>Size Z</key><integer>" + room_size[2] + "</integer>\n";

    doh = TO->query_dest_other();
    for ( int i=0; i < sizeof(doh); i += 2 ) {
        if ( !SHORTEN[doh[i]] )
            doh[i] = geographical_exit_direction(doh[i]);

        if ( SHORTEN[doh[i]] ) {
            string toroom = doh[i+1][0];
            if ( toroom[<2..] == ".c" )
                toroom = toroom[0..<3];
            xml += "\t\t<key>" + doh[i] + "</key><string>" + toroom + "</string>\n";
        }
    }

    xml +=
      "\t</dict>\n";

    xml_file = "/open/atlas/" + xml_file + ".xml";
    write_file(xml_file, xml, 1);
}
