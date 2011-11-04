/**
 * @main
 *
 * The channels handler.
 * All settings in all channels is determined by information in
 * each respective usergroup. Altering code in this handler is futile.
 *
 * The mud system holds the usergroup channels, party channels and the
 * personal channels. How to setup a channel in the usergroup system will
 * be described later. Pure usergroup channels are of the same type as
 * the usergroup.
 *
 * Party channels are controlled through the usergroup system, by the party
 * system internally. They are identified as channel type 'party, as a party
 * usergroup actually is.
 *
 * The personal channels are controlled through the channels command system.
 * They are identified as channel type 'personal'.
 *
 * The multi-mud channels are channels of usergroup type 'creator' that binds
 * the FR-muds together. Others can be set up, if wanted by an extended
 * method later.
 *
 * The intermud channels are controlled by the intermud channels handler, and
 * setup is done from there. Trusting that the intermud system does not
 * collide with this one, not without warnings though. They are identified
 * as type 'intermud'.
 *
 * When setup is loaded through these 5 channel types, each channel has its
 * own properties based on certain need-to-control settings.
 *
 * Each channel has these settings (deviations later):
 *   - name.
 *   - alias.
 *   - Group short.
 *   - type (as described above).
 *   - listeners, those who are in a channel.
 *   - No talkers, usergroups denied talking.
 *   - No listeners, usergroups denied listening.
 *
 * How to setup in usergroups, using properties, by example (watch close)
 *   - has_chan : yes              | Channel name becomes as usergroup.
 *   - has_chan : forest           | Alias becomes forest.
 *   - short : Priest of Grimbrand | Silbago [Priest of Grimbrand]: Kill dwarf
 *   - no_talk : timion,patron     | These groups can't talk on this channel
 *   - no_listen : players         | Can talk but not listen (emergency)
 *   - guest_group : masters,aod   | Other groups that can talk and listen
 *   - chan_ttl : (int)hours       | How many hours chats stays in history
 *
 * Aliasing with property has_chan :
 *   The guilds can use their respective guild channels with the command guild,
 *   though it would be like guild 'grimbrand'. One can also execute grimbrand.
 *   command, though guild would be far easier. Yes is no-alias ...
 *
 * Block groups from listening or talking on a channel.
 *   The value to the two mentioned properties will be exploded into an array
 *   and tested at listening/talking respectively. If you want to banish a
 *   given person from a channel, by talk or listen, or both. You have to use
 *   property on the person:
 *     "no_talk timion" : 1
 *     "no_listen timion" : 1
 *
 * Aliasing, technical description
 *   Internally a seperate mapping keeps track of aliases, and real channels
 *   they alias. If there is one alias, the argument is string. If there are
 *   more options to the alias, only one of them can be accessed by the user.
 *   And we need a quick query method to figure out which one. Here's the sad
 *   part, example from the three most common aliases (as a warning):
 *       #define ALIAS_CALLOUT ([ "guild" : "query_guild" ]) in channels.h
 *   If this is not checked out, query_access will search for the first
 *   channel in the alias array that has success rate != 0
 *
 *
 * Development
 * query who is on channel, more general function(s)
 * turn channel on remote, free access, no input back to avoid sneaking
 * on who can hear what. CTF needs it, other things too .... ?
 * @author Silbago
 */

#include <localtime.h>
#include <handlers.h>
#include <channels.h>
#include <council.h>
#include <network.h>
#include <cmds.h>

#define SAVE_FILE "/" + mud_name() + "/save/channels"

/**
 * A list of smileys, just to not make emote chat hickup.
 */
#define IS_SMILEY ({         \
    "-%", "-",  "P",  "-P",  \
    "D",  "-D", "p",  "-p",  \
    ")",  "-)", "/",  "-/",  \
    "(",  "=",  "=)", "%",   \
})

/**
 * This is really fubar, though it is efficient for checking.
 * Aliases are usually guild, race and group and those does
 * not need usergroup queries, faster to do a call_out directly.
 * Now, other types queries all usergroups in the alias array.
 */
#define ALIAS_CALL_OTHER ([       \
    "guild" : "query_guild",      \
    "group" : "query_sig",        \
    "race"  : "query_race_group", \
])

mapping save_channels;
private nosave mapping channel_aliases,
                       terminal,
                       channels;
private nosave string channel_name,
                      channel_alias,
                      *multiple_aliases;
private nosave object shouter_object;
/** @ignore start */
private nosave class channel channel_data;

class channel query_channel_data(string chan) {
    if ( !channels[chan] )
        return 0;
    return copy(channels[chan]);
}
/** @ignore end */
nomask int query_access(string name, object me);

mapping query_channels() { return copy(channels); }
string *query_channel_names() { return keys(channels); }
mapping query_channel_aliases() { return copy(channel_aliases); }

private void load_me() {
    class channel chandata;
    string group, *members;
    mixed ob;
    int i;

    restore_object(SAVE_FILE, 1);
    if ( !save_channels )
        return;
    foreach(group, members in save_channels) {
        if ( !channels[group] )
            continue;

        chandata = channels[group];
        for ( i=0; i < sizeof(members); i++ ) {
            if ( (ob = find_player(members[i])) )
                chandata->listeners += ({ ob });
            else {
                ob = filter(objects(), (: file_name($1) == $(members[i]) :));
                if ( sizeof(ob) )
                    chandata->listeners += ({ ob[0] });
            }
        }
    }
    save_channels = ([ ]);
}

