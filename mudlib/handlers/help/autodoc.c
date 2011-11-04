/**
 * @main
 *
 * Shattered Realms Autodoc parser, idea from Discworld and the concept is
 * originally by and for Java (javadoc). No code copying from any source.
 *
 * In brief, this object parses source code in mudlib and generates helpfiles.
 * The helpfiles are searchable by the adoc command, where class name and
 * functions are indexeable.
 *
 * The command interface is named autodoc, and is located in /commands/
 *
 * Helpfiles contains:
 *   - Description / comment in autodoc style
 *   - functions, private and public
 *   - defines
 *   - includes
 *   - inherits
 *
 * Comments are attached to each of these cathegories except for includes and
 * inherits, if done in autodoc style.
 *
 * The parsing is done in a specific pattern and when finished it is passed to
 * the autodoc_nroff handler.
 *
 * Committing a file for autodoc parsing is done each time a creator uses the
 * update command, and in addition, one can use the autodoc command to
 * -rebuild entire help database.
 *
 * These helpfiles are the entire mudlib documentation, other docs are more
 * tuturial-oriented and so forth
 *
 * Ok, what kind of flags do we have and what do they mean ?
 * The intention is to show information clearly on several categories.
 *     - @main        : To "tag" description as FILE description
 *     - @author      : NAME shows nicely as Written By
 *     - @comment     : Future to do with this piece of code.
 *     - @return      : About what the given function returns
 *     - @seealso     : Other information you should seek out
 *     - @version     : Program version number, changelog info etc.
 *     - @param       : Information about input parameters to fun
 *     - @dontignore  : To tag a "do not doc function" as NOT.
 *
 * @comment Some errors, and poor parsing still. Class is a big problem,
 * @comment and @THINGS + @@THINGS are not dealt with. So there'
 * @comment s still problems to deal with. #ifdef, #elif and so fort
 * @comment h also screws up. We should try to upgrade the parsing.
 * @comment Uh, ignore start and end stuff doesnt work. Any ideas ?
 * @comment Need to make @dontignore flag against IGNORE_FUNCTIONS
 * @author Thulsa
 */

#include <handlers.h>
#include <autodoc.h>
#define SAVE_FILE "/"+mud_name()+"/save/autodoc"
/**
 * Turns to next line, if passed last line. Forces saving helpdoc
 */
#define NEW_LINE \
    if ( new_line() ) {\
        call_out("make_helpfile", 0);\
        return;\
    }
/**
 * No White Space
 */
#define NOWS(XXX) (replace_string(XXX, " ", ""))

nosave mapping autodoc_nroff;      // This is the generated help document in map
nosave string *autodoc_file_lines, // Lines in array of currently being processed
              autodoc_file_line,   // Line in array being processed
              being_autodoced,     // filepath of object being processed
              autodoc_argument,    // To nroff mapping.
              tmp_str,
              tmp_str2,
              tmp_str3,
              tmp_str4,
              tmp_str5;
nosave mixed autodoc_comment;      // Latest comment generated
nosave int line_count,             // What line is being parsed/analyzed
           num_of_lines,           // Number of lines in object file
           i,
           is_main,                // Flag to notify that it is main description
           multi_line_define,      // Define macro goes over more than one line
           is_parsing;             // From <autodoc.h>, identifies what we have

nosave int start_time;             // Nice to keep track
/** @ignore start */
/* -----------------------PROTOTYPES----------------------- */
void check_call_out_alive();
void parse_include();
void parse_define();
void parse_function();
void parse_macro();
void parse_variable();
void parse_autodoc_comment();
void parse_class();
void make_helpfile();
int new_line();
varargs void add_to_autodoc_nroff(int IsMainDescription);
void add_adoc_mess(string Parameter, string Description);
int public_or_protected_function(string FuncDeclaration);
int brackets_in_text(string Bracket);
/* -------------------END OF PROTOTYPES-------------------- */
/** @ignore end */

void create() {
    autodoc_comment = 0;
    being_autodoced = 0;
    is_parsing = 0;
    seteuid("Doc");
}

/**
 * The queue controlled orders parsing of given file.
 * @dontignore
 */
