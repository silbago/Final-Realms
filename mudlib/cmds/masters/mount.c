/**
 * @main
 *
 * Command interface to read the list of mounts.
 * Sorry for the bad name of this command, there is a limit
 * to how many mount commands we can make.
 *
 * @author Silbago
 */

#define MOUNT_NAME    0
#define MOUNT_QUEST   1
#define MOUNT_COST    2
#define MOUNT_SIZE    3
#define MOUNT_WEIGHT  4
#define MOUNT_FASTER  5
#define MOUNT_SPEED   6
#define MOUNT_SEA     7
#define MOUNT_CLIMB   8
#define MOUNT_DMG     9
#define MOUNT_CHARGE 10
#define MOUNT_GUILD  11
#define MOUNT_NOGLD  12
#define MOUNT_RACE   13
#define MOUNT_NORACE 14

#include <cmd.h>
#include <mounts.h>
#include <handlers.h>

inherit CMD_BASE;

mixed *mounts;

string *query_syntax() {
    return ({
        "",
        "knight",
    });
}

mapping query_flags() {
    return ([
        "r" : 0,
    ]);
}

string query_short_help() {
    return
      "%^BOLD%^Syntax%^RESET%^\n"
      "    mount [ -r ]\n"
      "    mount [ -r ] knight\n"
      "\n"

      "%^BOLD%^Description%^RESET%^\n"
      "    This command lists the mounts available in /obj/mounts/ directory.\n"
      "    All mounts are located there, they must not, but they should.\n"
      "    You can probably have mounts in other dirs, but will cause a lot of bugs.\n"
      "    All this is intentional to keep the mounts there.\n"
      "\n"
      "    Mounts are integrated in the mudlib to work coherent with the rider.\n"
      "    They are designed to be innate in combat, but not completely or always.\n"
      "\n"
      "    Paladins and Anti-Paladins are hereforth commented as Knights.\n"
      "\n"

      "%^BOLD%^Best Practice%^RESET%^\n"
      "    Well what to say, mounts are not to revolutionize the game.\n"
      "    So dont push it, dont make it too good.\n"
      "    They are however designed with Knights to be Vital.\n"
      "\n"
      "    1) Dont give damage bonus mounts to slicers and others with multiple hits.\n"
      "    2) Charge mounts to well, Knights only.\n"
      "    3) We can probably invent some kick-mounts for special guilds\n"
      "    4) Climbing mounts should be underground/indoor.\n"
      "    5) We can probably invent tree-climbing mounts.\n"
      "    6) Sea-faring mounts must not ruin drowning or ships.\n"
      "    7) Finally file control, to keep Builders and Thanes in check !\n"
      "\n";
          
}

protected int pcmd(mixed *argv, object me, int match, mapping flags) {
    object ob;
    string ret;
    mixed bits;
    int i;

    if ( flags["r"] ) {
        me->msg("Rebuilding Stable List.\n");
        mounts = 0;
    }

    if ( !mounts ) {
        bits = get_files(MOUNT_DIR + "*.c");
        catch(map(bits, (: (MOUNT_DIR + $1)->boing() :)));
        bits = filter(bits, (: find_object(MOUNT_DIR + $1) :));
        bits = map(bits, (: clone_object(MOUNT_DIR + $1) :));
        bits = filter(bits, (: $1->query_is_mount() :));

        mounts = ({ });
        foreach(ob in bits) {
            mixed addon;

            ob->build_mount();

            i = 0;
            if ( ob->query_fast_mount() )
                i = 1;
            else if ( ob->query_slow_mount() )
                i = -1;

            addon = allocate(15);
            addon[MOUNT_NAME]   = ob->query_short();
            addon[MOUNT_QUEST]  = QUEST_INDEX->query_artifact_rewards(base_name(ob) + ".c");
            addon[MOUNT_COST]   = ob->query_default_cost();
            addon[MOUNT_SIZE]   = ob->query_mount_size();
            addon[MOUNT_WEIGHT] = ob->query_max_weight();
            addon[MOUNT_FASTER] = i;
            addon[MOUNT_SPEED]  = ob->query_mount_speed();
            addon[MOUNT_SEA]    = ob->query_sea_race() ? 1 : 0;
            addon[MOUNT_CLIMB]  = ob->query_climbing() ? 1 : 0;
            addon[MOUNT_DMG]    = ob->query_attack_steed();
            addon[MOUNT_CHARGE] = ob->query_charge_mount();
            addon[MOUNT_GUILD]  = ob->query_legal_guilds();
            addon[MOUNT_NOGLD]  = ob->query_illegal_guilds();
            addon[MOUNT_RACE]   = ob->query_legal_races();
            addon[MOUNT_NORACE] = ob->query_illegal_races();

            mounts += ({ addon });
        }

        mounts = sort_array(mounts, (: $2[MOUNT_COST] - $1[MOUNT_COST] :));
        set_no_dest();
    }

    ret = "%^CYAN%^" +
      sprintf("%-25s  %7s  %4s  %4s  %4s  %5s  %|5s  %|7s  %6s  %6s%%^RESET%%^",
              "Animal Name",
              "Quests",
              "Cost",
              "Size",
              "Encu",
              "Speed",
              "Water",
              "Climb",
              "DmgBon",
              "Charge") + "%^RESET%^\n";

    foreach(bits in mounts) {
        string tmp, shrt;

        shrt = strip_colours(bits[MOUNT_NAME]);
        shrt = repeat_string(" ", 25 - strlen(shrt));
        shrt = bits[MOUNT_NAME] + shrt + "%^RESET%^";

        if ( bits[MOUNT_FASTER] == 1 )
            tmp = "%^YELLOW%^";
        else if ( bits[MOUNT_FASTER] == -1 )
            tmp = "%^RED%^";
        else
            tmp = "";

        ret += sprintf("%-25s  %7s  %4d  %4d  %4d  %s%5d%%^RESET%%^  %|5s  %|7s  %6s  %6s\n",
            shrt,
            nice_list(bits[MOUNT_QUEST]),
            bits[MOUNT_COST],
            bits[MOUNT_SIZE],
            bits[MOUNT_WEIGHT],
            tmp,
            bits[MOUNT_SPEED],
            bits[MOUNT_SEA] ? "Yes" : "",
            bits[MOUNT_CLIMB] ? "Yes" : "",
            ({"", "One", "Two"})[bits[MOUNT_DMG]],
            ({"", "One", "Two"})[bits[MOUNT_CHARGE]]);
    }

    me->msg(ret);
    return 1;
}
