#ifndef PLATFORM_H
#define PLATFORM_H
#include "core.h"
#include <stdio.h>

#if defined(WINDOWS)
#include <ws2tcpip.h>
typedef WIN32_FIND_DATAA ITER_FILE;
typedef cs_ulong TRET, TSHND_PARAM;
typedef void Waitable;
typedef CRITICAL_SECTION Mutex;
typedef SOCKET Socket;
typedef HANDLE Thread, ITER_DIR;
typedef BOOL TSHND_RET;
#define TSHND_OK TRUE
#define MSG_NOSIGNAL 0
#elif defined(UNIX)
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

typedef DIR *ITER_DIR;
typedef struct dirent *ITER_FILE;
typedef void *TRET;
typedef pthread_t Thread;
typedef pthread_mutex_t Mutex;
typedef void TSHND_RET;
typedef cs_int32 TSHND_PARAM;
typedef struct {
	pthread_cond_t cond;
	Mutex *mutex;
	cs_bool signalled;
} Waitable;
typedef cs_int32 Socket;
#define INVALID_SOCKET (Socket)-1
#define SD_SEND SHUT_WR
#define TSHND_OK
#endif

typedef FILE *cs_file;
typedef void *TARG;
typedef TRET(*TFUNC)(TARG);
typedef TSHND_RET(*TSHND)(TSHND_PARAM);
#define THREAD_FUNC(N) \
static TRET N(TARG param)

enum _EIterType {
	ITER_INITIAL,
	ITER_READY,
	ITER_DONE,
	ITER_ERROR
};

typedef struct _DirIter {
	cs_byte state;
	cs_char fmt[256];
	cs_str cfile;
	cs_bool isDir;
	ITER_DIR dirHandle;
	ITER_FILE fileHandle;
} DirIter;

#define DBUF_SIZE 2048

typedef struct _DataBuffer {
	cs_char data[DBUF_SIZE];
	cs_int32 wptr, rptr;
	Mutex *mutex;
} DataBuffer;

#define Memory_Zero(p, c) Memory_Fill(p, c, 0)

cs_bool Memory_Init(void);
void Memory_Uninit(void);

API void *Memory_TryAlloc(cs_size num, cs_size size);
API void *Memory_TryRealloc(void *oldptr, cs_size new);
API cs_size Memory_GetSize(void *ptr);
API void *Memory_Alloc(cs_size num, cs_size size);
API void *Memory_Realloc(void *oldptr, cs_size new);
API void  Memory_Copy(void *dst, const void *src, cs_size count);
API void  Memory_Fill(void *dst, cs_size count, cs_byte val);
API void  Memory_Free(void *ptr);

API DataBuffer *DataBuffer_New(void);
API void DataBuffer_Lock(DataBuffer *dbuf);
API void DataBuffer_Unlock(DataBuffer *dbuf);
API cs_int32 DataBuffer_Push(DataBuffer *dbuf, cs_char *data, cs_int32 len);
API cs_int32 DataBuffer_Pop(DataBuffer *dbuf, cs_char *data, cs_int32 len);
API void DataBuffer_Free(DataBuffer *dbuf);

API cs_bool Iter_Init(DirIter *iter, cs_str path, cs_str ext);
API cs_bool Iter_Next(DirIter *iter);
API void Iter_Close(DirIter *iter);

API cs_bool File_Rename(cs_str path, cs_str newpath);
API cs_file File_Open(cs_str path, cs_str mode);
API cs_file File_ProcOpen(cs_str cmd, cs_str mode);
API cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp);
API cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_int32 len);
API cs_size File_Write(const void *ptr, cs_size size, cs_size count, cs_file fp);
API cs_int32 File_GetChar(cs_file fp);
API cs_int32 File_Error(cs_file fp);
API cs_int32 File_WriteFormat(cs_file fp, cs_str fmt, ...);
API cs_bool File_Flush(cs_file fp);
API cs_int32 File_Seek(cs_file fp, long offset, cs_int32 origin);
API cs_bool File_Close(cs_file fp);
API cs_bool File_ProcClose(cs_file fp);

API cs_bool Directory_Exists(cs_str dir);
API cs_bool Directory_Create(cs_str dir);
API cs_bool Directory_Ensure(cs_str dir);
API cs_bool Directory_SetCurrentDir(cs_str path);

cs_bool DLib_Load(cs_str path, void **lib);
cs_bool DLib_Unload(void *lib);
cs_char *DLib_GetError(cs_char *buf, cs_size len);
cs_bool DLib_GetSym(void *lib, cs_str sname, void *sym);

cs_bool Socket_Init(void);
void Socket_Uninit(void);
API Socket Socket_New(void);
API cs_bool Socket_NonBlocking(Socket sock, cs_bool enabled);
API cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port);
API cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port);
API cs_bool Socket_Bind(Socket sock, struct sockaddr_in *ssa);
API cs_bool Socket_Connect(Socket sock, struct sockaddr_in *ssa);
API Socket Socket_Accept(Socket sock, struct sockaddr_in *addr);
API cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags);
API cs_int32 Socket_ReceiveLine(Socket sock, cs_char *line, cs_int32 len);
API cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len);
API void Socket_Shutdown(Socket sock, cs_int32 how);
API void Socket_Close(Socket sock);

API Thread Thread_Create(TFUNC func, const TARG param, cs_bool detach);
API cs_bool Thread_IsValid(Thread th);
API void Thread_Detach(Thread th);
API void Thread_Join(Thread th);
API void Thread_Sleep(cs_uint32 ms);

API Mutex *Mutex_Create(void);
API void Mutex_Free(Mutex *handle);
API void Mutex_Lock(Mutex *handle);
API void Mutex_Unlock(Mutex *handle);

API Waitable *Waitable_Create(void);
API void Waitable_Free(Waitable *handle);
API void Waitable_Signal(Waitable *handle);
API void Waitable_Wait(Waitable *handle);
API void Waitable_Reset(Waitable *handle);

API void Time_Format(cs_char *buf, cs_size len);
API cs_uint64 Time_GetMSec(void);

API cs_bool Console_BindSignalHandler(TSHND handler);

API void Process_Exit(cs_int32 ecode);
#endif // PLATFORM_H
