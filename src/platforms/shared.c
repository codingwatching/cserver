#include <stdio.h>
#include "core.h"
#include "platform.h"
#include "cserror.h"
#include "str.h"

void *Memory_Alloc(cs_size num, cs_size size) {
	void *ptr = Memory_TryAlloc(num, size);
	if(!ptr) {
		Error_PrintSys(true);
	}
	return ptr;
}

void *Memory_Realloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryRealloc(oldptr, new);
	if(!newptr) {
		Error_PrintSys(true);
	}
	return newptr;
}

void Memory_Copy(void *dst, const void *src, cs_size count) {
	cs_byte *u8dst = (cs_byte *)dst,
	*u8src = (cs_byte *)src;
	while(count--) *u8dst++ = *u8src++;
}

void Memory_Fill(void *dst, cs_size count, cs_byte val) {
	cs_byte *u8dst = (cs_byte *)dst;
	while(count--) *u8dst++ = val;
}

cs_file File_Open(cs_str path, cs_str mode) {
	return fopen(path, mode);
}

cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fread(ptr, size, count, fp);
}

cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_int32 len) {
	cs_int32 ch, ilen = len;
	while(len > 1) {
		ch = File_GetChar(fp);
		if(ch == EOF)
			return 0;
		else if(ch == '\n')
			break;
		else if(ch != '\r') {
			*line++ = (cs_char)ch;
			len -= 1;
		}
	}
	*line = '\0';
	if(len == 1) return -1;
	return ilen - len;
}

cs_size File_Write(const void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fwrite(ptr, size, count, fp);
}

cs_int32 File_GetChar(cs_file fp) {
	return fgetc(fp);
}

cs_bool File_Error(cs_file fp) {
	return ferror(fp) != 0;
}

cs_bool File_WriteFormat(cs_file fp, cs_str fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	return !File_Error(fp);
}

cs_bool File_Flush(cs_file fp) {
	return fflush(fp) == 0;
}

cs_int32 File_Seek(cs_file fp, long offset, cs_int32 origin) {
	return fseek(fp, offset, origin);
}

cs_bool File_Close(cs_file fp) {
	return fclose(fp) != 0;
}

cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port) {
	ssa->sin_family = AF_INET;
	ssa->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ssa->sin_addr.s_addr);
}

cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port) {
	cs_int32 ret;
	if((ret = Socket_SetAddr(ssa, host, port)) == 0) {
		struct addrinfo *addr;
		struct addrinfo hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = 0;
		hints.ai_protocol = 0;

		cs_char strport[6];
		String_FormatBuf(strport, 6, "%d", port);
		if((ret = getaddrinfo(host, strport, &hints, &addr)) == 0) {
			*ssa = *(struct sockaddr_in *)addr->ai_addr;
			freeaddrinfo(addr);
			return true;
		}
	}
	return ret == 1;
}

Socket Socket_New(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

cs_bool Socket_Bind(Socket sock, struct sockaddr_in *addr) {
#if defined(UNIX)
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(cs_int32){1}, 4) == -1) {
		return false;
	}
#elif defined(WINDOWS)
	if(setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (void*)&(cs_int32){0}, 4) == -1) {
		return false;
	}
#endif

	if(bind(sock, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
		return false;
	}

	if(listen(sock, SOMAXCONN) == -1) {
		return false;
	}

	return true;
}

cs_bool Socket_Connect(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return connect(sock, (struct sockaddr *)addr, len) == 0;
}

Socket Socket_Accept(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return accept(sock, (struct sockaddr *)addr, &len);
}

cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags) {
	return recv(sock, buf, len, MSG_NOSIGNAL | flags);
}

cs_int32 Socket_ReceiveLine(Socket sock, cs_char *line, cs_int32 len) {
	cs_int32 start_len = len;
	cs_char sym;

	while(len > 1) {
		if(Socket_Receive(sock, &sym, 1, MSG_WAITALL) == 1) {
			if(sym == '\n') {
				*line++ = '\0';
				break;
			} else if(sym != '\r') {
				*line++ = sym;
				--len;
			}
		} else return 0;
	}

	*line = '\0';
	return start_len - len;
}

cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len) {
	return send(sock, buf, len, MSG_NOSIGNAL);
}

void Socket_Shutdown(Socket sock, cs_int32 how) {
	shutdown(sock, how);
}

cs_bool Directory_Ensure(cs_str path) {
	if(Directory_Exists(path)) return true;
	return Directory_Create(path);
}