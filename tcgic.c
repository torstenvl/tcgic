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



/* checkval is a simple, non-intrusive way to check the error status of a
 * variable, and upon finding an error, print out an error message. */
int checkval(char *value, unsigned long int line, char *file)
{
  if (value[0]=='\0') {
    fprintf(stderr, "Error at line %lu of %s: \n", line, file);
    fprintf(stderr, "Invalid value! %s\n", value);
    return 1;
  }
  return 0;
}



/* getval takes as its only argument a pointer to the beginning of a value
 * in a CGI data string. It returns a pointer to a string holding a copy of
 * the data contained withing the value to which its argument points. */
char *getval(char *loc)
{
  char nmrls[] = "0123456789ABCDEF";
  char *retstr;
  char tmp[2];
  size_t cnt;
  size_t nstr_cnt;
  size_t nstr_ln;

  /* Since this function'll usually be used in conjunction with pointloc,
   * it may be a good idea to check loc here, and if it is an error code,
   * we can just return it. That way, the user-programmer needn't worry about
   * testing loc before calling us with it. */
  if (loc[0]=='\0') {
    return loc;
  }

  /* Loop through until we find the end of the value, counting bytes as we go
   * along. Also, count hex codes, so we can subtract two bytes each, since
   * a three-digit hex code only translates to a one-byte character. */
  for (nstr_ln = 0, cnt=0; ((loc[nstr_ln] != '\0') && (loc[nstr_ln] != '&'));
       nstr_ln++) {
    if (loc[nstr_ln] == '%') {
      cnt++;
    }
  }
  nstr_ln -= (cnt * 2);

  /* Allocate the memory needed. */
  if ((retstr = malloc(nstr_ln + 1)) == NULL) {
    sprintf(tc_err, "Could not allocate enough memory to store the value "
            "of the variable");
    return &tc_memAllocErr;
  }

  /* Copy the value byte-by-byte, translating as we go. */
  /* Since we're translating hex codes, which go from three characters to one
   * on translation, we need to keep two seperate pointers to keep track of
   * where we are in each string: target and source. These are nstr_cnt and
   * cnt, respectively. */
  for (cnt = 0, nstr_cnt = 0; ((loc[cnt] != '\0') && (loc[cnt] != '&'));) {
    tmp[0] = loc[cnt];
    if (tmp[0] == '+') {        /* '+' is used for a space sometimes. */
      tmp[0] = ' ';
    } else if (tmp[0] == '%') { /* If it's a hex code... */
      tmp[0] = toupper(loc[++cnt]);
      tmp[1] = toupper(loc[++cnt]);
      tmp[0] = (char) (16 * (strchr(nmrls, tmp[0]) - nmrls) +
                       (strchr(nmrls, tmp[1]) - nmrls));
      /* The above code converts the two-digit hex code in tmp[0] and tmp[2]
       * into a character, stored in tmp[0]. It's an inflexible design, but
       * I believe it's justified by the lack of overhead. It serves its
       * purpose well. */
    }
    retstr[nstr_cnt++] = tmp[0];        /* Add character to new string. */
    cnt++;
  }
  retstr[nstr_cnt] = '\0';      /* Append null zero and return. */
  return retstr;
}



/* multvalof works like valof, searching the data string in the first argument
 * for the variable whose name is contained in the second argument, except
 * that multvalof facilitates handling of variables with multiple values.
 * multvalof returns the value numbering ordinally the value of the third
 * argument, unless the third argument is the number zero, in which case
 * multvalof concatenates successively found values, delimiting them by a
 * comma-space pair. */