void parse_this(string Path) {
    if ( base_name(previous_object()) != AUTODOC_QUEUE_HANDLER )
        return;

    being_autodoced = Path;
    call_out("autodoc_object", 20 + random(10));
}

/**
 * Reset Variables and starts cleaning of the file.
 */
void autodoc_object() {
    string AutodocFile; // We might not need this here

    autodoc_nroff = ([]);
    line_count = 0;
    multi_line_define = 0;
    autodoc_argument = 0;
    if ( file_size(being_autodoced) < 1 ) {
        warning("Attempting to autodoc empty file " + being_autodoced);
        AUTODOC_QUEUE_HANDLER->finished_parsing(being_autodoced);
        call_out("destruct_me", 0);
        return;
    }

    if ( !strlen(being_autodoced) ) {
        warning("Told to start autodoc file, but I havent been told what file.");
        return;
    }

    AutodocFile = read_file(being_autodoced);
    if ( !AutodocFile ) {
        warning("Autodoc Handler failed to read file: " + being_autodoced);
        call_out("destruct_me", 0);
        AUTODOC_QUEUE_HANDLER->finished_parsing(being_autodoced);
        return;
    }

    autodoc_file_lines = explode(AutodocFile, "\n");
    // We dont need to autodoc oneline'd objects :)
    // And we don't need the first line in any case
    if ( sizeof(autodoc_file_lines) < 2 ) {
        AUTODOC_QUEUE_HANDLER->finished_parsing(being_autodoced);
        call_out("destruct_me", 0);
        return;
    }

    // Some initial cleanups, removing empty lines
    //
    for ( int i=0; i < sizeof(autodoc_file_lines); i++ ) {
        if (    !autodoc_file_lines[i]
             || autodoc_file_lines[i] == ""
             || !strlen(replace_string(autodoc_file_lines[i], " ", "")) )
        {
            autodoc_file_lines[i] = 0;
        }
    }
    autodoc_file_lines -= ({ 0 });

    i = 0;
    call_out("continue_autodoc_object1", 3 + random(3));
} /* autodoc_object() */

/**
 * Method to remove code between @ignore start/end arguments
 * including the arguments themselves.
 */
protected void continue_autodoc_object1() {
    int do_ignore = 0;

    for ( i=0; i < sizeof(autodoc_file_lines); i++ ) {
        switch(autodoc_file_lines[i]) {
        case "/** @ignore end */":
        case "/** @ignore start */":
            do_ignore = !do_ignore;
            autodoc_file_lines[i] = 0;
        }
        if ( do_ignore )
            autodoc_file_lines[i] = 0;
    }
    autodoc_file_lines -= ({ 0 });
    line_count = sizeof(autodoc_file_lines);
    tmp_str4 = "one"; // Pretend that you dont see this
    i = -1;
    call_out("continue_autodoc_object2", 1);
} /* continue_autodoc_object1() */

/**
 * NOSTR is short for No STRingS.
 * This function removes "strings like this" in a line.
 * Kept short, and capitalized in macro style. (May the gods forgive me)
 * Keeps code cleaner and nicer.
 */
string NOSTR(string str) {
    int ii, StrLen, InStr = 0;
    string RetVal;

    // Doubble error protection
    if ( !str || str == "" )
        return "  ";
    // No usage of " whatsoever, bye bye
    if ( strsrch(str, "\"") == -1 )
        return str;

    RetVal = "";
    StrLen = strlen(str);
    for ( ii=0; ii < StrLen; ii++ ) {
        // This is causing a lot of eval cost errors in a well balanced
        // code otherwise. Especially when we scan hundreds of files
        // at the same time. Sigh.
        //
        if ( !(ii%20) )
            reset_eval_cost();
        if ( str[ii..ii] == "\"" && str[ii-1..ii] != "\\\"" ) {
            InStr = InStr ? 0 : 1;
            // Since we continue; We need to pick up last letter on line
            if ( (ii+1) >= StrLen )
                RetVal += str[ii..ii];
            continue;
        }
        if ( !InStr )
            RetVal += str[ii..ii];
    }
    return RetVal;
}

