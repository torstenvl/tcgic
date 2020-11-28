#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tcgic.h"










/* Please note that it is extremely difficult to understand what this library
 * does without the knowledge of the format of the data on which it works.
 * The standard form of CGI submission data is as follows:
   this=that&iq=146&cat=feline&dog=canine&this=can+go+on+forever%21
 * that is to say, basically, variable=value for each variable, delimited by
 * ampesands (&). A '+' stands for a space, and characters may be hex-encoded
 * by placing a '%' directly in front of a two-letter code (21 in hex is 33 in
 * decimal...a '!' in ASCII). */










/* tc_init determines the data submission method, collects the data,
 * allocates sufficient memory, and stores it in a null-terminated string. The
 * return value is a pointer to this string. */
char *cgi_init(void)
{
    char *qrystr;
    char *reqmeth;
    char *retstr;
    char *scntlen;
    size_t cntlen;

    /* Set all error characters to '\0'. */
    tc_memAllocErr = '\0';        /* Error allocating memory */
    tc_nullReqMeth = '\0';        /* REQUEST_METHOD not set/accessible */
    tc_nullQryStrn = '\0';        /* QUERY_STRING not set/accessible */
    tc_nullContLen = '\0';        /* CONTENT_LENGTH not set/accessible */
    tc_usupReqMeth = '\0';        /* Unsupported request method */
    tc_zeroContLen = '\0';        /* Zero content length (POST) */
    tc_negMultIndx = '\0';        /* Negative index in multvalof */
    tc_varNotFound = '\0';        /* Variable not in data string */
    tc_nullArgPssd = '\0';        /* Null pointers passed to valof */

    if ((reqmeth = getenv("REQUEST_METHOD")) == NULL) {
        snprintf(tc_err, TCEZ, "Request method not set--server error");
        return &tc_nullReqMeth;
    }

    // FOR POST METHODS
    if (!strcmp(reqmeth, "POST")) {

        if ((scntlen = getenv("CONTENT_LENGTH")) == NULL) {
            snprintf(tc_err, TCEZ, "Content length not set, though method is POST");
            return &tc_nullContLen;
        }

        if ((cntlen = (size_t) strtoul(scntlen, (char **) NULL, 10)) == 0) {
            snprintf(tc_err, TCEZ, "Content length is zero -- there is no data");
          return &tc_zeroContLen;
        }

        if ((retstr = malloc(cntlen + 1)) == NULL) {
            snprintf(tc_err, TCEZ, "Couldn't allocate enough memory even to store the submitted CGI data string");
            return &tc_memAllocErr;
        }

        retstr = fgets(retstr, cntlen + 1, stdin);

    }

    // FOR GET METHODS
    else if (!strcmp(reqmeth, "GET")) {

        if ((qrystr = getenv("QUERY_STRING")) == NULL) {
            snprintf(tc_err, TCEZ, "Query string not set, though method is GET");
            return &tc_nullQryStrn;
        }

        if ((retstr = malloc(strlen(qrystr) + 1)) == NULL) {
            snprintf(tc_err, TCEZ, "Couldn't allocate enough memory even to store the submitted CGI data string");
            return &tc_memAllocErr;
        }

        strcpy(retstr, qrystr);

    }

    // OTHERWISE IT'S AN ERROR
    else {
        snprintf(tc_err, TCEZ, "The request method used is not supported");
        return &tc_usupReqMeth;
    }

    return retstr;
}










/* Legacy valof function just finds the first instance of the variable sought,
 * no matter what. */
char *cgi_valof(char *valstr, char *s)
{
    return cgi_allvalof(valstr, s, 1);
}










/* Just wrapping this so user-programmer programs don't need to
 * #include <stdlib.h>, which contains the declaration of free(). Having this
 * built into the library very strongly hints that values should be freed when
 * they are no longer to be used. */
void cgi_free(void *var)
{
    free(var);
}










/* allvalof works like valof, but gets all values if index is 0, or gets the
 * index-th value if it is non-zero, i.e., it returns the value numbering
 * ordinally the value of the third argument. */
