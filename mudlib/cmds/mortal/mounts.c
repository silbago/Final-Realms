/**
 * @main
 * This is the command interface for players to rule their mounts.
 * Call the mount, dismiss the mount and list the mounts.
 * @author Silbago
 */

#include <cmd.h>
#include <mounts.h>
inherit CMD_BASE;

string query_short_help() {
    return
      "%^BOLD%^Syntax%^RESET%^\n"
      "    mounts\n"
      "    mounts call <number>\n"
      "    mounts kill <number>\n"
      "    mounts info <number>\n"
      "\n"

      "%^BOLD%^Description%^RESET%^\n"
      "    Mounts comes in many shapes and breeds, some are commonly known as Horses, as\n"
      "    such you probably can  acquire yourself a Mule,  a Warg or  even an Oliphant.\n"
      "\n"

      "    This command rules your mounts, you can list what mounts you control and call\n"
      "    upon them to join your side.  You can also kill them to get rid of them.  Yes\n"
      "    they do actually die - be aware.\n"
      "\n"

      "    Some mighty steeds only accept A Paladin,  some only accepts A Demon and some\n"
      "    ponies are simply way too small for a troll to mount ! A dwarf riding Camel ?\n"
      "\n"

      "%^BOLD%^Characteristics%^RESET%^\n"
      "    Train the riding skill to master your Mount, it will help you command faster.\n"
      "    An old steed walks slow,  while the youngest might be harder to control.  For\n"
      "    the expert horsemen, read Paladin and Anti-Paladin, great provess is rewarded\n"
      "    in combat.  Beasts of Burden ? Yes some, but not all,  watch the encumbrance!\n"
      "\n";
}

string *query_syntax() {
    return ({
        "",              // 0
        "call <number>", // 1
        "kill <number>", // 2
        "info <number>", // 3
    });
}

protected int pcmd(mixed *argv, object me, int match, mapping flags) {
    int i;
    string ret;
    mixed mountlist;
    object mount;
    mapping my_mounts = me->query_my_mounts();

    if ( !m_sizeof(my_mounts) ) {
        me->msg("You have no mounts, find a stable !\n");
        return 1;
    }

    mountlist = ({ });
    foreach(string file, mixed data in my_mounts)
        mountlist += ({ ({ file }) + data });

    if ( match && (ARG < 1 || ARG > sizeof(mountlist)) ) {
        me->msg("Try mount nr from $# to $#.\n", 1, sizeof(mountlist));
        return 1;
    }

    if ( match )
        ARG--;
    mount = me->query_mount();
    if ( match == 1 ) {
        if ( mount ) {
            me->msg("You can not call more than one mount at a time, "
                    "dismiss $Q to call another one.\n", mount);
            return 1;
        }

        if ( !sum(all_inventory(me)->query_bridle()) ) {
            notify_fail("You can not call your mount without a bridle.\n");
            return 0;
        }

        mount = clone_object(mountlist[ARG][0]);
        mount->set_mount_age_seconds(mountlist[ARG][1]);
        mount->set_mount_owner(me);
        mount->move(ENV(me));
        // Equipment and whatnot later ?
        mount->e_msg("$Q snorts gently towards $Q.\n", 0, mount, me);
        return 1;
    }

    if ( match == 2 ) {
        me->remove_mount(mountlist[ARG][0]);
        me->msg("The beast is no longer kept under your control.\n");
        if ( mount && base_name(mount) == mountlist[ARG][0] )
            me->msg("Release it to finally set it free, dismount will re-list it.\n");
        return 1;
    }

    if ( match == 3 ) {
        object ob = load_object(mountlist[ARG][0]);
        ob->set_mount_age_seconds(mountlist[ARG][1]);
        ob->build_mount();

        me->msg(ob->get_help("mount", me));
        return 1;
    }

    i = 0;
    ret = sprintf("%%^CYAN%%^Mount  Age  %-50s  Name%%^RESET%%^\n", "Abilities");
    foreach(mixed mnt in mountlist) {
        string abilities;

        object ob = load_object(mnt[0]);
        ob->set_mount_age_seconds(mnt[1]);
        ob->build_mount();

        abilities = ob->query_speed_short();
        if ( ob->query_amphibian() )
            abilities += ", is amphibian";
        if ( ob->query_levitating() )
            abilities += ", is levitating";
        if ( ob->query_climbing() )
            abilities += ", is climbing";
        switch(ob->query_attack_steed()) {
        case 1:
            abilities += ", attack bonus";
            break;
        case 2:
            abilities += ", great attack bonus";
            break;
        }
        switch(ob->query_charge_mount()) {
        case 1:
            abilities += ", kicks during charge";
            break;
        case 2:
            abilities += ", fiercly kicks during charge";
            break;
        }


        ret += sprintf("  %2d)  %3d  %-50s  %s%%^RESET%%^\n",
                   ++i, MOUNT_YEAR(mnt[1]), abilities, ob->query_short());
    }

    me->more_string(ret);
    return 1;
}
