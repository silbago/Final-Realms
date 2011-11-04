/**
 * @main
 * This is a visual studio.
 * Dont play with it, silbago was ehre
 *
 * Silbago Apr 2011:
 *   - Indent code removed, see file for how it (was) used.
 *     /global/mortal/more/BACKUP/visual.b4hack.110428.silbago
 * Silbago Jun 2011:
 *   - I changed the bloody ctrl-s to ctrl-f. Chrome use FIND
 *
 * @author Morth
 */

#include <keys.h> 
#include <input.h>
#include <handlers.h>

string *file=({});
mixed *lines=({});
mixed *flags=({});
nosave int tw,th;
nosave int *line=({}),*col=({}),*topline=({});
nosave int *modified=({});
nosave string *messages=({});
nosave string showfile;
nosave function notify;
nosave int *readonly=({});
nosave object editer;
nosave string searchstr,*cutlines=({});
nosave int curfile=0;
nosave string nprompt;
nosave int no_syntax_colors=0;

#define File file[curfile]
#define Lines lines[curfile]
#define Line line[curfile]
#define Col col[curfile]
#define Topline topline[curfile]
#define Modified modified[curfile]
#define Readonly readonly[curfile]
#define Flags flags[curfile]

void net_dead();
protected void open(string f,int ro);
protected void jump ();
protected void input(string key);
protected void help();

protected void create()
{
    seteuid(0);
}

void dest_me()
{
#if 0
    string f;

    if(clonep())
    {
        foreach(f in file)
        EDITM->done_edit(f);
    }
#endif
    destruct(this_object());
}

#define SNOTID "(^|[^A-Za-z0-9_])"
#define ENOTID "($|[^A-Za-z0-9_])"

/**
 * Syntax highlights the code (if not silenced).
 * Takes single line or chunks of lines (\n seperated).
 * Using the editers personal color drawer / syntax highlight.
 */
private void draw_colorized (string str) {
    string f;

    if ( sizeof (str) >= tw )
        str = str[0 .. tw - 1];

    if ( no_syntax_colors ) {
        editer->message(NOPARSE + NOFILTER, str);
        return;
    }

    f = replace_string(str, "%^", "%%^^");
    if ( explode(editer->query_color_drawer(), "/")[<1] == "volothamp" )
        f = editer->draw_colorized(f);
    else
        f = editer->draw_colorized(f)[0..<2];

    f = terminal_colour(f, editer->cmap());
    editer->message(NOPARSE + NOFILTER, f);
}

#define COMSTART  0x001
#define COMMENT   0x002
#define COMMEND   0x004
#define CONTOPEN  0x008
#define CONTEND   0x010
#define OPENBRACE 0x020
#define SUBNEXT   0x040
#define QUOTESGL  0x080
#define QUOTEDBL  0x100
#define TEXTBLOCK 0x200
#define PREPROC   0x400

protected void calc_flags (int start, int num)
{
    mixed *pf, *f;
    int ind, i, end;

    if (start && arrayp (Flags[start - 1]))
        pf = Flags[start - 1];
    if (!pf)
        pf = ({ 0, 0 });
    end = start + num;
    if (end > sizeof (Lines))
        end = sizeof (Lines);
    for (i = start; i < end; i++)
    {
        f = ({ 0, 0 });
        if (pf)
        {
            ind = pf[0];
            if (pf[1] & OPENBRACE)
                ind += 2;
            if (pf[1] & SUBNEXT)
                ind += 2;
            if (pf[1] & CONTOPEN)
                ind -= 6;
            if (pf[1] & CONTEND)
            {
                ind += 6;
                f[1] |= CONTOPEN;
            }
            if (pf[1] & (COMMENT | COMSTART))
            {
                if (strsrch (Lines[i], "*/") != -1)
                    f[1] |= COMMEND;
                else
                    f[1] |= COMMENT;
            }
            f[0] = ind;
        }
        pf = Flags[i] = f;
    }   
}

mixed *query_flags() { return copy (Flags); }