/**
 * We remove regular multi-lined comments. Not autodoc style.
 */
void continue_autodoc_object2() {
    string Line;
    int IsInComment = 0;

    foreach(Line in autodoc_file_lines) {
        i++;
        if ( !Line || Line == "" ) {
            autodoc_file_lines[i] = 0;
            continue;
        }
        if ( IsInComment == 1 ) {
            if ( sscanf(Line, "%s*/%s", tmp_str2, tmp_str) == 2 ) {
                autodoc_file_lines[i] = tmp_str != "" ? tmp_str : 0;
                IsInComment = 0;
            } else
                autodoc_file_lines[i] = 0;
            continue;
        }
        // We are inside autodoc comment, passing on to end
        if ( IsInComment == 2 ) {
            if ( Line == " */" )
                IsInComment = 0;
            continue;
        }
        if ( Line == "/**" || Line[0..3] == "/** " ) {
            if ( Line[<2..] != "*/" )
                IsInComment = 2;
            continue;
        }
        // We found a comment.
        if ( sscanf(NOSTR(Line), "%s/*%s", tmp_str, tmp_str2) == 2 ) {
            Line = tmp_str;
            // Start and end of comment at same line
            if ( sscanf(tmp_str2, "%s*/%s", tmp_str, tmp_str3) == 2 ) {
                Line += tmp_str3;
                if ( Line == "" )
                    Line = 0;
                autodoc_file_lines[i] = Line;
                continue;
            }
            autodoc_file_lines[i] = Line == "" ? 0 : Line;
            IsInComment = 1;
        }
    }
    autodoc_file_lines -= ({ 0 });
    // We run this twice to get rid of leftovers
    if ( tmp_str4 != "twice" ) {
        i = -1;
        tmp_str4 = "twice";
        call_out("continue_autodoc_object2", 1);
        return;
    }
    i=0;
    call_out("continue_autodoc_object3", 1);
} /* continue_autodoc_object2() */

/**
 * We remove single line comments
 */
void continue_autodoc_object3() {
    string Line;

    foreach(Line in autodoc_file_lines) {
        if ( Line ==  "" || Line[0..1] == "//" )
            autodoc_file_lines[i] = 0;
        else if ( strsrch(NOSTR(autodoc_file_lines[i]), "//") != -1 )
            autodoc_file_lines[i] = explode(Line, "//")[0];
        i++;
    }
    autodoc_file_lines -= ({ 0 });
    i = 0;
    call_out("continue_autodoc_object4", 1);
} /* continue_autodoc_object3() */

/**
 * Remove empty lines or lines with space only.
 */
void continue_autodoc_object4() {
    string Line;

    foreach(Line in autodoc_file_lines ) {
        if ( Line == "" || !strlen(replace_string(Line, " ", "")) )
            autodoc_file_lines[i] = 0;
        i++;
    }
    autodoc_file_lines -= ({ 0 });
    line_count = sizeof(autodoc_file_lines);
    i = -1;
    call_out("continue_autodoc_object5", 1);
} /* continue_autodoc_object4() */

/**
 * If we have macros, we remove all space at that line to make
 * the job easier for macro parsing. Needs to be done anyways.
 * That is, space between '#' _and the_ 'MACRO'
 */
void continue_autodoc_object5() {
    string Line;
    int j = 0;

    foreach(Line in autodoc_file_lines  ) {
        i++;
        if ( Line[0] != '#' )
            continue;
        while(Line[++j] == ' ');
        autodoc_file_lines[i] = "#"+Line[j..];
        j = 0;
    }
    i = -1;
    call_out("continue_autodoc_object6", 1);
} /* continue_autodoc_object5() */

/**
 * All whitespace at start of line is stripped. With exception of autodoc style
 * comments. Reason for this is efficiency for the other functions to come
 */
