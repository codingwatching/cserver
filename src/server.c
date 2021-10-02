#include "core.h"
#include "str.h"
#include "log.h"
#include "cserror.h"
#include "server.h"
#include "client.h"
#include "protocol.h"
#include "config.h"
#include "generators.h"
#include "heartbeat.h"
#include "platform.h"
#include "event.h"
#include "plugin.h"
#include "timer.h"
#include "consoleio.h"
#include "strstor.h"

CStore *Server_Config = NULL;
cs_str Server_Version = GIT_COMMIT_TAG;
cs_bool Server_Active = false, Server_Ready = false;
cs_uint64 Server_StartTime = 0, Server_LatestBadTick = 0;
Socket Server_Socket = 0;

INL static ClientID TryToGetIDFor(Client *client) {
	cs_int8 maxConnPerIP = Config_GetInt8ByKey(Server_Config, CFG_CONN_KEY),
	sameAddrCount = 1;
	ClientID possibleId = CLIENT_SELF;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other && possibleId == CLIENT_SELF) possibleId = i;
		if(other && other->addr == client->addr)
			++sameAddrCount;
		else continue;

		if(sameAddrCount > maxConnPerIP) {
			Client_Kick(client, Sstor_Get("KICK_MANYCONN"));
			return CLIENT_SELF;
		}
	}

	return possibleId;
}

INL static void ProcessClient(Socket sock, cs_ulong addr) {
	Client *client = Memory_TryAlloc(1, sizeof(Client));
	if(client) {
		client->addr = addr;
		client->id = TryToGetIDFor(client);
		if(client->id != CLIENT_SELF) {
			Client_Kick(client, Sstor_Get("KICK_FULL"));
			return;
		}
	} else Socket_Close(sock);
}

THREAD_FUNC(NetThread) {
	(void)param;

	struct sockaddr_in claddr;
	while(Server_Active) {
		Socket sock = Socket_Accept(Server_Socket, &claddr);
		if(sock != INVALID_SOCKET) ProcessClient(sock, claddr.sin_addr.s_addr);
		Thread_Sleep(32);
	}

	return 0;
}

INL static cs_bool Bind(cs_str ip, cs_uint16 port) {
	Server_Socket = Socket_New();
	if(!Server_Socket) {
		Error_PrintSys(true);
	}
	struct sockaddr_in ssa;
	
	if(Socket_SetAddr(&ssa, ip, port) > 0 && Socket_Bind(Server_Socket, &ssa) && Socket_NonBlocking(Server_Socket, true)) {
		Log_Info(Sstor_Get("SV_START"), ip, port);
		return true;
	}

	return false;
}

