/*
** /obj/handlers/profanity.c for Shattered Realms MUD
** Nip <nip@vr.net> 2000-10-17
**
** string clean_language(string)
**
**   Replaces certain words and phrases in the string with other words and
**   phrases.  The idea is this is used on 'public' statements (shouts, guild
**   chats, etc.) to clean up the language.
**
** mapping show_wordlist()
**
**   Displays the contents of the mapping holding the phrases and their
**   replacements.  This is for debugging, but I left it in because it's
**   small.
**
** If you need to use this, don't try to clone it or inherit it.  That won't
** work.  Copy the code and adjust the WORD_FILE define.  If there's sufficient
** interest I can make this into a base object so you _can_ inherit it.
** Another interesting change would be to make this a simul-efun.  That'd take
** a bit more work to manage multiple word lists, but shouldn't be too hard.
**
** The search phrases and their replacements come from the WORD_FILE.  This is
** a flat-text file.  Blank lines and lines with the first non-blank character
** being a sharp (#) are comments and ignored.  All other lines are of the form
**   phrase -> replacement
** where 'phrase' and 'replacement' are strings.  Sorry, but a phrase cannot
** contain the string "->".  Note that only the FIRST replacement for a phrase
** will be made; so if you add a phrase to the end and don't comment out the
** earlier occurance of the same phrase you won't see any change.  It's not a
** bug .. it's a feature.
**
** In this version, the phrases are lexically sorted (basically ascending alpha
** sorted) and applied in order.  A phrase will not match its replacement, but
** a lexically higher phraes will match the replacement of a lower one.  It is
** possible to have phrases match their own replacments, and to apply phrases
** until all possible matches are exhausted.  That's a lot of work, so I'll
** leave things are they are here unless someone *REALLY* begs.
**
** Notice that once you're using this, you don't need to do anything to tell
** it you've updated the WORD_FILE.  The next time it's used, if the file has
** changed, it's automagically reloaded.  This works on the time stamp of the
** last change.  Time warps can effect it; see the comments.
**
** The only bad thing this does (at least, I don't like that it does it) is
** it smashes out the white space.  It'd be nice to leave the white space
** alone but, to be honest, I don't see how to do that so we'll just have to
** live with it as it is.
*/

#define WORD_FILE "/table/bad_words.txt"

private nosave mapping BadWords = ([ ]);
private nosave int FingerPrint = 0;

private string reduce_whitespace(string s)
{
    return implode(explode(replace_string(s, "\t", " "), " ") - ({ "" }), " ");
}

private void read_init_file()
{
    mixed WordFileStatus = stat(WORD_FILE);
#define file_size WordFileStatus[0]
#define last_time_file_touched WordFileStatus[1]
#define time_object_loaded WordFileStatus[2]
    if (   (sizeof(WordFileStatus) != 3)
        || (typeof(file_size) != "int")
        || (typeof(last_time_file_touched) != "int")
        || (typeof(time_object_loaded) != "int")
        || (file_size < 0)
        || (last_time_file_touched < 0)
        || (time_object_loaded != 0)
       ) {
        /*
        ** The WORD_FILE does not exist, or is not a file, or is a loaded
        ** object.  If we have a WORD_FILE loaded, we'll assume it's been
        ** removed and we don't want this feature any more.
        */
        if (FingerPrint != 0){
            FingerPrint = 0;
            BadWords = ([ ]);
        }
    }
    else if (FingerPrint >= last_time_file_touched) {
        /*
        ** Time warp!  The WORD_FILE is suddenly older than the loaded
        ** copy.  Probably someone is doing some maintenance and mv'd in
        ** an older version.  We'll ignore this situation.
        */
    }
    else if (file_size == 0) {
        /*
        ** A little optimization here.  The file is newer, and zero bytes
        ** long.
        */
        FingerPrint = last_time_file_touched;
        BadWords = ([ ]);
    }
    else {
        /*
        ** OK, let's load it.
        */
        string File = read_file(WORD_FILE);
        if (! stringp(File)) {
            /*
            ** Uh .. wait a minute.  This can NOT happen.  If you think it
            ** did, it's a bug in the driver.  (In a real system race could
            ** cause this, but race isn't possible in LPC, right?)  This
            ** most likely means someone outside the driver was messing
            ** with the files.  Let's bail and see what happens the next time
            ** we're called.
            */
            FingerPrint = 0;
            BadWords = ([ ]);
        }
        else {
            /*
            ** Sorry about this.  The regular expression removes blank lines
            ** comment lines, and lines without the '->' separator.  The main
            ** reason for doing this is so we pre-allocate the proper size
            ** mapping to hold the data.  So this doesn't just remove comments,
            ** it also gives us a precise count of the number of rules we have.
            */
            string *Text = regexp(explode(File, "\n"),
"^[ \t]*[^# \t][^ \t]*(([ \t]+[^- \t][^ \t]*)|([ \t]+-([^>][^ \t]*)?))*[ \t]*->[ \t]*[^ \t]+");
            int Line = sizeof(Text);
            FingerPrint = last_time_file_touched;
            BadWords = allocate_mapping(Line);
            while (Line-- > 0) {
                string m;
                string r;
                /*
                ** This should always be two elements and a -> separator.  But
                ** just to be safe we'll check.  If you think there's a problem
                ** slipping past the regexp above, put a else on this to see
                ** what slipped by.
                */
                if (sscanf(Text[Line], "%s->%s", m, r) == 2) {
                    m = lower_case(reduce_whitespace(m));
                    r = reduce_whitespace(r);
                    BadWords[m] = r;
                }
            }
        }
    }
}