void continue_autodoc_object6() {
    string Line;
    int j=-1;

    foreach(Line in autodoc_file_lines   ) {
        i++;
        // We have an autodoc comment folx
        if ( Line[0..1] == " *" )
            continue;
        while(Line[++j] == ' ');
        if ( j > 0 )
            autodoc_file_lines[i] = Line[j..];
        j = -1;
    }
    autodoc_file_lines -= ({ 0, "" });
    // For debugging purposes
/* DISABLED !!!
    write_file("/save/Doc/autodoc_debug",
        implode(autodoc_file_lines, "\n"), 1);
*/
    num_of_lines = sizeof(autodoc_file_lines);
    line_count = -1;
    new_line();
    call_out("parse_autodoc", 1);
} /* continue_autodoc_object6() */

/**
 * This function finds out who shall take the job. And takes over
 * when those parsers are done.
 */
void parse_autodoc() {
    if ( autodoc_file_line[0..2] == "/**" ) {
        call_out("parse_autodoc_comment", 0);
        return;
    }
    if ( autodoc_file_line[0..7] == "inherit " ) {
        tmp_str = autodoc_file_line[8..];
        is_parsing = IS_INHERIT;
        autodoc_argument = replace_string(tmp_str, ([ "\"":"", ";":"" ]));
        add_to_autodoc_nroff();
        NEW_LINE
        call_out("parse_autodoc", 0);
        return;
    }
    if ( autodoc_file_line[0] == '#') {
        autodoc_file_line = autodoc_file_line[1..];
        call_out("parse_macro", 0);
        return;
    }
    if ( autodoc_file_line[0..4] == "class" ) {
        call_out("parse_class", 0);
        return;
    }
    // Might be function.
    if ( sscanf(autodoc_file_line, "%s(%s", tmp_str2, tmp_str3) == 2 ) {
        if ( strsrch(tmp_str2, "=") == -1 ) {
            call_out("parse_function", 0);
            return;
        }
    }
    call_out("parse_variable", 0);
} /* parse_autodoc() */

/**
 * Wraps in comments, for other parsers to grab later on
 * @comment More stress testing, looks good though
 */
void parse_autodoc_comment() {
    int NewLineFlag = 1;

    is_parsing = IS_AUTODOC_COMMENT;
    is_main = 0;
    if ( autodoc_file_line == "/** @ignore yes */" ) {
        autodoc_comment = "@ignore";
        NEW_LINE
        return parse_autodoc();
    }
    autodoc_comment = ([]);
    // First line of comment style is bound, and now we continue to next line
    // fix in @main somehow
    while(1) {
        NEW_LINE
        // Ok, last comment is ended
        if ( autodoc_file_line == " */" || autodoc_file_line == "*/" ) {
            NEW_LINE
            if ( is_main ) {
                add_to_autodoc_nroff(1);
            }
            call_out("parse_autodoc", 1);
            return;
        }
        // If we're ignoring, we just want to get to the end of it !
        if ( stringp(autodoc_comment) && autodoc_comment[0..6] == "@ignore" )
            continue;

        // Error escapes
        //
        if (    strlen(autodoc_file_line) < 2
             || !strlen(replace_string(autodoc_file_line, " ", "")) )
        {
            continue;
        }
        if ( autodoc_file_line[1] != '*' )
            autodoc_file_line = " * " + autodoc_file_line[2..];

        sscanf(autodoc_file_line, " *%s", tmp_str);
        if ( !tmp_str )
            tmp_str = "";
        if ( !strlen(replace_string(tmp_str, " ", "")) ) {
            if ( !autodoc_comment[IS_DESCRIPTION] )
                autodoc_comment[IS_DESCRIPTION] = "";
            autodoc_comment[IS_DESCRIPTION] += "\n";
            continue;
        }
        tmp_str = tmp_str[1..];
        if ( tmp_str == "@main" ) {
            is_main = 1;
            continue;
        }

        if ( tmp_str == "@unignore" ) {
            warning("flag @unignore, change to @dontignore in "
                    + being_autodoced);
            autodoc_comment[IS_DONTIGNORE] = 1;
            continue;
        }

        // Overruling IGNORE_FUNCTIONS array
        //
        if ( tmp_str == "@dontignore" ) {
            autodoc_comment[IS_DONTIGNORE] = 1;
            continue;
        }
        // Starting (yes) or stopping ignore (anything)
        //
        if ( tmp_str[0..6] == "@ignore" ) {
            tmp_str = tmp_str[8..];
            if ( tmp_str && tmp_str == "yes" )
                autodoc_comment = "@ignore";
            else
                autodoc_comment = 0;

            continue;
        }
        // Now we find out if there's params wanted, or just plaintext to add.
        if ( tmp_str[0] != '@' ) {
            if ( tmp_str == "" )
                tmp_str = " ";
            if ( NewLineFlag )
                tmp_str += "\n";
            if ( !autodoc_comment[IS_DESCRIPTION] )
                autodoc_comment[IS_DESCRIPTION] = tmp_str;
            else
                autodoc_comment[IS_DESCRIPTION] += (NewLineFlag?"":" ")+tmp_str;
        } else {
            tmp_str = tmp_str[1..];                              // parameter name
            tmp_str2 = implode(explode(tmp_str, " ")[1..], " "); // description
            tmp_str = explode(tmp_str, " ")[0];
            // add parameter, if not legal.. Plain string
            // isnt dealing well at all.. Dang Dang Dang
            add_adoc_mess(tmp_str, tmp_str2);
        }
    }

    NEW_LINE
    call_out("parse_autodoc", 1);
} /* parse_autodoc_comment() */

