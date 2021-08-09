/*************************************************************************\
* Copyright (c) 2007 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include <stdlib.h>

#include "iocsh.h"
#include "errlog.h"
#include "epicsExport.h"

int pvload(char *file, char *subst, int flags );

/* pvload */

static const iocshArg pvloadArg0 = { "file",iocshArgString};
static const iocshArg pvloadArg1 = { "subst",iocshArgString};
static const iocshArg pvloadArg2 = { "flags",iocshArgInt};
static const iocshArg * const pvloadArgs[3] =
{
    &pvloadArg0,&pvloadArg1,&pvloadArg2
};

static const iocshFuncDef pvloadFuncDef = {"pvload",3,pvloadArgs};
static void pvloadCallFunc(const iocshArgBuf *args)
{
    pvload (args[0].sval,args[1].sval,args[2].ival);
}


void epicsShareAPI pvloadRegistrar(void)
{
    iocshRegister(&pvloadFuncDef, pvloadCallFunc);
}

epicsExportRegistrar( pvloadRegistrar);
