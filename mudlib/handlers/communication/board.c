/**
 * @main
 * The bulletin board system, where things can be illuminated.
 * Board objects comes to handle this one.
 *
 * Written god knows when by god knows who.
 *
 * Traceable developers over the years:
 *    Silbago 2006 - Included security settings.
 *    Flode   ???? - Major structure work over years.
 *    Bishop/Driadan/Baldrick/Taniwha/ etc ad infinitum
 */

#include <board.h>
#include <cmds.h>
#include <master.h>
#include <channels.h>
#include <handlers.h>

/**
 * This need attending, is it any point to keep it ?
 * Besides, it dumps boards to unsecure areas ?
 */
#define ARCHIVE_DIR "/open/boards/"

/** @ignore start */
string name;
int *timeouts;
int _num;
int deny_delete;
string *granters, *writers, *readers, *denied;
mixed subjects;

private nosave string *board_names;
private nosave mapping board_data;
private nosave string active_board;
private nosave int num;

class one_board {
    string name;
    array subjects;
    int *timeouts;
    string *denied, *readers, *writers, *granters;
    int deny_delete;
}

private nosave class one_board current;

//workaround to lessen spamming on mortals.
#define BLARGH(XXX,YYY) {                                      \
    object *obs = ENV(YYY) ? all_inventory(ENV(YYY)) : ({ });  \
    obs = filter(obs, (: $1->query_board_name() :));           \
    if (    group_member(YYY, "creators")                      \
         || sizeof(obs) )                                      \
    {                                                          \
        notify_fail(XXX);                                      \
        return 0;                                              \
    }                                                          \
    return 0;                                                  \
  }

void expire_boards();
int query_board_level(string name, mixed me);
private int zap_message(string board, int num);
private int set_active_board(string board);
/** @ignore end */

int query_num() { return num; }
int query_prevent_shadow() { return 1; }
string *query_boards() { return keys(board_data); }

void create() {
    SETEUID;
    board_data = ([ ]);
    num = 1;
    call_out("reload_table", 1);
    call_out("expire_boards", 15);
}

/**
 * Externalized to be compliant with remote calls via the multimud network.
 */
void reload_table() {
    num = 1;
    board_names = filter(get_dir(BOARD_DIR), (: file_size(BOARD_DIR + $1) == -2 :) );
    board_names -= ({ ".", ".." }); // just in case
    restore_object(BOARD_FILE,1);
    num = _num < 1 ? num : _num;

    board_data = ([ ]);
    foreach(string b in board_names)
    {
        class one_board current = new(class one_board);
        name = 0;
        timeouts = 0;
        num = 0;
        deny_delete = 0;
        granters = 0;
        writers = 0;
        readers = 0;
        denied = 0;
        subjects = 0;
        restore_object(BOARD_DIR + b + "/" + b, 1);
        if ( !subjects )
            subjects = ({ });
        if ( !timeouts )
            timeouts = ({ -1, -1, -1 });
        if ( !granters )
            granters = ({ "lords" });
        if ( !readers )
            readers = ({ "everyone" });
        if ( !writers )
            writers = ({ "everyone" });
        if ( !denied )
            denied = ({ });
        current->subjects = copy(subjects);
        current->timeouts = copy(timeouts);
        current->granters = copy(granters);
        current->writers = copy(writers);
        current->readers = copy(readers);
        current->denied = copy(denied);
        current->deny_delete = deny_delete;
        current->name = b;
        board_data[b] = copy(current);
    }
}

private int set_active_board(string board) {
    active_board = 0;
    if ( !board_data[board] )
        return 0;

    current = board_data[board];
    active_board = board;
    subjects = current->subjects;
    timeouts = current->timeouts;
    deny_delete = current->deny_delete;
    granters = current->granters;
    writers = current->writers;
    readers = current->readers;
    denied = current->denied;
    return 1;
}

/** @ignore start */
/**
 * Returns the class data for the board.
 * @author Silbago
 */
class one_board query_board_data(string board, object me) {
    if ( !board_data[board] )
        return 0;
    if ( !(query_board_level(board, me) & READ_MASK) )
        return 0;

    return copy(board_data[board]);
}
/** @ignore end */

mixed get_subjects(string name, object me) {
    if ( !(query_board_level(name, me) & READ_MASK) )
        return ({ });
    return subjects;
}

/**
 * Gives count of posts on a board.
 * @author Silbago
 */
