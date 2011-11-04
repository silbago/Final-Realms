/**
 * @main
 * Standard mount code (steed, horse, oliphants etc).
 * Standard chars located in /obj/mounts/, clone those to play with.
 *
 * You can modify these two things only:
 *   set_mount_age()  - 1 to 20 years old. 12 hours online = 1 day.
 *   set_mount_size() - 1 to 4
 *   set_race()       - Determines aquatic abilities
 *
 * These determine things like strength, carry capacity, damage bonus,
 * hitpoints, controlleability (wildness).
 *
 * Equipment for the mount:
 *  - bridle     reins and stick in the mouth of the mount, control
 *  - saddle     Sit comfortably and managing your manouverability on the back
 *  - saddlebag  To strap on to have it carrying stuff, pack-beast.
 *
 * First mount system made by Nimwei, that was very great.
 * All credits for design and theme goes to Nimwei.
 * This mount system takes the full advantage of the mudlib and is therefore
 * a needed replacement.
 *
 * @author Nimwei
 * @author Silbago
 */

inherit "/obj/monster";

#include <mounts.h>

#define MY_MOUNT(XXX)                          \
    if ( !mount_owner || mount_owner != me ) { \
        notify_fail(XXX + "\n");               \
        return 0;                              \
    }

private nosave object mount_owner;
private nosave string owner_name,
                      *legal_guilds,
                      *legal_races,
                      *illegal_guilds,
                      *illegal_races;

private nosave int mount_size,      // 1 . 2 . 3 . 4
                   race_size,       // Regards race object
                   aging,           // For notice when it grows older
                   is_climbing,     // Fast up and down climb exits
                   is_amphibian,    // IF we set the monster to be amphibian
                   is_levitating,   // Well comment when it works
                   is_fast,         // Makes walking speed -1 better
                   is_slow,         // Makes walking speed +1 slower
                   attack_mod,      // Skill test modification
                   attack_steed,    // value 1 or 2, for specific guilds !!!
                   charge_mod,      // Skill adjusts as for attack_steed
                   charge_mount,    // Paladins and Anti-Paladins !!!
                   age_seconds,     // Seconds mount has been around
                   default_cost,    // 10 year old mount price
                   called_time,     // time() when mount was loaded
                   speed_mod,       // Skill test modification
                   mount_speed,     // -1, -2, -3 and -7 (-4 is default)
                   mount_max_carry, // Can carry up to
                   is_mounted;      // When the owner is sitting on beast

int    is_mounted        () { return is_mounted;    }
int    query_is_mount    () { return 1;             }
int    query_climbing    () { return is_climbing;   }
int    query_sea_race    () { return is_amphibian;  }
int    query_race_size   () { return race_size;     }
int    query_fast_mount  () { return is_fast;       }
int    query_slow_mount  () { return is_slow;       }
int    query_levitating  () { return is_levitating; }
int    query_mount_size  () { return mount_size;    }
int    query_default_cost() { return default_cost;  }
string query_owner       () { return owner_name;    }
object query_friend      () { return mount_owner;   }
object query_player_pet  () { return mount_owner;   }
object query_mount_owner () { return mount_owner;   }

int query_attack_steed() { return attack_steed+attack_mod; }
int query_charge_mount() { return charge_mount+charge_mod; }

/**
 * Method returns what object is mounted on this beast right now.
 */
object query_mounted_by() {
    if ( !is_mounted || !mount_owner )
        return 0;

    // Some failsafe warnings, need to track this shit.
    //
    if ( ENV(mount_owner) != ENV(TO) ) {
        warning("I am mount, My owner is not here but he is mounted");
        return 0;
    }

    return mount_owner;
}

int query_age_seconds() {
    int ret;

    ret = age_seconds + (time()-called_time);
    if ( ret < MOUNT_YEAR_VALUE )
        ret = MOUNT_YEAR_VALUE;

    return ret;
}

/**
 * Shows how many years old the steed is, in years.
 */
int query_age() {
    return MOUNT_YEAR(query_age_seconds());
}

/**
 * For the mount this is determined by size and age.
 * At 10 years age the mount is the strongest, reduce 5% per
 * year in distance from this age.
 */
int query_max_weight() {
    int i, maxweight;

    i = query_age();
    if ( i < 10 )
        i = 10 - i;
    else
        i = i - 10;

    maxweight = query_str() * 150;
    while(i--)
        maxweight -= maxweight / 20;

    return maxweight;
}

