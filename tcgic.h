#ifndef TCGIC_H



#define TCGIC_H
int checkval(char *, unsigned long, char *);
char *getval(char *);
char *multvalof(char *, char *, int);
char *pointloc(char *, char *);
char *tcgicinit(void);
char *valof(char *, char *);
void varfree(void *);
 
#define chkval(x) \
  checkval(x, __LINE__, __FILE__)

/* Define some global characters for error codes :-P */
/* Can test like this:
     ...
     a=valof( mystr, "this" );
     if (a[0]=='\0') {
       if (a==&tcgic_mem_error) {
         fprintf( stderr, "Memory error!\n" );
         exit( EXIT_FAILURE );
       }
     }
     ...
 * which is certainly different than the usual method... At any rate, we're
 * going to set all the characters to \0 so that: a) one can tell that an
 * error has occured without checking for all the possible errors, and b) so
 * that, should a user not check the return value, printing it out as a string
 * will only show a blank, rather than gobbledygook. */
char tc_memAllocErr;            /* Error allocating memory */
char tc_nullReqMeth;            /* REQUEST_METHOD not set/accessible */
char tc_nullQryStrn;            /* QUERY_STRING not set/accessible */
char tc_nullContLen;            /* CONTENT_LENGTH not set/accessible */
char tc_usupReqMeth;            /* Unsupported request method */
char tc_zeroContLen;            /* Zero content length (POST) */
char tc_negMultIndx;            /* Negative index in multvalof */
char tc_varNotFound;            /* Variable not in data string */
char tc_nullArgPssd;            /* Null pointers passed to valof */
 
/* Also define a global string of 256 characters (257 bytes) to be used to
 * store more helpful information about the error (like where it happened).
 * NOTE THAT THESE MAY CHANGE. They are for providing information ONLY; they
 * are NOT for identification of errors. Please see above for how to do
 * that. */
char tc_err[257];
 
 
 
#endif
