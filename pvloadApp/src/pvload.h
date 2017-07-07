/*+*********************************************************************
  Module:       pvload.h

  Author:       W. Lupton

  Description:  Shared definitions for pvload and pvsave tools
  *********************************************************************/

/* flag values */
#define OPT_DEBUG    1		/* send debug output to standard error */
#define OPT_NOINIT   2		/* suppress CA initialization */
#define OPT_ABORT    4		/* abort VxWorks shell script on failure */
#define OPT_TRUNCATE 8		/* truncate output files */
#define OPT_FAIL     16		/* fail if output file exists */

/* prototypes for functions in pvloadLib.c */

void
pvloadInitialize(
    int  isPvload,
    MAC_HANDLE *substHandle,
    FILE *outFd,
    int  flags );

void
pvloadShutdown();

void
pvloadStartGroup();

void
pvloadEndGroup();

void
pvloadStartVariable( int percent );

void
pvloadType( char *type );

void
pvloadName( char *name );

void
pvloadCount( int count );

void
pvloadEquals();

void
pvloadIndex( int index );

void
pvloadValue( double value );

void
pvloadString( char *value );

void
pvloadScale( char *label, double value );

void
pvloadEndVariable();

void
pvloadSleep( double delay );

void
pvloadPrint( char *string );

/*+*********************************************************************
 *$Log: pvload.h,v $
 *Revision 1.1  2011/11/19 01:24:43  tcsuuser
 *Initial Insertion
 *
 *Revision 1.2  2006/06/08 00:57:24  dcsdev
 *Merged split reorg back to main trunk
 *
 *Revision 1.1.2.1  2006/06/02 01:27:15  dcsdev
 *Reorg for epcom subsystem split; moved from pvload into src dir (ktt)
 *
 *Revision 1.1.2.1  2006/06/01 00:18:59  dcsdev
 *Moved to epcom/pvload from epcom/etc/lib/pvload for epcom split (ktt)
 *
 *Revision 1.5  1998/02/06 08:26:29  ktsubota
 *Initial EPICS R3.13 conversion
 *
 *Revision 1.2.2.2  1997/12/09 01:08:43  wlupton
 *added 'percent' arg to pvloadStartVariable() and new pvloadEquals() routine
 *
 *Revision 1.2.2.1  1997/11/03 18:54:11  wlupton
 *added substHandle to pvloadInitialize()
 *
 *Revision 1.3  1997/11/03 18:46:22  wlupton
 *added substHandle to pvloadInitialize()
 *
 * Revision 1.2  1997/09/23  00:27:05  wlupton
 * working with arrays; added notes to pvload.y; for LLNL
 *
 *Revision 1.1  1997/09/20 03:19:05  wlupton
 *pvsave working with scalars (and strings?) but not arrays
 *
 ***********************************************************************/