private void save_me() {
    string group;
    int i;

    save_channels = ([]);
    foreach(group, class channel chan in channels) {
        if ( chan->type == "intermud" )
            continue;
        chan->listeners -= ({ 0 });
        if ( !sizeof(chan->listeners) )
            continue;
        for ( i=0; i < sizeof(chan->listeners); i++ ) {
            if ( !save_channels[group] )
                save_channels[group] = ({ });
            if ( interactive(chan->listeners[i]) )
                save_channels[group] += ({ geteuid(chan->listeners[i]) });
            else
                save_channels[group] += ({ file_name(chan->listeners[i]) });
        }
    }

    save_object(SAVE_FILE);
}

int clean_up() {
    return 0;
}

int dest_me() {
    save_me();
    destruct(TO);
    return 1;
}

int dwep() {
    return dest_me();
}

/**
 * This function is used to setup channel data with information from
 * a usergroup. First user by create() when handler loads to create
 * the channels accordingly, then later to create party channels,
 * and thirdly to update when properties are altered in the usergroups
 * handler. Meaning, usergroups informs this handler when properties
 * to a usergroup is altered.
 *
 * Here is a summary of usergroup properties involved in channel
 * setup, for a full description see the header of this helpfile.
 *   - has_chan
 *   - no_talk
 *   - no_listen
 *   - guest_group
 *   - short
 * @author Silbago
 */
void update_usergroup_data(string group) {
    mixed bing;

    // usergroup is gone, or doesnt have channel any longer
    //
    if (    channels[group]
         && (!group_exists(group) || !group_property(group, "has_chan")) )
    {
        channel_data = channels[group];
        if ( channel_aliases[channel_data->alias] )
            channel_aliases = m_delete(channel_aliases, channel_data->alias);
        if ( channel_aliases[channel_data->name] )
            channel_aliases = m_delete(channel_aliases, channel_data->name);
        channels = m_delete(channels, group);
        return;
    }

    if ( !(channel_name = group_property(group, "has_chan")) )
        return;

    if ( channels[group] )
        channel_data = channels[group];
    else
        channel_data = new(class channel);

    if ( channel_name == "yes" )
        channel_data->alias = group;
    else
        channel_data->alias = channel_name;

    channel_data->name = group;
    channel_aliases[channel_data->name] = channel_data->alias;

    // For the matter of quick lookup, we reverse alias aliases.
    // For redundancy we make sure that real names are not overwritten
    // Multiple_aliases is a speedster for activate_channel
    //
    if ( !channel_aliases[channel_data->alias] )
        channel_aliases[channel_data->alias] = channel_data->name;
    else if (   !ALIAS_CALL_OTHER[channel_data->alias]
              && member_array(channel_data->alias, multiple_aliases) == -1 )
    {
        multiple_aliases += ({ channel_data->alias });
    }

    if ( !channel_data->listeners && !sizeof(channel_data->listeners) )
        channel_data->listeners = ({ });
    channel_data->granter = group_assistant(group);
    channel_data->usergroup = ({ group });
    if ( !channel_data->history && !sizeof(channel_data->history) )
        channel_data->history = ({ });
    channel_data->type = group_type(group);

    bing = group_property(group, "short");
    channel_data->short = bing ? bing : group;
    bing = group_property(group, "no_talk");
    channel_data->no_talk = bing ? explode(bing, ",") : ({ });
    bing = group_property(group, "no_listen");
    channel_data->no_listen = bing ? explode(bing, ",") : ({ });
    bing = group_property(group, "guest_group");
    if ( bing )
        channel_data->usergroup += explode(bing, ",");

    bing = group_property(group, "chan_ttl");
    bing = strlen(bing) ? to_int(bing) * 3600 : 3600 * 24;
    channel_data->ttl = bing;

    channels[group] = channel_data;
}

void create() {
    mixed bing;
    string group;

    seteuid("Root");
    channels = ([]);
    channel_aliases = ([]);
    multiple_aliases = ({ });
    terminal = TERMINAL_HANDLER->set_term_type("dumb");

    foreach(group in GROUPS_HANDLER->query_groups()) {
        if ( !(channel_name = group_property(group, "has_chan")) )
            continue;

        update_usergroup_data(group);
    }

    // Setting up the intermud channels.
    // Privileges is hardcoded in query access, maybe
    // We'll make an interface for this later ... ??
    //
    bing = keys(INTERMUD_C->query_local_channels());
    foreach(group in bing) {
        channel_data             = new(class channel);
        channel_data->name       = group;
        channel_data->group_type = group_type(group);
        channel_data->short      = CAP(group);
        channel_data->alias      = group;
        channel_data->type       = "intermud";
        channel_data->granter    = 0;
        channel_data->usergroup  = ({ "creators" });
        channel_data->listeners  = ({ });
        channel_data->no_talk    = ({ });
        channel_data->no_listen  = ({ });
        channel_data->history    = ({ });
        channel_data->ttl        = 3600*24;
        if ( group == "intercre" )
            channel_data->ttl    = 3600*24*3;
        channel_aliases[group]   = group;
        channels[group]          = channel_data;
    }

    channel_name = 0;
    load_me();
    bing = (time()/3600)*3600;
    bing = time() - bing;
    bing = 3600 - bing;
    call_out("clean_history", 5);
    call_out("clean_history", bing);
}

