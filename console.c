#include "core.h"
#include "log.h"
#include "console.h"
#include "command.h"
#include "platform.h"

int Console_ReadLine(char* buf, int buflen) {
	int len = 0;
	int c = 0;

	while((c = getc(stdin)) != EOF && c != '\n') {
		if(c != '\r') {
			buf[len] = c;
			len++;
			if(len > buflen) {
				len--;
				break;
			}
		}
	}
	buf[len] = 0;

	return len;
}

void Console_HandleCommand(char* cmd) {
	if(!Command_Handle(cmd, NULL)) {
		Log_Info("Unknown command");
	}
}

int Console_ThreadProc(void* lpParam) {
	char buf[4096] = {0};

	while(1) {
		if(Console_ReadLine(buf, 4096) > 0)
			Console_HandleCommand(buf);
	}
	return 0;
}

void Console_StartListen() {
	Console_Thread = Thread_Create(&Console_ThreadProc, NULL);
}

void Console_Close() {
	if(Console_Thread)
		Thread_Close(Console_Thread);
}
