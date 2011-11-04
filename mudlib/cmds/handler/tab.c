#define CMD_HANDLER "/cmds/handler/cmd_handler"

/**
 * When terminal client is in charmode we can look for <tab>
 * and ask if we can autocomplete. The given command shall
 * parse what it can auto-complete, meaning each command must
 * be compatible.
 */
private string parse_command_tab(string arg) {
    string *bits;

    if ( !strlen(arg) || arg[<1] != 9 )
        return arg;

    bits = explode(arg, " ");
    if ( sizeof(bits) == 1 )
        return CMD_HANDLER->autocomplete_command(TO, arg[0..<2]);

    return CMD_HANDLER->autocomplete_argument(TO, bits[0], implode(bits[1..], " ")[0..<2]);
}

/**
 * User used <tab> on first argument to command, search if a command
 * that user has access to can be found with exact match.
 * BEFORE RELEASE: If multiple matches, throw in a msg("list of commands")
 */
varargs string autocomplete_command(object me, string comm, int fullpath) {
    string ret = legal_verb(comm, me, 0, 0);
    if ( !ret )
        return comm;

    if ( fullpath )
        return ret;

    if ( ret[<2..] == ".c" )
        ret = ret[0..<3];

    return implode(ret, "/")[<1];
}

/**
 * User is writing argument to known command and wants autocomplete
 * by <tab> argument. Check if command is legal, then ask command object.
 * what we can help him with. Command must be compatible.
 * BEFORE RELEASE: If multiple matches, throw in a msg("list of commands")
 */
string autocomplete_argument(string comm, string arg) {
    string ret, path = autocomplete_command(me, comm, 1);
    if ( !path )
        return comm + " " + arg;

    ret = path->autocomplete_argument(me, arg);
    if ( !ret )
        return comm + " " + arg;

    return comm + " " + ret;
}
