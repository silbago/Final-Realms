/**
 * @main
 * A plain mounteable horse.
 * @author Silbago
 */

inherit "/obj/mount";

void setup() {
    set_default_cost(500);
    set_mount_size(3);

    set_name("cormyrian rider");
    add_alias(({"horse", "cormyrian", "rider"}));
    add_plural(({"horses", "cormyrians", "cormyrian riders"}));

    set_short("Cormyrian Rider");
    set_main_plural("Cormyrian Riders");
    set_long("A sleek horse, common in the realms, standing approximately "
             "15 hands tall.  It's breed is known to be easy to ride and "
             "as such, novice riders typically start here.  Cormyrian "
             "riders tend to be reddish in color, sometimes as light as "
             "an orange.  They are very friendly animals and remain loyal "
             "to their masters til the bitter end.  Of course, repeated "
             "cruelty will turn any animal from a friend into a foe.\n");
}
