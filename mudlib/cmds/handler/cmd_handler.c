/**
 * @main
 * Final Realms command handler, idea from Aurora Mud.
 *
 * Ok we have commands everywhere, this handler controls a lot of commands.
 * First we have a few commands in subdirs of /cmds/, read file /cmds/DIRS.
 *
 * Security is very rought here and have several layers, good luck.
 * Parsing of commands is first handled by action_queue.c, though we do
 * the bulk work with item commands here. In sequential order, no ranking.
 *
 * add_action() is a seperate project to be replaced by add_cmd().
 *
 * The /cmds/onetime/ commands run with a seperate system, calls to those commands
 * are forwarded internally here. Though this system does not touch how it works
 * The call to cmd() in handler is very important to protect, as it
 * will on several commands be hackeable. We test with call_stack() and
 * there are no chances to penetrate.
 *
 * Verbs are: Command executed, abbreviated to verb and real verb.
 * real_verb is the name of the file, verb might be alias.
 *
 * @author Silbago
 */

#include <cmd.h>
#include <cmds.h>

/** @ignore start */
#ifndef CHANNELS_CMD
#define CHANNELS_CMD "/cmds/living/channels"
#endif
/** @ignore end */

#include <handlers.h>
#define SOUL_OBJECT "/obj/handlers/soul"

/**
 * If people typo and use these letters as first letter in command
 * we get an error. Time will show if we need to remove these letters
 * from verb executed ... :(
 */
#define SSCANF_CRASH (["(":"", ")":"", "[":"", "]":"", "+":"", "*":"",  "\\":"", "?":""])

private mapping hash;
private string real_verb,    // Full version of the verb, actual version
               expand_verb;  // Expanded from abbreviation
private nosave int save_all; // Flode

void cmd_make_hash();
int soul_com(string str, object me);

int dwep               () { return 0;        }
int clean_up           () { return 0;        }
int query_save_all     () { return save_all; }
void dest_me           () { destruct(TO);    }
mapping query_hash     () { return filter(hash, (: mappingp($2) :)); }
mapping query_aliases  () { return filter(hash, (: stringp($2) :));  }
mapping query_full_hash() { return copy(hash);                       }
string query_alias(string verb) {
    if ( !hash[verb] || !stringp(hash[verb]) )
        return 0;
    return hash[verb];
}

/**
 * Test if the command as such name exists, not alias.
 */
int cmd_exists(string cmd) {
    return (hash[cmd] && mappingp(hash[cmd]));
}

/**
 * Test if the command as such name exists, not alias.
 */
int alias_exists(string alias) {
    return (hash[alias] && stringp(hash[alias]));
}

/**
 * Return usergroups for the command.
 */
string *cmd_groups(string cmd) {
    if ( !cmd_exists(cmd) )
        return 0;
    return hash[cmd]["groups"];
}

void create() {
    SETEUID;
    cmd_make_hash();
}

/**
 * Tests wether the user is allowed to call the commmand.
 * @param ignore_verb if GIVEN verb is to be ignored and we regexp it
 * @return filepath of the command.
 * @author Silbago
 */
