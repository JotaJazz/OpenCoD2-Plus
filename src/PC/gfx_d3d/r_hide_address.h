#ifndef R_HIDE_ADDRESS_H
#define R_HIDE_ADDRESS_H

/* cl_hideServerIp's mask, for streaming: the digits of every IPv4 address in text, and of its port,
   become '*', so "connect 94.199.215.103:10405" draws as "connect **.***.***.***:*****". The length
   stays, so an edit field's cursor keeps its place. An address is groups of 1-3 digits joined by
   dots, at least two dots, so one being typed hides from its second dot and "1.3" or "0.5" never
   does, but a version such as "1.4.4" does; a color code's digit (^7) is not part of one. An edit
   field scrolled into an address shows the groups before its last two dots. Host names stay. */
static inline int R_HideAddressIsDigit(char c)
{
    return c >= '0' && c <= '9';
}

static inline void R_HideAddresses(char *text)
{
    char prev = ' ';
    char *p = text;

    while (*p) {
        char *q;
        int dots;

        if (p[0] == '^' && p[1]) {
            p += 2;
            prev = ' ';
            continue;
        }
        /* after a dot too: "Connecting...1.2.3.4", or an edit field scrolled to ".2.3.4" */
        if (!R_HideAddressIsDigit(*p) || R_HideAddressIsDigit(prev)) {
            prev = *p++;
            continue;
        }
        q = p;
        dots = 0;
        for (;;) {
            int n = 0;
            while (R_HideAddressIsDigit(q[n]))
                n++;
            if (n == 0 || n > 3)
                break;
            q += n;
            if (*q != '.' || dots == 3)
                break;
            q++;
            dots++;
        }
        if (dots < 2) {
            while (R_HideAddressIsDigit(*p))
                prev = *p++;
            continue;
        }
        if (*q == ':') {
            q++;
            while (R_HideAddressIsDigit(*q))
                q++;
        }
        for (; p < q; p++) {
            if (R_HideAddressIsDigit(*p))
                *p = '*';
        }
        prev = q[-1];
    }
}

#endif
