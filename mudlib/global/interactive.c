/**********************************************************************\
*                                                                      *
* Copyright (c) 2003 The Shattered Realms                              *
* All rights reserved.                                                 *
*                                                                      *
* Use and distribution of this software are governed by the terms and  *
* conditions of the Shattered Realms Software License ("LICENSE").     *
*                                                                      *
* If you did not receive a copy of the license, it may be obtained     *
* online at http://shatteredrealms.sourceforge.net/license.html        *
*                                                                      *
\**********************************************************************/

/* $Id: interactive.c,v 1.1.2.45 2005/12/09 07:07:03 sr Exp $ */

/**
 * @main
 *
 * This file handles input and output between players and the mud.
 * It does handle _all_ output, regardless of what you produced it with.
 * It now also handles all input.
 *
 * @author Morth
 */

#include <handlers.h>
#include <input.h>
#include <keys.h>

protected nosave string new_line = "";
protected nosave object *filters = ({}), *snoopers = ({});
nosave mapping colour_map;
nosave string stored_chars = "", last_prompt;
nosave string stored_output = "";
nosave object snoopee;
nosave mixed *inputs = ({ });
nosave int input_echo;

string term_name = "ansi"; // Was"dumb" since beginning of time. Changed june 2011
int cols = 80, rows = 24;
int splitscreen;

void write_prompt ();

mapping cmap() { return colour_map; }
mixed query_inputs() { return copy(inputs); }

/**
 * Sets the terminal type.
 */
int set_term_type(string str) {
    if ( !str )
        return 0;

    if ( str == term_name )
        return 1;

    if ( member_array(str, (string *)TERM_HANDLER->query_term_types()) == -1 )
        return 1;

    colour_map = (mapping)TERM_HANDLER->set_term_type(str);
    term_name = str;
    return 1;
}

/**
 * Returns line-break argument, or not.
 */
string query_new_line() {  return new_line;  }

/**
 * Support for the options command.
 * @author Silbago
 */
int query_new_line_on() {  return new_line != "";  }

/**
 * Unsecure and uncontrolled, use it as it should be used.
 * Set amount of lines you want to split your screen, from bottom
 * and up. Parameter 0 shuts it off.
 *
 * Splitscreen will then be set up for the main command prompt, and
 * not for deeper filters. For the 'visual' editors the command
 * prompt (ctrl-z) will utilize this value.
 * - In other words, the inputs that wants to -> does.
 * @author Silbago
 * @author Morth
 */
void set_splitscreen(int lines) {
    inputs[0][2] = lines; // Inserting on default input
    splitscreen = lines;
}

int query_splitscreen() { return splitscreen; }

/**
 * This is used, but what does it do :) ?
 */
int query_single_target (object ob) {
    object e = ENV (ob);
    object *ap;
    mixed x;

    if (!e || e != ETO)
        return 0;

    ap = all_present (ob->query_name (), e);
    x = TO->query_it_them ();
    if ( sizeof (ap) == 1 )
        return 1;
    else if ( arrayp (x) ) {
        ap = filter (ap, (: member_array ($1, $2) != -1 :), x);
        if (sizeof (ap) <= 1 && member_array (ob, x) != -1)
        return 1;
    }
    return 0;
}