private varargs string legal_verb(string verb, object me, string tail, int ignore_verb) {
    string *groups;

    // Command executed has no command or alias, we search for
    // matches and test for what one the user has access to.
    //
    if ( !hash[verb] || ignore_verb ) {
        string *bits;

        // Scanning commands in this handler for match.
        // If nothing found, and it is an immortal. Look for personal cmd.
        //
        bits = regexp(keys(hash), "^" + verb);
        if ( ignore_verb )
            bits -= ({ verb });
        if ( !sizeof(bits) ) {
            string local_cmd;

            if ( !group_member(me, "creators") )
                return 0;

            local_cmd = "/w/" + geteuid(me) + "/commands/" + verb + ".c";
            if ( file_size(local_cmd) > 0 )
                return local_cmd;

            return 0;
        }

        // Testing matching commands for legal access.
        //
        bits = filter(bits, (: legal_verb($1, $(me), $(tail)) :));
        if ( !sizeof(bits) )
            return 0;

        bits -= ({ verb });
        if ( !sizeof(bits) )
            return 0;

        // If 3 letters we execute next possible match
        // If 1-2 letters we show matches
        //
        bits = sort_array(bits, 1);
        bits = filter(bits, (: legal_verb($1, $(me), $(tail)) :));
        if ( !sizeof(bits) )
            return 0;

        if ( strlen(verb) < 3 && sizeof(bits) > 1 ) {
            mixed adds;

            adds = map(me->query_commands(), (: $1[0] :));
            adds = regexp(adds, "^"+verb);
            bits = sort_array(adds+bits, 1); // Lacks matching cmds on items
            notify_fail(sprintf("%s%-*=s\n", "Ambiguous command: ",
                me->query_cols() - 20,
                nice_list(bits)+" match."), me);
            return 0;
        }

        verb = bits[0];
    }

    // We found a command, let's set real verb and groups
    //
    expand_verb = verb;
    real_verb = stringp(hash[verb]) ? hash[verb] : verb;
    groups = hash[real_verb]["groups"];
    me->add_static_property("last cmd", real_verb);

    if ( group_member(me, groups) || member_array("everyone", groups) != -1 )
        return hash[real_verb]["path"];

    if ( me->query_controllable() && member_array("CONTROLOBS", groups) != -1 )
        return hash[real_verb]["path"];

    return legal_verb(verb, me, tail, 1);
}

/**
 * Flode - temporary!
 * What is this ? Silbago
 */
int test_onetimedoer(object ob) {
    return ob != 0;
}

/**
 * Parses the syntax and the flags, fails if syntax is wrong. Gives the right
 * arguments to the target command object. Etc etc etc.
 * Finally if success on pattern(s) we do the call on the
 * command object with these parameters:
 *   object doer, mixed arguments, int match, mapping flags
 * @author Morth
 */
private int try_parse(string *av, mapping flags, string *syntax, object thisob,
       function fun, string failmsg, object ob, function filtfun, int flag)
{
  int i;
  mixed x;

  if(!syntax)
    syntax = ({ "<string>" });
  else if(stringp(syntax))
    syntax = ({ syntax });

  if(failmsg && failmsg != "HIDDEN")
    notify_fail(replace_string (failmsg, "$cmd$", av[0]), thisob);

  if ( m_sizeof(flags) ) {
     x = COMMAND_PARSER->get_flags(thisob, av[1 ..], flags);
     if ( arrayp(x) ) {
       av = ({ av[0] }) + x[0];
       flags = x[1];
     }
  }

  for(i = 0; i < sizeof(syntax); i++)
  {
    x = COMMAND_PARSER->expand_pattern (thisob, syntax[i], av[1..], ob, filtfun);
    if(arrayp(x))
    {
      if (flag & 1)
        x = x[0];
      else
        x = ({ av[0] }) + x;
      // Sending the call to the object with parameters, in order:
      // doer-ob, arguments, match, flags
      //
      if (stringp(fun))
      {
        if (call_other(ob, fun, thisob, x, i, flags))
          return 1;
      }
      else if ((*fun)(thisob, x, i, flags))
        return 1;
      if (!(flag & 2))
        return 0;
    }
    else if(!query_notify_fail(thisob) && failmsg != "HIDDEN")
    {
      if(x)
        notify_fail("There's a \"" + av[0] + "\" command, but couldn't find \""
              + av[x] + "\".\n", thisob);
     else if ( failmsg )
       notify_fail(failmsg);
     else
        notify_fail("Type %^BOLD%^syntax " + av[0] + "%^RESET%^ to see how to use %^BOLD%^" + av[0] + "%^RESET%^.\n", thisob);
    }
  }
  return 0;
}

/**
 * Function call from the living object.
 */
