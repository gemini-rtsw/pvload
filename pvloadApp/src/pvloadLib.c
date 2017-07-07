static char rcsid[] = "$Id: pvloadLib.c,v 1.1 2011/11/19 01:24:43 tcsuuser Exp $";

/*+*********************************************************************
  Module:       pvloadLib.c

  Author:       W. Lupton

  Description:  Library routines for pv (EPICS process variable) loader
		(pvload) and saver (pvsave).  This module contains all
		routines which interact with the EPICS channel access
		library (accessed via either the ezca library or
		directly via ca).

  Notes:	1. These routines are not re-entrant. It's not worth making
		   them so, since the yacc code contains various static
		   variables.

		2. ezca error codes are not checked, but automatic error
		   messages are switched on. This policy should be reviewed.

		3. Refer to the comments to pvload.y for syntax details.
  *********************************************************************/

/* Include files */
#include <stdio.h>		/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tsDefs.h"		/* EPICS includes */
#include "cadef.h"
#include "macLib.h"
#include "epicsPrint.h"

#ifdef EZCA
#include "ezca.h"		/* ezca include (if used) */
#endif

#include "pvload.h"		/* pvload definitions */

/* Static variables */
static int  isPvload;		/* command is "pvload"? */
static int  isPvsave;		/* command is "pvsave"? */
static MAC_HANDLE *substHandle; /* macro substitution handle (NULL if none) */
static FILE *outFd;		/* fd for pvsave output file */

static int  debugFlag;		/* outputting debug info? */
static int  noCaInitFlag;	/* not initializing CA? */

static int  inGroup;		/* currently in group? */
static int  firstValue;		/* first value for PV? */

#ifdef EZCA
#else
static CA_SYNC_GID groupId;	/* ca synchronous group id */
#endif

static int    previousPercent;	/* Attributes of previous variable */

static int    currentPercent;	/* Attributes of current variable */
static int    currentIsPvsave;
#ifdef EZCA
static char   currentType;
#else
static chtype currentType;
#endif
static char   *currentName;
static int    currentCount;
static int    currentEquals;
static int    currentIndex;
static double currentValue;
static char   *currentString;
static double *currentArray;
static int    *currentSet;

/* Macros */
#define INVOKE(_status, _message, _extra) \
do { \
    int zzzCaStatus = ( _status ); \
    if ( !( ( zzzCaStatus ) & CA_M_SUCCESS ) ) \
	epicsPrintf( "%s: %s%s: %s\n", \
	    ( isPvload ) ? "pvload" : "pvsave", _message, _extra, \
	    ca_message_text[CA_EXTRACT_MSG_NO( zzzCaStatus )] ); \
} while ( FALSE )

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Forward declarations */

void pvloadPrint( char * );

/*+*********************************************************************
  Function:     pvloadInitialize

  Author:       W. Lupton

  Abstract:     Initialize internal structures

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadInitialize(
    int  isPvloadArg,
    MAC_HANDLE *substHandleArg,
    FILE *outFdArg,
    int  flags )
{
    /* Set "is pvload" and "is pvsave" flags, macro substitution handle
       and output fd */
    isPvload = isPvloadArg;
    isPvsave = !isPvloadArg;
    substHandle = substHandleArg;
    outFd = outFdArg;
    
    /* Process flags */
    debugFlag = flags & OPT_DEBUG;
    noCaInitFlag = flags & OPT_NOINIT;

    /* Initialize remaining static variables to defaults */
    inGroup = FALSE;
    firstValue = FALSE;
    previousPercent = FALSE;

    /* Initialize those per-variable variables that are not initialized */
    currentValue  = 0.0;
    currentString = NULL;
    currentArray  = NULL;
    currentSet    = NULL;

#ifdef EZCA
    /* Enable auto ezca error messages */
    ezcaAutoErrorMessageOn();

    /* Set sensible timeout and retry count (should allow tuning) */
    ezcaSetTimeout( 0.2 );
    ezcaSetRetryCount( 10 );
#else

    /* Initialize access to ca routines */
    if ( !noCaInitFlag ) {
#if defined (vxWorks)
      INVOKE( ca_task_initialize(),
		"initialize", "" );
#else
      /* ktt: needs R3.14.12.1 needs to be preemptive */
      INVOKE( ca_context_create ( ca_enable_preemptive_callback ),
	      "initialize", "" );
#endif
    }