protected void draw_text() {
    int i;
    string str, text_body;

    // Clearing and preparing to write file content.
    // Line-by-line because of sprintf and string buffer size :(
    ansi_goto_xy(1, 2, editer);
    ansi_clear_screen_from_pos (editer);
    calc_flags (Topline, th - 1);
    text_body = "";
    for(i=Topline;i<Topline+th-1 && i<sizeof(Lines);i++)
    {
        if(sizeof(Lines[i])>=tw)
            str = Lines[i][0..tw-2]; // Save a space since some term emus might wrap.
        else
            str = Lines[i];
//      text_body += str;
        draw_colorized (str);
        if (i < Topline + th - 2)
//          text_body += "\n";
            editer->message (NOPARSE + NOFILTER, "\n");
    }
//  draw_colorized(text_body);

    // File content written, now lets write the header
    ansi_goto_xy(1,1, editer);
    ansi_set_mode(3, editer);
    if(showfile)
        editer->message(NOPARSE + NOFILTER,sprintf("%-*s",tw,showfile));
    else if (File)
        editer->message(NOPARSE + NOFILTER,sprintf("%-*s",tw,File+(Readonly?" (ro)":(Modified?"*":""))));
    else
        editer->message (NOPARSE + NOFILTER, sprintf("%-*s", tw, "(unknown)" + (Readonly? " (ro)" : (Modified? "*" : ""))));
    ansi_goto_xy(tw-13,1, editer);
    if(sizeof(messages))
        editer->message(CHAT + NOFILTER," %^B_RED%^**Message**");
    else
        editer->message(NOPARSE + NOFILTER,"C-g for help");
    ansi_set_mode(0, editer);
}

protected void draw_pos()
{
    ansi_goto_xy(40,1, editer);
    ansi_set_mode(3, editer);
    editer->message(NOPARSE + NOFILTER,sprintf("Row: %4d Col: %3d",Line+1,Col+1));
    ansi_set_mode(0, editer);
}

/**
 * Starting editing something in this editor
 *
 * Parameters
 *   f       File we're editing
 *   ro      Flags for read_only & 1, no_colors & 2
 *   show    Header line in editor, show this instead of Filepath
 *   notfun  Dont know what this is
 *   ee      Editer object
 *
 * @version End-Editor functional should be here somewhere.
 * @version Need no-parsing mode for board, mail etc.
 */
void edit(string f,int ro,string show,function notfun, object ee) {
    seteuid (getuid ());
    if (!editer && getuid (ee) != getuid())
      return;

    if(show)
        showfile=show;
    if(notfun)
        notify=notfun;

    if (!editer)
        editer = ee;

    editer->set_inhibit_prompt(1);
    tw = editer->query_cols ();
    th = editer->query_rows ();
    editer->add_filter();

    ansi_clear_screen(editer);

    if ( ro & 2 )
        no_syntax_colors = 1;
    open(f,ro&1);

    draw_text();
    draw_pos();
    ansi_goto_xy(1,2, editer);
    editer->add_input((: input :), IMODE_CHAR | IMODE_HIDDEN | IMODE_PROMPT);
    if (!editer->query_property ("visual help shown"))
    {
        help ();
        editer->add_property ("visual help shown", 1);
    }
}

protected void open(string f,int ro)
{
    string s;
    int i;

    file+=({f});
    curfile=sizeof(file)-1;
    topline+=({0});
    line+=({0});
    col+=({0});
    modified+=({0});
    if (f)
        s=read_file(f);
    if(!s)
        s="";
    if(/*EDITM->start_edit(f) ||*/ ro)
        readonly+=({1});
    else if (f)
        readonly+=({!master ()->valid_write (f, geteuid (), "write_file")});
    else
        readonly += ({ 0 });
    lines+=({explode("&"+s+"&","\n")});
    lines[<1][0] = lines[<1][0][1 ..];
    lines[<1][<1] = lines[<1][<1][0 .. <2];
    for(i=0;i<sizeof(Lines);i++)
        Lines[i]=replace_string(Lines[i],"\t","        ");
    flags += ({ allocate (sizeof (lines[<1])) });
}

protected void set_modified()
{
    Modified=1;
    if(!showfile)
    {
        ansi_goto_xy(1,1, editer);
        ansi_set_mode(3, editer);
        if (File)
            editer->message(NOPARSE + NOFILTER,File+"*");
        else
            editer->message (NOPARSE + NOFILTER, "(unknown)*");
        ansi_set_mode(0, editer);
    }
}

protected void done()
{
    int i;

    if(editer)
    {
        ansi_goto_xy(1,th, editer);
        ansi_clear_line_from_pos(editer);
        editer->remove_filter();
        editer->remove_input();
        for(i=0;i<sizeof(messages);i++)
            editer->message(messages[i][0],messages[i][1]);
    }
    if(notify)
        catch(evaluate(notify, file[0], editer));
    dest_me();
}

