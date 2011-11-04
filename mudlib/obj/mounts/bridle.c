/**
 * @main
 * You need to hold a bridle to be able to use the mounts.
 * The bridle is acquired through quest #
 *
 * Original Mount code by Nimwei, generalized in mudlib
 * by Silbago 2011.
 *
 * @author Silbago
 * @author Nimwei
 */

inherit "/std/item";

int query_bridle() { return 1; }

int query_auto_load() {
    return query_property("stable boy bridle");
}

void setup() {
    set_name("bridle");

    set_main_plural("Bridles");
    set_short("Bridle");
    set_long("A harness, consisting of a headstall, bit, and reins.\n"
      "This is used to restrain or guide an animal.\n");

    reset_drop();
    set_weight(10);
    set_value(0);
    lore(1);
}