cs_bool Server_Init(void) {
	if(!Generators_Init() || !Sstor_Defaults()) return false;

	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Active = true;
	Server_Config = cfg;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, Sstor_Get("CFG_SVIP_COMM"));
	Config_SetDefaultStr(ent, Sstor_Get("CFG_SVIP_DVAL"));

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CONFIG_TYPE_INT16);
	Config_SetComment(ent, Sstor_Get("CFG_SVPORT_COMM"));
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt16(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, Sstor_Get("CFG_SVNAME_COMM"));
	Config_SetDefaultStr(ent, Sstor_Get("CFG_SVNAME_DVAL"));

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CONFIG_TYPE_STR);
	Config_SetDefaultStr(ent, Sstor_Get("CFG_SVMOTD_DVAL"));

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, Sstor_Get("CFG_LOGLVL_COMM"));
	Config_SetDefaultStr(ent, Sstor_Get("CFG_LOGLVL_DVAL"));

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CONFIG_TYPE_BOOL);
	Config_SetComment(ent, Sstor_Get("CFG_LOP_COMM"));
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CONFIG_TYPE_INT8);
	Config_SetComment(ent, Sstor_Get("CFG_MAXPL_COMM"));
	Config_SetLimit(ent, 1, 127);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CONFIG_TYPE_INT8);
	Config_SetComment(ent, Sstor_Get("CFG_MAXCON_COMM"));
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_WORLDS_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, Sstor_Get("CFG_WORLDS_COMM"));
	Config_SetDefaultStr(ent, Sstor_Get("CFG_WORLDS_DVAL"));

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		cs_int32 line = 0;
		ECExtra extra = CONFIG_EXTRA_NOINFO;
		ECError code = Config_PopError(cfg, &extra, &line);
		if(extra == CONFIG_EXTRA_IO_LINEASERROR) {
			if(line != ENOENT) {
				Log_Error(Sstor_Get("SV_CFG_ERR2"), "open", MAINCFG, Config_ErrorToString(code), extra);
				return false;
			}
		} else {
			cs_str scode = Config_ErrorToString(code), 
			sextra = Config_ExtraToString(extra);
			if(line > 0)
				Log_Error(Sstor_Get("SV_CFGL_ERR"), line, MAINCFG, scode, sextra);
			else
				Log_Error(Sstor_Get("SV_CFG_ERR"), "parse", MAINCFG, scode, sextra);

			return false;
		}
	}
	Log_SetLevelStr(Config_GetStrByKey(cfg, CFG_LOGLEVEL_KEY));

	Broadcast = Memory_Alloc(1, sizeof(Client));
	Packet_RegisterDefault();
	Plugin_LoadAll();

	Directory_Ensure("worlds");
	cs_str worlds = Config_GetStrByKey(cfg, CFG_WORLDS_KEY);
	cs_uint32 wIndex = 0;
	if(*worlds == '*') {
		DirIter wIter;
		if(Iter_Init(&wIter, "worlds", "cws")) {
			do {
				if(wIter.isDir || !wIter.cfile) continue;
				World *tmp = World_Create(wIter.cfile);
				if(World_Load(tmp)) {
					Waitable_Wait(tmp->waitable);
					if(World_HasError(tmp)) {
						EWorldExtra extra = WORLD_EXTRA_NOINFO;
						EWorldError code = World_PopError(tmp, &extra);
						Log_Error(Sstor_Get("SV_WLOAD_ERR"), "load", World_GetName(tmp), code, extra);
						World_FreeBlockArray(tmp);
						World_Free(tmp);
					} else {
						World_Add(tmp);
						wIndex++;
					}
				} else World_Free(tmp);
			} while(Iter_Next(&wIter));
		}
		Iter_Close(&wIter);

		if(wIndex < 1) {
			cs_str wname = "world.cws";
			World *tmp = World_Create(wname);
			SVec defdims = {256, 256, 256};
			World_SetDimensions(tmp, &defdims);
			World_AllocBlockArray(tmp);
			if(!Generators_Use(tmp, "normal", NULL))
				Log_Error(Sstor_Get("WGEN_ERROR"), wname);
			AList_AddField(&World_Head, tmp);
		}
	} else {
		cs_bool skip_creating = false;
		cs_byte state = 0, pos = 0;
		cs_char buffer[64];
		SVec dims = {0, 0, 0};
		World *tmp = NULL;

		do {
			if(*worlds == ':' || *worlds == ',' || *worlds == '\0') {
				buffer[pos] = '\0';

				if(state == 0) {
					skip_creating = false;
					tmp = World_Create(buffer);
					if(World_Load(tmp)) {
						Waitable_Wait(tmp->waitable);
						if(World_HasError(tmp)) {
							EWorldExtra extra = WORLD_EXTRA_NOINFO;
							EWorldError code = World_PopError(tmp, &extra);
							if(code != WORLD_ERROR_IOFAIL && extra != WORLD_EXTRA_IO_OPEN) {
								skip_creating = true;
								Log_Error(Sstor_Get("SV_WLOAD_ERR"), "load", World_GetName(tmp), code, extra);
								World_FreeBlockArray(tmp);
								World_Free(tmp);
							}
						} else {
							if(World_IsReadyToPlay(tmp)) {
								AList_AddField(&World_Head, tmp);
								skip_creating = true;
								wIndex++;
							}
						}
					}
				} else if(!skip_creating && state == 1) {
					cs_char *del = buffer, *prev = buffer;
					for(cs_uint16 i = 0; i < 3; i++) {
						del = (cs_char *)String_FirstChar(del, 'x');
						if(del) *del++ = '\0'; else if(i != 2) break;
						((cs_uint16 *)&dims)[i] = (cs_uint16)String_ToInt(prev);
						prev = del;
					}
					if(tmp && dims.x > 0 && dims.y > 0 && dims.z > 0) {
						World_SetDimensions(tmp, &dims);
						World_AllocBlockArray(tmp);
						AList_AddField(&World_Head, tmp);
						wIndex++;
					} else {
						Log_Error(Sstor_Get("WGEN_INVDIM"), tmp->name);
						World_Free(tmp);
					}
				} else if(!skip_creating && state == 2) {
					GeneratorRoutine gr = Generators_Get(buffer);
					if(gr) {
						if(!gr(tmp, NULL))
							Log_Error(Sstor_Get("WGEN_ERROR"), tmp->name);
					} else
						Log_Error(Sstor_Get("WGEN_NOGEN"), tmp->name);
				}

				if(*worlds == ':')
					state++;
				else if(*worlds == '\0')
					state = 3;
				else
					state = 0;
				pos = 0;
			} else
				buffer[pos++] = *worlds;
			worlds++;
		} while(state < 3);
	}

	if(!World_Head) {
		Log_Error(Sstor_Get("SV_NOWORLDS"));
		return false;
	} else
		Log_Info(Sstor_Get("SV_WLDONE"), wIndex);

	cs_str ip = Config_GetStrByKey(cfg, CFG_SERVERIP_KEY);
	cs_uint16 port = Config_GetInt16ByKey(cfg, CFG_SERVERPORT_KEY);
	if(Bind(ip, port)) {
		Event_Call(EVT_POSTSTART, NULL);
		if(ConsoleIO_Init())
			Log_Info(Sstor_Get("SV_STOPNOTE"));
		Thread_Create(NetThread, NULL, true);
		Server_Ready = true;
		return true;
	}

	return false;
}

