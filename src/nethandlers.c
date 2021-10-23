#include "core.h"
#include "protocol.h"
#include "nethandlers.h"

static cs_bool PlayerIdent_Reader(Client *client, cs_byte id, cs_bool iscpe, void *data) {

}

static void PlayerIdent_Handler(cs_char *data, cs_int16 size, PacketHandler ph) {
	
}

static void NetHandlers_RegisterAll(void) {
	/*
		Client -> Server
	*/
	PacketInfo *pi = Protocol_GetInfo(0x00);
	pi->preader = PlayerIdent_Reader;
	pi->phandler = PlayerIdent_Handler;
	pi->size = 131;
}
