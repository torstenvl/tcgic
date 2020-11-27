# tcgic

Torsten's CGI in C Library

This is a simple library that makes writing CGI in C very easy. 

Usage:

    #include <stdio.h>
    #include "tcgic.h"
    
    int main(void) {
      char *cgi =  tcgicinit();
      char *name = valof(cgi, "username");
      
      printf("Content-type: text/html\n\n");
      printf("\
    <html>\n\
      <head><title>Hello, there</title></head>\n\
      <body>\n\
        <h1>Hello, %s!</h1>\n\
      <body>\n\
    </html>\n",
    username);
    
      free(name);
      free(cgi);
      
      return 0;
    }
    
To install this:

* Compile this along with the tcgic.c file into a .cgi file: `cc namedisp.c tcgic.c -o namedisp.cgi`
* Install the .cgi file on your webserver
* Ensure the execute permission is set on the .cgi file
* Ensure the web server is configured to execute .cgi files
* Ensure the directory is allowed to execute .cgi files
* Reference this .cgi file in the `action` attribute of your `<form>` tag






