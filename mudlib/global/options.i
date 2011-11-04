/**
 * @main
 *
 * Gathering of several systems to do setup on the character.
 * Code scavenged and rewritten to a common standard from many places.
 *
 * Contains code to do setting with:
 *   - verbose
 *   - consent
 *   - inform
 *   - colors
 *
 * @author Many
 * @author Silbago
 */

#include <cmds.h>
#include <handlers.h>

// This is for setting default settings for new character creation
// Some time later implement "default" part of options system.
//
mapping colors = ([
      "channel shout" : "%^BOLD%^",
      "tell"          : "%^CYAN%^",
    ]);
string *informs = ({
         "logon",        "news",
       }),
       *switch_list = ({
         "spell_colors", "combat_colors",   "colors",
         "commandtimer", "sort_inventory",  "melee",
       }),
       *consent_list = ({
         "give",       
       }),
       *verbose_list = ({
         "spellspamm",   "npcdeath", "bardsong",           // Pure Spamm settings
         "misshere",     "mymiss",   "hismiss",
         "protectspamm", "targetswap",
         "xpcount",      "walk",     "score",              // More vs Less in misc executions
         "digits",                                         // For easier reading
         "advance_dice", "dps",                            // Enhancing things
       });

mapping query_colors  () { return copy(colors);        }
string *query_informs () { return copy(informs);       }
string *query_switches() { return copy(switch_list);   }
string *query_consents() { return copy(consent_list);  }
string *query_verboses() { return copy(verbose_list);  }

int query_switch(string which) {
    return member_array(which, switch_list) != -1;
}

int query_inform(string which) {
    return member_array(which, informs) != -1;
}

int query_consent(string which) {
    return member_array(which, consent_list) != -1;
}

int query_verbose(string which) {
    return member_array(which, verbose_list) != -1;
}

/**
 * @return will never return 0, at least ""
 */
string query_color(string str) {
    if ( !colors[str] )
        return "";
    return colors[str];
}

int set_verbose(string which, int on) {
    if ( !on ) {
        verbose_list -= ({ which });
        return 1;
    }

    if ( member_array(which, verbose_list) != -1 )
        return 0;

    if (    base_name(previous_object()) != VERBOSE_CMD
         && base_name(previous_object()) != OPTIONS_HANDLER )
        return 0;

    verbose_list += ({ which });
    return 1;
}

int set_inform(string which, int on) {
    if ( !on ) {
        informs -= ({ which });
        return 1;
    }

    if ( member_array(which, informs) != -1 )
        return 0;

    if (    base_name(previous_object()) != INFORM_CMD
         && base_name(previous_object()) != OPTIONS_HANDLER )
        return 0;

    informs += ({ which });
    return 1;
}

int set_consent(string which, int on) {
    if ( !on ) {
        consent_list -= ({ which });
        return 1;
    }

    if ( member_array(which, consent_list) != -1 )
        return 0;

/* Doing it internally is ok, need to validate
    if (    base_name(previous_object()) != CONSENT_CMD
         && base_name(previous_object()) != OPTIONS_HANDLER )
        return 0;
*/
    consent_list += ({ which });
    return 1;
}

/**
 * Set color to the different events marked by type.
 * Input 0 to second argument, color, and it is deleted.
 * For support of this and that, if color is "0" or "" it is 0
 */
int set_color(string type, string color) {
    if ( !color ) {
        map_delete(colors, type);
        return 1;
    }

    if ( color == "" || color == "0" )
        return set_color(type, 0);

    colors[type] = color;
    return 1;
}

int set_switch(string which, int on) {
    if ( !on ) {
        switch_list -= ({ which });
        return 1;
    }

    if ( member_array(which, switch_list) != -1 )
        return 0;

    if (    base_name(previous_object()) != SWITCH_CMD
         && base_name(previous_object()) != OPTIONS_HANDLER )
        return 0;

    switch_list += ({ which });
    return 1;
}

#include "/global/mortal/foes.i"