#endif
}

/*+*********************************************************************
  Function:     pvloadShutdown

  Author:       W. Lupton

  Abstract:     Free internal structures

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadShutdown()
{
    /* Free anything not freed at end of each variable  */
    if ( currentString != NULL )
	free( ( void * ) currentString );
    if ( currentArray != NULL )
	free( ( void * ) currentArray );
    if ( currentSet != NULL )
	free( ( void * ) currentSet );

#ifdef EZCA
    /* Nothing to do at present */
#else

    /* Shut down access to ca routines */
    if ( !noCaInitFlag ) {
	INVOKE( ca_task_exit(),
		"shutdown", "" );
    }
#endif
}

/*+*********************************************************************
  Function:     pvloadStartGroup

  Author:       W. Lupton

  Abstract:     Start a group of PVs

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadStartGroup()
{
    /* Handle debug and pvsave output */
    pvloadPrint( "group {" );

    /* Groups are used only by pvload */
    if ( isPvload ) {
	inGroup = TRUE;

#ifdef EZCA
	/* Start ezca group */
	( void ) ezcaStartGroup();
#else

	/* Don't use groups for ca (suspected memory free problem) */
	inGroup = FALSE;

#if FALSE
	/* Start ca group */
	INVOKE( ca_sg_create( &groupId ),
		"start group", "" );
#endif
#endif
    }
}

/*+*********************************************************************
  Function:     pvloadEndGroup

  Author:       W. Lupton

  Abstract:     End a group of PVs

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadEndGroup()
{
    /* Handle debug and pvsave output */
    pvloadPrint( "};" );

    /* Groups are used only by pvload */
    if ( isPvload ) {
	inGroup = FALSE;
    
#ifdef EZCA
	/* End ezca group */
	( void ) ezcaEndGroup();
    
#else
    
	/* Don't use groups for ca (suspected memory free problem) */
#if FALSE
	/* End ca group */
	INVOKE( ca_sg_block( groupId, 2.0 ),
		"end group", "" );
    
	/* Delete ca group */
	INVOKE( ca_sg_delete( groupId ),
		"end group", "" );
#else
	INVOKE( ca_pend_io( 2.0 ),
		"end group", "" );
#endif
#endif
    }
}

/*+*********************************************************************
  Function:     pvloadStartVariable

  Author:       W. Lupton

  Abstract:     Start amassing info for a new PV

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadStartVariable( int percent )
{
    char buff[80];

    sprintf( buff, "%s", percent ? "%" : "" );
    pvloadPrint( buff );

    /* Set everything to its default (not values, which are retained) */
#ifdef EZCA
    currentType   = ezcaDouble;
#else
    currentType	  = DBR_DOUBLE;
#endif
    currentPercent= percent;
    currentName   = NULL;
    currentCount  = 1;
    currentEquals = FALSE;
    currentIndex  = 0;

    /* pvsave behavior is switched off by "%" lines which contain
       assignments (don't yet know whether there will be an assignment) */
    currentIsPvsave = isPvsave && !( currentPercent && currentEquals );
}

/*+*********************************************************************
  Function:     pvloadType

  Author:       W. Lupton

  Abstract:     Note the data type of the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadType( char *type )
{
    char buff[80];


    /* Handle debug and pvsave output */
    sprintf( buff, "%s ", type );
    pvloadPrint( buff );

    /* Convert the type string (assumed valid) to an ezca / ca type */
#ifdef EZCA
         if ( strcmp( type, "byte"   ) == 0 ) currentType = ezcaByte;
    else if ( strcmp( type, "string" ) == 0 ) currentType = ezcaString;
    else if ( strcmp( type, "short"  ) == 0 ) currentType = ezcaShort;
    else if ( strcmp( type, "long"   ) == 0 ) currentType = ezcaLong;
    else if ( strcmp( type, "float"  ) == 0 ) currentType = ezcaFloat;
    else if ( strcmp( type, "double" ) == 0 ) currentType = ezcaDouble;