/**
 * Ignoring class'es entirely
 * This is not working very well (at all)
 */
void parse_class() {
    int Brackets = 0;
    while(1) {
        if ( sscanf(autodoc_file_line, "%s}%s", tmp_str, tmp_str2) == 2 )
            Brackets -= brackets_in_text("}");
        if ( sscanf(autodoc_file_line, "%s{%s", tmp_str, tmp_str2) == 2 )
            Brackets += brackets_in_text("{");
        NEW_LINE
        if ( !Brackets ) {
            call_out("parse_autodoc", 0);
            return;
        }
    }
} /* parse_class() */

/**
 * Lists all define's. And support comments as functions.
 */
void parse_define() {
    multi_line_define = 0;
    is_parsing = IS_DEFINE;
    autodoc_argument = autodoc_file_line;
    if ( NOWS(autodoc_argument)[<1] == '\\' ) {
        sscanf(autodoc_argument, "%s\\", autodoc_argument);
        multi_line_define = 1;
    }
    if ( sscanf(autodoc_argument, "%s(", tmp_str) )
        autodoc_argument = tmp_str;

    autodoc_argument = explode(autodoc_argument, " ")[0];
    add_to_autodoc_nroff();

    // If we get here, we must find the end of it, quick and easy peasy
    // This will only be very few lines anyways, so dont worry about eval cost.
    while( multi_line_define ) {
        NEW_LINE
        tmp_str = NOWS(autodoc_file_line);
        if ( tmp_str[<1..] != "\\" )
            break;
    }
    NEW_LINE
    call_out("parse_autodoc", 0);
} /* parse_define() */

/**
 * Makes an include list, two types. One for /include dir, and one for the others.
 */
void parse_include() {
    is_parsing = IS_INCLUDE;
    is_parsing = autodoc_file_line[0..0] == "<" ? IS_GLOBAL_INCLUDE : IS_LOCAL_INCLUDE;
    autodoc_argument = autodoc_file_line[1..<2];
    add_to_autodoc_nroff();
    NEW_LINE
    call_out("parse_autodoc", 0);
} /* parse_include() */

/**
 * Parses in private and public funtions
 * @comment Doesnt deal well with prototypes
 */