INL static void DoStep(cs_int32 delta) {
	Event_Call(EVT_ONTICK, &delta);
	Timer_Update(delta);
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) {
			Client_Tick(client, delta);
			if(client->closed && client->canBeFreed) {
				Clients_List[client->id] = NULL;
				Client_Free(client);
			}
		}
	}
}

void Server_StartLoop(void) {
	if(!Server_Active) return;
	cs_uint64 last, curr = Time_GetMSec();
	cs_int32 delta;

	while(Server_Active) {
		last = curr;
		curr = Time_GetMSec();
		delta = (cs_int32)(curr - last);
		if(curr < last) {
			Server_LatestBadTick = Time_GetMSec();
			Log_Warn(Sstor_Get("SV_BADTICK_BW"));
			delta = 0;
		} else if(delta > 500) {
			Server_LatestBadTick = Time_GetMSec();
			Log_Warn(Sstor_Get("SV_BADTICK"), delta);
			delta = 500;
		}
		DoStep(delta);
		Thread_Sleep(1000 / TICKS_PER_SECOND);
	}

	Event_Call(EVT_ONSTOP, NULL);
}

INL static void UnloadAllWorlds(void) {
	AListField *tmp, *prev = NULL;

	List_Iter(tmp, World_Head) {
		if(prev) AList_Remove(&World_Head, prev);
		World *world = (World *)tmp->value.ptr;
		if(World_Save(world, true)) {
			Waitable_Wait(world->waitable);
			if(World_HasError(world)) {
				EWorldExtra extra = WORLD_EXTRA_NOINFO;
				EWorldError code = World_PopError(world, &extra);
				Log_Error(Sstor_Get("SV_WLOAD_ERR"), "save", World_GetName(world), code, extra);
			}
		}
		World_Free(world);
		prev = tmp;
	}

	if(prev) AList_Remove(&World_Head, prev);
}

void Server_Cleanup(void) {
	Log_Info(Sstor_Get("SV_STOP_PL"));
	Clients_KickAll(Sstor_Get("KICK_STOP"));
	if(Broadcast) Memory_Free(Broadcast);
	Log_Info(Sstor_Get("SV_STOP_SW"));
	UnloadAllWorlds();
	Socket_Close(Server_Socket);
	Log_Info(Sstor_Get("SV_STOP_SC"));
	Config_Save(Server_Config);
	Config_DestroyStore(Server_Config);
	Log_Info(Sstor_Get("SV_STOP_UP"));
	Plugin_UnloadAll(true);
	Sstor_Cleanup();
}