#else
         if ( strcmp( type, "byte"   ) == 0 ) currentType = DBR_CHAR;
    else if ( strcmp( type, "string" ) == 0 ) currentType = DBR_STRING;
    else if ( strcmp( type, "short"  ) == 0 ) currentType = DBR_SHORT;
    else if ( strcmp( type, "long"   ) == 0 ) currentType = DBR_LONG;
    else if ( strcmp( type, "float"  ) == 0 ) currentType = DBR_FLOAT;
    else if ( strcmp( type, "double" ) == 0 ) currentType = DBR_DOUBLE;
#endif
}

/*+*********************************************************************
  Function:     pvloadName

  Author:       W. Lupton

  Abstract:     Note the name of the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadName( char *name )
{
    char substName[80];

    /* Substitute macros */
    if ( substHandle == NULL )
	strcpy( substName, name );
    else
	( void ) macExpandString( substHandle, name,
				  substName, sizeof( substName ) );

    /* Handle debug and pvsave output */
    pvloadPrint( substName );
    firstValue = TRUE;

    /* Allocate memory for name */
    currentName = ( char * ) malloc( strlen( substName ) + 1 );

    /* Check memory was allocated */
    if ( currentName == NULL ) {
	epicsPrintf( "Fatal error: memory allocation error!\n" );
	abort();
    }

    /* Copy the name */
    strcpy( currentName, substName );
}

/*+*********************************************************************
  Function:     pvloadCount

  Author:       W. Lupton

  Abstract:     Note the element count of the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadCount( int count )
{
#ifdef EZCA
#else
    chid channelId;
#endif
    char buff[80];


    /* Handle debug and pvsave output */
    sprintf( buff, "[%d]", count );
    pvloadPrint( buff );

    /* String arrays are not yet (and probably never will be) supported. */
    /* Abort if someone's trying to use one. */
#ifdef EZCA
    if ( currentType == ezcaString ) {
#else
    if ( currentType == DBR_STRING ) {
#endif
	epicsPrintf( "Fatal error: string arrays are not supported!\n" );
	abort();
    }

    /* Copy the count */
    currentCount = count;

    /* If the count is >1 (i.e. an array), de-allocate and re-allocate
       the array; note that this is unconditional and implies that "%"
       lines are not supported for arrays */
    if ( currentCount > 1 ) {

	/* Free storage if already allocated */
	if ( currentArray != NULL )
	    free( ( void * ) currentArray );
	if ( currentSet != NULL )
	    free( ( void * ) currentSet );

	/* Allocate a zeroed double array of the appropriate size */
	currentArray = ( double * ) calloc( currentCount, sizeof( double ) );

	/* Allocate a parallel int array to track which elems have been set */
	currentSet = ( int * ) calloc( currentCount, sizeof( int ) );

	/* Check memory was allocated */
	if ( currentArray == NULL || currentSet == NULL ) {
	    epicsPrintf( "Fatal error: memory allocation error!\n" );
	    abort();
	}

	/* If pvsave, read current value of entire array */
	if ( currentIsPvsave ) {
#ifdef EZCA
	    ( void ) ezcaGet( currentName, ezcaDouble, currentCount,
			      currentArray );
#else
	    INVOKE( ca_search( currentName, &channelId ),
		    "search ", currentName );
	    INVOKE( ca_pend_io( 2.0 ),
		    "search ", currentName );
	    INVOKE( ca_array_get( DBR_DOUBLE, currentCount,
				  channelId, currentArray ),
		    "put ", currentName );
	    INVOKE( ca_pend_io( 2.0 ),
		    "put ", currentName );
	    INVOKE( ca_clear_channel( channelId ),
		    "clear ", currentName );
	    INVOKE( ca_pend_io( 2.0 ),
		    "put ", currentName );
#endif
	}
    }
}

/*+*********************************************************************
  Function:     pvloadEquals

  Author:       W. Lupton

  Abstract:     Note that an equals sign has been specified

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadEquals()
{
    /* Handle debug and pvsave output */
    pvloadPrint( " = " );

    /* Note that the equals sign has been seen */
    currentEquals = TRUE;

    /* pvsave behavior is switched off by "%" lines which contain
       assignments (re-evaluate it) */
    currentIsPvsave = isPvsave && !( currentPercent && currentEquals );
}