int query_post_count(string name) {
    if ( !set_active_board(name) )
        return 0;
    return sizeof(current->subjects);
}

/**
 * Method used to find the id number of the last post on the given board.
 * @return time() last post on board was posted.
 */
int query_latest_post(string board_name, object me) {
    int i, almost_last_mess, last_mess=0;

    if ( !set_active_board(board_name) )
        return 0;

    if ( !(query_board_level(board_name, me) & READ_MASK) )
        return 0;

    for ( i=0; i < sizeof(subjects); i++ ) {
        almost_last_mess = subjects[i][B_TIME];
        if ( almost_last_mess > last_mess )
            last_mess = almost_last_mess;
    }
    return last_mess;
}

string get_message(string board, int num, object me) {
    string name;
    if ( !set_active_board(board) )
        return 0;
    if ( !(query_board_level(board, me) & READ_MASK) )
        return 0;
    if (num < 0 || num >= sizeof(current->subjects))
        return 0;

    name = sprintf("%s%d",
        BOARD_DIR + board + "/",
        current->subjects[num][B_NUM]);
    if (file_size(name) <= 0)
        return 0;
    return read_file(name);
}

int save_me() {
    if ( !board_data[active_board] )
        return 0;

    if ( file_size(BOARD_DIR) != -2 )
        mkdir(BOARD_DIR);

    name = active_board;
    subjects = current->subjects;
    timeouts = current->timeouts;
    deny_delete = current->deny_delete;
    denied = current->denied;
    granters = current->granters;
    writers = current->writers;
    readers = current->readers;
    _num = 0;

    if ( file_size(BOARD_DIR + active_board) != -2 )
        mkdir(BOARD_DIR + active_board);

     save_object(BOARD_DIR + active_board + "/" + active_board, 1);
    _num = num;
     save_object(BOARD_FILE, 1);
}

int add_message(string board, string name, string subject, string body) {
    string fname;
    int irp;

    if ( !set_active_board(board) )
        return 0; 
    if ( !(query_board_level(board, name) & WRITE_MASK) )
        return 0;
    if ( !body )
        return notify_fail("You will need a message as well.\n");

    while(file_size(BOARD_DIR + board + "/" + num) > 0)
        num ++;
    fname = sprintf("%s%d", BOARD_DIR + board + "/", num);
    current->subjects += ({ ({ subject, name, num, time() }) });

    write_file(fname, body);
    save_me();
    if (    timeouts
         && timeouts[T_MAX]
         && timeouts[T_MAX] != -1
         && sizeof(current->subjects) > timeouts[T_MAX])
    {
        while(sizeof(current->subjects) > timeouts[T_MAX])
        {
            zap_message(board, 0);
            irp++;
        }
        event(users(), "inform", capitalize(name)+" posts a message to "+board+
          " and "+irp+" message"+(irp>1?"s":"")+
          " explodes in sympathy", "message");
    }
    else
        event(users(), "inform", capitalize(name)+" posts a message to "+board, 
          "message");

    // Updating the players regarding subscriptions
    //
    if ( member_array(board, SUBSCRIBEABLE_BOARDS) != -1 ) {
        object *bing, *bing2;

        bing = filter(users(), (: $1->query_inform("news") :));

        bing2 = filter(users(), (:
                    member_array($(board), $1->query_property(SUBSCRIBE_PROP)) != -1
                :));

        bing += filter(bing2, (: member_array($1, $(bing)) == -1 :));

        map(bing, (: tell_object($1, "\nYou have unread %^YELLOW%^" +
              CAP($(board)) + "%^RESET%^. Use 'news' command to review, "
              "or 'help news'.\n\n") :));
    }

    MULTIMUD_DAEMON->request_transmission((["remote":({BOARD_HANDLER,"reload_table"})]));
    return num-1;
}

/**
 * Board must be created manually by the board command, by a master at least.
 */