protected void message_mode();
protected int save();
protected void save_other();
protected void close();
protected void do_open();
protected void search(int again);

nosave string lastkey;

protected void input(string key)
{
    string s,*nl;
    int i;

    switch(key)
    {
    case C_A:
        Col=0;
        break;
    case C_B:
    case LEFT:
        if(Col>sizeof(Lines[Line]))
            Col=sizeof(Lines[Line]);
        else if(Col)
            Col--;
        else if(Line)
        {
            if(Line==Topline)
            {
                Topline=Line-th/2;
                if(Topline>sizeof(Lines)-th+1)
                    Topline=sizeof(Lines)-th+1;
                if(Topline<0)
                    Topline=0;
                draw_text();
            }
            Line--;
            Col=sizeof(Lines[Line]);
        }
        break;
    case C_E:
        Col=sizeof(Lines[Line]);
        break;
//  case C_F: I Wanna use this for FIND/search.
    case RIGHT:
        if(Col>sizeof(Lines[Line]))
            Col=sizeof(Lines[Line]);
        else if(Col<sizeof(Lines[Line]))
            Col++;
        else if(Line<sizeof(Lines)-1)
        {
            if(Line==Topline+th-2)
            {
                Topline=Line-th/2;
                if(Topline>sizeof(Lines)-th+1)
                    Topline=sizeof(Lines)-th+1;
                if(Topline<0)
                    Topline=0;
                draw_text ();
            }
            Line++;
            Col=0;
        }
        break;
    case C_N:
    case DOWN:
        if(Line<sizeof(Lines)-1)
        {
            if(Line==Topline+th-2)
            {
                Topline=Line-th/2;
                if(Topline>sizeof(Lines)-th+1)
                    Topline=sizeof(Lines)-th+1;
                if(Topline<0)
                    Topline=0;
                draw_text();
            }
            Line++;
        }
        break;
    case C_P:
    case UP:
        if(Line)
        {
            if(Line==Topline)
            {
                Topline=Line-th/2;
                if(Topline>sizeof(Lines)-th+1)
                    Topline=sizeof(Lines)-th+1;
                if(Topline<0)
                    Topline=0;
                draw_text();
            }
            Line--;
        }
        break;
    case A_HOME:
    case HOME:
        Topline=Line=Col=0;
        draw_text();
        break;
    case END:
        Line=sizeof(Lines)-1;
        Topline=maxi (({ Line-th+2, 0}));
        Col=sizeof(Lines[Line]);
        draw_text();
        break;
    case A_PGUP:
    case PGUP:
        Topline-=th-1;
        Line-=th-1;
        if(Topline<0)
        {
            Line-=Topline;
            Topline=0;
        }
        draw_text();
        break;
    case A_PGDN:
    case PGDN:
        Topline+=th-1;
        Line+=th-1;
        i=Topline+th-1-sizeof(Lines);
        if(i>0)
        {
            Line-=i;
            Topline-=i;
        }
        draw_text();
        break;
    case C_Z:
        message_mode();
        return;
    case C_K:
        if(lastkey!=C_K)
            cutlines=({});
        cutlines+=({Lines[Line]});
        if(Readonly)
            input(C_N);
        else
        {
            if(Line==sizeof(Lines)-1)
            {
                Lines=Lines[0..Line-1];
                Flags = Flags[0 .. Line - 1];
                if(Line)
                {
                    Line--;
                    if(Topline)
                        Topline--;
                }
            }
            else
            {
                Lines = delete (Lines, Line, 1);
                Flags = delete (Flags, Line, 1);
            }
            if(!Modified)
                set_modified();
            draw_text();
        }
        break;
    case C_L:
        Topline=Line-th/2;
        if(Topline>sizeof(Lines)-th+1)
            Topline=sizeof(Lines)-th+1;
        if (Topline < 0)
            Topline = 0;
        draw_text();
        break;
    case C_T:
        if(showfile)
            help();
        else
            do_open();
        return;
    case C_U:
        if(showfile)
        {
            help();
            return;
        }
        else
        {
            curfile++;
            if(curfile>=sizeof(file))
                curfile=0;
            draw_text();
        }
        break;
    case C_V:
        jump ();
        return;
    case C_W:
        if(Readonly)
            editer->message(NOPARSE + NOFILTER, BEL);
        else
            save();
        break;
    case C_Y:
        if(Readonly)
            editer->message(NOPARSE + NOFILTER, BEL);
        else if(sizeof(cutlines))
        {
            if(Line)
            {
                Lines = Lines[0..Line-1]+cutlines+Lines[Line..];
                Flags = Lines[0 .. Line - 1] + allocate (sizeof (cutlines)) + Lines[Line ..];
            }
            else
            {
                Lines = cutlines+Lines;
                Flags = allocate (sizeof (cutlines)) + Flags;
            }
            calc_flags (Line, sizeof (cutlines));
            Line += sizeof (cutlines);
            if (Line - Topline >= th - 1)
            {
                Topline = Line - th / 2;
                if (Topline > sizeof (Lines) - th + 1)
                    Topline = sizeof (Lines) - th + 1;
                if (Topline < 0)
                    Topline = 0;
            }
            if(!Modified)
                set_modified();
            Col = 0;
            draw_text();
            draw_pos ();
        }
        break;
    case C_R:
        if(showfile)
        {
            help();
            return;
        }
        else
            save_other();
        return;
    case C_F:
        search(lastkey==C_F);
        lastkey=C_F;
        return;
    case C_S:
        search(lastkey==C_S);
        lastkey=C_S;
        return;
    case C_X:
        if (!save ())
            return;
        /* fallthrough */
    case C_C:
    case C_Q:
        close();
        return;
    case BKSP:
        if(Readonly)
        {
            editer->message(NOPARSE + NOFILTER, BEL);
            break;
        }
        if(Col>sizeof(Lines[Line]))
        {
            Col=sizeof(Lines[Line]);
            ansi_goto_xy(Col+1,Line-Topline+2, editer);
        }
        else if(Col)
        {
            if(Col>1)
                s=Lines[Line][0..Col-2];
            else
                s="";
            Lines[Line]=s+Lines[Line][Col..];
            Col--;
            ansi_move_left(1, editer);
            editer->message(NOPARSE + NOFILTER,Lines[Line][Col..]);
            ansi_clear_line_from_pos (editer);
            if(!Modified)
                set_modified();
        }
        else if(Line)
        {
            nl=Lines[0..Line-1];
            Col=sizeof(nl[Line-1]);
            nl[Line-1]+=Lines[Line];
            if(Line<sizeof(Lines)-1)
                nl+=Lines[Line+1..];
            Lines=nl;
            Flags = delete (Flags, Line, 1);
            Line--;
            if(!Modified)
                set_modified();
            draw_text();
        }
        break;
    case C_D:
    case DEL:
        if(Readonly)
        {
            editer->message(NOPARSE + NOFILTER, BEL);
            break;
        }
        if(Col>sizeof(Lines[Line]))
        {
            Col=sizeof(Lines[Line]);
            ansi_goto_xy(Col+1,Line-Topline+2, editer);
        }
        else if(Col<sizeof(Lines[Line]))
        {
            if(Col<sizeof(Lines[Line])-1)
                s=Lines[Line][Col+1..];
            else
                s="";
            if(Col)
                Lines[Line]=Lines[Line][0..Col-1]+s;
            else
                Lines[Line]=s;
            editer->message(NOPARSE + NOFILTER,Lines[Line][Col..]);
            ansi_clear_line_from_pos(editer);
            if(!Modified)
                set_modified();
        }
        else if(Line<sizeof(Lines)-1)
        {
            nl=Lines[0..Line];
            nl[Line]+=Lines[Line+1];
            if(Line<sizeof(Lines)-2)
                nl+=Lines[Line+2..];
            Lines=nl;
            Flags = delete (Flags, Line + 1, 1);
            if(!Modified)
                set_modified();
            draw_text();
        }
        break;
    case NL:
    case CR:
// Pressing enter forces through new line twice.
// Holding in SHIFT-ENTER runs one new line only.
// This happens BEFORE it gets to process_input()
// Therefore it is something that happens in driver and/or client.
// Not sure how to adapt it here, it must be here somewhere.
// I think key 13 (sprintf("%c", 13) is the culprit
// It brings in new line together with 10
// Any chance it is the same as ^M microsoft-new-line ?
if ( key[0] == 10 )
    break;
        if(Readonly)
        {
            editer->message(NOPARSE + NOFILTER, BEL);
            break;
        }
        if(Col>sizeof(Lines[Line]))
            Col=sizeof(Lines[Line]);
        if(!Modified)
            set_modified();
        if(Col)
            s=Lines[Line][0..Col-1];
        else
            s="";
        if(Line)
            nl=Lines[0..Line-1]+({s});
        else
            nl=({s});
        nl+=({Lines[Line][Col..]});
        if(Line<sizeof(Lines)-1)
            nl+=Lines[Line+1..];
        Lines=nl;
        if(Line==Topline+th-2)
            Topline++;
        Line++;
        Flags = insert (Flags, 0, Line);
        Col=0;
        draw_text();
        break;
    case TAB:
        if(Readonly)
            editer->message(NOPARSE + NOFILTER, BEL);
        break;
    default:
        if(key[0]<32)
        {
            help();
            return;
        }
        if(Readonly)
        {
            editer->message(NOPARSE + NOFILTER, BEL);
            break;
        }
        if(Col>sizeof(Lines[Line]))
        {
            Col=sizeof(Lines[Line]);
            ansi_goto_xy(Col+1,Line-Topline+2, editer);
        }
        if(Col)
            s=Lines[Line][0..Col-1];
        else
            s="";
        Lines[Line]=s+key+Lines[Line][Col..];
        editer->message(NOPARSE + NOFILTER,key);
        Col++;
        if(Col<sizeof(Lines[Line]))
            editer->message(NOPARSE + NOFILTER,Lines[Line][Col..]);
        else if(key=="{" || key=="}")
        {
            Col=sizeof(Lines[Line]);
        }
        if(!Modified)
            set_modified();
        break;
    }
    lastkey=key;
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}