/*+*********************************************************************
  Function:     pvloadIndex

  Author:       W. Lupton

  Abstract:     Note the index of the current value for the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadIndex( int index )
{
    char buff[80];


    /* Handle debug and pvsave output */
    if ( index >= 0 )
	sprintf( buff, "%s[%d] ", firstValue ? "" : ", ", index );
    else
	sprintf( buff, "%s", firstValue ? "" : ", " );
    pvloadPrint( buff );
    firstValue = FALSE;

    /* Ignore the rest if the index is negative */
    if ( index >= 0 ) {

	/* Set the index of the next value (validated below) */
	currentIndex = index;
    }
}

/*+*********************************************************************
  Function:     pvloadValue

  Author:       W. Lupton

  Abstract:     Note the current value for the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadValue( double value )
{
#ifdef EZCA
#else
    chid channelId;
#endif


    /* If pvsave, if this is the first line after a "%" line, use the value
       from the previous "%" line; otherwise, ignore the supplied value and
       instead read it from the online system (arrays have already been read
       so just copy the appropriate element) */
    if ( currentIsPvsave ) {
	if ( !currentPercent && previousPercent ) {
	    if ( currentCount == 1 ) {
		value = currentValue;
	    }
	    else {
		value = currentArray[currentIndex];
	    }
	}
	else {
	    if ( currentCount == 1 ) {
#ifdef EZCA
		( void ) ezcaGet( currentName, ezcaDouble, 1, &value );
#else
		INVOKE( ca_search( currentName, &channelId ),
			"search ", currentName );
		INVOKE( ca_pend_io( 2.0 ),
			"search ", currentName );
		INVOKE( ca_array_get( DBR_DOUBLE, 1,
				      channelId, &value ),
			"get ", currentName );
		INVOKE( ca_clear_channel( channelId ),
			"clear ", currentName );
		INVOKE( ca_pend_io( 2.0 ),
			"get ", currentName );
#endif
	    }
	    else {
		value = currentArray[currentIndex];
	    }
	}
    }

    /* If scalar, copy the current value (check that index is zero) */
    if ( currentCount == 1 && currentIndex == 0 ) {
	currentValue = value;
    }

    /* If an array and within bounds, copy the current value, note it's */
    /* set, and increment the index (warn if overwrite already set value) */
    else if ( currentIndex < currentCount ) {
	if ( currentSet[currentIndex] ) {
	    epicsPrintf( "Warning: %s[%d] = %g already set to %g; "
			    "overwritten\n", currentName, currentIndex,
				   value, currentArray[currentIndex] );
	}
	currentArray[currentIndex] = value;
	currentSet[currentIndex] = TRUE;
	currentIndex++;
    }

    /* If out of bounds, warn and ignore */
    else {
	epicsPrintf( "Warning: %s[%d] = %g index too large (max=%d); ignored\n",
			   currentName, currentIndex, value, currentCount - 1 );
    }

    /* Debug and pvsave output is deferred until the scale is known */
}

/*+*********************************************************************
  Function:     pvloadString

  Author:       W. Lupton

  Abstract:     Note the current string value for the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadString( char *value )
{
#ifdef EZCA
#else
    chid channelId;
#endif
    char buff[80];


    /* If pvsave, this is the first line after a "%" line, and a string is
       defined, it will be used, so jump to exit processing */
    if ( currentIsPvsave && !currentPercent && previousPercent &&
			    		currentString != NULL ) goto exit;

    /* Allocate memory for string value */
    if ( currentString != NULL )
	free( ( void * ) currentString );
    if ( currentIsPvsave ) {
	currentString = ( char * ) malloc( MAX_STRING_SIZE + 1 );
	currentString[MAX_STRING_SIZE] = '\0';
    }
    else
	currentString = ( char * ) malloc( strlen( value ) + 1 );

    /* Check memory was allocated */
    if ( currentString == NULL ) {
	epicsPrintf( "Fatal error: memory allocation error!\n" );
	abort();
    }

    /* Quietly force current type to be string */
#ifdef EZCA
    currentType = ezcaString;
#else
    currentType = DBR_STRING;
