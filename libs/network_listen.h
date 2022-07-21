#pragma once

#include "network.h"

/* 监听器，用于服务器。为每一个socket维护一个connectObj */
class NetworkListen :public Network
{
public:
	bool Init( ) override;
	void RegisterMsgFunction( ) override;
	bool Listen(std::string ip, int port);
	void Update( ) override;

protected:
	virtual int Accept();
};
