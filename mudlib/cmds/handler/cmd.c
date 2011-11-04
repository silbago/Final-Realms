/**
 * @main
 *
 * This is the base object for the cmds.
 *
 * Notice the standard flag parsing.
 *
 * @version QUITE a few has altered this one
 * @author Someone@aurora mud
 */

#pragma optimize
#include <cmd.h>
#include <help.h>

nosave int _do_clone_command = 0,
           last_execute,
           _do_not_dest_command = 0;
nosave object command_giver;
private nosave string *cmd_flags;

void create() {
    seteuid(getuid());
    TO->setup();
}

mixed    query_syntax      () { return 0;     }
mapping  query_flags       () { return 0;     }
function query_filter      () { return 0;     }
void     dest_me           () { destruct(TO); }
int      query_is_cmd      () { return 1;     }
int      query_parse_flags () { return 0;     }

/**
 * The "cmd" commands has 4 default settings for metadata for the helpssystem.
 * If you want to tailor it, do so. The array you customize must adhere to the
 * setting we have here. ranking, category, subtags and source-types.
 * @author Silbago
 */
mixed query_help_tags() {
    return ({ 2, "character", ({ "command" }), SOURCE_ON_THE_FLY });
}

/**
 * If you need to command object to be cloned specific for
 * the one use. Normally used to avoid problems with global
 * variables, or other setting issues. Etc.
 */
protected void set_do_clone() {
    _do_clone_command = 1;
}

/**
 * If you do not want the command object to destruct itself
 * after the command call was sent. It is going to be used
 * further, for example help command where there are input_to()
 * usages. Or who command, that is constantly used.
 * @param 'has' 1 for up to 1 hour unused, 2 neverdest.
 */
protected varargs void set_no_dest(int has_functional) {
    _do_not_dest_command = has_functional ? 2 : 1;
}

mixed clean_up() {
    if ( !_do_not_dest_command ) {
        destruct(TO);
        return 0;
    }

    if ( _do_not_dest_command == 2 )
        return;

    if ( (last_execute + 3600) < time() ) {
        destruct(TO);
        return 0;
    }

    return 1;
}

int query_do_clone() { return _do_clone_command; }
int query_do_not_dest() { return _do_not_dest_command; }
void reset_no_dest() { _do_not_dest_command = 0; }

/**
 * If you use this one in setup, the fourth parameter to cmd()
 * will be mapping flags. The tail will be parsed clean for
 * the flags and sent to cmd.
 * If you set flag ({ "n","p" })
 *   Command will be like "who -p -n bah" and flags = (["p":1,"n":1 ])
 *   while tail = "bah". "who -pn" or "who -np" works too.
 * @author Silbago
 */
protected void set_flags(string *flags) {
    cmd_flags = flags;
}

string *query_cmd_flags() { return cmd_flags; }
void setup() { return; }

protected varargs int cmd(string tail, object thisob, string verb, mapping flags) {
    tail=thisob=verb=flags=0;
    return 0;
}

string query_usage() { return 0; }
string query_short_help() { return 0; }

int _cmd(string tail, object thisob, string verb) {
    mapping flags;
    string euid,
           str,
           tmp;
    int i;

    if ( base_name(previous_object()) != CMD_HANDLER )
        return 0;

    last_execute = time();
    command_giver = thisob;

    seteuid(euid = geteuid(command_giver));

    // Going through the command argument, finding the flags.
    //
    flags = ([ ]);
    if ( cmd_flags && tail && tail != "" && tail != "-" ) {
        string head = "-";
        while(    (i=sscanf(tail, "-%s %s", str, tmp)) == 2
               || (i=sscanf(tail, "-%s", str)) )
        {
            tail = i == 1 ? "" : tmp;
            if ( member_array(str[0..0], cmd_flags) == -1 )
                head += str;
            else foreach(str in explode(str, ""))

                if ( member_array(str, cmd_flags) != -1 )
                    flags[str] = 1;
                else
                    head += str;
        }

        // This trick with head is to not "flag in" what is not a legal flag
        //
        if ( head != "-" )
            tail = head + " " + tail;

        if ( tail == "" )
            tail = 0;
    }

    return cmd(tail, thisob, verb, flags);
}

protected int pcmd(mixed *argv, object me, int match, mapping flags)
{
  argv = me = flags = match = 0;
  return 0;
}


/**
 * This is the call to the command with arguments from the cmd
 * handler. Syntax usage of the command is pre-parsed and data
 * needed for the command is fixed and setup in argv. See
 * /include/cmd.h for defines that will help you.
 *
 * Notice that internally in pcmd() in the command we run order of
 * arguments in old style. As in command objects elsewhere we
 * are doing it different (aka add_action(), add_command() etc).
 */
int _pcmd(object me, mixed *argv, int match, mapping flags) {
    if ( base_name(previous_object()) != CMD_HANDLER )
        return 0;

    last_execute = time();
    seteuid(geteuid(me));
    return pcmd(argv, me, match, flags);
}

/*
 * Added by Baldrick.
 * using this_player ain't good.
int notify_fail(string fa)
  {
  this_player()->set_notified(1);
  fail_msg = fa;
  return 0;
}
*/