/**
 * Loads data for channel into the global class for easy access.
 * @return 0 means that channel does NOT exist
 */
private int activate_channel(string name, object me) {
    // Global variables setup for handling channel 'name'
    //
    if ( name == channel_name )
        return 1;

    // If it is not here, it does not exist, period.
    //
    if ( !channel_aliases[name] )
        return 0;

    // Ok, we are dealing with an alias
    //
    if ( !channels[name] ) {
        // The most common aliases we treat with a function call on object.
        // Else we check if it is a multiple alias
        // Else, this chan is the only one using given alias
        //
        if ( ALIAS_CALL_OTHER[name] ) {
            name = call_other(me, ALIAS_CALL_OTHER[name]);
            if ( !name || !channels[name] )
                return 0;
        } else if ( member_array(name, multiple_aliases) != -1 ) {
            string *chans =keys(filter(channel_aliases, (: $2 == $(name) :)));
            chans = filter(chans, (: query_access($1, $(me)) :));
            if ( !sizeof(chans) )
                return 0;

            // It really should not occur that we have more than one option to this
            // If it should happen, deal with the recoding at that point.
            //
            name = chans[0];
        } else
            name = channel_aliases[name];
    }

    channel_name = name;
    channel_alias = channel_aliases[name];
    channel_data = channels[name];
    return 1;
}

/**
 * Testing if object me has LISTEN and/or TALK privileges on channel 'name'
 * @return bits of LISTEN and TALK
 */
nomask int query_access(string name, object me) {
    int ret;

    if ( !activate_channel(name, me) )
        return 0;

    // Npcs needs to be able to talk, listen is a NONONONONO
    //
    if ( !interactive(me) )
        return TALK;

    // Testing usergroup membership, personal channels are different.
    //
    if ( channel_data->type == "personal" ) {
        if ( channel_data->granter == geteuid(me) )
            return TALK | LISTEN;

        if ( member_array(me, channel_data->listeners) != -1 )
            return TALK | LISTEN;

        return 0;
    } else if ( group_member(me, channel_data->usergroup) )
        ret = TALK | LISTEN;

    // If you cant talk we say so, else we try to strip more rights.
    //
    if ( !ret )
        return ret;

    // Immortals and their testers below alchemist, exluding patrons are denied
    //
    switch(channel_data->group_type) {
    case "tower":
    case "guild":
    case "sig":
    case "race_group":
        if (    group_member(me, ({"builders","patrons","testers"}))
             && !query_access_level(me, channel_data->name, PATRON_ACCESS_LEVEL) )
            return 0;
    }

    if ( sizeof(channel_data->no_talk) && group_member(me, channel_data->no_talk) )
        ret &= ~TALK;
    if ( sizeof(channel_data->no_listen) && group_member(me, channel_data->no_listen) )
        ret &= ~LISTEN;
    if ( me->query_property("no_listen " + channel_name) )
        ret &= ~LISTEN;
    if ( me->query_property("no_talk " + channel_name) ) {
        int long;
        mixed why = me->query_property("no_talk " + channel_name);

        if ( !stringp(why) )
            why = "You are denied access to talk on " + channel_name + ".\n";
        long = me->query_time_remaining("no_talk " + channel_name);
        if ( long )
            why = "Lock will last for another " + pretty_time(long) + ". " + why;

        notify_fail(why, me);
        ret &= ~TALK;
    }

    return ret;
}

/**
 * If you use query_access() you only get return wether u can or not.
 * Now query_acesss() does not tell you the real name of the channel.
 * So if you want to know what actual channel you are dealing with,
 * throw in this later.
 *
 * Be carefull to know that u got access first :)
 */
string query_channel_name() {
    return channel_name;
}

/**
 * On occations someone wants to grant access, or ungrant access on a user.
 * At that time we use this one. It will test: LISTEN | TALK | GRANT
 */
nomask int query_grant_access(string chan, object me) {
    int ret = query_access(chan, me);
    if ( ret != (LISTEN + TALK) )
        return 0;

    if ( channel_data->granter == geteuid(me) )
        ret |= GRANT;
    else if ( COUNCIL_HANDLER->access_level(me, channel_name) >= A_LEADER_ACCESS_LEVEL )
        ret |= GRANT;        
    else if ( group_member(me, "lords") )
        ret |= GRANT;

    return ret;
}

object *query_party_members(string name, object me) {
    if ( !me || me != TP || !group_member(me, "lords") )
        return 0;
    if ( !activate_channel(name, me) )
       return 0;

    channel_data->listeners -= ({ 0 });
    return channel_data->listeners;
}

/**
 * For intermud services to be able to run a query.
 */