/**
 * Finds how much the thing is carrying, supports all mudlibs.
 */
int query_carry_weight() {
#ifdef STANDARD
    return query_loc_weight();
#endif
    return query_contents_weight();
}

int query_value() {
    return PRICE_RANGE[query_age()] * 500 * default_cost / 100;
}

/**
 * Movement speed by adjust_time_left() in action_queue().
 * ---- Debug amphibians on land and in sea, balance it
 */
int query_mount_speed() {
    if ( is_amphibian && !ENV(TO)->query_ocean() )
        return -7;

    return mount_speed + speed_mod;
}

/**
 * This method tells wether this mounts accepts rider.
 * If rider is owner it is a nobrainer (no cheats please).
 * Race, Guild and Size controls.
 * For legal, illegal settings.
 * Size determines legality on race as well.
 * And shapeshifters, no get into right shape.
 * No real_race() checks.
 */
int accept_rider(object rider) {
    if ( mount_owner )
        return rider == mount_owner;

    if ( sizeof(legal_guilds) )
        if ( member_array(rider->query_guild(), legal_guilds) == -1 )
            return 0;
    if ( sizeof(illegal_guilds) )
        if ( member_array(rider->query_guild(), illegal_guilds) != -1 )
            return 0;
    if ( sizeof(legal_races) )
        if ( member_array(rider->query_race(), legal_races) == -1 )
            return 0;
    if ( sizeof(illegal_races) )
        if ( member_array(rider->query_race(), illegal_races) != -1 )
            return 0;

    // Almost correct short list of races and their sizes
    // hafling                   6
    // goblin gnome              7
    // dwarf duergar             8
    // hobgoblin                10
    // elf drow                 14
    // ilythiiri                16
    // human                    12
    // centaur  djinni          20
    // bear dragon              25
    //
    switch(mount_size) {
    case 1:
        if ( rider->query_race_ob()->query_race_size() > 15 )
            return 0;
        break;
    case 4:
        if ( rider->query_race_ob()->query_race_size() < 10 )
            return 0;
        break;
    }

    return 1;
}

void create() {
    legal_guilds   = ({ });
    legal_races    = ({ });
    illegal_guilds = ({ });
    illegal_races  = ({ });

    mount_size  = 1;
    called_time = time();
    set_wimpy(50);
    set_race("animal");
    set_limbs(4);
    speed_mod  = 0;
    charge_mod = 0;
    attack_mod = 0;

    ::create();

    add_alias("mount");
    add_plural("mounts");
    remove_alias("animal");
    remove_plural("animals");
    alias -= ({ "monster" });
    plurals -= ({ "monsters" });

    race_size = ({4000,8000,12000,18000})[mount_size-1];
    set_alignment("neutral");

// ------ Tier skills differently ?
    adjust_skill_level("awareness", 100);
    adjust_skill_level("unarmed", 50);
    adjust_skill_level("anatomy", 30);
    adjust_skill_level("botany", 40);
    adjust_skill_level("speed", 100);

    add_cmd("calm", "<me>");
    add_cmd("mount", "<me>");
    add_cmd("control", "<me>");
    add_cmd("dismiss", "<me>");
    add_cmd("release", "<me>");
    add_cmd("dismount", "<me>");

    if ( base_name(TO) == "/obj/mount" )
        return;

    call_out("build_mount", 0);
}

void set_legal_races   (string *grp) { legal_races = grp;    }
void set_legal_guilds  (string *grp) { legal_guilds = grp;   }
void set_illegal_races (string *grp) { illegal_races = grp;  }
void set_illegal_guilds(string *grp) { illegal_guilds = grp; }

/**
 * Makes the rider more dangerous in melee combat, if competent rider.
 * As the rider has the 'high ground'.
 * Mainly made for Paladins and Anti-Paladins
 *
 * Setting 1 or 2, a little or massive.
 * Simply gives +1 or +2 damage bonus to rider.
 */
void set_attack_steed(int i) {
    if ( i < 1 || i > 2 ) {
        warning("set_attack_steed(" + i + "), illegal value.", 1);
        i = 0;
    }

    attack_steed = i;
}

/**
 * Makes the mount charge, 2 for very hard.
 */
void set_charge_mount(int i) {
    if ( i < 1 || i > 2 ) {
        warning("set_chrge_mount(" + i + "), illegal value.", 1);
        i = 0;
    }

    charge_mount = i;
}