protected int save()
{
    if (!File)
    {
        save_other ();
        return 0;
    }
    if(Modified)
    {
        string s=implode(Lines,"\n");
        if(sizeof (s) && s[<1] != '\n')
            s+="\n";
        editer->version_file_backup(File);
        write_file(File,s,1);
        Modified=0;
        if(!showfile && editer)
        {
            ansi_goto_xy(1,1, editer);
            ansi_set_mode(3, editer);
            editer->message(NOPARSE + NOFILTER,File+" ");
            ansi_set_mode(0, editer);
            ansi_goto_xy(Col+1,Line-Topline+2, editer);
        }
    }
    return 1;
}

protected void get_open(string f);

protected void do_open()
{
    ansi_goto_xy(1, th, editer);
    nprompt = "%^UNDERLINE%^Enter file to open:%^RESET%^ ";
    editer->add_input((: get_open :), IMODE_ONCE | IMODE_PROMPT);
}

protected void get_open(string f)
{
    if(f!="")
    {
        f = editer->get_path (f);
#if 0
        if(!EDITM->can_edit(f))
#endif
            open(f,0);
#if 0
        else
            editer->message(NOPARSE + NOFILTER, BEL);
#endif
    }
    draw_text();
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}

protected void get_save(string f);

protected void save_other()
{
    ansi_goto_xy(1, th, editer);
    nprompt = "%^UNDERLINE%^Enter file to save to:%^RESET%^ ";
    editer->add_input((: get_save :), IMODE_ONCE | IMODE_PROMPT);
}