string *query_listeners(string name) {
    object *ret;

    if ( !activate_channel(name, 0) || channel_data->type != "intermud" )
        return ({ });
    
    ret = filter(channel_data->listeners, (: !$1->query_invis() :));
    return map(ret, (: geteuid($1) :));
}

/**
 * Attempt to join channel by object me.
 * @comment 'me' is told what happens, if message is generated.
 * @return 1 for success, 0 for failure.
 */
int join_channel(string name, object me) {
    int i;

    if ( !activate_channel(name, me) )
        return 0;

    if ( me->query_property("no_listen " + channel_name) ) {
        i = me->query_time_remaining("no_listen " + channel_name);
        if ( i )
            me->msg("%^CYAN%^You have been banished from listening to " +
                channel_data->short + "%^RESET%^%^CYAN%^ channel for a "
                "remaining duration of " + pretty_time(i) + " by " +
                CAP(me->query_timed_property("no_listen " + channel_name)) +
                ".%^RESET%^\n");
        else
            me->msg("%^CYAN%^You have been permanently banished from "
                "listening to " + channel_data->short + "%^RESET%^%^CYAN%^ "
                "channel.%^RESET%^\n");

        return 1;
    }

    if ( !(i=query_access(channel_name, me)) )
        return 0;

    if ( !(i & LISTEN) ) {
        notify_fail("\n%^CYAN%^You can not listen to channel " +
                        channel_data->short + ".%^RESET%^\n", me);
        return 0;
    }

    if ( member_array(me, channel_data->listeners) != -1 ) {
        notify_fail("\n%^CYAN%^You are already listening to " +
               channel_data->short + " %^RESET%^%^CYAN%^channel.%^RESET%^\n", me);
        return 0;
    }

    channel_data->listeners -= ({ 0 });
    channel_data->listeners += ({ me });
    tell_object(me, "\n%^CYAN%^Turning " + channel_data->short +
                        " %^RESET%^%^CYAN%^channel on.%^RESET%^\n");
    return 1;
}

/**
 * Attempt to leave channel, success message returned.
 */
int leave_channel(string name, object me) {
    if ( !activate_channel(name, me) )
        return 0;

    if ( member_array(me, channel_data->listeners) == -1  ) {
        if ( !query_access(channel_name, me) )
            return 0;

        tell_object(me, "\n%^CYAN%^You are not listening to " +
               channel_data->short + " channel.%^RESET%^\n");
        return 1;
    }

    channel_data->listeners -= ({ me, 0 });
    tell_object(me, "\n%^CYAN%^Turning " + channel_data->short +
           " %^RESET%^%^CYAN%^channel off.%^RESET%^\n\n");
    return 1;
}

/**
 * Reviewing channel history.
 */
string show_channel_history(string name, object me, int lines) {
    string ret, temp, mulmu;
    mixed bing;

    if ( base_name(previous_object()) != CHANNELS_CMD )
        return 0;

    if ( !(query_access(name, me) & LISTEN) )
        return 0;

    if ( sizeof(channel_data->history[<lines..]) < 1 )
        return "Noone has talked on that channel yet.\n";

    ret = "%^CYAN%^---Begin channel review---%^RESET%^\n";
    foreach(bing in channel_data->history[<lines..]) {
        string garbled, when_string;

        if ( !bing )
            continue;

        when_string = ctime(bing[HISTORY_TIME]);
        if ( (bing[HISTORY_TIME] + (365 * 24 * 60 * 60)) < time() )
            when_string = sprintf("%s  %s", when_string[4..9], when_string[20..23]);
        else if ( localtime(bing[HISTORY_TIME])[LT_MDAY] == localtime(time())[LT_MDAY] )
            when_string = ctime(bing[HISTORY_TIME])[11..18];
        else
            when_string = when_string[4..18];

        mulmu = bing[HISTORY_MULMU] ? "(%s)" : "[%s]";
        if ( bing[HISTORY_EMOTE] )
            temp = sprintf("%s " + mulmu + " %s",
                when_string,
                bing[HISTORY_SHORT],
                bing[HISTORY_NAME]);
        else
            temp = sprintf("%s %s " + mulmu + ":",
                when_string,
                bing[HISTORY_NAME],
                bing[HISTORY_SHORT]);

        // Fixing languages
        //
        garbled = bing[HISTORY_MESS];
        if ( name == "shout" ) {
            string language;
            language = 0;
            sscanf(garbled, "$language:%s$%s", language, garbled);

            if ( language && !me->query_known_language(language) ) {
                object lang_ob;
                mixed garbl;

                lang_ob = LANGUAGE_HANDLER->query_garble_object(language);
                if ( lang_ob ) {
                    garbl = lang_ob->garble_shout("bah", garbled);
                    if ( sizeof(garbl) == 2 )
                        garbled = garbl[1];
                }
            }
        }

        ret += sprintf_chat(temp + " " + garbled, me->query_cols(), 9) + "%^RESET%^\n";
    }

    ret += "%^CYAN%^----End channel review----%^RESET%^\n";
    return ret;
}

/**
 * For save_history command to grab history pretty
 */