int create_board(string board, object me) {
    class one_board cr;

    if ( !board || board_data[board] )
        return 0;

    if ( board == "bf" ) return 0;

    if (     base_name(previous_object()) != BOARD_CMD
         || !group_member(me, "masters") )
    {
        warning("Attempt to create board " + board + ", user not legal", 2);
        return 0;
    }
    active_board = board;
    cr = new(class one_board);

    // Setting up the default data in the board
    //
    cr->name = board;
    cr->subjects = subjects = ({ });
    cr->timeouts = timeouts = ({ DEFAULT_MIN, DEFAULT_MAX, DEFAULT_TIMEOUT });
    cr->deny_delete = deny_delete = 0;
    cr->granters = granters = ({ "lords" });
    cr->writers = writers = ({ "everyone" });
    cr->readers = readers = ({ "everyone" });
    cr->denied = denied = ({ });
    board_data[board] = cr;

    if ( board_names )
        board_names += ({ board });
    else
        board_names = ({ board });

    current = cr;
    save_me();
    write("Created board "+board+".\n");
    return 1;
}

/**
 * Internal method that deletes message number 'off' in board 'board.
 * Deals with checks and data cleanup required.
 */
private int zap_message(string board, int off) {
    int num;
    string nam, archive;

    if ( !set_active_board(board) )
        BLARGH("Can't find board.", TP)

    if ( current->deny_delete )
        BLARGH("You lack the sufficient privilege to delete post.\n", TP)

    if ( name == "quests" )
        BLARGH("Now listen closely, I will only say this once before you get demoted.\n"
               "   %^YELLOW%^*** You may ---NOT--- *** delete ANY posts in the quests board ***%^RESET%^\n"
               "Send a request to Silbago or Titan about this.\n"
               "If we find it possible, we will do it for you.\n"
               "Meanwhile don't try to be a smartass and hack it.\n"
               "If you didnt know, consider this FYI.\n"
               "If you're testing if it can be done, read first line in this message again.\n"
               "If you're about to manage to hack it, you will be demoted.\n", TP)

    if ( off < 0 || off >= sizeof(current->subjects) )
        BLARGH("Post number is off.\n", TP)

    num = current->subjects[off][B_NUM];
    nam = sprintf("%s%d", BOARD_DIR + board + "/", num);
    archive = ARCHIVE_DIR + board;

    if ( archive ) {
        array stuff = current->subjects[off];

        write_file(archive,
          sprintf("\n----\nNote #%d by %s posted at %s\nTitle: '%s'\n\n",
            off, capitalize(stuff[B_NAME]), ctime(stuff[B_TIME]),
            stuff[B_SUBJECT])+
          read_file(nam));
    }
    current->subjects = delete(subjects,off,1);
    rm(nam);
    save_me();
    MULTIMUD_DAEMON->request_transmission((["remote":({BOARD_HANDLER,"reload_table"})]));
    return 1;
}

/**
 * Method to delete a message on a board by external query.
 */
int delete_message(string board, int off, object me) {
    if ( !set_active_board(board) )
        return 0;

    //Balo ganked some code to allow for bounds checking of eaten notes. 5/20/00
    if ( off < 0 || off >= sizeof(current->subjects) )
        BLARGH("Post id is off.\n", TP)

    if ( base_name(previous_object()) != BOARDS_CMD )
        BLARGH("Wrong command.\n", TP)

    if ( !me || (TP && TP != me) || (call_user() && call_user() != geteuid(me)) ) {
        warning("Delete message, unsufficient doer " + sprintf("%O\n", previous_object()));
        return 0;
    }

    // Requirements met/not met to delete a post
    //
    if ( !(query_board_level(board, me) & READ_MASK) )
        BLARGH("Access denied, no read.\n", TP)
    if (    (query_board_level(board, me) & WRITE_MASK)
         && subjects[off][B_NAME] == geteuid(me) );
    else if ( !(query_board_level(board, me) & GRANT_MASK) )
        BLARGH("No grant access!\n", TP)

    return zap_message(board, off);
}

int delete_board(string board) {
    if ( !set_active_board(board) )
        return 0;
    if ( !(query_board_level(board, TP) & GRANT_MASK) )
        return 0;

    foreach(board in map(current->subjects, (: $2 :)))
        rm(BOARD_DIR + board + "/" + board);
    rm(BOARD_DIR + board + "/" + board + ".o");
    board_data = m_delete(board_data, board);

    return 1;
}

/**
 * Returns list of the boards on FR, that 'me' can read.
 */
string *list_of_boards(mixed me) {
    if ( !me )
        me = geteuid(TP);

    return filter(board_names, (: query_board_level($1, $(me)) & READ_MASK :));
}

/**
 * If board has max boards set ( != -1 ), it deletes old posts.
 * If board has more than minimum boards ( != -1 ) and timeout
 * is set, it deletes posts older than timeout.
 */
