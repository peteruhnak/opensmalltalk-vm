/* sqUnixMozilla.c -- support for Squeak Netscape plugin
 *
 * Author: Bert Freudenberg <bert@isg.cs.uni-magdeburg.de>
 * 
 * Last edited: Tue 05 Mar 2002 09:52:03 by bert on balloon
 *
 * Originally based on Andreas Raab's sqWin32PluginSupport
 * 
 * Notes: The plugin window handling stuff is in sqXWindow.c.
 *        browserProcessCommand() is called when data is available
 */

#include "sq.h"

#ifndef HEADLESS

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#define DEBUG

#ifdef DEBUG
void DPRINT(char *format, ...)
{
  static int debug= 42;
  if (42 == debug) 
    debug= (NULL != getenv("NPSQUEAK_DEBUG"));

  if (!debug) 
    {
      return;
    }
  else
    {
      static FILE *file= 0;
      if (!file) 
	{
	  file= fopen("/tmp/npsqueak.log", "a+");
	}

      {
	va_list ap;
	va_start(ap, format);
	vfprintf(file, format, ap);
	va_end(ap);
	fflush(file);
      }
    }
}
#else
static int DPRINT(char *, ...) { }
#endif

/* from sqXWindow.c */
extern Display* stDisplay;
extern Window   stWindow;
extern Window   browserWindow;
extern Window   stParent;
extern int      browserPipes[2];

/* from interpret.c */
int stackObjectValue(int);
int stackIntegerValue(int);
int isBytes(int);
int byteSizeOf(int);
void *firstIndexableField(int);
int push(int);
int pop(int);
int positive32BitIntegerFor(int);
int nilObject();
int instantiateClassindexableSize(int, int);
int classByteArray();
int failed();
int pushBool(int);

/* prototypes */

static void browserReceive(void *buf, size_t count);
static void browserSend(const void *buf, size_t count);
static void browserSendInt(int value);
static void browserReceiveData();
static void browserGetURLRequest(int id, char* url, int urlSize,
				char* target, int targetSize);
static void browserPostURLRequest(int id, char* url, int urlSize, 
				 char* target, int targetSize, 
				 char* postData, int postDataSize);

typedef struct sqStreamRequest {
  char *localName;
  int semaIndex;
  int state;
} sqStreamRequest;

#define MAX_REQUESTS 128

#define SQUEAK_READ 0
#define SQUEAK_WRITE 1

#define inBrowser\
  (-1 != browserPipes[SQUEAK_READ])

#define CMD_BROWSER_WINDOW 1
#define CMD_GET_URL        2
#define CMD_POST_URL       3
#define CMD_RECEIVE_DATA   4

static sqStreamRequest *requests[MAX_REQUESTS];



/* primitives called from Squeak */



/*
  primitivePluginBrowserReady
  Return true if a connection to the browser
  has been established. Only necessary if some
  sort of asynchronous communications are used.
*/
int primitivePluginBrowserReady()
{
  pop(1);
  pushBool(inBrowser);
  return 1;
}


/*
  primitivePluginRequestUrlStream: url with: semaIndex
  Request a URL from the browser. Signal semaIndex
  when the result of the request can be queried.
  Returns a handle used in subsequent calls to plugin
  stream functions.
  Note: A request id is the index into requests[].
*/
int primitivePluginRequestURLStream()
{
  sqStreamRequest *req;
  int id, url, length, semaIndex;

  if (!inBrowser) return primitiveFail();

  DPRINT("VM: primitivePluginRequestURLStream()\n");

  for (id=0; id<MAX_REQUESTS; id++) {
    if (!requests[id]) break;
  }
  if (id >= MAX_REQUESTS) return primitiveFail();

  semaIndex= stackIntegerValue(0);
  url= stackObjectValue(1);
  if (failed()) return 0;

  if (!isBytes(url)) return primitiveFail();

  req= calloc(1, sizeof(sqStreamRequest));
  if (!req) return primitiveFail();
  req->localName= NULL;
  req->semaIndex= semaIndex;
  req->state= -1;
  requests[id]= req;

  length= byteSizeOf(url);
  browserGetURLRequest(id, firstIndexableField(url), length, NULL, 0);
  pop(3);
  push(positive32BitIntegerFor(id));
  DPRINT("VM:   request id: %i\n", id);
  return 1;
}