string grab_save_history(string name, object me, int lines) {
    string ret, temp;
    mixed bing;

    if ( base_name(previous_object()) != SAVE_HISTORY_CMD )
        return 0;

    ret = "";
    if ( !(query_access(name, me) & LISTEN) )
        ret += "  >> Not allowed to listen to channel.\n";
    if ( !(query_access(name, me) & TALK) )
        ret += "  >> Not allowed to talk to channel.\n";
    if ( !channel_aliases[name] )
        return ret + "  >> No channel with name " + name + " exists.\n";

    if ( sizeof(channel_data->history[<lines..]) < 1 )
        return ret + "  >> Noone has talked on the channel.\n";

    foreach(bing in channel_data->history[<lines..]) {
        string garbled;

        if ( !bing )
            continue;

        if ( bing[HISTORY_EMOTE] )
            temp = sprintf("[%s] %s",
                bing[HISTORY_SHORT],
                bing[HISTORY_NAME]);
        else
            temp = sprintf("%s [%s]:",
                bing[HISTORY_NAME],
                bing[HISTORY_SHORT]);

        garbled = strip_colours(bing[HISTORY_MESS]);
        if ( name == "shout" ) {
            string language;
            language = 0;
            sscanf(garbled, "$language:%s$%s", language, garbled);
        }

        ret += sprintf("  [%s] %-*=s\n",
                       ctime(bing[HISTORY_TIME])[11..18], 70, temp + " " + garbled);
    }

    return ret;
}

/**
 * Returns 1 if 'me' is in channel. For channels command only.
 */
int query_member_of(string name, object me) {
    if ( !activate_channel(name, me) )
        return 0;
    channel_data->listeners -= ({ 0 });
    return member_array(me, channel_data->listeners) != -1;
}

/**
 * Formatting a list of users in a channel, security checks involved.
 * Query by channels command only.
 */
int show_on_channel(string name, object me, string mud) {
    mixed listeners;

    if ( base_name(previous_object()) != CHANNELS_CMD )
        return 0;

    if ( !(listeners = query_access(name, me)) )
        return 0;

    if ( !(listeners & LISTEN) )
        return notify_fail("You do not have access to listen to " +
                           channel_name + ".\n", me);

    // Intermud queries
    //
    if ( mud && channel_data->type == "intermud" ) {
        if ( !group_member(me, "creators") )
            return notify_fail("Immortals only.\n", me);

        SERVICES_D->eventSendChannelWhoRequest(name, mud, geteuid(me));
        tell_object(me, "Query who is on " + name + " channel sent to " + mud + "\n");
        return 1;
    }

    channel_data->listeners -= ({ 0 });;
    listeners = channel_data->listeners - ({ 0 });
    if ( !group_member(me, "lords") ) {
        if ( !group_member(me, "creators") ) {
            listeners = filter(listeners, (: !$1->query_invis() :));
            // People wants to hide what guild they have some times.
            //
            if (    group_type(channel_name) == "guild"
                 || group_type(channel_name) == "tower" )
                listeners = filter(listeners, (: $1 == $(me) :));
        } else
            listeners = filter(listeners, (: $1->query_invis() < 2:));
    }

    listeners = map(listeners, (: $1->query_cap_name() :));
    listeners = sort_array(listeners, 1);
    tell_object(me, csprintf("%%^CYAN%%^%s:%%^RESET%%^ %-=*s\n",
           "People hearing " + channel_data->short,
           me->query_cols() - 30,
           nice_list(listeners)));
    return 1;
}

/**
 * Remote multimud channels query for who is on channel.
 */
string remote_cre_query(string chan) {
    if ( base_name(previous_object()) != MIRROR_D )
        return 0;
    if ( !activate_channel(chan, 0) )
        return 0;
    channel_data->listeners -= ({ 0 });
    return implode(sort_array(map(copy(channel_data->listeners), (: CAP(geteuid($1)) :)), 1), "\n");
}

int create_personal_channel(object me) {
    string name = "personal." + geteuid(me);

    if ( channels[name] || base_name(previous_object()) != CHANNELS_CMD )
        return 0;
    channel_data = new(class channel);
    channel_data->name      = name;
    channel_data->short     = CAP(geteuid(me));
    channel_data->alias     = name;
    channel_data->type      = "personal";
    channel_data->granter   = geteuid(me);;
    channel_data->usergroup = ({ });
    channel_data->listeners = ({ me });
    channel_data->no_talk   = ({ });
    channel_data->no_listen = ({ });
    channel_data->history   = ({ });
    channel_data->ttl       = 3600 * 12;
    channel_aliases[name]   = name;
    channels[name]          = channel_data;
    return 1;
}

/**
 * Method singled out to end personal and party channels.
 * Brings up a message on those who are in the channel when it happens.
 * Automatic at logoff on personal, on request by party.
 */
private int end_channel(string name) {
    if ( !activate_channel(name, 0) )
        return 0;

    foreach(object ob in channel_data->listeners) {
        if ( ob )
            tell_object(ob, "\n%^CYAN%^Terminating " + channel_data->short +
                " %^RESET%^%^CYAN%^channel.%^RESET%^\n\n");
    }

    channels = m_delete(channels, name);
    channel_aliases = m_delete(channel_aliases, name);
    return 1;
}

int end_personal_channel(object me) {
    if ( base_name(previous_object()) != CHANNELS_CMD )
        return 0;
    end_channel("personal." + geteuid(me));
}