void parse_function() {
    string FunctionName, FunctionDeclarations;
    int Eof = 0,
        Brackets = 0,
        AddToFunDecl = 1,
        FirstBracketFound = 0;

    autodoc_argument = 0;
    is_parsing = IS_FUNCTION;

    // It just could be a prototype, Oneliners.
    if ( sscanf(NOWS(autodoc_file_line), "%s(%s);%s",
                tmp_str3, tmp_str2, tmp_str) == 3 )
    {
        if ( tmp_str == "" ) {
            autodoc_argument = 0;
            autodoc_comment = 0;
            NEW_LINE
            call_out("parse_autodoc", 0);
            return;
        }
    }

    if ( !autodoc_comment || !mappingp(autodoc_comment) )
        autodoc_comment = ([]);

    // Let's find name of the function
    FunctionDeclarations = autodoc_file_line;
    sscanf(FunctionDeclarations, "%s(%s", FunctionName, tmp_str);
    i = -1;
    while(FunctionName[++i] == ' ' );
    if ( i > 0 )
        FunctionName = FunctionName[i..];
    i = strlen(FunctionName);
    while(FunctionName[--i] == ' ' );
    if ( i < strlen(FunctionName) )
        FunctionName = FunctionName[0..i];
    FunctionName = explode(FunctionName, " ")[<1];
    if ( sscanf(FunctionName, "*%s", tmp_str) )
        FunctionName = tmp_str;

    // We just might be lucky, and the entire function is one
    if ( sscanf(autodoc_file_line, "%s(%s)%s{%s}%s",
                tmp_str, tmp_str2, tmp_str3, tmp_str4, tmp_str5) == 5 )
    {
        // Ok, let's keep the @dontignore functions among the IGNORE funs
        //
        if (    !autodoc_comment[IS_DONTIGNORE]
             && member_array(FunctionName, IGNORE_FUNCTIONS) != -1 )
        {
            autodoc_argument = 0;
            autodoc_comment = 0;
            NEW_LINE
            call_out("parse_autodoc", 0);
            return;
        }
        autodoc_argument = FunctionName;
        sscanf(autodoc_file_line, "%s)%s", tmp_str, tmp_str2);
        autodoc_comment[IS_FULL_FUNCTION] = tmp_str+")";
        is_parsing = public_or_protected_function(tmp_str) ?
                     IS_PROTECTED_FUNCTION : IS_PUBLIC_FUNCTION;
        add_to_autodoc_nroff();
        NEW_LINE
        call_out("parse_autodoc", 0);
        return;
    }

    // Find the full function declaration. Then end of function, last } bracket.
    if ( sscanf(autodoc_file_line, "%s)%s", FunctionDeclarations, tmp_str) == 2 ) {
           FunctionDeclarations += ")";
           AddToFunDecl = 0;
    } else
        FunctionDeclarations = "";
    while(1) {
        // Something ugly happens here, fix later
        // On last function, we dont stop at last }
        //
        if ( AddToFunDecl && FunctionDeclarations != autodoc_file_line ) {
            if ( sscanf(autodoc_file_line, "%s)%s", tmp_str, tmp_str2) == 2 ) {
                FunctionDeclarations += autodoc_file_line+")";
                AddToFunDecl = 0;
            } else
                FunctionDeclarations += autodoc_file_line;
        }
        if ( sscanf(autodoc_file_line, "%s{%s", tmp_str, tmp_str2) == 2 ) {
            Brackets += brackets_in_text("{");
            FirstBracketFound = 1;
        }
        if ( sscanf(autodoc_file_line, "%s}%s", tmp_str, tmp_str2) == 2 ) {
            Brackets -= brackets_in_text("}");
        }
        if ( (Eof=new_line()) || (!Brackets && FirstBracketFound) )
            break;
    }

    // Ok, let's keep the @dontignore functions among the IGNORE funs
    //
    if (    !autodoc_comment[IS_DONTIGNORE]
         && member_array(FunctionName, IGNORE_FUNCTIONS) != -1 )
    {
        autodoc_argument = 0;
        autodoc_comment = 0;
        call_out("parse_autodoc", 0);
        return;
    }

    autodoc_argument = FunctionName;
    autodoc_comment[IS_FULL_FUNCTION] = FunctionDeclarations;
    is_parsing = public_or_protected_function(FunctionDeclarations) ?
                 IS_PROTECTED_FUNCTION : IS_PUBLIC_FUNCTION;

    add_to_autodoc_nroff();
    if ( Eof )
        call_out("make_helpfile", 1);
    else
        call_out("parse_autodoc", 1);
} /* parse_function() */

/**
 * Lots of things can be done with #. We ignore all but include and define.
 */