void create() {
    SETEUID;
    read_init_file();
}

string clean_language(string str, object shouter)
{
    if (str && (strlen(str) > 0)) {
        /*
        ** I hate to reduce the whitespace here, but it's the cleanest way to
        ** avoid strange output and be able to match to the search phrases.  I
        ** really don't want to do a word-by-word parse.
        */
        str = reduce_whitespace(str);
        read_init_file();
        if (BadWords && (sizeof(BadWords) > 0)) {
            string raw_mess = strip_colours(str);
            string lowered = lower_case(str);
            /*
            ** Mappings are randomly ordered.  Sort the search phrases so the
            ** behavior is consistent.
            */
            string *bad_words = sort_array(keys(BadWords), 0);
            int i;
            for (i = 0; i < sizeof(bad_words); i++) {
                int loc = strsrch(lowered, bad_words[i]);
                while (loc >= 0) {
                    int nextloc;

                    // Nasty little package, socials down to 1/3rd, lockout 8 hours
                    //
                    hack(CAP(geteuid(shouter)) + " shouts word '" +
                         bad_words[i] + "', entire message: " + raw_mess);
                    tell_object(shouter, "\n\n\n    Oh no you didnt do just that.\n\n\n");
                    shouter->set_max_social_points(shouter->query_max_social_points()*2/3);
                    call_out("setup_suspension", 0, shouter, bad_words[i]);
                    secure_log_file("PUNISH", ctime(time(), 7) + " - Auto - " +
                        CAP(geteuid(shouter)) + " - 1 - shout - Foul language.\n");

                    str = str[0 .. loc-1]
                        + BadWords[bad_words[i]]
                        + str[loc+strlen(bad_words[i]) .. ];
                    lowered = lower_case(str);
                    /*
                    ** The following three lines are critical to loop
                    ** prevention.  The first makes sure that the current
                    ** phrase is not applied to its replacement phrase.  The
                    ** next two step to the next search phrase.  Don't mess
                    ** with these unless you add code to snub runaways.  If
                    ** you want to try to get infinite-depth replacement going
                    ** these are the three lines you'll need to replace.
                    */
                    loc += strlen(BadWords[bad_words[i]]);
                    nextloc = strsrch(lowered[loc .. ], bad_words[i]);
                    loc = (nextloc < 0) ? -1 : nextloc + loc;
                }
            }
        }
    }
    return str;
}

mapping show_wordlist()
{
    read_init_file();
    return BadWords;
}

/**
 * Inserting suspension AFTER the shout is finished, after all we do have a filter
 * that cleans up the language. Want the shout to complete, then block it for 8 hours.
 */
protected void setup_suspension(object me, string word) {
    if ( !me ) return;
    me->add_timed_property("no_talk shout",
        "You shouted '" + word + ", do not do that.\n",
        8 * 1800);
}