int name_personal_channel(object me, string new_name) {
    string name = "personal." + geteuid(me);

    if ( !channels[name] || base_name(previous_object()) != CHANNELS_CMD )
        return 0;

    channel_data = channels[name];
    channel_data->short = new_name;
    return 1;
}

int invite_personal_channel(string name, object him, object me) {
    if ( base_name(previous_object()) != CHANNELS_CMD )
        return 0;
    if ( !activate_channel(name, me) )
        return 0;
    if ( !(query_grant_access(name, me) & 4) )
        return 0;
    if ( member_array(him, channel_data->listeners) != -1 )
        return 0;
    channel_data->listeners += ({ him });
        tell_object(him, "\n%^CYAN%^Turning " + channel_data->short +
                        " %^RESET%^%^CYAN%^channel on.%^RESET%^\n");
    return 1;
}

/**
 * This is truly sprintf colour madness fix.
 * Not anymore! -V
 */
void send_msg(object ob, string head, string tail) {
    string wrap_color = ob->query_color("channel " + channel_data->name);
    //int freehead;

    /* tmp, spacer, tails, line_max, ptr 
     * are variables used for the cosmetic fix
     * below. Volothamp, Oct 2010
     */    
    string tmp, spacer;
    string *tails;
    int line_max, ptr;


    if ( shouter_object ) {
        object env_ob;
        string str;
        
        env_ob = environment(shouter_object);
        str = env_ob->query_short() + "";
        
        if (env_ob->query_is_ship() && ENV(env_ob->query_ship()))
            str += "%^RESET%^ at " + ENV(env_ob->query_ship())->query_short();
            
        str = replace_string(str, "\n", "");
        head += "%^RESET%^ from " + str;
    }

    //freehead = strlen(strip_colours(head));
    if ( strlen(wrap_color) )
        head = wrap_color + head;
    head += "%^RESET%^";

    //freehead = (ob->query_cols() - freehead) / 2;

    tmp = tail;
    tails = ({ });
    line_max = ob->query_cols() - 11; /* linemax is -1 from cols + the indention level = 10 */
    spacer = repeat_string(" ", 10);  /* indention level = 10 */
   
   /* Following while statement iterates over the message and divides it into chunks of 
    * of length line_max, then it looks for the last space in that segment, and cuts it there
    * then it goes onto the next segment of length line_max
    */
    while (strlen(tmp)) {
         ptr = -1;
         ptr = strsrch(tmp[0..line_max], 32, -1);
         if (ptr > 0) { /* is there a space in the line? */
            if (strlen(tmp) <= line_max) { /* is it smaller than max? then it's the last one*/
               tails += ({ tmp[0..] + "\n" });
               tmp = "";
            }
            else { /* it's longer than max, we divide */
                tails += ({ tmp[0..ptr] + "\n" });
                tmp = tmp[ptr+1..];
            }
         }
         else { /* no space in line, just add it */
            tails += ({ tmp[0..line_max-1] + "\n" });
            tmp = tmp[line_max-1..];
         }
    }
    tmp = "";
    foreach(string line in tails) /* concatenate all lines with the spacer */
       tmp += spacer + line;
    tail = tmp;

    /* Cosmetic fast-fix by Volothamp, Oct 2010
     * Added above, edited out below
     * No energy fix sprintf choking on colors right now
     * With so different query_short on rooms it would 
     * look weird if the previous indention was kept.
     * My fix will always have the message at the same
     * indention level for all players, but the 
     * "<Player> shouts <from>", will always be of variable
     * length.
    if (freehead > 0)
        tail = repeat_string(" ", freehead) + tail;
     */


    ob->msg(head + ":\n" + tail + "%^RESET%^\n");

/*  Let's do radical csprintf() magic later
    string freehead, ret,
           wrap_color = ob->query_color("channel " + channel_data->name);
    int headsize;

    if ( shouter_object ) {
        string str = ENV(shouter_object)->query_short();
        str = replace_string(str, "\n", "");
        head += "%^RESET%^ from " + str;
    }

    freehead = strip_colours(head);
    headsize = strlen(freehead);

    ret = sprintf("%s: %-=*s\n",
                  freehead,
                  ob->query_cols() - (3 + headsize),
                  tail);

    if ( !wrap_color )
        wrap_color = "";
    ret[0..headsize-1] = wrap_color + head + "%^RESET%^";
    ob->msg(ret);
*/
}

