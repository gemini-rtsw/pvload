/* $Id: fopenData.c,v 1.6 2006/06/08 00:57:09 dcsdev Exp $ */

/*
 * Replacement for fopen() which searches for a file by following a search
 * path (if the supplied name is not an absolute name). It should be used
 * whenever a data file is to be opened.
 *
 * The search parameter consists of a colon-separated list of absolute or
 * relative paths. If a path does not end in a "/" then the string "/data"
 * will be appended.
 */

#include <stdio.h>
#include <string.h>

#include <epicsPrint.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

FILE *fopenData(
    char *file,		/* file to open */
    char *type,		/* access type */
    char *search,	/* directory search path */
    char *path )	/* returned full path (not set if NULL) */
{
    char *start;	/* ptr to start of current search dir */
    char *end;		/* ptr to char after current search dir */
    long done;		/* controls loop exit */
    FILE *fd;		/* file descriptor pointer */
    long slash;		/* whether current search dir ends in "/" */
    char full[256];	/* full path name for fopen() */

    /* initially set the returned full path to the supplied file name */
    if ( path != NULL )
	strcpy( path, file );

    /* if the file name is an absolute path or no search path was provided,
       no search is necessary */
    if ( file[0] == '/'  || search == NULL || search[0] == '\0' ) {
	return fopen( file, type );
    }
	
    /* otherwise, loop while the search path is not exhausted */
    start = search;
    done  = FALSE;
    fd    = NULL;
    while ( !done ) {

	/* find the end of the next directory */
	end = strchr( start, ':' );
	if ( end == NULL ) {
	    end  = search + strlen( search );
	    done = TRUE;
	}

	/* if blank, pass original name straight through */
	if ( end == start ) {
	    strcpy( full, file );
	}

	/* check for slash and generate the full file name */
	else {
	    slash = ( *( end - 1 ) == '/' );
	    sprintf( full, "%.*s%s%s", end - start, start,
				       slash ? "" : "/data/", file );
	}

	/* set the returned full path */
	if ( path != NULL )
	    strcpy( path, full );

	/* attempt to open it */
	fd = fopen( full, type );

	/* for now, log (it's useful to know where files were found) */
#if SHOWFILE
	epicsPrintf( "fopenData( %s, %s, %s, %s ) -> %s\n", file, type,
		     search, full, ( fd == NULL ) ? "NULL" : "OK" );
#endif

	/* if successful, we're done; otherwise continue */
	if ( fd != NULL )
	    break;

	/* set the start of the next directory */
	start = end + 1;
    }

    /* return the fd, NULL if failed */
    return fd;
}
