#pragma once
#include "network.h"

class ConnectObj;
class Packet;

/* 通常连接器用于客户端，只有维护一个connectObj */
class NetworkConnector : public Network
{
public:
	bool Init( ) override;
	void RegisterMsgFunction( ) override;
	
	virtual bool Connect(std::string ip, int port);
	void Update( ) override;

	bool HasRecvData();
	Packet* GetRecvPacket();
	void SendPacket(Packet* pPacket);

	bool IsConnected() const;

private:	
	ConnectObj* GetConnectObj();
	void TryCreateConnectObj();

protected:
	std::string _ip{ "" };
	int _port{ 0 };
};