private int do_chat(string me, string mess, int emote, int force) {
    object *listeners, ob;
    string current_short, frame;
    string talker_name;
    int sendflags, is_multimud = 0;

    if ( call_stack(2)[1] == "person_chat" ) {
        if ( channel_data->type == "intermud" )
            INTERMUD_C->do_chat(channel_name, me, mess, emote);
        else if (    channel_data->type == "creator"
                  || channel_data->type == "domain"
                  || channel_data->name == "info" )
            MULTIMUD_DAEMON->do_chat(channel_name, me, mess, emote, force);
    } else
        is_multimud = call_stack(2)[1] == "multimud_chat";

    talker_name = lower_case(explode(me, " ")[0]);
    channel_data->history += ({
        ({
            time(),
            me,
            channel_data->short,
            mess,
            emote,
            is_multimud,
        })
    });
    while(sizeof(channel_data->history) > MAX_HISTORY )
        channel_data->history = channel_data->history[1..];

    // Shouting is not doing all things channels does, so we do it here.
    //
    if ( channel_name == "shout" ) {
        int *shouter_coord;

        string language, garble_frame, garble_mess;

        // We need the object of the user shouting for distances.
        // We dont want this spammy mess for immortals.
        //
        /*
        if ( the_shouter ) {
            if ( group_member(the_shouter, "creators") )
                the_shouter = 0;
            else if ( sizeof((shouter_coord = ENV(the_shouter)->query_coordinates())) != 4 )
                shouter_coord = ({ 0, 0, 0, 4 });
        }
        */

        frame = me + " shouts";
        if ( mess[<1] == '!' )
            frame += " exclaimingly";
        else if ( mess[<1] == '?' )
            frame += " asking";

        // Language for shout channel.
        //
        sscanf(mess, "$language:%s$%s", language, mess);
        if ( language ) {
            object lang_ob = LANGUAGE_HANDLER->query_garble_object(language);
            mixed garbl;
            if ( lang_ob ) {
                garbl = lang_ob->garble_shout(frame + " in " + language, mess);
                if ( sizeof(garbl) == 2 ) {
                    garble_frame = garbl[0];
                    garble_mess = garbl[1];
                    frame += " in " + language;
                }
            }
        }

        foreach(ob in channel_data->listeners - ({ 0 })) {
            string dist_frame = "";

            // Checking for blocks
            //
            if ( BLOCK_HANDLER->blocking_channels(ob, find_player(talker_name)) )
                continue;

            // Alright parsing the distance messages. Not for immortals
            /*
            if (    the_shouter
                 && the_shouter != ob
                 && !group_member(ob, "creators") )
            {
                int distance,        // distance in elevation
                    actual_distance, // 3 dimensional distance
                    distance_ratio,  // Ratio of elevation and 3d distance
                    *listener_coord; // Coordinates to the listener
                string dist_string = "",
                       dir_angle   = "",
                       dir_height  = "";

                actual_distance = query_coordinate_distance(ob, the_shouter);
                switch(actual_distance) {
                case -1:
                    dist_string = "somewhere";
                    break;
                case 0..25:
                    dist_string = "right around here";
                    break;
                case 26..250:
                    dist_string = "not far away";
                    break;
                case 251..750:
                    dist_string = "far away";
                    break;
                case 751..1500:
                    dist_string = "in a long distance";
                    break;
                default:
                    dist_string = "from very far away";
                    break;
                }
                if ( dist_string != "" )
                    dist_string = dist_string + " ";

                if ( sizeof((listener_coord = ENV(ob)->query_coordinates()))  != 4 )
                    listener_coord = ({ 0, 0, 0, 4 });

                distance = shouter_coord[2] - listener_coord[2];
                // As elevation distance is small due to total we subdue messaging
                //
                if ( actual_distance == 0 )
                    actual_distance = 1;
                distance_ratio = distance < 0 ? -distance : distance;
                distance_ratio = 100 * distance / actual_distance;
                switch(distance_ratio) {
                case 80..100:
                    break;
                case 0..25:
                    distance = 0;
                    break;
                default:
                    distance = distance * distance_ratio / 100;
                    if ( distance < 25 && distance > -25 )
                        distance = 0;
                    break;
                }

                switch(distance) {
                case -9999..-1500:
                    dir_height = "from the depths of hell";
                    break;
                case -1499..-500:
                    dir_height = "far down underground";
                    break;
                case -499..-100:
                    dir_height = "deep underground";
                    break;
                case -99..-25:
                    dir_height = "underneath";
                    break;
                case -24..-1:
                    dir_height = "under your heel";
                    break;
                case 0:
                    dir_height = "";
                    break;
                case 1..24:
                    dir_height = "just overhead";
                    break;
                case 25..100:
                    dir_height = "above";
                    break;
                case 101..500:
                    dir_height = "far above";
                    break;
                case 501..1500:
                    dir_height = "on top of the mountain";
                    break;
                case 1501..9999:
                    dir_height = "from the skies";
                    break;
                }
                if ( dir_height != "" )
                    dir_height = dir_height + " ";

                dir_angle = query_direction_string(ob, the_shouter);
                dir_angle = dir_angle ? "to the " + dir_angle + " " : "";

                dist_frame = CAP(dist_string + dir_height + dir_angle);
            }
            */
            shouter_coord = 0;

            if ( language && !ob->query_known_language(language) )
                send_msg(ob, CAP(dist_frame + garble_frame), garble_mess);
            else
                send_msg(ob, CAP(dist_frame + frame), mess);
        }

        return 1;
    }

    if ( force )
        listeners = filter(users(), (: query_access(channel_name, $1) & LISTEN :));
    else
        listeners = channel_data->listeners - ({ 0 });

    // Checking for blocks
    //
    listeners = filter(listeners, (:
        !BLOCK_HANDLER->blocking_channels($1, find_player($(talker_name)))
    :));

    current_short = channel_data->short;
    if ( force && channel_name != "emergency" && channel_name != "info" )
        current_short = "Forced-" + current_short;

    sendflags = 0;
    if ( is_multimud )
        sendflags = 2;
//  else if ( is_intermud )
//      sendflags = 4;
    if ( emote )
        sendflags |= 1;
    if ( channel_data->name[0..5] == "party." )
        sendflags |= 8;

    foreach(ob in listeners) {
        ob->receive_channel_message(sendflags,          mess,
                                    channel_data->name, talker_name,
                                    current_short,      me);
    }
}