/**
 * Lizard kinds, swims like crazy and breathe under water.
 * Should find method to make it slower on land later.
 */
void set_amphibian() {
    if ( is_levitating ) {
        warning("Levitating mounts can not be amphibian", 1);
        return;
    }

    is_amphibian = 1;
    adjust_attribute("water breathing", 1);
    add_property("free_action", 1);
    adjust_skill_level("swimming", 110 - (-mount_speed * 10));
}

/**
 * Bare beginning of concept, need to check mudlib how support is.
 * Levitating to make climbing very fast and floating with swim.
 */
void set_levitating() {
    if ( is_amphibian ) {
        warning("Amphibians mounts can not be levitating", 1);
        return;
    }

    is_levitating = 1;
}

/**
 * Makes the you move faster up and down climb exits while mounted.
 * Improves range a bit.
 */
void set_climbing() { is_climbing = 1; }

/**
 * Use this for very special mounts only.
 * It improves walking speed by -1 overall ages.
 */
void set_fast() { is_fast = 1; }

/**
 * Opposite of fast obviously, mules :)
 */
varargs void set_slow(int i) {
    if ( i < 1 )
        i = 1;
    is_slow = i;
}

/**
 * Pricing is very simple on mounts.
 * This is the cost you set for a 10 years old mount.
 * It fluctuates by a strict standard you're not allowed
 * to tinker with. Shops can deal with it either.
 */
void set_default_cost(int plats) {
    default_cost = plats;
}

/**
 * Horses are 1 to 20 years old, at 20 they die.
 *   - Low age, wild, low hps low speed
 *   - High age, dosile, low hps low speed
 *
 * They age naturally, but can be replenished by magic.
 * Aging is 1 year per 5 playing hours, when active.
 *
 * The level of the mount becomes age * 5
 *
 */
void set_mount_age(int i) {
    if ( i < 1 || i > 20 ) {
        warning("set_mount_age("+i+"), larger than 1 or higher than 20.", 1);
        i = 20;
    }

    age_seconds = 12*3600*i;
}

void set_mount_age_seconds(int i) { age_seconds = i; }

/**
 * The size of the beast is 1 to 4.
 * Race checks again this, controlling the smallest and
 * the largest. So size 1 and 4 will be unaccessible for either.
 *
 * Size Limit  Type of animals and who they work well for
 *   1   <15   Halfling gnome dwarf
 *             Mounts like: pony, hound, wolf
 *   2   ---   Small horses, warg, mule
 *   3   ---   Large steeds, big warg, charger
 *   4   >10   Troll, Demon, Dragon, Ilythiiri
 *             Oliphant, Pegasus, Unicorn
 */
void set_mount_size(int i) {
    if ( i < 1 || i > 4 ) {
        warning("set_mount_size("+i+"), larger than 1 or higher than 4.", 1);
        i = 1;
    }

    mount_size = i;
}

/**
 * Bridle clones the mount and sends call here.
 * Then this function loads the savedata from the owner for
 * setup of the eventual old mount.
 */
void set_mount_owner(object me) {
    mount_owner = me;
    owner_name = me->query_name();
    set_wimpy(0);
    mount_owner->set_mount(TO);
}

int do_calm(object me) {
    object *obs;
    MY_MOUNT("The beast utterly ignores you.\n")

    obs = query_attacker_list() + query_call_outed();
    if ( !sizeof(obs) ) {
        me->msg("$Q is calm.\n", TO);
        return 1;
    }

    for ( int i=0; i < sizeof(obs); i++ )
        call_out("stop_fight", i+20, obs[i]);

    me->msg("$Q calms down slowly.\n", TO);
    return 1;
}

int do_control(object me) {
    if ( mount_owner ) {
        notify_fail("This mount is already loyal to its owner !\n");
        return 0;
    }

    if ( !accept_rider(me) ) {
        notify_fail("This mount refuses you totally !\n");
        return 0;
    }

    if ( me->query_mount() ) {
        notify_fail("You can not control several mounts at the same time.\n");
        return 0;
    }

    if ( !sum(all_inventory(me)->query_bridle()) ) {
        notify_fail("You can not control a mount without a bridle.\n");
        return 0;
    }

    is_mounted  = 0;
    mount_owner = me;
    me->set_mount(TO);
    set_protector(0);
    set_wimpy(0);
    me->add_mount(TO, ({ query_age_seconds(), ({}), ([]) }));

    me->e_msg("$Q snuggles up to $Q.\n", 0, TO, me);
    return 1;
}

