#include "core.h"
#include "str.h"
#include "log.h"
#include "error.h"
#include "server.h"
#include "client.h"
#include "protocol.h"
#include "config.h"
#include "command.h"
#include "websocket.h"
#include "generators.h"
#include "heartbeat.h"
#include "platform.h"
#include "event.h"
#include "plugin.h"
#include "lang.h"
#include "timer.h"

static void AcceptFunc(void) {
	struct sockaddr_in caddr;
	Socket fd = Socket_Accept(Server_Socket, &caddr);

	if(fd != INVALID_SOCKET) {
		if(!Server_Active) {
			Socket_Close(fd);
			return;
		}

		cs_uint32 addr = ntohl(caddr.sin_addr.s_addr);
	 	Client *tmp = Client_New(fd, addr);
		cs_int8 maxConnPerIP = Config_GetInt8ByKey(Server_Config, CFG_CONN_KEY),
		sameAddrCount = 1;

		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *other = Clients_List[i];
			if(other && other->addr == addr)
				++sameAddrCount;
			else continue;

			if(sameAddrCount > maxConnPerIP) {
				Client_Kick(tmp, Lang_Get(LANG_KICKMANYCONN));
				Client_Free(tmp);
				return;
			}
		}

		cs_uint8 attempt = 0;
		while(attempt++ < 5) {
			if(Socket_Receive(fd, tmp->rdbuf, 5, MSG_PEEK) == 5) {
				if(String_CaselessCompare(tmp->rdbuf, "GET /")) {
					WsClient *wscl = Memory_Alloc(1, sizeof(WsClient));
					wscl->recvbuf = tmp->rdbuf;
					wscl->sock = tmp->sock;
					tmp->websock = wscl;
					if(WsClient_DoHandshake(wscl))
						goto client_ok;
					else break;
				} else goto client_ok;
			}
			Sleep(100);
		}
		Client_Kick(tmp, Lang_Get(LANG_KICKPACKETREAD));
		Client_Free(tmp);

		client_ok:
		if(!Client_Add(tmp)) {
			Client_Kick(tmp, Lang_Get(LANG_KICKSVFULL));
			Client_Free(tmp);
		}
	}
}

THREAD_FUNC(AcceptThread) {
	(void)param;
	while(Server_Active) AcceptFunc();
	return 0;
}

static void Bind(cs_str ip, cs_uint16 port) {
	Server_Socket = Socket_New();
	struct sockaddr_in ssa;
	switch (Socket_SetAddr(&ssa, ip, port)) {
		case 0:
			ERROR_PRINT(ET_SERVER, EC_INVALIDIP, true);
			break;
		case -1:
			Error_PrintSys(true);
			break;
	}

	Client_Init();
	Log_Info(Lang_Get(LANG_SVSTART), ip, port);
	if(!Socket_Bind(Server_Socket, &ssa)) {
		Error_PrintSys(true);
	}
}

THREAD_FUNC(ConsoleThread) {
	(void)param;
	char buf[192];

	while(Server_Active) {
		if(File_ReadLine(stdin, buf, 192))
			if(!Command_Handle(buf, NULL))
				Log_Info(Lang_Get(LANG_CMDUNK));
	}
	return 0;
}

