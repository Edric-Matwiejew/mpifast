/*
	sybintfc.c		Execute a Sybase command using a
				printf-style format for command building

	MODULE:	sybintf
	FILES:	sybintf.c (this one) and sybintf.h.

	This module contains functions which permit a printf-style format
	to be used for building and executing Sybase queries.  This
	avoids having the application have to piece the command together
	from a bunch of little pieces using multiple calls to dbcmd.

	Note:	This package was adapted for the Medline project from
		the same module in the GenInfo Backbone database project.
		Some minor changes had to be made to conform to Medline
		style conventions.

	Dependencies:

	    voutf.c	A package supporting printf-style formats with
			output directed via function calls instead of
			to a file or memory buffer.

	Edit history:

	    25 July 1991 - Rand S. Huntzinger
		Moved to Medline project area and adapted the code to
		meet Medline project requirements.

	    5 April 1991 - Added a print query debugging option.

	Original Implementation: 7 Dec 89 - Rand S. Huntzinger.
*
*
* RCS Modification History:
* $Log: sybintfc.c,v $
* Revision 6.0  1997/08/25 18:37:05  madden
* Revision changed to 6.0
*
* Revision 1.2  1995/05/17 17:56:08  epstein
* add RCS log revision history
*
*/

#include <stdio.h>
#include <ctype.h>
#include "sybintfc.h"
#include "voutf.h"


/* These global variables are used for debugging.  You can set them to cause
   queries to be written to a file.  */

FILE	*SybaseQueryPrintFile = stdout;		/* Default = standard output */
int	 SybaseQueryPrintCount = 0;		/* Default, don't print! */


/* These variables are used internally by the voutf processing procedure */

static	DBPROCESS	*dbproc;
static	RETCODE		 status;

/*
	build_sybase_command		Voutf string collection procedure.

	This procedure collects the little strings generated by the
	voutf function in interpreting it's format and directs them
	into the dbcmd() procedure for insertion into the Sybase
	command.

	Assumptions:

	  * The calling program has set the static variable "dbproc"
	    to reference the DBPROCESS to be used for building the
	    command.

	  * The static variable "status" will return the status of the
	    last call to this procedure when voutf exits.  If voutf
	    exits with an error, this will contain the error status
	    returned by dbcmd().

	Parameters: (defined by voutf protocol)

	    s		The string coming in from voutf().

	Returns: (defined by voutf protocol)

	    TRUE (non-zero) if the command failed,
	    FALSE (zero) if the command was successful.
*/

/*
*	intbuild_sybase_command(chars)
*/
static int build_sybase_command( char *s )
{
    /* Add the string to the command */

    return( (status = dbcmd( dbproc, s )) != SUCCEED );
}


/*
*		DumpSybaseCommand(DBPROCESSdbproc,FILEfd)
*/
void	DumpSybaseCommand( DBPROCESS *dbproc, FILE *fd )
{
	char	 buffer[61];
	int	 len = dbstrlen( dbproc );
	char	*stub = "Query:";
	int	 index = 0;
	int	 copy_count;
	register char *p;

	/* Dump the query out to SybaseQueryPrintFile */
 
	while( index < len )
	{
	    copy_count = (( len - index ) > (sizeof(buffer) - 1))
		? (sizeof(buffer) - 1) : (len - index);
	    dbstrcpy( dbproc, index, copy_count , buffer );
	    for(p=buffer; *p; p++) if(isspace(*p)) *p = ' ';
	    fprintf(fd, "%-12.12s || %s ||\n", stub, buffer);
	    stub = "";
	    index += copy_count;
        }
	fprintf(fd, "\n" );
}


/*
	VExtendSybaseCommand	Add to a Sybase command from a vector

	This procedure extends the Sybase command buffer using a format and
	a argument list pointer (usually passed by a variable argument
	list calling procedure).

	Parameters:

	    db		The Sybase database process to be used.

	    fmt		A printf-style format for generating this
			piece of the command.

	    args	An argument vector containing the arguments
			which are to be interpreted according to the
			format to generate this piece of the command
			string.

	Returns:

	    Zero if successful, non-zero on an error.
*/

/*
*		VExtendSybaseCommand(DBPROCESSdb,charfmt,voidargs)
*/
RETCODE	VExtendSybaseCommand( DBPROCESS *db, char *fmt, void *args )
{
    /* Load the command into the command buffer */

    dbproc = db;
    (void) voutf( build_sybase_command, fmt, args );

    /* Done */

    return( status );
}

/*
	VLoadSybaseCommand	Load a Sybase command from a vector

	This procedure loads the Sybase command buffer using a format and
	a argument list pointer (usually passed by a variable argument
	list calling procedure).  This is identical to the
	VExtendSybaseCommand procedure except any existing Sybase command
	is cleared out before loading the new text.

	Parameters:

	    db		The Sybase database process to be used.

	    fmt		A printf-style format for generating this
			piece of the command.

	    args	An argument vector containing the arguments
			which are to be interpreted according to the
			format to generate this piece of the command
			string.

	Returns:

	    Zero if successful, non-zero on an error.
*/

/*
*		VLoadSybaseCommand(DBPROCESSdb,charfmt,voidargs)
*/
RETCODE	VLoadSybaseCommand( DBPROCESS *db, char *fmt, void *args )
{
   RETCODE	status;

    /* Reset the database channel */

    dbproc = db;
    if( (status = dbcancel( dbproc )) != SUCCEED )
	return( status );

    /* Load the command into the command buffer */

    (void) voutf( build_sybase_command, fmt, args );

    /* Done */

    return( status );
}

