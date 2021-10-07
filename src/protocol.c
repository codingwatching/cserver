#include "core.h"
#include "client.h"
#include "protocol.h"

struct _PacketInfo {
	cs_int16 size, cpesize;
	cs_uint32 cpeextension;
	cs_int32 cpeextversion;
	PacketReader preader;
	PacketHandler handler;
} pinf[256];

void Protocol_Init(void) {
	for(cs_int32 i = 0; i < 256; i++) {
		pinf[i].size = -1;
		pinf[i].cpesize = -1;
		pinf[i].cpeextension = 0;
		pinf[i].preader = NULL;
		pinf[i].handler = NULL;
	}
}

INL cs_int16 GetPacketSizeFor(Client *client) {
	PacketManager *pm = &client->pm;
	struct _PacketInfo *pi = &pinf[pm->packetId];
	if(pi->cpeextension && Client_GetExtVer(client, pi->cpeextension) == pi->cpeextversion)
		return pi->cpesize;
	return pi->size;
}

EPMReturn PacketManager_Step(Client *client) {
	if(client->pm.state == PM_STATE_CLOSED) return PM_RETURN_SOCKETCLOSED;
	if(client->pm.state == PM_STATE_DATAOK) return PM_RETURN_OK;

	if(client->pm.state == PM_STATE_INITIAL) {
		if(Socket_Receive(client->sock, &client->pm.packetId, 1, 0) > 0) {
			if((client->pm.need = GetPacketSizeFor(client)) != -1) {
				if(client->pm.need > 0) {
					client->pm.state = PM_STATE_WAITDATA;
					client->pm.buffer = Memory_Alloc(client->pm.need, 1);
					client->pm.readed = 0;
				} else client->pm.state = PM_STATE_DATAOK;
			} else return PM_RETURN_INVALIDPACKET;
		} else {
			client->pm.state = PM_STATE_CLOSED;
			return PM_RETURN_SOCKETCLOSED;
		}
	}

	if(client->pm.state == PM_STATE_WAITDATA) {
		cs_int16 rem = client->pm.need - client->pm.readed;
		cs_int16 rd = Socket_Receive(client->sock, client->pm.buffer + client->pm.readed, rem, 0);

		if(rd > 0) {

		}
	}
}
