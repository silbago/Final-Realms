/**
 * @main
 *  Handler for the block command
 *  By Driadan, Mar 2002
 *  Data structure:
 *  mapping data = ([
 *                   "player1": ([ "hatredplayer1": block_type ]),
 *                   "player2": ([ "hatredplayer2": block_type,
 *                                 "hatredplayer1": block_type ]),
 *                 ]);
 *
 * CLeanup at retire ?
 * Flode 021203 - counter. tell/soul someone, they're able to respond
 *                the next 60 seconds regardless of blocks.
 *                cleaner with counter.
 *
 * Feb 2010: Silbago cleaned and simplified
 *           Cleaner remove, I'll make a new one later.
 *
 * @author Driadan
 */

inherit "/std/room";

#include <block.h>
#include <handlers.h>

/**
 * List of account owners of people who try to scare everyone away.
 */
#define SABOTEURS ({ \
})

private nosave string *saboteurs;

mapping data = ([ ]);

void load_me() { restore_object(SAVE_FILE); }
void save_me() { save_object(SAVE_FILE);    }

mapping query_data    () { return copy(data);      }
string query_saboteurs() { return copy(saboteurs); }

/**
 * Who user is blocking in any form
 */
string *query_blocking(string user) {
    if ( !data[user] )
        return 0;

    return keys(data[user]);
}

/**
 * List of block settings for user.
 */
mapping hated_list(string user) {
    if ( !data[user] )
        return 0;

    return data[user];
}

void setup() {
    set_short("Block handler: Hall of Hatred");
    set_long("Block handler: Hall of Hatred\n\n"
      "This is where part of the hatress some players feel "
      "against others is stored. As such, the walls are "
      "black, engraved with silvery runes of grudge and "
      "evil thoughts. You hear faint whispers, cursing "
      "in all the known (and some unknown) tongues.\n\n"
      "Command interface: Block\n");
    set_light(30);
    add_property("no_clean_up", 1);
    add_exit("northwest", "/room/admin/communication");

    load_me();
    saboteurs = ({ });
    foreach(string saboteur in SABOTEURS) {
        string *argh = ACCOUNT_HANDLER->query_members(saboteur);
        if ( sizeof(argh) )
            saboteurs += argh;
    }

    call_out("cleanup_retired", 50);
}

/**
 * Mostly internal, sets up the flags directly.
 */
void set_block_flags(string listener, string talker, int blockflags) {
    object ob;
    if ( !data[listener] )
        data[listener] = ([ ]);
    data[listener][talker] = blockflags;

    save_me();
    if ( (ob=find_player(listener)) )
        ob->upgrade_quickfoes();
    if ( (ob=find_player(talker)) )
        ob->upgrade_quickfoes();
}

/**
 * Removing talker from listeners block list.
 */
void remove_block(string listener, string talker) {
    object ob;

    if ( !data[listener] || undefinedp(data[listener][talker]) )
        return;

    data[listener] = m_delete(data[listener], talker);
    if ( !m_sizeof(data[listener]) )
        data = m_delete(data, listener);

    save_me();
    if ( (ob=find_player(listener)) )
        ob->upgrade_quickfoes();
    if ( (ob=find_player(talker)) )
        ob->upgrade_quickfoes();
}

/**
 * Some players are aggressive and are muffled.
 * Can not shout, can only talk to other that has them foe-friend listed.
 * @author Silbago
 */
int is_saboteur(mixed listener, mixed talker) {
    if ( objectp(talker) ) {
        if ( !interactive(talker) )
            return 0;
        talker = geteuid(talker);
    }

    if ( member_array(talker, saboteurs) == -1 )
        return 0;

    if ( objectp(listener) ) {
        if ( !interactive(listener) )
            return 0;
        if ( listener->query_foe("friend", talker) )
            return 0;
    }

    if ( objectp(talker) && protected_player(talker) )
        return 1;

    return 0;
}

/**
 * The general check if listener does not want contact with the talker.
 * The flag 'BLOCK_FLAG' is the specific flag we test for.
 */
int blocking_flags(mixed listener, mixed talker, int BLOCK_FLAG) {
    string listener_name, talker_name;
    int my_flags;

    talker_name = 0;
    if ( objectp(talker) ) {
        if ( !interactive(talker) )
            return 0;
        if ( is_saboteur(listener, talker) )
            return 1;

        talker_name = geteuid(talker);
    }

    listener_name = 0;
    if ( objectp(listener) ) {
        if ( !interactive(listener) )
            return 0;

        listener_name = geteuid(listener);
    } else if ( stringp(listener) )
        listener_name = listener;

    if ( !data[listener_name] )
        return 0;

    if ( objectp(talker) && talker->query_property("guest") ) {
        if ( data[listener_name] && data[listener_name]["guest"] )
            return 1;
        return 0;
    }

    if ( !data[listener_name][talker_name] ) {
#       if 0
        :::This is too much really:::System not fast enough yet:::Optimize account queries later:::
        where to put in BLOCK_ACCOUNT flag in a sensible matter ?
        foreach(string account in ACCOUNT_HANDLER->query_members(talker_name))
            if ( data[listener_name][account] ) {
                my_flags = data[listener_name][account];
                if ( omiq_in_progress() && !(my_flags = BLOCK_OMIQ) )
                    return 0;
                if ( my_flags & BLOCK_FLAG )
                    return 1;
            }
#       endif

        return 0;
    }

    // During omiqs we might bypass
    //
    my_flags = data[listener_name][talker_name];
    if ( !(my_flags & BLOCK_OMIQ) && omiq_in_progress() )
        return 0;

    return (my_flags & BLOCK_FLAG);
}

/**
 * Testing if 'who' blocks tells from 'hated'.
 * Also includes soul as soul handler use this one.
 */
int blocking_tells(mixed listener, mixed talker) {
    return blocking_flags(listener, talker, BLOCK_TELLS);
}

/**
 * Listener does not want to HEAR what talker say on channel.
 */
int blocking_channels(mixed listener, mixed talker) {
    return blocking_flags(listener, talker, BLOCK_CHANNELS);
}

/**
 * Listener does not want to HEAR what talker say on channel.
 */
int blocking_souls(mixed listener, mixed talker) {
    return blocking_flags(listener, talker, BLOCK_SOUL);
}

void cleanup_retire() {
    foreach(string listener, mapping blocks in data) {
        if ( !user_exists(listener) ) {
            data = m_delete(data, listener);
            continue;
        }

        foreach(string talker, int flags in blocks)
            if ( !user_exists(talker) && listener != "guest" ) {
                data = m_delete(data[listener], talker);
                continue;
            }

        if ( !m_sizeof(blocks) )
            data = m_delete(data, listener);
    }
}
