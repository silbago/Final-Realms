/**
 * @main
 * This handler tells those who want to know that a file is changed.
 * Listeners may only be objects inside /secure and /handlers dirs.
 *
 * Filechange methods are rm, write_file and rename.
 * The listener object must decide what to do with the messages.
 *   listener->event_file_change(string file, mixed uid, string func)
 *
 * Simul efun object makes it happen, by call out 0 argument.
 * This means it comes in after single-threaded operation.
 * Also means the LAST operation is the only notification.
 *
 * Finally, when this object reloads it cleans everything.
 * So the object that wants to listen must answer to what files
 * it wants to listen to. I'm afraid so, else the list will be
 * forever building. string *query_file_events() return ...
 *
 * @author Silbago
 */

#define SAVE_FILE "/" + mud_name() + "/save/Handler/filechange"

void dwep    () { }
void dest_me () { }
void clean_up() { }

private nosave int not_listening; // Need a calm reboot
private nosave string *ignoring_root;
private nosave mapping ignoring, listen_files, listen_dirs;
private mapping listeners;

mapping query_ignoring    () { return copy(ignoring);     }
mapping query_listeners   () { return copy(listeners);    }
mapping query_listen_dirs () { return copy(listen_dirs);  }
mapping query_listen_files() { return copy(listen_files); }

private void save_me() {    save_object(SAVE_FILE, 1); }
private void load_me() { restore_object(SAVE_FILE, 1); }

/**
 * simul_efun() tells us that a file has been changed.
 * Rename has target file, would be nice to get that in as well.
 *
 * For rename the origin gets both messages, the target gets write_file
 */
void event_change_file(string filepath, string uid, string funct, string renamed) {
    if ( not_listening )
        return;

    if ( filepath[0] != 47 )
        filepath = "/" + filepath;
    if ( renamed && renamed[0] != 47 )
        renamed = "/" + renamed;

    // Ewww a listener is changed, takes a lot of work to make stable
    //
    if ( listeners[filepath] )
        call_out("validate_listener", 0, filepath, uid, funct);
    if ( renamed && listeners[renamed] )
        call_out("validate_listener", 0, renamed, uid, funct);

    if ( listen_files[filepath] )
        foreach(string listener in listen_files[filepath])
            if ( listener != filepath )
                listener->event_change_file(filepath, uid, funct, renamed);

    if ( renamed && !listen_files[filepath] && listen_files[renamed] )
        foreach(string listener in listen_files[renamed])
            if ( listener != renamed )
                listener->event_change_file(filepath, uid, funct, renamed);

    foreach(string dir, string *list in listen_dirs) {
        string *done;

        done = ({ });
        if ( filepath[0..strlen(dir) - 1] == dir ) {
            list->event_change_file(filepath, uid, funct, renamed);
            done = copy(list);
        }

        // Let's not repeat the information to the same listeners
        //
        if ( renamed && renamed[0..strlen(dir) - 1] == dir ) {
            done = list - done;
            if ( sizeof(done) )
                done->event_change_file(filepath, uid, funct, renamed);
        }
    }
}

private void add_listener(string listener, string filepath) {
    if ( file_size(filepath) == -2 ) {
        if ( !listen_dirs[filepath] )
            listen_dirs[filepath] = ({ });

        if ( member_array(listener, listen_dirs[filepath]) == -1 )
            listen_dirs[filepath] += ({ listener });
    } else {
        if ( !listen_files[filepath] )
            listen_files[filepath] = ({ });

        if ( member_array(listener, listen_files[filepath]) == -1 )
            listen_files[filepath] += ({ listener });
    }

    if ( !listeners[listener] )
        listeners[listener] = ({ });

    if ( member_array(filepath, listeners[listener]) == -1 )
        listeners[listener] += ({ filepath });

    save_me();
}

private void remove_listener(string listener, string filepath) {
    if ( listen_dirs[filepath] ) {
        listen_dirs[filepath] -= ({ listener });
        if ( !sizeof(listen_dirs[filepath]) )
            listen_dirs = m_delete(listen_dirs, filepath);
    }

    if ( listen_files[filepath] ) {
        listen_files[filepath] -= ({ listener });
        if ( !sizeof(listen_files[filepath]) )
            listen_files = m_delete(listen_files, filepath);
    }

    if ( listeners[listener] ) {
        listeners[listener] -= ({ filepath });
        if ( !sizeof(listeners[listener]) )
            listeners = m_delete(listeners, listener);
    }

    save_me();
}

/**
 * Remove all references from listener, popular at object reload.
 */
varargs void purge_listener(string listener) {
    if ( origin() != "local" || !listener )
        listener = base_name(previous_object()) + ".c";

    if ( !listeners[listener] )
        return;

    foreach(string filepath in listeners[listener])
        remove_listener(listener, filepath);
}