char *multvalof(char *valstr, char *s, int index)
{
  char *curvarval;
  char *loc=NULL;
  char *locinvalstr = &valstr[0];
  char *retstr=NULL;
  int ctr;
  size_t nstr_ln;

  /* You'll notice the very explicit &valstr[0] instead of valstr. This is
   * because SOME STUPID COMPILERS (*ahem* BORLAND) copy the string when they
   * come across that kind of code. */

  /* Sanity checks... */
  if ((valstr == NULL) || (s == NULL)) {
    sprintf(tc_err, "NULL passed as an argument to multvalof()");
    return &tc_nullArgPssd;
  }

  if (index < 0) {

    /* We can't find the -nth value! */
    sprintf(tc_err, "Cannot find a negative instance of something! Think!" );
    return &tc_negMultIndx;

  } else if (index > 0) {

    /* If index is greater than zero, we are being asked to find a specific
     * instance of a variable within the string. So lets get it... */
    /* For ctr and index to line up, we have to do it this way... */
    for (ctr = 1; ctr <= index; ctr++) {
      loc = pointloc(locinvalstr, s);
      if (loc[0] == '\0') {
        /* If we get an error from trying to find the value, pass it back up
         * to the calling function...duh! :-P */
        return loc;
      }
      if (ctr != index) {
        /* We've found the next value of the variable, but it isn't the one
         * that we want. The value of the current instance is past the name
         * of the current instance, so lets just use loc as a safe starting
         * place for the next search. Since loc is already safe, we needn't
         * worry about incrementing by some arbitrary value and going past the
         * end of the string. */
        locinvalstr = loc;
      }
    }
    /* If we're here, we've pointed to the next value (index) times. As such,
     * we're now pointing to the (index)th value, the 'right' one, so we
     * should get it. */
    retstr = getval(loc);

  } else if (index == 0) {

    /* Concatenate all values, so we have a comma-delimited list. */
    /* Get the initial value. If we get an error, then return the error, no
     * matter what the error is. We know that sooner or later, we'll get one
     * about the value not being found, and that usually won't be an error
     * because it just means we've finished. However, if we get that message
     * the first time, then there exist NO values for the variable, and we
     * should return the error just like any other. */

    /* Get the first value and store it in retstr. We need to do this first,
     * or we'll end up calling strlen( NULL ) and seg-faulting. */
    loc=pointloc( locinvalstr, s);
    retstr=getval( loc );
    if (retstr[0]=='\0') {
      return retstr;
    }

    /* Set up locinvalstr and loc for the first iteration. */
    locinvalstr=loc;
    loc=pointloc( locinvalstr, s );
    while (loc[0]!='\0') {
      /* Process -- Get the next value of the variable and store it in
       * curvarval. If there is an error, we need to free what we have in
       * retstr so far, and return the error code. */
      curvarval = getval(loc);
      if (curvarval[0]=='\0') {
        free( retstr);
        return curvarval;
      }
      /* If we don't have an error, we're going to reallocate enough memory
       * to add on a ", " and the value we just got. */
      nstr_ln = strlen(retstr) + strlen(curvarval) + 2;
      if ((retstr = realloc(retstr, nstr_ln + 1)) == NULL) {
        sprintf(tc_err, "Could not allocate enough memory to store the "
                "value of the variable");
        /* If we cannot get enough memory, free both retstr and curvarval
         * before returning. */
        free(curvarval);
        free(retstr);
        return &tc_memAllocErr;
      }
      /* And when (if?) we finish checking for errors without finding any,
       * we need to concatenate the ", " and the value we just found. After
       * we've done that, we can (SHOULD) free the value, because it's already
       * been placed in the comma-space-delimited list, and because, if we
       * don't, the pointer will be moved on the next iteration and we'll
       * cause a memory leak. */
      strcat(retstr, ", ");
      strcat(retstr, curvarval);
      free(curvarval);
      /* Set up for next iteration. */
      locinvalstr=loc;
      loc=pointloc( locinvalstr, s );
    }

    /* At this point, loc[0] is '\0' so there's an 'error'. If we're lucky,
     * this 'error' is just that no more values can be found, but lets make
     * sure. */
    if (loc==&tc_memAllocErr) {
      free( retstr );
      return loc;
    }

  } /* else if index==0 */
  return retstr;
}



/* pointloc searches the data string passed as the first argument for the
 * first instance of the variable whose name is stored in the second argument,
 * and returns a pointer to the beginning of the value of that instance of
 * that variable. */
