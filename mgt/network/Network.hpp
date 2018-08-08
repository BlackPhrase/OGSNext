/// @file

#pragma once

#include "network/INetwork.hpp"

struct ISystem;
struct IConsole;

class CNetwork final : public INetwork
{
public:
	CNetwork();
	~CNetwork();
	
	bool Init(CreateInterfaceFn afnEngineFactory) override;
	void Shutdown() override;
	
	bool GetPacket(netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message) override;
	void SendPacket(netsrc_t sock, int length, void *data, netadr_t to) override;
	
	bool CompareAdr(netadr_t *a, netadr_t *b) const override;
	char *AdrToString(netadr_t *a) const override;
private:
	int UDP_OpenSocket(int port);
	void GetLocalAddress();
	
	ISystem *mpSystem{nullptr};
	IConsole *mpConsole{nullptr};
};