void expire_boards() {
    int num;
    string name;
    class one_board current;

return; // Need to fix some bugs, silbago nov 07
    foreach(name, current in board_data) {
        if ( current->deny_delete || name == "quests" )
            continue;
        if ( current->timeouts[T_MAX] == -1 )
            continue;
        if ( sizeof(current->subjects) <= current->timeouts[T_MIN] )
            continue;
        if ( current->timeouts[T_TIMEOUT] == -1 )
            continue;
        num = 0;
        while(sizeof(current->subjects) > current->timeouts[T_MIN] &&
              current->subjects[0][B_TIME]+(current->timeouts[T_TIMEOUT]*24*60*60) < time())
        {
            zap_message(name, 0);
            num++;
        }
        if ( num )
            event(users(), "inform", "Board handler removes "+num+" message"+
              (num == 1 ? "" : "s") + " from "+name, "message");
    }

    if ( find_call_out("expire_boards") == -1 )
        call_out("expire_boards", 60*60);
}

/**
 * Data modifying from the board command
 */
void set_var(string var, string name, mixed set, object me) {
    if ( base_name(previous_object()) != BOARD_CMD )
        return;

    if ( !(query_board_level(name, me) & GRANT_MASK) )
        return 0;
    if ( !set_active_board(name) )
        return;

    switch(var) {
    case "maximum posts":
        set = to_int(set);
        if ( set < DEFAULT_MIN || set > 2000 )
            return;
        current->timeouts[T_MAX] = set;
        break;
    case "minimum posts":
        set = to_int(set);
        if ( set < 0 )
            return;
        else if ( set < DEFAULT_MIN && set > current->timeouts[T_MAX] )
            return;
        else if ( set > 500 )
            return;
        current->timeouts[T_MIN] = set;
        break;
    case "timeout":
        set = to_int(set);
        if ( set < 3 || set > 365 )
            return;
        current->timeouts[T_TIMEOUT] = set;
        break;
    case "toggle delete":
        current->deny_delete = !current->deny_delete;
        break;
    case "add granter":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->granters) != -1 )
            return;
        current->granters += ({ set });
        break;
    case "add writer":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->writers) != -1 )
            return;
        current->writers +=  ({ set });
        break;
    case "add reader":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->readers) != -1 )
            return;
        current->readers +=  ({ set });
        break;
    case "add denied":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->denied) != -1 )
            return;
        current->denied +=  ({ set });
        break;
    case "remove granter":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->granters) == -1 )
            return;
        current->granters -= ({ set });
        break;
    case "remove writer":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->writers) == -1 )
            return;
        current->writers -=  ({ set });
        break;
    case "remove reader":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->readers) == -1 )
            return;
        current->readers -=  ({ set });
        break;
    case "remove denied":
        if ( !user_exists(set) && !group_exists(set) )
            return;
        if ( member_array(set, current->denied) == -1 )
            return;
        current->denied -=  ({ set });
        break;
    default:
        return;
    }

    save_me();
}

/**
 * This method determines access level user 'me' has to given
 * board. It also loads the data on the board so you won't need
 * to worry about that. What is exceptional here, is that
 * boards where grant level is given to a player organization
 * of type sig/guild/clan, the council members is given
 * grant access.
 * @author Silbago
 */
int query_board_level(string name, mixed me) {
    string *cou;

    if ( !set_active_board(name) )
        return 0;

    if ( base_name(previous_object()) == "/net/web/services" )
        return WRITE_MASK | READ_MASK;

    if ( !me ) // this is a bug actually
        me = TP;
    if ( !me )
        return 0;
    if ( objectp(me) )
        me = geteuid(me);

    if ( member_array(me, current->denied) != -1 || group_member(me, current->denied) )
        return 0;
    if ( member_array(me, current->granters)  != -1 || group_member(me, current->granters) )
        return GRANT_MASK | WRITE_MASK | READ_MASK;
    if ( sizeof(current->granters) )
        cou = map(current->granters, (: $1 + "_council" :));
    if ( group_member(me, cou) )
        return GRANT_MASK | WRITE_MASK | READ_MASK;
    if ( member_array(me, current->writers)  != -1 || group_member(me, current->writers) )
        return WRITE_MASK | READ_MASK;
    if ( member_array(me, current->readers)  != -1 || group_member(me, current->readers) )
        return READ_MASK;
    return 0;
}