protected void get_save(string f)
{
    if(f!="")
    {
        File=editer->get_path (f);
        Modified=1;
        save();
        ansi_goto_xy(1,1, editer);
        ansi_clear_line(editer);
    }
    draw_text();
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}

protected void get_close(string str);

protected void close()
{
    if(Modified)
    {
        ansi_goto_xy (1, th, editer);
        nprompt = "%^UNDERLINE%^File not saved! Really close?%^RESET%^ [y]/n: ";
        editer->add_input((: get_close :), IMODE_CHAR | IMODE_HIDDEN | IMODE_ONCE | IMODE_PROMPT);
    }
    else if(sizeof(file)>1)
    {
#if 0
        EDITM->done_edit(File);
#endif
        if(curfile)
        {
            file=file[0..curfile-1]+file[curfile+1..];
            lines=lines[0..curfile-1]+lines[curfile+1..];
            line=line[0..curfile-1]+line[curfile+1..];
            col=col[0..curfile-1]+col[curfile+1..];
            topline=topline[0..curfile-1]+topline[curfile+1..];
            modified=modified[0..curfile-1]+modified[curfile+1..];
            readonly=readonly[0..curfile-1]+readonly[curfile+1..];
            flags = flags[0 .. curfile - 1] + flags[curfile + 1 ..];
        }
        else
        {
            file=file[1..];
            lines=lines[1..];
            line=line[1..];
            col=col[1..];
            topline=topline[1..];
            modified=modified[1..];
            readonly=readonly[1..];
            flags = flags[1 ..];
        }
        if(curfile>=sizeof(file))
            curfile=0;
        draw_text();
        draw_pos();
        ansi_goto_xy(Col+1,Line-Topline+2, editer);
    }
    else
        done();
}

