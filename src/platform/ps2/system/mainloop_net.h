#pragma once

#include "types.h"

extern "C" {
#include "netplay_ee.h"
}

extern char *_MainLoop_NetConfigPaths[];

/* AURORA_SMB_NET_FROM_CNF_20260924: configure sm0 from the SMB.CNF network
   override lines. NULL or empty parameters keep the built-in static default
   (192.168.2.240/24, gw 192.168.2.1). */
Bool _MainLoopConfigureNetwork(const char *pPs2Ip, const char *pNetmask,
                               const char *pGateway);
Bool _MainLoopInitNetwork(Char **ppSearchPaths);
Bool _MainLoopWaitForNetwork(Int32 timeoutMs);
void *_MainLoopNetCallback(NetPlayCallbackE eCallback, char *data, int size);
int _MainLoopNetworkEvent(Uint32 Type, Uint32 Parm1, void *Parm2);