int cmd(string verb, string tail, object thisob) {
    int ret;
    object ob, e;
    string file,   // File of the command object
           bcmd,   // Yeah
           *av;    // Morth command parser parameters
    mixed syntax, flags, x, blocked;
    mapping cmd;

    // If people typo and use strange letters, sscanf will not stand for it
    //
    verb = replace_string(verb, SSCANF_CRASH);
    if ( !strlen(verb) ) {
        notify_fail("Error in verb executed.\n", thisob);
        return 0;
    }

    seteuid(geteuid(thisob));

    // Who the hell did this. No control on npcs.
    //
    if ( interactive(thisob) && call_user() != geteuid(thisob) ) {
        if ( member_array(verb, DEFENSELESS_COMMANDS) == -1 ) {
            hack("Command hacked: " + verb+ ".\n"
                 "By euid stack " + call_user() + " on euid " + geteuid(thisob) + ".\n"
                 "Command \"" + verb +"\", tail \"" + tail + "\"");

            if ( group_member(thisob, "creators") )
                return 0;
        }
    }

    // Flode 200104 - onetime commands
    // Silbago 130206 - Silbago decapitated for new cmd handler
    //
    if ( test_onetimedoer(thisob) ) {
        if ( ONETIME_HANDLER->cmd(verb, tail, thisob) )
            return 1;
    }

    blocked = thisob->query_property ("blocked commands");
    if (blocked)
    {
      blocked = evaluate (blocked[bcmd], bcmd, thisob);
      if (stringp (blocked))
      {
        thisob->message (STDERR, blocked, -1, verb, thisob);
        return 1;
      }
      else if (blocked)
      {
        thisob->message (STDERR, "You can't use $o right here or now.\n", -1, verb);
        return 1;
      }
    }

    if(tail)
      av = ({verb}) + explode(tail, " ");
    else
      av = ({verb});

    // Check commands around the player.
    e = ENV(thisob);
    if(e)
    {
      blocked = e->query_property ("blocked commands");
      if (blocked)
      {
        blocked = evaluate (blocked[bcmd], bcmd, thisob);
        if (stringp (blocked))
        {
          thisob->message (STDERR, blocked, -1, verb, thisob);
          return 1;
        }
        else if (blocked)
        {
          thisob->message (STDERR, "You can't use $o right here or now.\n", -1, verb);
          return 1;
        }
      }
      if (bcmd == verb)
        bcmd = 0;

      // First objects in room.
      x = all_inventory(e)->query_cmd(verb) - ({ 0 });
      if (bcmd)
        x += all_inventory (e)->query_cmd (bcmd) - ({ 0 });
      foreach(cmd in x)
      {
        if(try_parse(av, cmd["flags"], cmd["syntax"], thisob, cmd["function"],
              cmd["failmsg"], cmd["object"], cmd["filter"], cmd["add_action"]))
          return 1;
      }

      // Then the room itself
      if((cmd = e->query_cmd(verb) || e->query_cmd (bcmd)))
      {
        if(try_parse(av, cmd["flags"], cmd["syntax"], thisob, cmd["function"],
              cmd["failmsg"], cmd["object"], cmd["filter"], cmd["add_action"]))
          return 1;
      }
    }
    else if (bcmd == verb)
      bcmd = 0;

    // Then inventory.
    x = all_inventory(thisob)->query_cmd(verb) - ({ 0 });
    if (bcmd)
      x += all_inventory (thisob)->query_cmd (bcmd) - ({ 0 });
    foreach(cmd in x)
    {
      if(try_parse(av, cmd["flags"], cmd["syntax"], thisob, cmd["function"],
            cmd["failmsg"], cmd["object"], cmd["filter"], cmd["add_action"]))
        return 1;
    }

    // Running channels check
    //
    if ( !hash[verb] && CHANNELS_CMD->_cmd(tail, thisob, verb) )
        return 1;

    // Get the filepath of the command, and set alias
    // Alias is the real verb in any case.
    // Not determined where to handle onetime and soul
    //
    if ( !(file=legal_verb(verb, thisob, tail)) )
        return 0;
    verb = expand_verb;

    // Counting usage, rough method to determine what we set to nodest
    //
    hash[real_verb]["count"]++;

    // Ok this is a really dirty hack, really.
    //
    if ( verb == "nastiness" )
        cmd("masters", ":just ran a nastiness.", thisob);

    if ( !(ob=load_object(file)) )
        return 0;

    if ( ob->query_do_clone() )
        ob = clone_object(file);

    // Unix type command parsing, get all ready before we kick off.
    // function _cmd() in no pre-parsing, and _pcmd in Parsed.
    //
    if ( tail )
        av = ({ verb }) + explode(tail, " ");
    else
        av = ({ verb });

    syntax = ob->query_syntax(thisob);
    if ( syntax ) {
        if ( stringp(syntax) )
            syntax = ({ syntax });

        flags = ob->query_flags(thisob);
/*      Why did I remove this ?
        if ( flags ) {
            x = COMMAND_PARSER->get_flags(thisob, av[1 ..], flags);
            if ( arrayp(x) ) {
                av = ({ av[0] }) + x[0];
                flags = x[1];
            } else
                flags = ([ ]);
        }
*/

        ret = try_parse(av, flags, syntax, thisob,
          (: call_other, ob, "_pcmd" :), ob->query_usage(), ob,
          ob->query_filter(), ob->query_parse_flags ());
    } else
        ret = (int)ob->_cmd(tail, thisob, verb);

    if ( !ob->query_do_not_dest() )
        ob->dest_me();

    return ret;
}

