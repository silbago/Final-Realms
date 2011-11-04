Project to generalize evaluation for armours.
Next step is weapons (more item types).

Need to customize theorietical max against (or not) what given user can use.
If we dont test at what is legal to use for user, find absolute theoretical max.

Titan's Tier system with my tier system needs revamp with this really.

First part in this project is however to wrap up result of single object instead
of wide parsing of array of objects.
We want the body-type (object subtype) returned
 + quality tier
 + tier level
 + item parameters graphical bar AC for armours
 + Other bars if I can use titan's tier results





#include <handlers.h>
#define ARMOUR_TABLE "/table/armour_table"

private nosave object *armours;
private nosave string retval, term, *armour_list, *legal_armours, *banned_armours;

string *repair_mess;         // ({ player mess, room mess })
string appraise_mess;        // player mess
string evaluate_mess;        // player mess


/**
 * This method finds maximum ac value for given armour type depending on
 * what is legal to wear for a certain guild by list of legal armours.
 */
private int find_max_ac(int armour_type) {
    string arm;
    int maxval, *data, banned;

    // Some guilds can have any armour, so we scan table default list
    //
    maxval = 0;
    if ( !sizeof(legal_armours) || legal_armours[0] == "all" || sizeof(banned_armours) ) {
        foreach(arm in armour_list) {
            // THIS should -NOT- happen, bad setup in guild object !!!
            data = ARMOUR_TABLE->lookup_armour_data(arm);
            if ( !data )
                continue;
            if ( data[5] != armour_type )
                continue;
            if ( member_array(arm, banned_armours) != -1 )
                continue;
            if ( data[4] > maxval )
                maxval = data[4];
        }

        return maxval;
    }

    foreach(arm in legal_armours) {
        if ( banned ) {
            if ( member_array(arm, legal_armours) != -1 )
                continue;
        } else if ( member_array(arm, legal_armours) == -1 )
            continue;

        // THIS should -NOT- happen, bad setup in guild object !!!
        data = ARMOUR_TABLE->lookup_armour_data(arm);
        if ( !data )
            continue;

        if ( data[5] != armour_type )
            continue;
        if ( data[4] > maxval )
            maxval = data[4];
    }

    return maxval;
}

/**
 * We find max ac this player is allowed to have by armour type.
 * Then we recalculate enchant ac according to set_enchant() in armour.c
 * To that we find max_ac and check agains query_ac() on each piece.
 */
void bar_armour(int armour_type, int enchant_ac, string short) {
    object *bars = filter(copy(armours), (: $1->query_armour_type() == $(armour_type) :));
    int max_type_ac = find_max_ac(armour_type);

    foreach(object armour in bars) {
        string quality_tier, condition;
        int max_ac,
            ac = armour->query_ac() - armour->query_attribute("armour class");

        max_ac = max_type_ac + enchant_ac;

... actual ac is some times more than 100%, why is that ?
... Need coherent documentation of that

        /* Measure to use for debugging
        retval += sprintf("Theoretical max %3d (%2d+%2d), Actual %3d    ",
                  max_ac, max_type_ac, enchant_ac, ac);
        */

        retval += sprintf("%%^BOLD%%^%-11s%%^RESET%%^  %.35-35s  %s  %-10s  %s\n",
...                  short, strip_colours(armour->short()),
...                  make_status_bar("", ac, max_ac, 18, 0, 2, term,),
                  CAP(condition),
                  CAP(quality_tier));
        short = "";
    }

elements interesting in this function is what we return in the end
  short variable interesting as bodypart-type
  then make status bar
}

int evaluate_armours(object me) {
    object race_ob;

    if ( !armour_list )
        armour_list = ARMOUR_TABLE->get_armour_names();

    term = me->query_term_name();
    armours = all_inventory(me) + all_inventory(ENV(me));
    armours = filter(armours, (: $1->query_armour() :));
    race_ob = me->query_race_ob();
    armours = filter(armours, (: $(race_ob)->query_legal_armour($1) :));
    race_ob = me->query_guild_ob();
    armours = filter(armours, (: $(race_ob)->query_legal_armour($1->query_armour_name()) :));
    armours = filter(armours, (: $(me)->query_requirement($1) :));

    if ( !sizeof(armours) ) {
        me->msg("$o can not find anything of interest to evaluate.\n", TO);
        return 1;
    }

    // Legal armours for the player, it might list banned and not legal.
    //
    legal_armours = race_ob->query_legal_armours();
    if ( legal_armours[0] == "banned" ) {
        banned_armours = legal_armours[1..];
        legal_armours  = ({ });
    } else
        banned_armours = ({ });

    retval = "\n%^CYAN%^Armour Type  Armour Name                           "
             "Armour Class Yield   Condition   Quality Tier%^RESET%^\n";

    bar_armour( 1, 39, "Body Armour");
    bar_armour( 2, 39, "Shield");
    bar_armour( 3, 29, "Helmet");
    bar_armour( 4, 29, "Boots");
    bar_armour( 5, 13, "Amulet");
    bar_armour( 6, 26, "Cloak");
    bar_armour( 7, 13, "Ring");
    bar_armour( 8, 29, "Gloves");
    bar_armour( 9, 26, "Belt");
    bar_armour(10, 13, "Backpack");
    bar_armour(11, 26, "Trousers");
    bar_armour(12, 26, "Shirt");
    bar_armour(18, 39, "Badge");

    me->more_string(retval);
    return 1;
}