void intermud_chat(string name, string me, string mess, int emote) {
    if ( base_name(previous_object()) != INTERMUD_C )
        return;

    shouter_object = 0;
    activate_channel(name, 0);
    do_chat(me, mess, emote, 0);
}

void multimud_chat(string name, string me, string mess, int emote, int force) {
    if ( base_name(previous_object()) != MULTIMUD_DAEMON )
        return;

    shouter_object = 0;
    activate_channel(name, 0);
    do_chat(me, mess, emote, force);
}

/**
 * Call this from external object to send message to channel.
 * This is a talk only method.
 * Parameters:
 *     name    This is the usergroup name of the channel
 *     me      Name of who is talking (short)
 *     mess    The message
 *     emote   This is an emote
 *     force   Force everyone who can listen to the channel to get the message
 */
void unsecure_chat(string name, string me, string mess, int emote, int force) {
    if ( interactive(previous_object()) )
        return;

    shouter_object = previous_object();
    activate_channel(name, 0);
    do_chat(me, mess, emote, force);
}

int person_chat(string name, object me, string mess) {
    int acc_lvl, emote, force;
    string realname, str;

    if ( base_name(previous_object()) != CHANNELS_CMD )
        return 0;

    acc_lvl = query_access(name, me);
    if ( !(acc_lvl & TALK) )
        return 0;

    if ( (acc_lvl & LISTEN) && member_array(me, channel_data->listeners) == -1 ) {
        tell_object(me, "\n%^CYAN%^Turning " + channel_data->short +
                        " %^RESET%^%^CYAN%^channel on.%^RESET%^\n");
        channel_data->listeners += ({ me });
        channel_data->listeners -= ({ 0 });
    }

    if ( mess[0] == '!' ) {
        force = 1;
        mess = mess[1..];
    } else if ( mess[0] == ':' && member_array(mess[1..], IS_SMILEY) == -1 ) {
        emote = 1;
        mess = mess[1..];
    } else if ( mess[0] == '@' ) {
        mapping soul;
        int length;
        string soul_verb;

        mess = mess[1..];
        length = strsrch(mess, " ");
        if ( length == -1 ) {
            soul_verb = mess;
            mess = 0;
        } else {
            soul_verb = mess[0..length-1];
            mess = mess[length+1..];
        }

        channel_data->listeners -= ({ 0 });
        soul = SOUL_HANDLER->get_messages(me, soul_verb, mess,
          copy(channel_data->listeners));

        if ( !soul["status"] || stringp(soul["status"]))
            return notify_fail("%^BOLD%^That soul attempt was not understood."
              "%^RESET%^\n");

        emote = 1;
        soul_verb = me->query_cap_name();
        if ( strlen(me->query_surname()) )
            soul_verb += " " + me->query_surname();
        sscanf(soul["default"], soul_verb + " %s", mess);
    }

    if ( !strlen(mess) ) {
        return write("%^CYAN%^You'll need a message as well.%^RESET%^\n");
        return 1;
    }

    if ( channel_name == "emergency" || channel_name == "info" )
        force = 1;

    // Tricky thing, want shifters and tricksters to show real name
    //
    if ( !interactive(me) )
        realname = me->query_short();
    else {
        realname = me->query_cap_name();
        if( sizeof(me->query_surname()) )
            realname += " " + me->query_surname();
    }

    str = me->filter_speech(mess, "channel", channel_data->alias);
    if ( str )
        mess = str;
    shouter_object = me;
    do_chat(realname, mess, emote, force);
    return 1;
}

void notify_logoff(object me) {
    string *mine = filter(keys(channels), (: query_member_of($1, $(me)) :));
    mine -= ({ "personal." + geteuid(me) });
    me->add_property("Open Channels", mine);
    end_channel("personal." + geteuid(me));
}

void notify_logon(object me) {
    string *mine = me->query_property("Open Channels");
    if ( !sizeof(mine) )
        return;

    mine = filter(mine, (: query_access($1, $(me)) & LISTEN :));
    for ( int i=0; i < sizeof(mine); i++ ) {
        channel_data = channels[mine[i]];
        channel_data->listeners += ({ me });
    }
}

void clean_history() {
    class channel cleandata;

    foreach(string group, cleandata in channels) {
        mixed *chanhist;

        if ( !sizeof(cleandata->history) )
            continue;

        chanhist = cleandata->history;
        for ( int i=0; i < sizeof(chanhist); i++ )
            if ( chanhist[i][0] < (time()-cleandata->ttl) )
                chanhist[i] = 0;

        cleandata->history = chanhist - ({ 0 });
    }

    if ( find_call_out("clean_history") == -1 )
        call_out("clean_history", 3600);
}
