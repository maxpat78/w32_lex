/***
*stdargv.c - standard & wildcard _setargv routine
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       processes program command line, with or without wildcard expansion
*
*******************************************************************************/


#include <windows.h>
#include <stdlib.h>
#ifdef _MBCS
#include <mbctype.h>
#endif  /* _MBCS */
#include <tchar.h>
#include <limits.h>

#define NULCHAR    _T('\0')
#define SPACECHAR  _T(' ')
#define TABCHAR    _T('\t')
#define DQUOTECHAR _T('\"')
#define SLASHCHAR  _T('\\')

/***
*static void parse_cmdline(cmdstart, argv, args, numargs, numchars)
*
*Purpose:
*       Parses the command line and sets up the argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, args
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only counting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numchars has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       _TSCHAR *cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       _TSCHAR **argv - where to build argv array; NULL means don't
*                       build array
*       _TSCHAR *args - where to place argument text; NULL means don't
*                       store text
*
*Exit:
*       no return value
*       int *numargs - returns number of argv entries created
*       int *numchars - number of characters used in args buffer
*
*Exceptions:
*
*******************************************************************************/

__declspec(dllexport)
#ifdef WPRFLAG
void __cdecl wparse_cmdline (
#else  /* WPRFLAG */
void __cdecl parse_cmdline (
#endif  /* WPRFLAG */
    _TSCHAR *cmdstart,
    _TSCHAR **argv,
    _TSCHAR *args,
    int *numargs,
    int *numchars
    )
{
        _TSCHAR *p;
        _TUCHAR c;
        int inquote;                    /* 1 = inside quotes */
        int copychar;                   /* 1 = copy char to *args */
        unsigned numslash;              /* num of backslashes seen */

        *numchars = 0;
        *numargs = 1;                   /* the program name at least */

        /* first scan the program name, copy it, and count the bytes */
        p = cmdstart;
        if (argv)
            *argv++ = args;

#ifdef WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* A quoted program name is handled here. The handling is much
           simpler than for other arguments. Basically, whatever lies
           between the leading double-quote and next one, or a terminal null
           character is simply accepted. Fancier handling is not required
           because the program name must be a legal NTFS/HPFS file name.
           Note that the double-quote characters are not copied, nor do they
           contribute to numchars. */
        inquote = FALSE;
        do {
            if (*p == DQUOTECHAR )
            {
                inquote = !inquote;
                c = (_TUCHAR) *p++;
                continue;
            }
            ++*numchars;
            if (args)
                *args++ = *p;

            c = (_TUCHAR) *p++;
#ifdef _MBCS
            if (_ismbblead(c)) {
                ++*numchars;
                if (args)
                    *args++ = *p;   /* copy 2nd byte too */
                p++;  /* skip over trail byte */
            }
#endif  /* _MBCS */

        } while ( (c != NULCHAR && (inquote || (c !=SPACECHAR && c != TABCHAR))) );

        if ( c == NULCHAR ) {
            p--;
        } else {
            if (args)
                *(args-1) = NULCHAR;
        }

        inquote = 0;

        /* loop on each argument */
        for(;;) {

            if ( *p ) {
                while (*p == SPACECHAR || *p == TABCHAR)
                    ++p;
            }

            if (*p == NULCHAR)
                break;              /* end of args */

            /* scan an argument */
            if (argv)
                *argv++ = args;     /* store ptr to arg */
            ++*numargs;

#ifdef WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* loop through scanning one argument */
        for (;;) {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
               2N+1 backslashes + " ==> N backslashes + literal "
               N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == SLASHCHAR) {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == DQUOTECHAR) {
                /* if 2N backslashes before, start/end quote, otherwise
                    copy literally */
                if (numslash % 2 == 0) {
                    if (inquote && p[1] == DQUOTECHAR) {
                        p++;    /* Double quote inside quoted string */
                    } else {    /* skip first quote char and copy second */
                        copychar = 0;       /* don't copy quote */
                        inquote = !inquote;
                    }
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--) {
                if (args)
                    *args++ = SLASHCHAR;
                ++*numchars;
            }

            /* if at end of arg, break loop */
            if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
                break;

            /* copy character into argument */
#ifdef _MBCS
            if (copychar) {
                if (args) {
                    if (_ismbblead(*p)) {
                        *args++ = *p++;
                        ++*numchars;
                    }
                    *args++ = *p;
                } else {
                    if (_ismbblead(*p)) {
                        ++p;
                        ++*numchars;
                    }
                }
                ++*numchars;
            }
            ++p;
#else  /* _MBCS */
            if (copychar) {
                if (args)
                    *args++ = *p;
                ++*numchars;
            }
            ++p;
#endif  /* _MBCS */
            }

            /* null-terminate the argument */

            if (args)
                *args++ = NULCHAR;          /* terminate string */
            ++*numchars;
        }

        /* We put one last argument in -- a null ptr */
        if (argv)
            *argv++ = NULL;
        ++*numargs;
}