int do_mount(object me) {
    int riding;
    MY_MOUNT("The beast utterly ignores you.\n")

    if ( is_mounted ) {
        notify_fail("You are already mounted.\n");
        return 0;
    }

    is_mounted = 1;
    me->set_mount(TO);
    set_protector(me);
    me->setmout("@$N rides $T.");
    me->setmin("@$N rides in from the $F.");

    riding = me->query_skill("riding");
    me->notify_skill_usage("riding");
    if ( member_array(me->query_guild(), ({"paladin","antipaladin"})) != -1 )
        riding += riding * 15 / 100;

    speed_mod = 0;
    if ( query_age() <= 5 && riding < 40 ) {
        me->msg("You're not able to control this young beast properly.\n");
        speed_mod--;
    }

    if ( is_fast && riding < 70 ) {
        me->msg("You're not able to control extraordinary fast beast properly.\n");
        speed_mod--;
    }

    attack_mod = 0;
    if ( attack_steed ) {
        int i = attack_steed;

        attack_mod = 0;
        if ( i >= 1 && riding < 50 ) {
            me->msg("Your riding skills does not give you attack bonus on $Q.\n", TO);
            attack_mod--;
            i--;
        }
        if ( i >= 2 && riding < 60 ) {
            me->msg("Your riding skills does not give you full attack bonus on $Q.\n", TO);
            attack_mod--;
            i--;
        }
        i = charge_mount;
        if ( i >= 1 && riding < 30 ) {
            me->msg("You are not able to control $Q during a charge.\n", TO);
            charge_mod--;
            i--;
        }
        if ( i >= 2 && riding < 90 ) {
            me->msg("You are not able to fully control $Q during a charge.\n", TO);
            charge_mod--;
            i--;
        }

        if ( i > 2 )
            i = 2;
        if ( i > 0 )
            me->adjust_attribute("damage bonus", i);
        if ( (attack_steed+attack_mod) == 1 )
            me->msg("You have a stronger control of your environment on top of $Q.\n", TO);
        if ( (attack_steed+attack_mod) == 2 )
            me->msg("You can fight your enemies far more efficient on top of $Q.\n", TO);
        if ( (attack_steed+attack_mod) == 3 )
            me->msg("Your $Q will aid your effort in a charge, mighty Sir Knight !\n", TO);
    }

    if ( is_amphibian )
        me->add_timed_property("swimming", 1, 300);

    me->e_msg("$Q mount$s $Q.\n", 0, me, TO);
    return 1;
}

int do_dismount(object me) {
    MY_MOUNT("You can not dismount what you have not mounted.\n")

    if ( !is_mounted ) {
        notify_fail("You can not dismount what you have not mounted.\n");
        return 0;
    }

    is_mounted = 0;
    set_protector(0);
    me->setmin("@$N arrives from the $F.");
    me->setmout("@$N leaves $T.");
    me->remove_attribute(TO);

    me->e_msg("$Q dismount$s $Q.\n", 0, me, TO);
    return 1;
}

/**
 * The owner sending the mount away for duration he does not want
 * to use it. In real world put it in a stable, wont work here so
 * we just give them this. Can call it back later.
 */
int do_dismiss(object me) {
    object *obs;
    MY_MOUNT("The beast utterly ignores you.\n")

    obs = all_inventory(TO);
    if ( sizeof(obs) )
        e_msg("$Q shakes $Q over to the side and it drops on the ground.\n", 0, TO, obs);
    obs->move(ENV(TO));
    me->e_msg("$Q leaves $w side and quickly runs off happy to "
            "return later.\n", 0, TO, me, me);

    me->remove_attribute(TO);
    mount_owner->add_mount(TO, ({ query_age_seconds(), ({}), ([ ]) }));
    destruct(TO);
    return 1;
}

/**
 * Clean exiting, this should not happen btw.
 * @param kill_me used at old age, doesnt stable the thing.
 * @dontignore
 */
varargs void dest_me(int kill_me) {
    if ( mount_owner && is_mounted && !kill_me ) {
        do_dismount(mount_owner);
        mount_owner->add_mount(TO, ({ query_age_seconds(), ({}), ([ ]) }));
    }

    ::dest_me();
}

