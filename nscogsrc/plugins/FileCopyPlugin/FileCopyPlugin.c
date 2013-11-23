/* Automatically generated by
	SmartSyntaxPluginCodeGenerator VMMaker.oscog-eem.517 uuid: 14ff7126-70ec-4cc4-9f55-70256e6a3d35
   from
	FileCopyPlugin VMMaker.oscog-eem.517 uuid: 14ff7126-70ec-4cc4-9f55-70256e6a3d35
 */
static char __buildInfo[] = "FileCopyPlugin VMMaker.oscog-eem.517 uuid: 14ff7126-70ec-4cc4-9f55-70256e6a3d35 " __DATE__ ;



#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Default EXPORT macro that does nothing (see comment in sq.h): */
#define EXPORT(returnType) returnType

/* Do not include the entire sq.h file but just those parts needed. */
/*  The virtual machine proxy definition */
#include "sqVirtualMachine.h"
/* Configuration options */
#include "sqConfig.h"
/* Platform specific definitions */
#include "sqPlatformSpecific.h"

#define true 1
#define false 0
#define null 0  /* using 'null' because nil is predefined in Think C */
#ifdef SQUEAK_BUILTIN_PLUGIN
#undef EXPORT
// was #undef EXPORT(returnType) but screws NorCroft cc
#define EXPORT(returnType) static returnType
#endif

#include "sqMemoryAccess.h"


/*** Constants ***/


/*** Function Prototypes ***/
static VirtualMachine * getInterpreter(void);
EXPORT(const char*) getModuleName(void);
static sqInt halt(void);
static sqInt msg(char *s);
EXPORT(sqInt) primitiveFileCopyNamedTo(void);
EXPORT(sqInt) setInterpreter(struct VirtualMachine*anInterpreter);
static sqInt sqAssert(sqInt aBool);


/*** Variables ***/

#if !defined(SQUEAK_BUILTIN_PLUGIN)
static sqInt (*failed)(void);
static void * (*firstIndexableField)(sqInt oop);
static sqInt (*isBytes)(sqInt oop);
static sqInt (*pop)(sqInt nItems);
static sqInt (*primitiveFail)(void);
static sqInt (*slotSizeOf)(sqInt oop);
static sqInt (*stackValue)(sqInt offset);
static sqInt (*success)(sqInt aBoolean);
#else /* !defined(SQUEAK_BUILTIN_PLUGIN) */
extern sqInt failed(void);
extern void * firstIndexableField(sqInt oop);
extern sqInt isBytes(sqInt oop);
extern sqInt pop(sqInt nItems);
extern sqInt primitiveFail(void);
extern sqInt slotSizeOf(sqInt oop);
extern sqInt stackValue(sqInt offset);
extern sqInt success(sqInt aBoolean);
extern
#endif
struct VirtualMachine* interpreterProxy;
static const char *moduleName =
#ifdef SQUEAK_BUILTIN_PLUGIN
	"FileCopyPlugin VMMaker.oscog-eem.517 (i)"
#else
	"FileCopyPlugin VMMaker.oscog-eem.517 (e)"
#endif
;



/*	Note: This is coded so that plugins can be run from Squeak. */

static VirtualMachine *
getInterpreter(void)
{
	return interpreterProxy;
}


/*	Note: This is hardcoded so it can be run from Squeak.
	The module name is used for validating a module *after*
	it is loaded to check if it does really contain the module
	we're thinking it contains. This is important! */

EXPORT(const char*)
getModuleName(void)
{
	return moduleName;
}

static sqInt
halt(void)
{
	;
	return 0;
}

static sqInt
msg(char *s)
{
	fprintf(stderr, "\n%s: %s", moduleName, s);
	return 0;
}

EXPORT(sqInt)
primitiveFileCopyNamedTo(void)
{
	char *dstName;
	sqInt dstSz;
	sqInt ok;
	char *srcName;
	sqInt srcSz;

	success(isBytes(stackValue(1)));
	srcName = ((char *) (firstIndexableField(stackValue(1))));
	success(isBytes(stackValue(0)));
	dstName = ((char *) (firstIndexableField(stackValue(0))));
	if (failed()) {
		return null;
	}
	srcSz = slotSizeOf(((sqInt)(long)(srcName) - BaseHeaderSize));
	dstSz = slotSizeOf(((sqInt)(long)(dstName) - BaseHeaderSize));
	ok = sqCopyFilesizetosize(srcName, srcSz, dstName, dstSz);
	if (!ok) {
		primitiveFail();
	}
	if (failed()) {
		return null;
	}
	pop(2);
	return null;
}


/*	Note: This is coded so that it can be run in Squeak. */

EXPORT(sqInt)
setInterpreter(struct VirtualMachine*anInterpreter)
{
	sqInt ok;

	interpreterProxy = anInterpreter;
	ok = ((interpreterProxy->majorVersion()) == (VM_PROXY_MAJOR))
	 && ((interpreterProxy->minorVersion()) >= (VM_PROXY_MINOR));
	if (ok) {
		
#if !defined(SQUEAK_BUILTIN_PLUGIN)
		failed = interpreterProxy->failed;
		firstIndexableField = interpreterProxy->firstIndexableField;
		isBytes = interpreterProxy->isBytes;
		pop = interpreterProxy->pop;
		primitiveFail = interpreterProxy->primitiveFail;
		slotSizeOf = interpreterProxy->slotSizeOf;
		stackValue = interpreterProxy->stackValue;
		success = interpreterProxy->success;
#endif /* !defined(SQUEAK_BUILTIN_PLUGIN) */
	}
	return ok;
}

static sqInt
sqAssert(sqInt aBool)
{
	/* missing DebugCode */;
}


#ifdef SQUEAK_BUILTIN_PLUGIN

void* FileCopyPlugin_exports[][3] = {
	{"FileCopyPlugin", "getModuleName", (void*)getModuleName},
	{"FileCopyPlugin", "primitiveFileCopyNamedTo", (void*)primitiveFileCopyNamedTo},
	{"FileCopyPlugin", "setInterpreter", (void*)setInterpreter},
	{NULL, NULL, NULL}
};

#endif /* ifdef SQ_BUILTIN_PLUGIN */