protected void get_close(string str)
{
    if(str[0]=='n')
    {
        draw_text();
        draw_pos();
        ansi_goto_xy(Col+1,Line-Topline+2, editer);
    }
    else if(sizeof(file)>1)
    {
#if 0
        EDITM->done_edit(File);
#endif
        if(curfile)
        {
            file=file[0..curfile-1]+file[curfile+1..];
            lines=lines[0..curfile-1]+lines[curfile+1..];
            line=line[0..curfile-1]+line[curfile+1..];
            col=col[0..curfile-1]+col[curfile+1..];
            topline=topline[0..curfile-1]+topline[curfile+1..];
            modified=modified[0..curfile-1]+modified[curfile+1..];
            readonly=readonly[0..curfile-1]+readonly[curfile+1..];
            flags = flags[0 .. curfile - 1] + flags[curfile + 1 ..];
        }
        else
        {
            file=file[1..];
            lines=lines[1..];
            line=line[1..];
            col=col[1..];
            topline=topline[1..];
            modified=modified[1..];
            readonly=readonly[1..];
            flags = flags[1 ..];
        }
        if(curfile>=sizeof(file))
            curfile=0;
        draw_text();
        draw_pos();
        ansi_goto_xy(Col+1,Line-Topline+2, editer);
    }
    else
        done();
}

protected void get_search(string str);

protected void search(int again)
{
    if(again)
        get_search(searchstr);
    else
    {
        nprompt = "%^UNDERLINE%^Search for:%^RESET%^ ";
        ansi_goto_xy(1, th, editer);
        editer->add_input((: get_search :), IMODE_ONCE | IMODE_PROMPT);
    }
}

protected void get_search(string str)
{
    int l,c=-1;

    if(strlen(str))
    {
        searchstr=str;
        if(Col<sizeof(Lines[Line])-1)
        {
            c=strsrch(Lines[Line][Col+1..],str);
            if(c!=-1)
                Col+=c+1;
        }
        if(c==-1)
        {
            l=Line+1;
            while(l!=Line)
            {
                c=strsrch(Lines[l],str);
                if(c!=-1)
                    break;
                l++;
                if(l>=sizeof(Lines))
                    l=0;
            }
            if(c==-1 && Col)
                c=strsrch(Lines[l][0..Col-1],str);
            if(c==-1)
                editer->message(NOPARSE + NOFILTER,BEL);
            else
            {
                Line=l;
                Col=c;
                if(Line-Topline>=th-1 || Topline>Line)
                {
                    Topline=Line-th/2;
                    if(Topline>sizeof(Lines)-th+1)
                        Topline=sizeof(Lines)-th+1;
                    if(Topline<0)
                        Topline=0;
                }
            }
        }
    }
    draw_text();
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}

void window_size(int width,int height)
{
    int i;
    tw=width;
    th=height;
    i=curfile;
    for(curfile=0;curfile<sizeof(file);curfile++)
    {
        if(Line-Topline>=th-1)
            Topline=Line-th+1;
        else if(Topline>sizeof(Lines)-th+1)
            Topline=sizeof(Lines)-th+1;
        if(Topline<0)
            Topline=0;
    }
    curfile=i;
    draw_text ();
}

nosave int mess_mode=0;

string filter_message(mixed cl,string message)
{
    if(mess_mode || cl==STDERR)
        return message;
    if(!sizeof(messages))
    {
        ansi_push_pos(editer, editer);
        ansi_goto_xy(tw-13,1, editer);
        ansi_set_mode(3, editer);
        editer->message(NOFILTER + CHAT, " %^B_RED%^**Message**");
        ansi_set_mode(0, editer);
        editer->message (NOPARSE + NOFILTER, BEL);
        ansi_goto_xy (Col+1,Line-Topline+2,editer);
        ansi_pop_pos (editer); // Many emus don't support this (why?) but best I could do
    }
    messages+=({({cl,message})});
    return 0;
}