void parse_macro() {
    int MacroCount = 1,
        FirstMacro = 0;

    if ( autodoc_file_line[0..6] == "define " ) {
        autodoc_file_line = autodoc_file_line[7..];
        return parse_define();
    }
    if ( autodoc_file_line[0..7] == "include " ) {
        autodoc_file_line = autodoc_file_line[8..];
        return parse_include();
    }
    if (    autodoc_file_line[0..4] == "undef"
         || autodoc_file_line[0..5] == "pragma" )
    {
        NEW_LINE
        autodoc_argument = 0;
        autodoc_comment = 0;
        return parse_autodoc();
    }
    // #define in #ifdef's are ignored entirely, easier that way.
    NEW_LINE
    while(1) {
        if ( autodoc_file_line[0] != '#' ) {
            NEW_LINE
            continue;
        }
        tmp_str = explode(autodoc_file_line[1..], " ")[0];
        if ( member_array(tmp_str, ({ "if", "ifdef", "elif" })) != -1 ) {
            MacroCount++;
            FirstMacro = 1;
        } else if ( tmp_str == "endif" )
            MacroCount--;
        NEW_LINE
        if ( !MacroCount && FirstMacro ) {
            autodoc_argument = 0;
            autodoc_comment = 0;
            return parse_autodoc();
        }
    }
} /* parse_macro() */

/**
 * This function is called when it is a variable. And all it does is
 * going while() until it is bypassed.
 */
void parse_variable() {
    while( sizeof(NOWS(autodoc_file_line)) && NOWS(autodoc_file_line)[<1] != ';' )
        NEW_LINE
    NEW_LINE
    call_out("parse_autodoc", 0);
} /* parse_variable() */

/**
 * Someone likes to write {} brackets in text. This is always done inside "".
 * @param Bracket is hex value of bracket
 * @return number of start brackets at line, excluding text.
 */
int brackets_in_text(string Bracket) {
    int Count = 0,
        ii;
    string Line = NOSTR(autodoc_file_line);

    for ( ii=0; ii < strlen(Line); ii++ ) {
        if ( Line[ii..ii] == Bracket && Line[ii-1..ii+1] != "'"+Bracket+"'" )
            Count++;
    }
    return Count;
} /* brackets_in_text() */

/**
 * A somewhat "bigger" and more standarized method of adding comments.
 * Now every key is a integer rather than string. Saving space ?
 */
void add_adoc_mess(string Parameter, string Description) {
    int IsParsing = -1;

    if ( !Parameter || !Description || Parameter == "" || Description == "" )
        return;
    switch(Parameter) {
    case "version":
        IsParsing = IS_VERSION;
        break;
    case "param":
        IsParsing = IS_PARAM;
        break;
    case "return":
        IsParsing = IS_RETURN;
        break;
    case "author":
        IsParsing = IS_AUTHOR;
        break;
    case "seealso":
        IsParsing = IS_SEEALSO;
        break;
    case "comment":
        IsParsing = IS_COMMENT;
        break;
    }
    switch(IsParsing) {
    case IS_PARAM:
    case IS_RETURN:
        if ( !autodoc_comment[IsParsing] )
            autodoc_comment[IsParsing] = "";
        autodoc_comment[IsParsing] += Description;
        break;
    case IS_SEEALSO:
    case IS_VERSION:
    case IS_AUTHOR:
    case IS_COMMENT:
        if ( !autodoc_comment[IsParsing] )
            autodoc_comment[IsParsing] = ({ });
        autodoc_comment[IsParsing] += ({ Description });
        break;
    default:
       if ( !autodoc_nroff[IS_DESCRIPTION] )
           autodoc_nroff[IS_DESCRIPTION] = "@"+Parameter+" "+Description;
       else
           autodoc_nroff[IS_DESCRIPTION] += " @"+Parameter+" "+Description;
        break;
    }
} /* add_adoc_mess() */

/**
 * Method to find if it is a public or private function
 * @return 0 if public
 */