/**
 * Parses a message for colors and objects.
 * cl is describes under message(). If it contains NOPARSE, no parsing
 * is done. If it contains CHAT, only color parsing is done.
 *
 * ime indicates the actor(s). It's either an object/object * or and
 * index into the obs array.
 * 
 * message is searched for occurrencies of special tokens of the form
 * $<letter>:
 *    $o        The name of the next item in obs, which can be an object,
 *              an array of objects or a string.
 *    $d        Determinate article version of $o. Usually means the
 *              name is prepended with "the " (see set_det_article()).
 *    $i        Indeterminated article version of $o. Usually means the
 *              name is prepended with "a " or "an " (see set_article()).
 *    $q        Same as $o but always display name even if invisible.
 *    $v        An attampt at making a target $ (v is for victim).
 *              Like $i but will use a reflexive pronoun if the argument
 *              is the same as the actor.
 *
 *    $p        Personal pronoun of the next item in obs.
 *    $t        Objective pronoun of the next item in obs.
 *    $w        Possessive pronoun of the next item in obs.
 *
 *    $#        A numeric number. Can't use $o since it must error on 0.
 *    $l        A text (long) number.
 *
 *    All above can be capitaliced ($O, $D etc) and then the text will be too
 *    (for start of sentence).
 *
 *    $s        "" if the actor is TO, "s" otherwise.
 *    $e        "" if the actor is TO, "es" otherwise.
 *    $y        "y" if the actor is TO, "ies" otherwise.
 *    $r        "are" if the actor is TO, "is" or "are" otherwise.
 *
 *    The above will work fine even if the actor is multiple objects.
 *
 *    $_        Is not replaced by anything but consumes an item in obs.
 *              Use this to put the actor in obs when it's not in the
 *              message (somewhat obsolete, use the numbered indexes
 *              instead).
 *
 *    $$        A single $. 
 *
 *    $.        End replacements. All $ after this one will be kept.
 *
 * You can also give a number between the $ and the letter. In that case
 * the given index into obs will be used instead of the next one. This way
 * each object only has to be given once. After a numbered object, an
 * unnumbered will use the index right after that (for example with "$2o $o"
 * the second $o will use index 3).
 *
 * Example parse_message(GENERAL, "$O slap$s $o hard.\n", 0, ({ TO, foe1 }));
 *
 * @author Morth
 */