int do_release(object me) {
    MY_MOUNT("The beast utterly ignores you.\n")

    is_mounted  = 0;
    mount_owner = 0;
    me->set_mount(0);
    set_protector(0);
    me->remove_mount(TO);
    me->remove_attribute(TO);

    me->e_msg("$Q release$s $Q into the wild$Q.\n", 0, me, TO);
    set_wimpy(50);
    return 1;
}

/**
 * A tier logics setup of the mount to strict rules.
 * Shops needs to call this directly to deal with age
 */
void build_mount() {
    int i, mnt_age;

    // Find out when the beast turns 1 year older.
    //
    i = age_seconds;
    mnt_age = query_age();
    age_seconds += MOUNT_YEAR_VALUE;
    mnt_age = query_age();
    age_seconds = i;
    i = (mnt_age * 12 * 3600) - i;
    if ( find_call_out("build_mount") != -1 )
        remove_call_out("build_mount");
    call_out("build_mount", i);

    set_con(20);
    set_dex(16);
    set_int(5);
    set_wis(8);
    set_cha(13);
    set_str(17);
    set_extreme_str(3);

    mnt_age = query_age();
    if ( !aging )
        aging = mnt_age;
    else if ( aging < mnt_age ) {
        if ( mount_owner ) {
            mount_owner->msg("$Q has become $# years old.\n", TO, aging + 1 );
            if ( mnt_age >= 21 ) {
                mount_owner->msg("$Q died of old age !\n", TO);
                dest_me(1);
                return;
            }
        }
        aging = mnt_age;
    }

    switch(mount_size) {
    case 2:
        adjust_str(2);
        set_extreme_str(5);
        break;
    case 3:
        adjust_str(3);
        set_extreme_str(8);
        break;
    case 4:
        adjust_str(4);
        set_extreme_str(10);
        break;
    }

    set_level(mnt_age * 5);
    switch(mnt_age) {
    case 1:
        i = 500;
        set_level(mnt_age * 5);
        break;
    case 2:
        i = 600;
        adjust_str(1);
        set_level(mnt_age * 5);
        break;
    case 3:
        i = 700;
        adjust_str(2);
        set_extreme_str(query_extreme_str() + 2);
        set_level(mnt_age * 5);
        break;
    case 4:
        i = 800;
        adjust_str(3);
        set_extreme_str(query_extreme_str() + 3);
        set_level(mnt_age * 5);
        break;
    case 5:
        i = 900;
        adjust_str(4);
        set_extreme_str(query_extreme_str() + 5);
        set_level(mnt_age * 5);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        i = 1000;
        adjust_str(5);
        set_extreme_str(query_extreme_str() + 8);
        set_level(mnt_age * 5);
        break;
    case 13:
    case 14:
        i = 950;
        adjust_str(4);
        set_extreme_str(query_extreme_str() + 7);
        set_level(mnt_age * 5);
        break;
    case 15:
    case 16:
        i = 800;
        set_extreme_str(query_extreme_str() + 5);
        set_level(15*5);
        break;
    case 17:
        set_level(15*5);
    case 18:
        i = 550;
        adjust_str(3);
        set_extreme_str(query_extreme_str() + 2);
        if ( mnt_age == 18 )
            set_level(14*5);
        break;
    case 19:
        i = 400;
        adjust_str(1);
        set_level(13*5);
        break;
    case 20:
        i = 300;
        set_level(13*5);
        break;
    }

    set_max_hp(i * mount_size);
    if ( mount_size < 1 || mount_size > 4 ) {
        warning("Mount size sneaked into bad value: " + mount_size);
        mount_size = 4;
    }

    mnt_age = query_age();
    if ( mnt_age < 1 || mnt_age > 20 ) {
        warning("Mount age sneaked into bad value: " + mnt_age);
        TO->e_msg("$Q died of old age !\n", 0, TO);
        mnt_age = 20;
        call_out("dest_me", 0, 1);
        return;
    }

    i = (mnt_age - 1) + ((mount_size - 1) * 20);
    mount_speed = SPEED_RANGE[i];
    if ( is_fast )
        mount_speed++;
    if ( is_slow )
        mount_speed -= is_slow;
}

void attack_by(object ob) {
    if ( is_mounted && mount_owner && ENV(mount_owner) == ENV(TO) )
        mount_owner->attack_ob(ob);

    ::attack_by(ob);
}

int do_death(object ob) {
    if ( mount_owner ) {
        if ( ENV(mount_owner) != ENV(TO) )
            mount_owner->msg("You feel something terrible happened to your $Q.\n", TO);
        mount_owner->remove_mount(TO);
    }

    return ::do_death(ob);
}