/*
  primitivePluginRequestURL: url target: target semaIndex: semaIndex
  Request a URL into the given target.
*/
int primitivePluginRequestURL()
{
  sqStreamRequest *req;
  int url, urlLength;
  int target, targetLength;
  int id, semaIndex;

  if (!browserWindow) return primitiveFail();
  for (id=0; id<MAX_REQUESTS; id++) {
    if (!requests[id]) break;
  }

  if (id >= MAX_REQUESTS) return primitiveFail();

  semaIndex= stackIntegerValue(0);
  target= stackObjectValue(1);
  url= stackObjectValue(2);

  if (failed()) return 0;
  if (!isBytes(url) || !isBytes(target)) return primitiveFail();

  urlLength= byteSizeOf(url);
  targetLength= byteSizeOf(target);

  req= calloc(1, sizeof(sqStreamRequest));
  if(!req) return primitiveFail();
  req->localName= NULL;
  req->semaIndex= semaIndex;
  req->state= -1;
  requests[id]= req;

  browserGetURLRequest(id, firstIndexableField(url), urlLength, firstIndexableField(target), targetLength);
  pop(4);
  push(positive32BitIntegerFor(id));
  return 1;
}


/*
  primitivePluginPostURL
*/
int primitivePluginPostURL()
{
  fprintf(stderr, "primitivePluginPostURL() not yet implemented\n");
  return primitiveFail(); 
}

/* 
  primitivePluginRequestFileHandle: id
  After a URL file request has been successfully
  completed, return a file handle for the received
  data. Note: The file handle must be read-only for
  security reasons.
*/
int primitivePluginRequestFileHandle()
{
  sqStreamRequest *req;
  int id, fileHandle;

  id= stackIntegerValue(0);
  if (failed()) return 0;
  if (id < 0 || id >= MAX_REQUESTS) return primitiveFail();
  req= requests[id];
  if (!req || !req->localName) return primitiveFail();
  fileHandle= nilObject();
  if (req->localName) {
    DPRINT("VM: Creating file handle for %s\n", req->localName);
    {

#if defined(FILEPLUGIN_EXTERNAL) /* otherwise loading won't work */

      int fRS, fVO, sFO;

      fRS= ioLoadFunctionFrom("fileRecordSize", "FilePlugin");
      fVO= ioLoadFunctionFrom("fileValueOf", "FilePlugin");
      sFO= ioLoadFunctionFrom("sqFileOpen", "FilePlugin");

      if ( !fRS || !fVO || !sFO) {
	fprintf(stderr, "Squeak Plugin: Couldn't load functions from FilePlugin!\n");
	return primitiveFail();
      }

# define fileRecordSize ((int(*)(void))fRS)
# define fileValueOf ((int(*)(int))fVO)
# define sqFileOpen ((int(*)(int, int, int, int))sFO)

#else /*! defined(FILEPLUGIN_EXTERNAL) */

      int fileRecordSize(void);
      int fileValueOf(int);
      int sqFileOpen(int, int, int, int);

#endif

      fileHandle= instantiateClassindexableSize(classByteArray(), fileRecordSize());
      sqFileOpen( fileValueOf(fileHandle),(int)req->localName, strlen(req->localName), 0);

      if (failed())
	DPRINT("VM:   file open failed\n");

#undef fileRecordSize
#undef fileValueOf
#undef sqFileOpen

    }
    if (failed()) return 0;
  }
  pop(2);
  push(fileHandle);
  return 1;
}


/*
  primitivePluginDestroyRequest: id
  Destroy a request that has been issued before.
*/
int primitivePluginDestroyRequest()
{
  sqStreamRequest *req;
  int id;

  id= stackIntegerValue(0);
  if (id < 0 || id >= MAX_REQUESTS) return primitiveFail();
  req= requests[id];
  if (req) {
    if (req->localName) free(req->localName);
    free(req);
  }
  requests[id]= NULL;
  pop(1);
  return 1;
}


/*
  primitivePluginRequestState: id
  Return true if the operation was successfully completed.
  Return false if the operation was aborted.
  Return nil if the operation is still in progress.
*/
int primitivePluginRequestState()
{
  sqStreamRequest *req;
  int id;

  id= stackIntegerValue(0);
  if (id < 0 || id >= MAX_REQUESTS) return primitiveFail();
  req= requests[id];
  if (!req) return primitiveFail();
  pop(2);
  if (req->state == -1) push(nilObject());
  else pushBool(req->state);
  return 1;
}