char *cgi_allvalof(char *valstr, char *s, int index)
{
    char *curvarval = NULL;
    char *loc = NULL;
    char *locinvalstr = NULL;
    char *retstr = NULL;
    int ctr = 0;
    size_t nstr_ln = 0L;


    // SANITY: Check for NULLs
    if ((valstr == NULL) || (s == NULL)) {
        snprintf(tc_err, TCEZ, "NULL passed as an argument to multvalof()");
        return &tc_nullArgPssd;
    }


    // SANITY: Check for negative index
    if (index < 0) {
        snprintf(tc_err, TCEZ, "Cannot find a negative instance of something! Think!" );
        return &tc_negMultIndx;
    }


    // Why &valstr[0] instead of valstr? Because SOME STUPID COMPILERS
    // (*ahem* BORLAND) copy the whole fucking string when they
    // come across that kind of code.
    locinvalstr = &valstr[0];


    // Get tthe specific instance of the value that the user requested
    if (index > 0) {
        for (ctr = 1; ctr <= index; ctr++) {
            loc = findvalue(locinvalstr, s);

            // If there's an error, propagate it up.
            if (loc[0] == '\0') return loc;

            // If ctr isn't there yet, move our iterator to juuuuuust past
            // the ctr-th instance of the value name, and keep looking.
            if (ctr != index) locinvalstr = loc;
        }
        return extractvalue(loc);
    }


    // Concatenate all values into a comma-delimited list.
    if (index == 0) {
        loc = findvalue(locinvalstr, s);
        retstr = extractvalue(loc);

        if (retstr[0]=='\0') return retstr;

        /* Set up locinvalstr and loc for the first iteration. */
        locinvalstr = loc;
        loc = findvalue(locinvalstr, s);

        while (loc[0] != '\0') {
            curvarval = extractvalue(loc);

            // If there's an error, free allocated memory and return error.
            if (curvarval[0]=='\0') {
                free (retstr);
                return curvarval;
            }

            // New length sufficient for s + ", " + s_addition
            nstr_ln = strlen(retstr) + 2 + strlen(curvarval);

            // Reallocate memory or return memory allocation error
            if ((retstr = realloc(retstr, nstr_ln + 1)) == NULL) {
                snprintf(tc_err, TCEZ, "Could not allocate enough memory to store the value of the variable");
                free(curvarval);
                free(retstr);
                return &tc_memAllocErr;
            }

            // Concatenate the values. This is a safe use of sprintf() because
            // we pre-calculated and pre-allocated.
            sprintf(retstr + strlen(retstr), ", %s", curvarval);

            free(curvarval);

            locinvalstr = loc;
            loc = findvalue(locinvalstr, s);
        }

        if (loc==&tc_memAllocErr) {
            free(retstr);
            return loc;
        }

        return retstr;
    }
}










/* extractstring gets the CGI value beginning at p and returns a newly
 * allocated string holding a copy of it. */
char *extractvalue(char *p)
{
    char numerals[] = "0123456789ABCDEF";
    char *newstring;
    size_t i;
    size_t j;

    size_t hexcount = 0;

    /* By internal convention, a null zero at [0] means an error. Return the same. */
    if (p[0]=='\0') return p;


    /* Count bytes until we reach the end of the value, keeping track of the
     * number of % signs. These bytes will be followed by two more bytes in
     * the source, which will encode one byte in the result, affecting the
     * overall length. */
    for (i = 0, hexcount = 0; (p[i] != '\0') && (p[i] != '&'); i++) {
        if (p[i] == '%') hexcount += 2;
    }


    /* Allocate the memory needed. */
    newstring = malloc(i - hexcount + 1);

    if (newstring == NULL) {
        snprintf(tc_err, TCEZ, "Could not allocate enough memory to store the value of the variable");
        return &tc_memAllocErr;
    }


    /* Copy the value byte-by-byte, translating as we go. */
    for (i = 0, j = 0; (p[i] != '\0') && (p[i] != '&'); ) {

        /* Translate '+' to ' ' */
        if (p[i] == '+') {
            newstring[j++] = ' ';
            i++;
        }

        /* Translate hex codes to bytes */
        else if (p[i] == '%') {
            newstring[j] = (char)16 * (strchr(numerals, p[++i]) - numerals);
            newstring[j] += (strchr(numerals, p[++i]) - numerals);
            i++;
        }

        else {
            newstring[j++] = p[i++];
        }
    }

    newstring[j] = '\0';

    return newstring;
}










/* pointloc searches the data string passed as the first argument for the
 * first instance of the variable whose name is stored in the second argument,
 * and returns a pointer to the beginning of the value of that instance of
 * that variable. */
char *findvalue(char *valstr, char *s)
{
    char *loc;
    char *search;
    size_t s_ln = strlen(s);

    /* Point loc to the location of the variable name, then move it up the
     * length of that name so it points to the string containing the requested
     * variable's value. Check the beginning of the data string first, then
     * use a library function to check for a "&name=" string. */
    if ((strncmp(valstr, s, s_ln) == 0) && (valstr[s_ln] == '=')) {
        loc = valstr + s_ln + 1;
    } else {
        /* Make enough room for s plus the & and the = and the n-t zero. */
        if ((search = malloc(s_ln + 3)) == NULL) {
            snprintf(tc_err, TCEZ, "Could not allocate enough memory to store the search string");
            return &tc_memAllocErr;
        }
        sprintf(search, "&%s=", s);
        if ((loc = strstr(valstr, search)) == NULL) {
            /* If we can't find the variable in the data string, free the search
             * string and return NULL. */
            free(search);
            snprintf(tc_err, TCEZ, "Variable not found within the search string");
            return &tc_varNotFound;
        }
        /* If we did find it, still free the search string and move the location
         * to the beginning of the value for the found variable. */
        free(search);
        loc += (s_ln + 2);
    }
    return loc;
}