string parse_message(string cl, string message, mixed ime, mixed *obs) {
    int me;
    string *strs, str, s;
    int i, j;
    mixed x;
    mixed actors; // Input sanity needs fix
//  object *actors;
    int warn_ime = 0;
  
    if ( strsrch(cl, NOPARSE) >= 0 || strsrch(cl, SNOOP) >= 0 )
        return message;
  
    if ( !colour_map )
        colour_map = TERM_HANDLER->set_term_type(term_name);
  
  // We only parse channel messages for color.
  if(strsrch(cl, CHAT) >= 0)
    return terminal_colour(message + "%^RESET%^", colour_map);
  
  strs = explode("&"+message,"$");
  if(!sizeof(strs))
    return message;
  
  if(ime == -1)
  {
    me = (TP == TO);
    actors = ({ TP });
    warn_ime = 1;
  }
  else if (objectp (ime))
  {
    me = (ime == TO);
    actors = ({ ime });
  }
  else if (arrayp (ime))
  {
    if (sizeof (ime) != 1)
      me = 1;
    else
      me = (ime[0] == TO);
    actors = ime;
  }
  else if (ime >= sizeof (obs))
  {
    me = 0;
    actors = ({ });
  }
  else if(arrayp(obs[ime]))
  {
    if(sizeof(obs[ime]) != 1)
      me = 1;
    else
      me = (obs[ime][0] == TO);
    actors = obs[ime];
  }
  else
  {
    me = (obs[ime] == TO);
    actors = obs[ime];
  }
  
  message = strs[0][1..];
  i = 0;
  for(j = 1; j < sizeof(strs); j++)
  {
    str = strs[j];
    if (str[0] >= '0' && str[0] <= '9')
    {
      i = 0;
      do
      {
        i = 10 * i + str[0] - '0';
        str = str[1 ..];
      } while (str[0] >= '0' && str[0] <= '9');
    }
    switch(str[0])
    {
    case 'O':
    case 'o':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $o (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
        s = query_multiple_short(obs[i], TO);
      else if(stringp(obs[i]))
        s = obs[i];
      else if(obs[i] == TO)
        s = "you";
      else
        s = obs[i]->short();
      if (!sizeof (s))
        s = "someone";
      i++;
      if(str[0]=='O')
        s = capitalize(s);
      break;
    case 'Q':
    case 'q':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $q (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
        s = query_multiple_short(obs[i]->query_short (), TO);
      else if(stringp(obs[i]))
        s = obs[i];
      else if(obs[i] == TO)
        s = "you";
      else
        s = obs[i]->query_short();
      if (!sizeof (s))
        s = "someone";
      i++;
      if(str[0]=='Q')
        s = capitalize(s);
      break;
    case 'V':
    case 'v':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $v (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
      {
        x = actors - obs[i];
        if (sizeof (x) == 1)
        {
          if (x[0] == TO && sizeof (actors) > 1)
            x = ({ "you" }) + obs[i] - x;
          else if (x[0] == TO)
            x = ({ "yourself" }) + obs[i] - x;
          else
            x = ({ x[0]->query_objective () + "self" }) + obs[i] - x;
        }
        else if (sizeof (x))
        {
          if (member_array (TO, x) != -1)
            x = ({ "yourselves" }) + obs[i] - x;
          else
            x = ({ "themselves" }) + obs[i] - x;
        }
        else
          x = obs[i];
        s = query_multiple_short(x, TO);
      }
      else if(stringp(obs[i]))
        s = obs[i];
      else if(obs[i] == TO)
      {
        if (sizeof (actors) == 1 && actors[0] == TO)
          s = "yourself";
        else
          s = "you";
      }
      else if (sizeof (actors) == 1 && actors[0] == obs[i])
        s = obs[i]->query_objective () + "self";
      else if (query_single_target (obs[i]))
        s = determinate (obs[i]);
      else
        s = articulate (obs[i]);
      if (!sizeof (s))
        s = "someone";
      i++;
      if(str[0]=='V')
        s = capitalize(s);
      break;
    case 'D':
    case 'd':
      if (i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $d (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if (arrayp (obs[i]))
        s = query_multiple_short (obs[i], TO); // Fix later.
      else if (stringp(obs[i]))
        s = obs[i]; // No action on strings.
      else if (obs[i] == TO)
        s = "you";
      else if (obs[i]->short ())
        s = determinate (obs[i]);
      else
        s = "someone";
      i++;
      if (str[0] == 'D')
        s = capitalize (s);
      break;
    case 'I':
    case 'i':
      if (i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $i (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if (arrayp (obs[i]))
        s = query_multiple_short (obs[i], TO); // Fix later.
      else if (stringp(obs[i]))
        s = obs[i]; // No action on strings.
      else if (obs[i] == TO)
        s = "you";
      else if (obs[i]->short ())
      {
        if (query_single_target (obs[i]))
          s = determinate (obs[i]);
        else
          s = articulate (obs[i]);
      }
      else
        s = "someone";
      i++;
      if (str[0] == 'I')
        s = capitalize (s);
      break;
    case 'p':
    case 'P':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $p (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
      {
        if(member_array(TO, obs[i]) >= 0)
          s = "you";
        else
          s = "they";
      }
      else if(obs[i] == TO)
        s = "you";
      else
        s = obs[i]->query_pronoun();
      i++;
      if(str[0]=='P')
        s = capitalize(s);
      break;
    case 't':
    case 'T':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $t (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
      {
        if(member_array(TO, obs[i]) >= 0)
          s = "you";
        else
          s = "them";
      }
      else if(obs[i] == TO)
        s = "you";
      else
        s = obs[i]->query_objective ();
      i++;
      if(str[0]=='T')
        s = capitalize(s);
      break;
    case 'w':
    case 'W':
      if(i >= sizeof(obs))
        s = "$" + str[0 .. 0];
      else if (!obs[i])
        error ("Bad argument to $w (argument " + (i + 1) +
              "). Expected object | object * | string, got 0.");
      else if(arrayp(obs[i]))
      {
        if(member_array(TO, obs[i]) >= 0)
          s = "your";
        else
          s = "their";
      }
      else if(obs[i] == TO)
        s = "your";
      else
        s = obs[i]->query_possessive();
      i++;
      if(str[0]=='W')
        s = capitalize(s);
      break;
    case '#':
      if (i >= sizeof (obs))
        s = "$#";
      else if (intp (obs[i]))
        s = "" + obs[i];
      else if (floatp (obs[i]))
        s = sprintf ("%.2f", obs[i]);
      else
        error ("Bad argument to $# (argument " + (i + 1) +
            "). Expected int | float, got: " + stringify (obs[i]));
      i++;
      break;
    case 'l':
    case 'L':
      if (i >= sizeof (obs))
        s = "$" + str[0 .. 0];
      else if (intp (obs[i]))
        s = add_num ("", obs[i]);
      else
        error ("Bad argument to $l (argument " + (i + 1) +
            "). Expected int, got: " + stringify (obs[i]));
      i++;
      if (str[0] == 'L')
        s = capitalize (s);
      break;
    case 's':
      if (warn_ime)
        warning ("$s/$e/$y/$r used without explicit actor (add object argument before the message string).\n", 2);
      if(me)
        s = "";
      else
        s = "s";
      break;
    case 'e':
      if (warn_ime)
        warning ("$s/$e/$y/$r used without explicit actor (add object argument before the message string).\n", 2);
      if(me)
        s = "";
      else
        s = "es";
      break;
    case 'y':
      if (warn_ime)
        warning ("$s/$e/$y/$r used without explicit actor (add object argument before the message string).\n", 2);
      if(me)
        s = "y";
      else
        s = "ies";
      break;
    case 'r':
      if (warn_ime)
        warning ("$s/$e/$y/$r used without explicit actor (add object argument before the message string).\n", 2);
      if (me)
        s = "are";
      else
        s = "is";
      break;
    case 0:
      if(j < sizeof(strs) - 1)
        s = "$$" + strs[++j];
      else
        s = "$";
      break;
    case '.':
      if(j < sizeof(strs) - 1)
        s = str[1..] + "$" + implode(strs[j + 1..], "$");
      else
        s = str[1..];
      str = ".";
      j = sizeof(strs);
      break;
    default:
      s = "$" + str[0..0];
      break;
    }
    message += s + str[1..];
  }
  return terminal_colour(message + "%^RESET%^", colour_map);
}

/**
 * This function sends messages to the player. The message is parsed
 * with parse_message(), and is also filtered unless cl contains
 * NOFILTER.
 * 
 * You use it by selecting one or more classes from GENERAL, NOPARSE
 * CHAT, NOFILTER, STDERR. To use more than one just add them together
 * (for example GENERAL + NOFILTER). In addition you can specify a
 * specific user group that the message will be filtered to by
 * putting GROUP + "groupname" (for example GROUP + "creators").
 * You can only filter to one group. All that goes into the cl
 * argument.
 * Use STDERR when displaying error messages to commands. This is used
 * to avoid piping. notify_fail () uses this.
 * message, ime and obs are as described in parse_message, but notice
 * that obs have ..., that is, each additional argument is one entry in
 * obs.
 * 
 * Example: ({ me, friend1, friend2 })->message(GENERAL,
 *             "$O tell $o: hello\n", 0, ({me, friend1}), friend2);
 *
 * @seealso i_message
 * @seealso e_message
 * @author Morth
 */
void message(string cl, string message, mixed ime, mixed *obs...) {
  object ob;
  int i, j;
  
  if((i = strsrch(cl, GROUP)) >= 0)
  {
    i += sizeof(GROUP);
    j = strsrch(cl[i..], '|');
    if(j == -1)
      j = sizeof(cl);
    else
      j += i;
    if(!group_member(TO, cl[i .. j]))
      return;
  }
  
  message = parse_message(cl, message, ime, obs);
  if(strsrch(cl, NOFILTER) == -1)
  {
    filters -= ({0});
    foreach(ob in filters)
    {
      message = ob->filter_message(cl, message);
      if(!message)
        return;
    }
  }

  if(strsrch(cl, SNOOP) >= 0)
    message="]"+message;
  else
  {
    snoopers -= ({0});
    snoopers->message(cl + SNOOP, message, ime);
  }
  
  // Quote IAC characters.
  message = replace_string (message, "\xFF", "\xFF\xFF");
  
  i = 0;
  if ( sizeof(inputs) && sizeof(inputs[<1]) >= 3 )
    i = inputs[<1][2];  //TO->query_property ("splitscreen");
  if (i && strsrch(cl, NOFILTER) == -1)
  {
    stored_output += message;
    if (input_echo && last_prompt)
    {
      stored_output = last_prompt + stored_output;
      last_prompt = 0;
    }
    i = rows - (i-1); // Morth had - 1 (turn -i-1 ?) here
    j = strsrch (stored_output, '\n', -1);
    if (j >= 0)
    {
      receive (ESC "7" ESC "[1;" + i + "r" ESC "[" + (i-1) + "B"
          + (input_echo? "\n"/*ESC "[2K"*/ : "\n") + stored_output[0 .. j - 1]
          + ESC "[r" ESC "8");
      stored_output = stored_output[j + 1 ..];
    }
    input_echo = 0;
  }
  else
    receive(message);
}

/**
 * This is just a simpler form of message(). You can easily use this instead
 * of write(), tell_object(), etc. If the first argument is an object or array,
 * that will be considered the actor(s). The second argument is then the
 * message. If no actor is required you can give the message as the first
 * argument.
 * It will call message() with class set to GENERAL and ime to the actor(s).
 *
 * Example: me->msg("You failed to do that.\n");
 *
 * @seealso i_msg
 * @seealso e_msg
 * @author Morth
 */
void msg(mixed *message...)
{
  mixed ime = -1;
  
  if (objectp (message[0]) || arrayp (message[0]))
  {
    ime = message[0];
    message = message[1 ..];
  }
  message(GENERAL, message[0], ime, message[1..]...);
}

/**
 * For the message efun. Might be useful to someone.
 * If mess is an array, the first object is the message and the rest the
 * arguments.
 * @author Morth
 */
void receive_message (string cl, mixed mess) {
    if (arrayp (mess))
        message (cl, mess[0], -1, mess[1..]...);
    else
        message (cl, mess, -1);
}

/**
 * Adds the caller as a filter for all messages this object receives.
 * The filter will be a call to the function
 * filter_message(string cl, string message)
 * on the object calling this function, which should return the message
 * to actually be displayed, if any. filter_message() is called after all
 * parsing and can thus contain raw color codes and other stuff.
 *
 * Generally, only special objects can add filters. You can however add a filter
 * to yourself with an object in your home directory.
 *
 * @author Morth
 */
int add_filter() {
    object ob = previous_object();
  
/*  Ok security breach !
    if ( geteuid (ob) != geteuid() && !is_group_call("lords") )
        return 0;
*/
  
    filters += ({ ob });
    return 1;
}

void remove_filter() {
    filters -= ({ previous_object() });
}

/**
 * Adds a snooper to this object. The snooper must be a player.
 * This is now used instead of the snoop() efun (which has been removed).
 * quiet will only work for lords.
 *
 * @author Morth
 */
int add_snooper(object ob, int quiet)
{
  if(!interactive(ob))
    return 0;
  snoopers += ({ ob });
  if(!quiet || !group_member(ob, "lords") || group_member(TO, "lords"))
    msg ("You are being snooped by " + capitalize(geteuid(ob)) + ".\n");
  return 1;
}

void remove_snooper(object ob) {
    if ( !is_group_call("lords") && group_member(ob, "lords") )
        return;
    snoopers -= ({ ob });
}

void set_snoopee(object ob) { snoopee = ob; }
object query_snoopee() { return snoopee; }

/**
 * Applied from user client through connection
 */
void terminal_type(string term) {
    if (    !TO->query_property("auto term type")
         && TO->query_name() != "object" )
        return;

    switch(lower_case(term)) {
    case "xterm":
        term_name = "xterm";
        break;
    case "vt100":
    case "ansi":
    case "screen-w":
        term = "ansi";
        break;
    case "zmud":
    case "cmud":
        term = "ansi"; // ZMud needs seperate terminals ... !!!!
        break;
    default:
        term = "dumb";
        break;
    }

    if ( TO->set_term_type(term) )
        tell_object(TO, "Your machine told " + mud_long_name() + " that you are using "
            + term + " type terminal.\n");
}

/**
 * This function is called by the driver each time a user changes
 * his window size. That way window wize can be automatic.
 * This is disabled by default for the user, he can enable it
 * by "cols automatic" and "rows automatic".
 * @author Silbago
 */
void window_size(int c, int r) {
    if ( c > 0 && TO->query_property("automatic columns") )
        cols = c - 1;
    if ( r > 0 && TO->query_property("automatic rows") )
        rows = r - 1;
    if ( TO->query_property("automatic cols") || TO->query_property("automatic rows") )
        tell_object(TO, "Your machine told our machine that your terminal "
            "has " + rows + " rows and " + cols + " columns.\n");

    filters -= ({ 0 });
    filters->window_size(c, r);
}

protected void nag_about_winsize() {
    msg("Your machine told our machine that your terminal has " + rows +
        " rows and " + cols + " columns.\n");
}

protected void net_dead () {
  filters -= ({ 0 });
  filters->net_dead ();
}

int set_term_name(string term) {
    if ( !is_user_call (TO) )
        return 0;
    if ( member_array(term, TERM_HANDLER->query_term_types()) != -1 ) {
        colour_map = TERM_HANDLER->set_term_type(term);
        term_name = term;
        return 1;
    }
    return 0;
}

string query_term_name() { return term_name; }
int query_cols        () { return cols;      }
int query_rows        () { return rows;      }
void set_cols    (int i) { cols = i;         }
void set_rows    (int i) { rows = i;         }

/**
 * Add an input receiver for this player, just like input_to() or get_char()
 * but better. fun is the function to be called, it can be either a string
 * or a function pointer or an array with an object and a string. mode is the
 * sum of zero or more out of IMODE_HIDDEN, IMODE_NOESC, IMODE_CHAR,
 * IMODE_ONCE and IMODE_PROMPT. Splitscreen value (not much used).
 * The rest of the arguments are passed along to the function.
 *   - function/pointer
 *   - charmode
 *   - splitscreen
 *   - arguments
 *
 * The definition of the function should be
 * void fun (string str)
 *     plus parameters for any extra arguments you pass to the
 *     add_input() function.
 *
 * IMODE_HIDDEN means the text the user is typing should not be shown on
 *     screen.
 * IMODE_NOESC means you can't use ! to enter a command instead. This is
 *     subject to some security checking (you can only add it to yourself).
 * IMODE_CHAR means call directly after one character is received. This one
 *     will extract special characters as well, so if the user presses the up
 *     key for example ESC "[A" will be sent to the fun. See <keys.h> for some
 *     useful defines. It implies IMODE_NOESC, and is thus checked for
 *     security as well.
 * IMODE_ONCE is used to simulate input_to(). It means that after this input
 *     is used once, delete it.
 * IMODE_PROMPT is used to replace the default prompt. When it's time to
 *     write the prompt, write_prompt() will be called on the object 
 *     receiving the function call, with the player who's prompt should be
 *     written as the first arguments, and the extra arguments after that.
 *     If this function is not defined, no prompt will be written.
 * IMODE_HISTORY means the lines should be recorded in the history. Doesn't
 *     work with IMODE_CHAR.
 *
 * Note: You'll have to #include <input.h>
 *
 * @seealso <input.h>
 * @seealso <keys.h>
 * @author Morth
 */
varargs void add_input (mixed fun, int mode, int split, mixed *args...) {
  object ob;
  
  ob = call_stack (1)[<1];
  if ( origin () != "local" && ob != TO && !group_member (ob, "lords") ) {
    secure_log_file ("unauth", "add_input called on " + TO->query_name () + " by "
        + ob->query_name () + ".\n");
    error ("add_input called by other user.");
  }
  
  if (functionp (fun))
    ob = function_owner (fun);
  else if (arrayp (fun))
    ob = fun[0];
  else if (origin () == "local")
    ob = TO;
  else
    ob = previous_object ();
  
  if (mode & IMODE_CHAR)
    mode |= IMODE_NOESC;

  if ((mode & IMODE_NOESC) && geteuid (ob) != getuid ())
    mode &= ~(IMODE_NOESC | IMODE_CHAR);
  
  if (mode & IMODE_CHAR)
    mode &= ~IMODE_HISTORY;
  
  if (stringp (fun))
    fun = ({ ob, fun });
  if (arrayp (fun))
    fun = bind ((: call_other, fun[0], fun[1] :), fun[0]); // Fix for static.
  else if (!functionp (fun))
    error ("Invalid argument 1 to add_input ()");

  inputs += ({ ({ fun, mode, split, args}) });
}

/**
 * Compatibility/conveniance function.
 * Calls add_input with IMODE_ONCE | IMODE_PROMPT set in addition to modes given.
 *
 * Original documentation from MudOS
 * Name
 *   input_to() - causes next line of input to be sent to a specified
 *   function
 * 
 * Synopsis
 *   varargs void input_to(string fun, int flag, ...);
 * 
 * Description
 *   Enable next line of user input to be sent to the local function
 *   `fun' as an argument. The input line will not be parsed by the
 *   driver.
 * 
 *   Note that input_to is non-blocking which means that the object
 *   calling input_to does not pause waiting for input.  Instead the
 *   object continues to execute any statements following the input_to.
 *   The specified function 'fun' will not be called until the user
 *   input has been collected.
 * 
 *   If "input_to()" is called more than once in the same execution,
 *   only the first call has any effect.
 * 
 *   If optional argument `flag' is non-zero, the line given by the
 *   player will not be echoed, and is not seen if snooped (this is
 *   useful for collecting passwords).
 * 
 *   The function 'fun' will be called with the user input as its first
 *   argument (a string). Any additional arguments supplied to input_to
 *   will be passed on to 'fun' as arguments following the user input.
 * 
 * See also
 *   call_other(3), get_char(3).
 *
 * @author Morth
 */
varargs void input_to(mixed fun, int mode, mixed *args...) {
  object ob;
  
  ob = call_stack (1)[<1];
  if (origin () != "local" && ob != TO && !group_member (ob, "lords") && group_member(TO, "creators"))
  {
    secure_log_file ("unauth", "input_to called on " + TO->query_name () + " by "
        + ob->query_name () + ".\n");
    error ("input_to called by other user.");
  }
  
  if (origin () == "local")
    ob = TO;
  else
  {
    ob = previous_object();
    if ( ob == load_object("/secure/simul_efun") )
      ob = previous_object(1);
  }
  
  if (stringp (fun))
    fun = ({ ob, fun });
  
  add_input (fun, mode | IMODE_ONCE | IMODE_PROMPT, 0, args...);
}

/**
 * Compatibility/conveniance function.
 * Calls add_input with IMODE_ONCE | IMODE_PROMPT | IMODE_CHAR set in
 * addition to modes given.
 *
 * @author Morth
 */
varargs void get_char(mixed fun, int mode, mixed *args...) {
  object ob;
  
  ob = call_stack (1)[<1];
  if (origin () != "local" && ob != TO && !group_member (ob, "lords"))
  {
    secure_log_file ("unauth", "get_char called on " + TO->query_name () + " by "
        + ob->query_name () + ".\n");
    error ("get_char called by other user.");
  }
  
  if (origin () == "local")
    ob = TO;
  else
    ob = previous_object ();
  
  if (stringp (fun))
    fun = ({ ob, fun });
  
  add_input (fun, mode | IMODE_ONCE | IMODE_PROMPT | IMODE_CHAR, 0, args...);
}

/**
 * Removes the last input the object calling remove_input()
 * will receive. Perhaps this call will change later on.
 *
 * @author Morth
 */
void remove_input() {
  object ob;
  mixed fun;
  int i;
  
  if (origin () == "local")
    ob = TO;
  else
    ob = previous_object ();
  
  for (i = sizeof (inputs) - 1; i >= 0; i--)
  {
    fun = inputs[i][0];
    if (function_owner (fun) != ob)
      continue;
    
    inputs = delete (inputs, i, 1);
    break;
  }
}

protected string *extract_chars(string str) {
  string *res = ({ });
  int mode = 0, i;
  
  if (!sizeof (str))
    str = "\b";
  
  str = stored_chars + str;
  
  i = 0;
  while (i < sizeof (str))
  {
    switch(mode)
    {
    case 0:
      switch(str[0])
      {
      case '\b':
      case 127:
        res += ({ "\b" });
        str = str[1 ..];
        break;
      case '\x1b':
        mode = i = 1;
        break;
      default:
        res += ({ str[0 .. 0] });
        str = str[1 ..];
        break;
      }
      break;
    case 1:
      if (str[1] == '[')
        mode = i = 2;
      else
      {
        res += ({ str[0 .. 1] });
        str = str[2 ..];
        mode = i = 0;
      }
      break;
    case 2:
      if ((str[2] >= '0' && str[2] <= '9') || str[2] == 'O')
        mode = i = 3;
      else
      {
        switch (str[0 .. 2])
        {
        case ESC "[H":
          res += ({ HOME });
          break;
        case ESC "[F":
          res += ({ END });
          break;
        default:
          res += ({ str[0 .. 2] });
          break;
        }
        str = str[3 ..];
        mode = i = 0;
      }
      break;
    case 3:
      if (str[i] >= '0' && str[i] <= '9')
        i++;
      else
      {
        res += ({ str[0 .. i] });
        str = str[i + 1 ..];
        mode = i = 0;
      }
      break;
    }
  }
  stored_chars = str;
  
  return res;
}

/**
 * Put here as well as sfun for consistency.
 *
 * Note: There are many projects to notify_fail()
 *       This one should be worth following, as Morth
 *       were on the path to piping commands.
 *       STDOUT vs STDERR
 *
 * @author Morth
 */
void notify_fail (mixed str) {
    if ( intp(str) )
        str = "" + str;
    else if ( !stringp (str) && !arrayp (str) )
        error ("Invalid argument 1 to notify_fail (). Expected string or array.");
    TO->add_static_property ("notify_fail", str);
}

#include "/global/mortal/prompt.i"