/**
 * Method builds the command and alias hash tables.
 */
void cmd_make_hash() {
    int pl;
    mixed bits;
    mapping dir_hash;

    // Get the directories where the commands lives
    //
    seteuid("Root");
    dir_hash = ([ ]);
    bits = filter(get_dir("/cmds/", -1), (: $1[1] == -2 :));
    bits = map(bits, (: $1[0] :)) - ({ ".", "..", "handler", "onetime", "ATTIC"});
    foreach(string dir in copy(bits)) {
        bits = explode(dir, "+") - ({ "+" });
        bits = filter(bits, (: group_exists :));
        if ( !sizeof(bits) )
            continue;

        // Throwing in support code for 'players'
        //
        if ( member_array("players", bits) != -1 )
            bits |= ({ "creators" });

        dir_hash["/cmds/" + dir + "/"] = bits;
    }

    // Legacy, adding in support for the old structure
    /*
           Example(s) here as this isnt used at the time being.
    */
    dir_hash["/cmds/living/"]  = ({ "everyone" });
    dir_hash["/cmds/mortal/"]  = ({ "players", "creators", "CONTROLOBS", });

    // Soft method for the CPU if player initiated.
    //
    pl = !TP || !group_member(TP, "players") ? 0 : 1;

    hash = ([ ]);
    reset_eval_cost();
    foreach(string dir, string *groups in dir_hash) {
        string alias_file, *files = get_dir(dir + "*.c");
        int i;

        if ( !sizeof(files) )
            continue;

        files = map(files, (: $1[0..<3] :));
        if ( !sizeof(files) )
            continue;

        foreach(string file in files) {
            object cmd_ob;
            int loaded_object;

            if ( hash[file] ) {
                warning("Parsing two instances of command " + file + "\n"
                        "Source 2 ignored: " + dir);
                continue;
            }

            loaded_object = find_object(dir + file) ? 1 : 0;
            catch( (cmd_ob=load_object(dir + file)) );
            reset_eval_cost();
            if ( !cmd_ob || !cmd_ob->query_is_cmd() ) {
                if ( cmd_ob && !pl )
                    call_out("delayed_dest_me", 10 + random(10), cmd_ob);
                continue;
            }

            hash[file] = ([
                "path"   : dir + file,
                "groups" : groups,
                "count"  : 0,
            ]);

            i++;
            if ( !loaded_object && !pl )
                call_out("delayed_dest_me", 10 + random(10), cmd_ob);
        }

        // Loading aliases if we have commands here.
        //
        if ( !i )
            continue;

        alias_file = read_file(dir + "_CMD_ALIASES");
        if ( !strlen(alias_file) )
            continue;

        foreach(string line in explode(alias_file, "\n")) {
            string *aliases;

            line = replace_string(line, "\t", " ");
            if ( line[0..5] != "alias " )
                continue;

            aliases = explode(line[6..], " ");
            if ( sizeof(aliases) < 1 )
                continue;

            foreach(string alias in aliases[1..])
                if ( hash[aliases[0]] ) {
                    if ( hash[alias] ) {
                        warning("Parsing two instances of alias " + alias + "\n"
                                "Source 2 ignored: " + dir);
                        continue;
                    }

                    hash[alias] = aliases[0];
                } else
                    warning("alias " + alias + " not loaded for command " +
                             aliases[0] + "\n"
                            "Source alias file: " + dir + "_CMD_ALIASES");
        }
    }

    // If you look closely, you'll see here and above with destruction
    // that we carefully load balance loading and dest_me().
    //
    call_out("delayed_load_object", 2, ONETIME_HANDLER, "make_hash");

    if ( !pl ) {
        call_out("delayed_load_object", 4, GENERATE_CMDS, "bingbong");
        call_out("delayed_load_object", 6, CMDS_HELP,     "handler_rehashed");
    }

    seteuid("CMD");
}

