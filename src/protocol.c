#include "core.h"
#include "client.h"
#include "protocol.h"
#include "nethandlers.h"

PacketInfo pinf[256];

void Protocol_Init(void) {
	for(cs_int32 i = 0; i < 256; i++) {
		pinf[i].size = -1;
		pinf[i].cpesize = -1;
		pinf[i].cpeextension = 0;
		pinf[i].preader = NULL;
		pinf[i].phandler = NULL;
	}

	NetHandlers_RegisterAll();
}

PacketInfo *Protocol_GetInfo(cs_byte id) {
	return &pinf[id];
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
	struct _PacketInfo *cpi = &pinf[client->pm.packetId];

	if(client->pm.state == PM_STATE_INITIAL) {
		if(Socket_Receive(client->sock, &client->pm.packetId, 1, 0) > 0) {
			cpi = &pinf[client->pm.packetId];

			if(cpi->cpeextension && Client_GetExtVer(client, cpi->cpeextension) == cpi->cpeextversion)
				client->pm.need = cpi->cpesize;
			else
				client->pm.need = cpi->size;
			
			if(client->pm.need != -1) {
				if(client->pm.need) {
					client->pm.state = PM_STATE_WAITDATA;
					client->pm.readed = 0;
					if(!client->pm.buffer.data) {
						client->pm.buffer.size = client->pm.need;
						client->pm.buffer.data = Memory_Alloc(client->pm.need, 1);
					} else if(client->pm.buffer.size < client->pm.need) {
						client->pm.buffer.size = client->pm.need;
						client->pm.buffer.data = Memory_Realloc(client->pm.buffer.data, client->pm.need);
					}
				} else client->pm.state = PM_STATE_DATAOK;
			} else return PM_RETURN_INVALIDPACKET;
		} else {
			client->pm.state = PM_STATE_CLOSED;
			return PM_RETURN_SOCKETCLOSED;
		}
	}

	if(client->pm.state == PM_STATE_WAITDATA) {
		cs_int16 rem = client->pm.need - client->pm.readed;
		cs_int16 rd = Socket_Receive(client->sock, client->pm.buffer.data + client->pm.readed, rem, 0);

		if(rd > 0) {
			client->pm.readed += rd;
			if(client->pm.need == client->pm.readed)
				client->pm.state = PM_STATE_DATAOK;
		}
	}

	if(client->pm.state == PM_STATE_DATAOK) {
		if(cpi->preader(client->pm.buffer.data, client->pm.readed, cpi->phandler)) {
			client->pm.state = PM_STATE_INITIAL;
			client->pm.packetId = 0xFF;
			client->pm.readed = 0;
			client->pm.need = 0;
			return PM_RETURN_OK;
		} else {
			client->pm.state = PM_STATE_CLOSED;
			return PM_RETURN_HANDLINGFAILED;
		}
	}

	return PM_RETURN_FAIL;
}