protected void message_input(string str);

protected void message_mode()
{
    int i;

    if (showfile && !editer->query_static_property ("blocked all exits"))
    {
        mess_mode = 2;
        editer->add_static_property ("blocked all exits", "You can't leave while editing.\n");
    }
    else
        mess_mode=1;

    ansi_goto_xy(1,2, editer);
    ansi_clear_screen_from_pos (editer);
    for(i=0;i<sizeof(messages);i++)
        editer->message(/*messages[i][0]*/NOPARSE + NOFILTER,messages[i][1]);
    messages=({});

    editer->add_input((: message_input :),IMODE_HISTORY | IMODE_PROMPT, editer->query_splitscreen());
    editer->set_inhibit_prompt(0);
}

protected void message_input(string str)
{
    if(!sizeof(str))
    {
        editer->set_inhibit_prompt(0);
        if (mess_mode == 2)
            editer->remove_static_property ("blocked all exits");
        mess_mode=0;
        editer->remove_input();
        draw_text();
        draw_pos();
        ansi_goto_xy(Col+1,Line-Topline+2, editer);
        return;
    }
    editer->process_editor_command(str);
}

void write_prompt ()
{
    if (mess_mode)
        editer->message (NOPARSE + NOFILTER, "[edit]> ");
    else if (nprompt)
    {
        editer->message (CHAT + NOFILTER, nprompt);
        ansi_clear_line_from_pos (editer);
        nprompt = 0;
    }
}

protected void end_help(string str);

protected void help()
{
    ansi_goto_xy(1,2, editer);
    ansi_clear_screen_from_pos(editer);
    editer->add_input((: end_help :),IMODE_CHAR | IMODE_HIDDEN | IMODE_ONCE | IMODE_PROMPT);
    editer->message(CHAT + NOFILTER,@END
%^BOLD%^You must use raw telnet for this editor. If you aren't type q now to exit%^RESET%^
Help for edit: C-a means hold ctrl and press a.

Move using arrow keys, C-a, C-e, C-b, C-f, C-n, C-p, PgUp, PgDn, Home and End.
Edit by typing text. C-d or Del to delete forward.

Other commands:
Tab            Indent C-like. (needs work)
C-q/C-c        %^BOLD%^%^BLUE%^Close / quit%^RESET%^ (at any time).%^RESET%^
C-w            Save.
C-x            Save and close.
C-l            Center on cursor and redraw screen.
C-v            Jump to line.
C-g            Help
C-f            Find / Search
C-k            Cut/copy current line. You can cut multiple lines by
               pressing C-k again directly.
C-y            Paste cut lines.
C-z            Read messages and get command line.
               Press enter on empty line to return.
END);

    if(!showfile)
        editer->message(NOPARSE + NOFILTER,@END
C-r            Save to different file.
C-t            Open file. Use C-u to switch between.
END);

    editer->message(NOPARSE + NOFILTER,@END
Press q to abort or any other key to continue
END);
}

protected void end_help(string str)
{
    if (sizeof (str) && (str[0] == 'q' || str[0] == 'Q'))
    {
        editer->remove_property ("visual help shown");
        done ();
        return;
    }
    draw_text();
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}

void net_dead()
{
    if(notify)
        catch(evaluate(notify,file[0]));
    if(!showfile && Modified && group_member(TO, "creators"))
    {
        File="/w/"+getuid()+"/DEADEDIT";
        save();
    }
    dest_me();
}

protected void get_jump (string str);

protected void jump ()
{
    ansi_goto_xy(1,th, editer);
    nprompt = "%^UNDERLINE%^Jump to line:%^RESET%^ ";
    editer->add_input((: get_jump :), IMODE_ONCE | IMODE_PROMPT);
}

protected void get_jump (string str)
{
    int i = to_int (str);

    if (i < 1)
        editer->message (NOPARSE + NOFILTER, BEL);
    else
    {
        if (i > sizeof (Lines))
            i = sizeof (Lines);
        Line = i - 1;
        Topline=Line-th/2;
        if(Topline>sizeof(Lines)-th+1)
            Topline=sizeof(Lines)-th+1;
        if(Topline<0)
            Topline=0;
    }
    draw_text();
    draw_pos();
    ansi_goto_xy(Col+1,Line-Topline+2, editer);
}