protected void delayed_dest_me(object ob) {
    if ( ob )
        ob->dest_me();
}

protected void delayed_load_object(string path, string fun) {
    call_other(path, fun);
}

/**
 * Method finds all aliases that uses this verb/command
 * @author Silbago
 */
string *query_verb_aliases(string verb) {
    mixed bits = filter(hash, (: $2 == $(verb) :));
    if ( !bits )
        return 0;
    return sort_array(keys(bits), 1);
}

/**
 * Method used by help command to find filepath
 */
string find_cmd(string cmd) {
    // Supporting aliases
    if ( hash && stringp(hash[cmd]) )
        cmd = hash[cmd];

    if ( !hash[cmd] || !mappingp(hash[cmd]) ) {
        if (    TP && group_member(TP, "creators")
             && file_size("/w/"+geteuid(TP)+"/commands/"+cmd+".c") > 0 )
            return "/w/"+geteuid(TP)+"/commands/"+cmd;
        return 0;
    }

    return hash[cmd]["path"];
}

/**
 * Added by Baldrick.
 */
int soul_com(string str, object me) {
    string str1, str2;
    int i;

    if ( sscanf(str,"%s %s", str1, str2) != 2 )
        str1 = str;

    if ( !me->query_property("nosoul") ) {
        if ( !load_object(SOUL_OBJECT) ) {
            me->msg("Soul errors!  Notify an immortal.\n");
            me->msg("Use nosoul to turn the soul back on when it is fixed.\n");
            me->add_property("nosoul",1);
            return 0;
        }

        i = SOUL_OBJECT->soul_command(str1, str2, me);

        /* souls are trivial */
        if ( i )
            me->set_trivial_action();

        return i;
    }

    return 0;
}

void set_save_all() {
    if ( base_name(previous_object()) == "/secure/toys/triggers/nastiness" )
        save_all = 1;
}

void reset_save_all() {
    if ( base_name(previous_object()) == "/secure/toys/triggers/nastiness" )
        save_all = 0;
}

/**
 * Method to get help information directly from the command
 * Some prefers having help in query functions in command, some
 * writes it manually.
 * @author Silbago
 */
string do_help(string com, object me) {
    object ob;
    string usage, help, file, retval;

    com = replace_string(com, SSCANF_CRASH);
    if ( !(file=legal_verb(com, me, 0)) )
        return 0;
    if ( !(ob=load_object(file)) )
        return 0;

    help = ob->query_short_help(me);
    usage = ob->query_usage(me);
    if ( !usage && !help )
        return 0;

    seteuid("Root");
    retval = HELP_CMD->make_help_header(file, last_touched(file+".c"));
    if ( !retval )
        retval = "";
    if ( usage ) {
        retval += "Usage\n";
        com = "";
        if ( usage )
            com += usage;
        retval += "    "+com+"\n\n";
    }

    return retval + help;
}