#endif

    /* If pvsave, ignore the supplied value and instead read it from the
       online system */
    if ( currentIsPvsave ) {
#ifdef EZCA
	( void ) ezcaGet( currentName, ezcaString, 1, currentString );
#else
	INVOKE( ca_search( currentName, &channelId ),
		"search ", currentName );
	INVOKE( ca_pend_io( 2.0 ),
		"search ", currentName );
	INVOKE( ca_array_get( DBR_STRING, 1,
			      channelId, currentString ),
		"get ", currentName );
	INVOKE( ca_clear_channel( channelId ),
		"clear ", currentName );
	INVOKE( ca_pend_io( 2.0 ),
		"get ", currentName );
#endif
    }

    /* Otherwise, copy the supplied value to the current string value */
    else {
	strcpy( currentString, value );
    }

    /* Handle debug and pvsave output (problem if string contains quotes?) */
exit:
    if ( currentEquals ) {
	sprintf( buff, "\"%s\"", currentString );
	pvloadPrint( buff );
    }
}

/*+*********************************************************************
  Function:     pvloadScale

  Author:       W. Lupton

  Abstract:     Note the scale factor for the current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        <optional-notes>
  *********************************************************************/
void
pvloadScale( char *label, double value )
{
    double temp;
    char buff[80];


    /* If string, warn if scale is not 1, and return immediately  */
#ifdef EZCA
    if ( currentType == ezcaString ) {
#else
    if ( currentType == DBR_STRING ) {
#endif
	if ( value != 1.0 ) {
	    epicsPrintf( "Warning: %s = \"%s\", scale %g ignored\n",
		         currentName, currentString, value );
	}
	return;
    }

    /* If value is zero, quietly set it to 1 to avoid divide by zero */
    if ( value == 0.0 ) value = 1.0;

    /* If scalar, scale the current value (check that index is zero) */
    if ( currentCount == 1 && currentIndex == 0 ) {
	if ( currentIsPvsave ) {
	    currentValue /= value;
	    temp = currentValue;
	}
	else {
	    temp = currentValue;
	    currentValue *= value;
	}
    }

    /* If an array and within bounds, scale the current value */
    else if ( currentCount > 1 && currentIndex >= 1 &&
				  currentIndex <= currentCount ) {
	if ( currentIsPvsave ) {
	    currentArray[currentIndex-1] /= value;
	    temp = currentArray[currentIndex-1];
	}
	else {
	    temp = currentArray[currentIndex-1];
	    currentArray[currentIndex-1] *= value;
	}
    }

    /* If out of bounds, warn and ignore */
    else {
	epicsPrintf( "Warning: %s[%d] = %g bad index (max=%d); ignored\n",
		       currentName, currentIndex - 1, value, currentCount - 1 );
    }

    /* Handle debug and pvsave output (includes deferred output of value;
       couldn't output value until scale was known) */
    if ( currentEquals ) {
	if ( label != NULL )
	    sprintf( buff, "%g %s", temp, label );
	else if ( value != 1.0 )
	    sprintf( buff, "%g %g", temp, value );
	else
	    sprintf( buff, "%g", temp );
	pvloadPrint( buff );
    }
}

/*+*********************************************************************
  Function:     pvloadEndVariable

  Author:       W. Lupton

  Abstract:     Put value for current variable

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        Direct ca blocks awaiting completion of the ca_search()
		before doing the put since the put can't be done until
		the channel id is valid. A better implementation would
		avoid this.
  *********************************************************************/
void
pvloadEndVariable()
{
#ifdef EZCA
#else
    chid channelId;
#endif


    /* If no equals sign was seen, call pvloadValue() / pvloadScale() or
       pvloadString() (depending on type) to correspond to '= 0.0' or
       '= ""' defaults */
    if ( !currentEquals ) {
#ifdef EZCA
	if ( currentType != ezcaString ) {
#else
	if ( currentType != DBR_STRING ) {
#endif
	    pvloadValue( 0.0 );
	}
	else {
	    pvloadString( "" );
	}
	pvloadScale( NULL, 1.0 );
    }

    /* Handle debug and pvsave output */
    pvloadPrint( ";" );

    /* If pvsave or pvload with "%" line, jump to exit processing */
    if ( currentIsPvsave || ( isPvload && currentPercent ) ) goto exit;

    /* If it's an array, warn if any elements have not been set */
    if ( currentCount > 1 ) {
	int index;
	for ( index = 0; index < currentCount; index++ )
	    if ( ! currentSet[index] )
		epicsPrintf( "Warning: %s[%d] never set; will be zero\n",
							currentName, index );
    }

    /* Put the value */
#ifdef EZCA
    if ( currentCount > 1 )
	( void ) ezcaPut( currentName, ezcaDouble, currentCount, currentArray );
    else if ( currentType != ezcaString )
	( void ) ezcaPut( currentName, ezcaDouble, 1, &currentValue );
    else
	( void ) ezcaPut( currentName, ezcaString, 1, currentString );
#else
    INVOKE( ca_search( currentName, &channelId ),
	    "pvloadEndVariable->search ", currentName );
    INVOKE( ca_pend_io( 2.0 ),
	    "search ", currentName );

    if ( inGroup ) {
	if ( currentCount > 1 )
	    INVOKE( ca_sg_array_put( groupId, DBR_DOUBLE, currentCount,
				     channelId, currentArray ),
		    "put ", currentName );
	else if ( currentType != DBR_STRING )
	    INVOKE( ca_sg_array_put( groupId, DBR_DOUBLE, 1,
				     channelId, &currentValue ),
		    "put ", currentName );
	else
	    INVOKE( ca_sg_array_put( groupId, DBR_STRING, 1,
				     channelId, currentString ),
		    "put ", currentName );
    }
    else {
	if ( currentCount > 1 )
	    INVOKE( ca_array_put( DBR_DOUBLE, currentCount,
				  channelId, currentArray ),
		    "put ", currentName );
	else if ( currentType != DBR_STRING ) {
	    INVOKE( ca_array_put( DBR_DOUBLE, 1,
				  channelId, &currentValue ),
		    "put ", currentName );
	}
	else {
	    INVOKE( ca_array_put( DBR_STRING, 1,
				  channelId, currentString ),
		    "put ", currentName );
	}
	INVOKE( ca_pend_io( 2.0 ),
		"put ", currentName );
	INVOKE( ca_clear_channel( channelId ),
		"clear ", currentName );
	INVOKE( ca_pend_io( 2.0 ),
		"put ", currentName );
    }
#endif

    /* Free storage for name (hang on to string and array in case needed
       for the next line) */
exit:
    if ( currentName != NULL )
	free( ( void * ) currentName );

    /* Remember if this was a "%" line */
    previousPercent = currentPercent;
}

/*+*********************************************************************
  Function:     pvloadSleep

  Author:       W. Lupton

  Abstract:     Sleep for a specified number of seconds

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        
  *********************************************************************/
void
pvloadSleep( double delay )
{
    char buff[80];

    /* Handle debug and pvsave output */
    sprintf( buff, "sleep %.3f;\n", delay );
    pvloadPrint( buff );

#ifdef vxWorks
    taskDelay( ( int ) ( sysClkRateGet() * delay ) );
#else
    sleep( ( int ) ( delay + 0.5 ) );
#endif
}

/*+*********************************************************************
  Function:     pvloadPrint

  Author:       W. Lupton

  Abstract:     Print string to standard output and/or pvsave output file

  Description:  <Description-for user>

  Method:       <Description-for-maintainer>

  Notes:        
  *********************************************************************/
void
pvloadPrint( char *string )
{


    /* Handle debug output (to stdout) */
    if ( debugFlag && !( isPvsave && outFd == stdout ) )
	fprintf( stdout, "%s", string );

    /* Handle pvsave output (also handle $pvsaveLog$ lines); note that
       "string" does not have a new line character */
    if ( isPvsave ) {
	char *start = strstr( string, "$pvsaveLog$" );

	fprintf( outFd, "%s", string );
	if ( start != NULL ) {
#ifdef vxWorks
	    char user[128]; /* set below */
	    char pass[128]; /* set below */
#else
	    char *user; /* set below */
#endif
	    char host[128]; /* set below */
	    time_t now = time( NULL );
	    struct tm *utc = gmtime( &now );
	    char *ascUtc = asctime( utc );

#ifdef vxWorks
	    remCurIdGet( user, pass );
#else
	    user = getenv( "USER" ) ? getenv( "USER" ) : "<user>";
#endif
	    strcpy( host, "<host>" );
	    gethostname( host, sizeof( host ) );
	    fprintf( outFd, "\n%.*ssaved by %s@%s on %.24s UTC",
		     start - string, string, user, host, ascUtc );
	}
    }
}
