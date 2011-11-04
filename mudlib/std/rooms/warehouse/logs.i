/**
 * @main
 * I wanted a very advanced log handling system, but discarded it.
 * See backup on this file
 *   /std/rooms/warehouse/BACKUP/logs.i.110112.silbago
 * @author Silbago
 */

#define DEPOSIT_DIR file_dir(TO) + "logs/"

string log_deposit(object item, object me, object vaultroom_ob, object warehouse_ob) {
    string vault_log, str, tmp, logname;
    int i;

    if ( file_size(DEPOSIT_DIR) != -2 )
        mkdir(DEPOSIT_DIR);

    if ( item->query_static_property("anonymous deposit") )
        logname = "Anonymous";
    else
        logname = me->query_cap_name();

    tmp = item->query_short();
    i = strlen(strip_colours(tmp));
    if ( i < 29 )
        tmp += repeat_string(" ", 29 - i);
    str = sprintf("%-11s deposited: %s%%^RESET%%^ on %-24s",
          logname, tmp, ctime(time(), 5));

    vault_log = DEPOSIT_DIR + file_end(vaultroom_ob);
    write_file(vault_log, str + "\n");

    str += " at " + vaultroom_ob->query_short();
    vault_log = DEPOSIT_DIR + "masterlog";
    write_file(vault_log, str + "\n");
}

string log_retrieve(object item, object me, object vaultroom_ob, object warehouse_ob) {
    string vault_log, str, tmp;
    int i;

    if ( file_size(DEPOSIT_DIR) != -2 )
        mkdir(DEPOSIT_DIR);

    tmp = item->query_short();
    i = strlen(strip_colours(tmp));
    if ( i < 29 )
        tmp += repeat_string(" ", 29 - i);

    str = sprintf("%-11s retrieved: %s%%^RESET%%^ on %-24s",
          me->query_cap_name(), tmp, ctime(time(), 5));

    vault_log = DEPOSIT_DIR + file_end(vaultroom_ob);
    write_file(vault_log, str + "\n");

    str += " at " + vaultroom_ob->query_short();
    vault_log = DEPOSIT_DIR + "masterlog";
    write_file(vault_log, str + "\n");
}

/**
 * Reading the vault activity log.
 *
 * Use parameter tail_it (== 1) if you want a "tail" that is the
 * last lines equally to rows - 2 of the user "me"
 *
 * The mastervault reads the general logfile.
 */
varargs string read_log(object me, int tail_it, int masterlog) {
    int line;
    string ret, vault_log;

    if ( masterlog )
        vault_log = DEPOSIT_DIR + "masterlog";
    else
        vault_log = DEPOSIT_DIR + file_end(previous_object());

    if ( file_size(vault_log) < 1 )
        return "Vault log is empty";

    ret = "";
    line = file_length(vault_log);
    if ( tail_it == 1)
        tail_it = me->query_rows() - 3;
    else
        tail_it = -1;

    for ( int i=line; i > 0; i-- ) {
        ret += "%^RESET%^" + read_file(vault_log, i, 1);
        if ( tail_it != -1 ) {
            if ( tail_it == 0 )
                break;
            tail_it--;
        }
    }

    return ret;
}

