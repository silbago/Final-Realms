/**
 * @main
 * Help information for the warehouse settings based on what type
 * of warehouse this is ... Indicating better customization to
 * draw different paint on the entire thing through and through.
 * Therefore a seperate file for help.
 * @author Silbago
 */

string get_help(string arg) {
    if ( arg != "warehouse" )
        return 0;

    return
      "%^BOLD%^Syntax%^RESET%^\n"
      "    deposit [-p | -P] [ -a ] <item>\n"
      "    list [ ... flags ] [<match item name>]\n"
      "    retrieve [ -s ] <item | #number> \n"
      "    browse <item | #number>\n"
      "    evaluate [<match pattern>]\n"
      "    logs\n"
      "    latest\n"
      "\n"

      "%^BOLD%^Abstract%^RESET%^\n"
      "    This is a warehouse, it is not quite like the classic Final Realms vaults.\n"
      "    It is a massive storage house, therefore it is a Warehouse.\n"
      "    Capacity is only limited by the size of your Treasury, pay for extensions.\n"
      "    Each Vaultroom you own is connected to the Warehouse, though nothing is stored\n"
      "    in each actual Vaultroom. Thus everything is stored in the Warehouse.\n"
      "    This is what makes it different from Classic Final Realms Vaults.\n"
      "\n"
      "    %^BOLD%^Labels%^RESET%^\n"
      "        Each Vaultroom comes with a set of labels, each item you deposit in a Vault\n"
      "        is labelled as such.   Every Vaultroom with similar label(s) can access the\n"
      "        very same items. So be carefull with what labels you setup your vaults with\n"
      "\n"
      "    %^BOLD%^What does it matter ?%^RESET%^\n"
      "        Now the advantage,you can have as many vaultrooms as you feel like with any\n"
      "        label(s) you feel like.  This does  not affect the total  storage capacity.\n"
      "\n"
      "        So you can have as many vaultrooms you like,  it does not affect the total\n"
      "        storage capacity.  Use labels to tailor what content you have in each.\n"
      "\n"

      "%^BOLD%^Description%^RESET%^\n"
      "    This is a vault,  an item locker.  Use described  commands to deposit,  list or\n"
      "    retrieve  items from within these shelves.  Notice that the item limit is setup\n"
      "    for the entire warehouse, not this single vaultroom.\n"
      "\n"

      "%^BOLD%^Depositing Private items%^RESET%^\n"
      "    You can mark items in the vault as your private.  If you do not trust others to\n"
      "    not \"steal\" your items.   With the -p flag you mark it as your own,  but others\n"
      "    can take it still - it is a mark of respect.\n"
      "\n"
      "    If you use the -P flag (capitalized letter P)  noone but you are able to get it\n"
      "    out of the vault.\n"
      "\n"
      "    %^BOLD%^Examples%^RESET%^\n"
      "        deposit armours\n"
      "        deposit long sword\n"
      "        deposit -P ring of hind\n"
      "\n"

      "%^BOLD%^Retrieving items%^RESET%^\n"
      "    Private items marked with -p flag at deposit can be retrieved by anyone as said\n"
      "    above.  These shows with a green asterix in front of Contributor on list.\n"
      "    However items marked with -P (capital letter P) can  not be retrieved by anyone\n"
      "    but the one who deposited it.  All well described above.\n"
      "    Council members of the group can however override this.\n"
      "\n"

      "    Another trick is to retrieve item and deposit it again and keeping the username\n"
      "    as to who deposited it, this is the sticky flag.\n"
      "    %^BOLD%^Examples%^RESET%^\n"
      "        retrieve -s long sword\n"
      "        deposit long sword\n"
      "\n"

      "%^BOLD%^Listing items%^RESET%^\n"
      "    The list command lists the contents of the given vaultroom,  while the evaluate\n"
      "    command shows the same list with different credentials.\n"
      "\n"
      "    You might want to list a limited amount of items, seeing things named ring only\n"
      "    and so forth.  Now it goes deeper,  you  can list private marked items with the\n"
      "    same flags you use with deposit: -p and -P\n"
      "\n"
      "    %^BOLD%^Flags%^RESET%^\n"
      "        -p              Listing items marked as private - Please dont touch.\n"
      "        -P              Listing items marked as private - Others cant touch.\n"
      "        -a              Anonymous deposit.\n"
      "        -t <type>       Listing items of type: armour, weapon, scroll, wand, potion.\n"
      "        -c <condition>  Showing items of given condition.\n"
      "                        Note: list -c very_good (use underscore between words)\n"
      "        -u <user>       List items deposited by given user.\n"
      "        -q <tier>       mundane, exceptional, superior and legendary.\n"
      "\n"
      "    %^BOLD%^Examples%^RESET%^\n"
      "        list -p -u silbago    --- Listing deposited by silbago, marked private.\n"
      "        list -c excellent -t legendary   --- The real good stuff !!!\n"
      "\n"

      "%^BOLD%^Examples%^RESET%^\n"
      "    retrieve amulet\n"
      "    retrieve #13\n"
      "    list\n"
      "    list rings\n";
}