mixed *stats() {
    return ::stats() + ({
        ({ "mount age: seconds", query_age_seconds() }),
        ({ "mount age: years",   query_age()    }),
        ({ "mount size",         mount_size     }),
        ({ "mount speed",        mount_speed    }),
        ({ "mount encumbrance",  mount_max_carry}),
    });
}

string query_age_short() {
    switch(query_age()) {
    case 1:
        return "very young";
        break;
    case 2:
        return "young";
        break;
    case 3:
        return "somewhat young";
        break;
    case 4:
        return "growing up";
        break;
    case 5:
        return "almost grown up";
        break;
    case 6..12:
        return "in its prime age";
        break;
    case 13..14:
        return "past its prime";
        break;
    case 15:
        return "aging";
        break;
    case 16:
        return "growing old";
        break;
    case 17:
        return "old";
        break;
    case 18:
        return "quite old";
        break;
    case 19:
        return "very old";
        break;
    }

    return "extremely old";
}

string query_size_short() {
    return ({"very small","small","large","very large"})[mount_size-1];
}

string query_speed_short() {
    switch(mount_speed) {
    case -1:
        return "lightning fast";
        break;
    case -2:
        return "as a whirlwind";
        break;
    case -3:
        return "at a fast pace";
        break;
    case -4:
        return "daft as any gnome";
        break;
    case -5:
        return "quite slow";
        break;
    case -6:
        return "very slow";
        break;
    case -7:
        return "as a dead turtle";
        break;
    }

    warning("Mount speed was found to be " + mount_speed);
    return "Bug with movement speed";
}

/**
 * External help queries ofcourse, but need specialized query for age.
 */
varargs string get_help(string str, object me) {
    string ret;

    if ( !id(str) || (mount_owner && mount_owner != me) )
        return 0;

    ret = "You are browsing a proud " + query_short() + "%^RESET%^.\n" +
           query_long() + "\n\n"
          "The animal is " + query_age() + " years old, " + query_age_short() +
          ". It moves across the " + (is_amphibian ? "seas " : "plains ") + query_speed_short() +
          ". With the strength to carry " + query_max_weight() + " lbs. ";

    if ( attack_steed ) {
        if ( attack_steed == 1 )
            ret += "You will have attack bonus.";
        else if ( attack_steed == 2 )
            ret += "You will have great attack bonus.";
        if ( charge_mount == 1)
            ret += "The mount will kick during charges.";
        else if ( charge_mount == 2 )
            ret += "The mount will fiercly kick during charges.";
    }
    ret += "\n";

    if ( is_climbing )
        ret += "The animal will move faster than normal up and down crevices and "
               "climb required exits. Where other mounts will not move at all.\n";

    if ( is_amphibian )
        ret += "The animal can swim, but is more or less useless on land. It will "
               "allow you free action unless it goes under water.\n";

    if ( is_levitating )
        ret += "The animal hovers lightly on the ground, travelling hillsides and "
               "climbing should be a charm.  It can also stay above water for a short "
               "duration - but be aware, when it goes down it does so.\n";

    if ( sizeof(legal_guilds) ) {
        string *bits = map(legal_guilds, (: group_property($1, "short") :));
        ret += sprintf("Will accept riders from these guilds only\n    %s\n",
                    nice_list(sort_array(bits, 1)));
    }

    if ( sizeof(legal_races) )
        ret += sprintf("Will accept riders from these races only\n    %s\n",
                    nice_list(sort_array(legal_races, 1)));

    if ( sizeof(illegal_guilds) ) {
        string *bits = map(illegal_guilds, (: group_property($1, "short") :));
        ret += sprintf("Will not accept riders from these guilds\n    %s\n",
                    nice_list(sort_array(bits, 1)));
    }

    if ( sizeof(illegal_races) )
        ret += sprintf("Will not accept riders from these races\n    %s\n",
                    nice_list(sort_array(illegal_races, 1)));

    ret += "\nCommands to control the beast\n"
           "    calm <mount>     - Makes it stop fights\n"
           "    mount <mount>    - To climb up on its back\n"
           "    control <mount>  - If you find a wild animal, to master it\n"
           "    release <mount>  - Sets it free, for someone else to control\n"
           "    dismiss <mount>  - To call on it later with mounts command\n"
           "    dismount <mount> - To climb off its back\n"
           "    mounts           - See help mounts, external command\n";

    return ret;
}