/**
 * This is to avoid hell on earth.
 * If listener is changed it might not work, if it doesnt we
 * remove it from lists. If it works we tell it that it has
 * been changed, after all it might want to know.
 */
protected void validate_listener(string filepath, mixed uid, string funct) {
    object ob;

    if ( catch( (ob=load_object(filepath)) ) || !ob ) {
        purge_listener(filepath);
        return;
    }

    // Telling the listener that it has changed.
    // Causes a lot of unwanted attention and sanity checks.
    //
//  ob->event_change_file(filepath, uid, funct);
}

/**
 * Removing single entry ?
 */
void remove_file(string filepath) {
    string listener = base_name(previous_object()) + ".c";

    if ( !listeners[listener] || member_array(filepath, listeners[listener]) == -1 )
        return;

    listeners[listener] -= ({ filepath });
    if ( !sizeof(listeners[listener]) )
        listeners = m_delete(listeners, listener);

    remove_listener(listener, filepath);
}

/**
 * Object calls here to tell what file/dir argument to listen to.
 *
 * Dirs and files of log and save not allowed:
 *   /w/, /mud_name()/, /secure/log/, /tmp/, /open/ and /save/
 *
 * Only /handlers and /secure may do this. Therefore no read_file check.
 *
 * @param listener Internal handling for second argument only.
 */
varargs void add_file(string filepath, string listener) {
    if ( origin() != "local" || !listener )
        listener = base_name(previous_object()) + ".c";

    if ( listener[0..7] != "/secure/" && listener[0..9] != "/handlers/" ) {
        warning("add_file(\"" + filepath + "\") illegal handler.", 1);
        return;
    }

    foreach(string ignore, int len in ignoring) {
        if ( filepath == ignore ) {
            warning("add_file(\"" + filepath + "\") ignored..", 1);
            return;
        }

        if ( !len )
            continue;

        if ( filepath[0..len] == ignore ) {
            warning("add_file(\"" + filepath + "\") ignored.", 1);
            return;
        }
    }

    if ( file_size(filepath) == -2 && filepath[0] != 47 ) {
        warning("add_file(\"" + filepath + "\") aborted, directory "
                "lacking leading / letter.", 1);
        return;
    }

    if ( !listeners[listener] )
        listeners[listener] = ({ });
    if ( member_array(filepath, listeners[listener]) != -1 )
        return;

    listeners[listener] += ({ filepath });
    add_listener(listener, filepath);
}

/**
 * This method rewrites the indexes of files.
 * When this object loads we might wanna resubmit everything.
 */
protected varargs void rewrite_table(int resubmit) {
    mapping listen_copy = copy(listeners);

    not_listening = 0;
    listen_files  = ([ ]);
    listen_dirs   = ([ ]);
    listeners     = ([ ]);
    foreach(string listener, string *files in listen_copy) {
        object ob;

        if ( catch( (ob=load_object(listener)) ) || !ob || file_size(listener) < 1 )
            continue;

        if ( resubmit ) {
            ob->submit_filechange_list();
            continue;
        }
 
        foreach(string file in files)
            add_listener(file, listener);
    }
}

protected void daily_check() {
    if ( find_call_out("daily_check") == -1 )
        call_out("daily_check", 86400);

    rewrite_table(1);
}

void create() {
    ignoring = ([
        "/w/"                   : strlen("/w/") - 1,
        "/tmp/"                 : strlen("/tmp/") - 1,
        "/open/"                : strlen("/open/") - 1,
        "/save/"                : strlen("/save/") - 1,
        "/immortals/"           : strlen("/immortals/") - 1,
        "/secure/log/"          : strlen("/secure/log/") - 1,
        "/secure/dump/"         : strlen("/secure/dump/") - 1,
        "/doc/help/index/"      : strlen("/doc/help/index/") - 1,
        base_name(TO) + ".c"    : 0,
        "/" + mud_name() + "/"  : strlen("/" + mud_name() + "/") - 1,
    ]);

    ignoring_root = map(keys(filter(ignoring, (: $2 && sizeof(explode($1, "/")) == 1 :))), (: explode($1, "/")[0] :));
    listeners = ([ ]);
    listen_dirs = ([ ]);
    listen_files = ([ ]);
    load_me();

    // We want a slacker start after reboot.
    // The mud -REALLY- has a lot to do at that time.
    // The databases that "rely" on this must clean themselves at loading.
    // However, this object must be loaded before the listeners.
    //
    not_listening = 1;
    call_out("daily_check", 0);
}

/**
 * This is for the simul efun to early abort.
 * Since definitions are located here I wanted to keep the check here.
 */
int ignoring_root(string filepath) {
    if ( member_array(explode(filepath, "/")[0], ignoring_root) != -1 )
        return 1;

    if ( filepath[<1] == 126 )
        return 1;

    return 0;
}