/* helper functions */

static void browserReceive(void *buf, size_t count)
{
  ssize_t n;
  n= read(browserPipes[SQUEAK_READ], buf, count);
  if (n == -1)
    perror("Squeak read failed:");
  if (n < count)
    fprintf(stderr, "Squeak read too few data from pipe\n");
}


static void browserSend(const void *buf, size_t count)
{
  ssize_t n;
  n= write(browserPipes[SQUEAK_WRITE], buf, count);
  if (n == -1)
    perror("Squeak plugin write failed:");
  if (n < count)
    fprintf(stderr, "Squeak wrote too few data to pipe\n");
}

static void browserSendInt(int value)
{
  browserSend(&value, 4);
}


/*
  browserReceiveData:
  Called in response to a CMD_RECEIVE_DATA message.
  Retrieves the data file name and signals the semaphore.
*/
static void browserReceiveData()
{
  char *localName= NULL;
  int id, ok;

  browserReceive(&id, 4);
  browserReceive(&ok, 4);

  DPRINT("VM:  receiving data id: %i state %i\n", id, ok);

  if (ok == 1) {
    int n, length= 0;
    browserReceive(&length, 4);
    if (length) {
      localName= malloc(length+1);
      browserReceive(localName, length);
      localName[length]= 0;
      DPRINT("VM:   got filename %s\n", localName);
    }
  }
  if (id >= 0 && id < MAX_REQUESTS) {
    sqStreamRequest *req= requests[id];
    if (req) {
      req->localName= localName;
      req->state= ok;
      DPRINT("VM:  signaling semaphore, state=%i\n", ok);
      /*  synchronizedSignalSemaphoreWithIndex(req->semaIndex);*/
      signalSemaphoreWithIndex(req->semaIndex);
    }
  }
}


/*
  browserGetURLRequest:
  Notify plugin to get the specified url into target
*/
static void browserGetURLRequest(int id, char* url, int urlSize, 
				char* target, int targetSize)
{
  if (!inBrowser) {
    fprintf(stderr, "Cannot submit URL request -- "
	    "there is no connection to a browser\n");
    return;
  }

  browserSendInt(CMD_GET_URL);
  browserSendInt(id);

  browserSendInt(urlSize);
  if (urlSize > 0)
    browserSend(url, urlSize);

  browserSendInt(targetSize);
  if (targetSize > 0)
    browserSend(target, targetSize);
}


/*
  browserPostURLRequest:
  Notify plugin to post data to the specified url and get result into target
*/
static void browserPostURLRequest(int id, char* url, int urlSize, 
				 char* target, int targetSize, 
				 char* postData, int postDataSize)
{
  if (!inBrowser) {
    fprintf(stderr, "Cannot submit URL post request -- "
	    "there is no connection to a browser\n");
    return;
  }

  browserSendInt(CMD_POST_URL);
  browserSendInt(id);

  browserSendInt(urlSize);
  if (urlSize > 0)
    browserSend(url, urlSize);

  browserSendInt(targetSize);
  if (targetSize > 0)
    browserSend(target, targetSize);

  browserSendInt(postDataSize);
  if (postDataSize > 0)
    browserSend(postData, postDataSize);
}


/***************************************************************
 * Functions called from sqXWindow.c
 ***************************************************************/

/*
  browserProcessCommand:
  Handle commands sent by the plugin.
*/
void browserProcessCommand()
{
  static int firstTime= 1;
  int cmd, n;

  if (firstTime)
    {
      firstTime= 0;
      /* enable non-blocking reads */
      fcntl(browserPipes[SQUEAK_READ], F_SETFL, O_NONBLOCK);
    }
  DPRINT("VM: browserProcessCommand()\n");

  n= read(browserPipes[SQUEAK_READ], &cmd, 4);
  if (0 == n || (-1 == n && EAGAIN == errno))
    return;

  switch (cmd)
    {
    case CMD_RECEIVE_DATA:
      /* Data is coming in */
      browserReceiveData();
      break;
    case CMD_BROWSER_WINDOW:
      /* Parent window has changed () */
      browserReceive(&browserWindow, 4);
      stParent= browserWindow;
      DPRINT("VM:  got browser window 0x%X\n", browserWindow);
      break;
    default:
      fprintf(stderr, "Unknown command from Plugin: %i\n", cmd);
    }
}

#endif /* HEADLESS */
