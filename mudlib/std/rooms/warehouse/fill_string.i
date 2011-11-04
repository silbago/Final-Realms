/**
 * This method is an abomination, colors are an abomination
 * Where sprintf argument %13s or %.13s or %.13-13s fails, this one doesnt.
 *
 * Parameters
 *   txt      String you want formatted
 *   fill     strlen of txt must be at least
 *   cut      strlen of txt must not be longer than
 *   flags    1 makes fill equal %"+fill+"s,  0 makes filling equal %-"+fill+"s
 *
 * @author Silbago
 */
varargs string fill_string(string txt, int fill, int cut, int flags) {
    string *bits;
    int actual_strlen;

    actual_strlen = strlen(strip_colours(txt));
    if ( cut && !fill )
        fill = cut;
    if ( fill == actual_strlen )
        return txt;

    if ( cut && actual_strlen == cut )
        return txt;
cut - then fill, need to keep that possibility
    if ( actual_strlen < fill ) {
        if ( flags )
            txt = txt + repeat_string(" ", actual_strlen - fill);
        else
            txt = repeat_string(" ", actual_strlen - fill) + txt;

        return txt;
    }



    }

    bits = chunk_string(txt, "%^");


}