void Server_InitialWork(void) {
	if(!Socket_Init()) return;

	Log_Info(Lang_Get(LANG_SVLOADING), MAINCFG);
	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Config = cfg;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CFG_STR);
	Config_SetComment(ent, "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\".");
	Config_SetDefaultStr(ent, "0.0.0.0");

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CFG_INT16);
	Config_SetComment(ent, "Use specified port to accept clients. [1-65535]");
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt16(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CFG_STR);
	Config_SetComment(ent, "Server name and MOTD will be shown to the player during map loading.");
	Config_SetDefaultStr(ent, "Server name");

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CFG_STR);
	Config_SetDefaultStr(ent, "Server MOTD");

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CFG_STR);
	Config_SetComment(ent, "I - Info, C - Chat, W - Warnings, D - Debug.");
	Config_SetDefaultStr(ent, "ICWD");

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CFG_BOOL);
	Config_SetComment(ent, "Any player with ip address \"127.0.0.1\" will automatically become an operator.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CFG_INT8);
	Config_SetComment(ent, "Max players on server. [1-127]");
	Config_SetLimit(ent, 1, 127);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CFG_INT8);
	Config_SetComment(ent, "Max connections per one IP. [1-5]");
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_KEY, CFG_BOOL);
	Config_SetComment(ent, "Enable ClassiCube heartbeat.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_HEARTBEATDELAY_KEY, CFG_INT32);
	Config_SetComment(ent, "Heartbeat request delay. [1-60]");
	Config_SetLimit(ent, 1, 60);
	Config_SetDefaultInt32(ent, 10);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_PUBLIC_KEY, CFG_BOOL);
	Config_SetComment(ent, "Show server in the ClassiCube server list.");
	Config_SetDefaultBool(ent, false);

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		Config_PrintError(cfg);
		Process_Exit(1);
	}
	Log_SetLevelStr(Config_GetStrByKey(cfg, CFG_LOGLEVEL_KEY));

	Packet_RegisterDefault();
	Command_RegisterDefault();

	Log_Info(Lang_Get(LANG_SVLOADING), "plugins");
	Plugin_Start();

	Directory_Ensure("worlds");
	WorldID wIndex = 0;
	dirIter wIter;
	if(Iter_Init(&wIter, "worlds", "cws")) {
		do {
			if(wIter.isDir || !wIter.cfile) continue;
			World *tmp = World_Create(wIter.cfile);
			tmp->id = wIndex++;
			if(!World_Load(tmp) || !World_Add(tmp))
				World_Free(tmp);
		} while(Iter_Next(&wIter) && wIndex < MAX_WORLDS);
	}
	Iter_Close(&wIter);

	if(wIndex < 1) {
		World *tmp = World_Create("world.cws");
		SVec defdims = {256, 256, 256};
		World_SetDimensions(tmp, &defdims);
		World_AllocBlockArray(tmp);
		Generator_Flat(tmp);
		Worlds_List[0] = tmp;
	}

	if(Config_GetBoolByKey(cfg, CFG_HEARTBEAT_KEY)) {
		Log_Info(Lang_Get(LANG_HBEAT));
		Heartbeat_Start(Config_GetInt32ByKey(cfg, CFG_HEARTBEATDELAY_KEY));
	}

	Server_Active = true;
	cs_str ip = Config_GetStrByKey(cfg, CFG_SERVERIP_KEY);
	cs_uint16 port = Config_GetInt16ByKey(cfg, CFG_SERVERPORT_KEY);
	Thread_Create(AcceptThread, NULL, true);
	Bind(ip, port);
	Event_Call(EVT_POSTSTART, NULL);
	Thread_Create(ConsoleThread, NULL, true);
}

void Server_DoStep(cs_int32 delta) {
	Event_Call(EVT_ONTICK, &delta);
	Timer_Update(delta);
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) Client_Tick(client, delta);
	}
}

void Server_StartLoop(void) {
	cs_uint64 last, curr = Time_GetMSec();
	cs_int32 delta;

	while(Server_Active) {
		last = curr;
		curr = Time_GetMSec();
		delta = (cs_int32)(curr - last);
		if(delta < 0) {
			Log_Warn(Lang_Get(LANG_SVDELTALT0));
			delta = 0;
		}
		if(delta > 500) {
			Log_Warn(Lang_Get(LANG_SVLONGTICK), delta);
			delta = 500;
		}
		Server_DoStep(delta);
		Sleep(10);
	}
}

void Server_Stop(void) {
	Event_Call(EVT_ONSTOP, NULL);
	Log_Info(Lang_Get(LANG_SVSTOP0));
	Clients_KickAll(Lang_Get(LANG_KICKSVSTOP));
	Log_Info(Lang_Get(LANG_SVSTOP1));
	Worlds_SaveAll(true, true);

	Socket_Close(Server_Socket);
	Log_Info(Lang_Get(LANG_SVSAVING), MAINCFG);
	Config_Save(Server_Config);
	Config_DestroyStore(Server_Config);

	Log_Info(Lang_Get(LANG_SVSTOP2));
	Plugin_Stop();
}