int public_or_protected_function(string FuncDeclaration) {
    string TmpStr, TmpStr2;
    sscanf(FuncDeclaration, "%s(",TmpStr);
    if ( !TmpStr || TmpStr == "" || !strlen(replace_string(TmpStr, " ", "")) )
        return 0;
    if ( sscanf(TmpStr, "%sprivate ", TmpStr2) )
        return 1;
    if ( sscanf(TmpStr, "%snosave ", TmpStr2) )
        return 1;
    if ( sscanf(TmpStr, "%snosave ", TmpStr2) )
        return 1;
    if ( sscanf(TmpStr, "%sprotected ", TmpStr2) )
        return 1;
    return 0;
} /* public_or_protected_function() */

/**
 * Sends the premade mapping to nroff handler, that simply saves it.
 * @comment Protected because it enables nroff handler. And it saves things!
 */
void make_helpfile() {
    if ( !being_autodoced )
        return;

    remove_call_out("make_helpfile");
    call_out("destruct_me", 5);

    foreach(int loc in ({IS_FUNCTION, IS_PUBLIC_FUNCTION, IS_PROTECTED_FUNCTION}))
        foreach(string crap in BUGGED_PARSING_FUNCTIONS)
            if ( autodoc_nroff[loc] && autodoc_nroff[loc][crap] ) {
                warning("Autodoc Parsing error in " + being_autodoced + "\n"
                        "Function: " + crap);
                AUTODOC_QUEUE_HANDLER->finished_parsing(being_autodoced);
                return;
            }

    AUTODOC_NROFF_HANDLER->save_helpfile(being_autodoced, autodoc_nroff);
    AUTODOC_QUEUE_HANDLER->finished_parsing(being_autodoced);
}

/**
 * Adds +1 line
 * @return 1 if last line is reached
 */
int new_line() {
    if ( ++line_count >= sizeof (autodoc_file_lines) ) {
        call_out("make_helpfile", 1);
        return 1;
    }
    autodoc_file_line = autodoc_file_lines[line_count];
    return 0;
} /* new_line() */

/**
 * Adds to nroff mapping, preprosessed by each parser function
 */
varargs void add_to_autodoc_nroff(int IsMainDescription) {
    if ( autodoc_comment == "@ignore" || is_parsing == IS_UNKNOWN ) {
        autodoc_argument = 0;
        autodoc_comment = 0;
        return;
    }
    // We trust that the formats are correct folx, since they are
    // formatted correctly through add_adoc_mess()
    if ( IsMainDescription ) {
        int DescVal, StringOrArray, ii;
        for ( ii=0; ii < m_sizeof(autodoc_comment); ii++ ) {
            DescVal = keys(autodoc_comment)[ii];
            StringOrArray = stringp(autodoc_comment[DescVal]);
            if ( !autodoc_nroff[DescVal] ) {
                if ( StringOrArray )
                    autodoc_nroff[DescVal] = "";
                else
                    autodoc_nroff[DescVal] = ({});
            }
            autodoc_nroff[DescVal] += autodoc_comment[DescVal];
        }
        autodoc_argument = 0;
        autodoc_comment = 0;
        return;
    }

    switch(is_parsing) {
    case IS_PUBLIC_FUNCTION:
    case IS_PROTECTED_FUNCTION:
    case IS_DEFINE:
        if ( !autodoc_argument || autodoc_argument == "" )
            break;
        if ( !autodoc_nroff[is_parsing] )
            autodoc_nroff[is_parsing] = ([]);
        autodoc_nroff[is_parsing][autodoc_argument] = autodoc_comment;
        break;
    case IS_INHERIT:
    case IS_LOCAL_INCLUDE:
    case IS_GLOBAL_INCLUDE:
        if ( !autodoc_nroff[is_parsing] )
            autodoc_nroff[is_parsing] = ({});
        autodoc_nroff[is_parsing] += ({ autodoc_argument });
        break;
    }
    autodoc_argument = 0;
    autodoc_comment = 0;
} /* add_to_autodoc_nroff() */

int dest_me     () { return 0;     }
int clean_up    () { return 0;     }
void destruct_me() { destruct(TO); }

string query_being_autodoced    () { return being_autodoced;    }
int query_start_time() { return start_time; }
string query_line_being_parsed  () { return autodoc_file_line;  }
int query_line_count            () { return line_count;         }
int query_num_of_lines          () { return num_of_lines;       }