char *pointloc(char *valstr, char *s)
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
      sprintf(tc_err, "Could not allocate enough memory to store the search "
              "string");
      return &tc_memAllocErr;
    }
    sprintf(search, "&%s=", s);
    if ((loc = strstr(valstr, search)) == NULL) {
      /* If we can't find the variable in the data string, free the search
       * string and return NULL. */
      free(search);
      sprintf(tc_err, "Variable not found within the search string");
      return &tc_varNotFound;
    }
    /* If we did find it, still free the search string and move the location
     * to the beginning of the value for the found variable. */
    free(search);
    loc += (s_ln + 2);
  }
  return loc;
}



/* tcgicinit determines the data submission method, collects the data,
 * allocates sufficient memory, and stores it in a null-terminated string. The
 * return value is a pointer to this string. */
char *tcgicinit(void)
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

  /* Get the request method. If valid, act accordingly to get the submission
   * data. Note that fgets() is guaranteed to null-terminate the string. */
  if ((reqmeth = getenv("REQUEST_METHOD")) == NULL) {
    sprintf(tc_err, "Request method not set--server error");
    return &tc_nullReqMeth;
  }

  if (strcmp(reqmeth, "POST") == 0) {

    /* POST method feeds data to stdin, puts number of bytes in
     * CONTENT_LENGTH. First get the string... */
    if ((scntlen = getenv("CONTENT_LENGTH")) == NULL) {
      sprintf(tc_err, "Content length not set, though method is POST");
      return &tc_nullContLen;
    }
    /* Convert to a number. */
    if ((cntlen = (size_t) strtoul(scntlen, (char **) NULL, 10)) == 0) {
      sprintf(tc_err, "Content length is zero -- there is no data");
      return &tc_zeroContLen;
    }
    /* Return code if mem alloc err, otherwise get POST data from stdin. */
    if ((retstr = malloc(cntlen + 1)) == NULL) {
      sprintf(tc_err, "Couldn't allocate enough memory even to store the "
              "submitted CGI data string");
      return &tc_memAllocErr;
    }
    /* fgets's second argument takes the n-t zero into account. */
    retstr = fgets(retstr, cntlen + 1, stdin);

  } else if (strcmp(reqmeth, "GET") == 0) {

    /* In GET method, data string is in environment variable QUERY_STRING.
     * Get it and copy into memory. */
    if ((qrystr = getenv("QUERY_STRING")) == NULL) {
      sprintf(tc_err, "Query string not set, though method is GET");
      return &tc_nullQryStrn;
    }
    if ((retstr = malloc(strlen(qrystr) + 1)) == NULL) {
      sprintf(tc_err, "Couldn't allocate enough memory even to store the "
              "submitted CGI data string");
      return &tc_memAllocErr;
    }
    strcpy(retstr, qrystr);

  } else {
    /* We don't deal with any other methods...yet. */
    /* It may be useful to provide for others...of course, we'd need to know
     * standards for others. Perhaps there is a default for the way in which
     * data is sent, so the primary factor determining what methods may be
     * used isn't the server but the CGI itself; as long as using an arbitrary
     * method doesn't keep us from getting the data, we can do anything. Of
     * course, we could always implement such features a layer up, and perhaps
     * it would be better to do so. It may be worthwhile to explore the
     * possibilities... */
    sprintf(tc_err, "The request method used is not supported");
    return &tc_usupReqMeth;
  }
  return retstr;
}



/* Legacy valof function just finds the first instance of the variable sought,
 * no matter what. */
char *valof(char *valstr, char *s)
{
  char *loc;
  char *retstr;

  /* Make sure we weren't passed any NULL pointers. */
  if ((valstr == NULL) || (s == NULL)) {
    sprintf(tc_err, "NULL passed as an argument to valof()");
    return &tc_nullArgPssd;
  }
  loc = pointloc(valstr, s);
  retstr = getval(loc);
  return retstr;
}



/* Just wrapping this so user-programmer programs don't need to
 * #include <stdlib.h>, which contains the declaration of free(). Having this
 * built into the library very strongly hints that values should be freed when
 * they are no longer to be used. */
void varfree(void *var)
{
  free(var);
}