/*
	LoadSybaseCommand	Load a Sybase command via a format.

	This procedure loads the Sybase command buffer using a variable
	argument list containing a format which is used to generate the
	command using printf-style conversions.

	Parameters:

	    db		The Sybase database process to be used.

	    fmt		A printf-style format for generating this
			piece of the command.

	    ...		The remaining arguments are interpreted according
			to the format (fmt) using printf conventions to
			generate the string to be loaded into the Sybase
			command buffer.

	Returns:

	    Zero if successful, non-zero on an error.
*/

/*
*		LoadSybaseCommand(DBPROCESSdbproc,charfmt,...)
*/
RETCODE	LoadSybaseCommand( DBPROCESS *dbproc, char *fmt,  ... )
{
    va_list	args;

    /* Extract the fixed parameters */

    va_start( args, char *fmt );

    /* Put the command in the buffer */

    (void) VLoadSybaseCommand( dbproc, fmt, args );
    va_end( args );

    /* Done */

    return( status );
}


/*
	ExtendSybaseCommand	Add to a Sybase command via a format.

	This procedure extends the Sybase command buffer using a variable
	argument list containing a format which is used to generate the
	command using printf-style conversions.  This procedure is
	identical to LaodSybaseCommand execept that the string generated
	here is appended to the existing Sybase command buffer instead of
	replacing it.

	Parameters:

	    db		The Sybase database process to be used.

	    fmt		A printf-style format for generating this
			piece of the command.

	    ...		The remaining arguments are interpreted according
			to the format (fmt) using printf conventions to
			generate the string to be loaded into the Sybase
			command buffer.

	Returns:

	    Zero if successful, non-zero on an error.
*/

/*
*		ExtendSybaseCommand(DBPROCESSdbproc,charfmt,...)
*/
RETCODE	ExtendSybaseCommand( DBPROCESS *dbproc, char * fmt, ... )
{
    va_list	 args;

    /* Extract the fixed parameters */

    va_start( args, fmt );
    dbproc = va_arg( args, DBPROCESS * );
    fmt = va_arg( args, char * );

    /* Put the command in the buffer */

    (void) VExtendSybaseCommand( dbproc, fmt, args );
    va_end( args );

    /* Done */

    return( status );
}


/*
	ExecSybaseCommand	Execute an existing Sybase command

	This procedure executes the command in the Sybase command buffer
	and returns the results of the execution.

	Parameters:

	    dbproc		The Sybase database process pointer.

	Returns:

	    The return code of dbsqlexec() if it fails; otherwise,
	    the return code of dbresults().  Basically, this should
	    amount to SUCCEED if successful, or FAIL on an error.
*/

/*
*		ExecSybaseCommand(dbproc)
*/
RETCODE	ExecSybaseCommand( dbproc )
	DBPROCESS	*dbproc;
{
    /* Execute the query */

    if((status = dbsqlexec( dbproc )) != SUCCEED)
        return( status );

    /* If the query succeeded, see if we want to print it out */

    if( SybaseQueryPrintCount != 0 ) {
	DumpSybaseCommand( dbproc, SybaseQueryPrintFile );
	if( SybaseQueryPrintCount > 0 ) SybaseQueryPrintCount--;
    }

    /* Load the results of the command */

    return( dbresults( dbproc ) );
}


/*
	RunSybase	Execute a Sybase command specified using a
			printf-style format and variable argument list.

	This procedure permits a printf-style format and variable argument
	list to be used to build & execute a sybase command.  It performs
	the first three steps of the Sybase command execution process,
	1) command construction using dbcmd(), 2) execution using dbseqlexec
	and 3) the retrieval of results using dbresults().

	Parameters: (variable argument list)

	    dbproc	A pointer to an open Sybase database process
			record.  (ie. a channel to the database)

	    fmt		The format string to be interpreted.

	    ...		The remaining arguments are interpreted
			according to the format given in the "fmt"
			argument using printf conventions.

	Returns:

	    The last return code returned from Sybase.  If successful,
	    this should be SUCCEED; otherwise, it will be the status
	    of the first step which failed.
*/

/*
*		RunSybase(DBPROCESSdbproc,charfmt,...)
*/
RETCODE	RunSybase( DBPROCESS *dbproc, char *fmt, ... )
{
    va_list	 args;

    /* Extract the fixed parameters */

    va_start( args, fmt );

    /* Load the command */

    (void) VLoadSybaseCommand( dbproc, fmt, args );
    va_end( args );
    if( status != SUCCEED ) return( status );

    /* Execute the Sybase command */

    return( ExecSybaseCommand( dbproc ) );
}


/*
	RunSybaseCheck	Count records returned by a Sybase command

	Executes a Sybase command and returns the number of records
	retrieved by the command.  The data is not available.


	Parameters: (variable argument list)

	    dbproc	A pointer to an open Sybase database process
			record.  (ie. a channel to the database)

	    fmt		The format string to be interpreted.

	    ...		The remaining arguments are interpreted
			according to the format given in the "fmt"
			argument using printf conventions.

	Returns:

	    The number of rows retrieved or -1 on an error.
	
*/

/*
*		RunSybaseCheck(DBPROCESSdbproc,charfmt,...)
*/
int	RunSybaseCheck( DBPROCESS *dbproc, char *fmt, ... )
{
    va_list	 args;
    int		 count;

    /* Extract the fixed parameters */

    va_start( args, fmt );
    dbproc = va_arg( args, DBPROCESS * );
    fmt = va_arg( args, char * );

    /* Load the command */

    (void) VLoadSybaseCommand( dbproc, fmt, args );
    va_end( args );
    if( status != SUCCEED ) return( -1 );

    /* Execute the Sybase command */

    if( ExecSybaseCommand( dbproc ) != SUCCEED )
	return( -1 );

    count = DBROWS( dbproc );
    (void) dbcancel( dbproc );

    return( count );
}
