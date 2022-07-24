#include <iostream>

#include "connect_obj.h"

#include "network.h"
#include "network_buffer.h"

#include "packet.h"
#include "thread_mgr.h"

ConnectObj::ConnectObj(Network* pNetWork, SOCKET socket) :_pNetWork(pNetWork), _socket(socket)
{
    _recvBuffer = new RecvNetworkBuffer(DEFAULT_RECV_BUFFER_SIZE, this);
    _sendBuffer = new SendNetworkBuffer(DEFAULT_SEND_BUFFER_SIZE, this);
}

ConnectObj::~ConnectObj()
{
    if (_recvBuffer != nullptr)
        delete _recvBuffer;

    if (_sendBuffer != nullptr)
        delete _sendBuffer;
}

void ConnectObj::Dispose()
{
    //std::cout << "close socket:" << _socket << std::endl;
    _sock_close(_socket);

    _recvBuffer->Dispose();
    _sendBuffer->Dispose();
}

bool ConnectObj::HasRecvData() const
{
    return _recvBuffer->HasData();
}

Packet* ConnectObj::GetRecvPacket() const
{
    return _recvBuffer->GetPacket();
}

bool ConnectObj::Recv() const
{
    bool isRs = false;
    char *pBuffer = nullptr;
    while (true)
    {
        /* 总空间数据不足一个头的大小，扩容 */
        if (_recvBuffer->GetEmptySize() < (sizeof(PacketHead) + sizeof(TotalSizeType)))
        {
            _recvBuffer->ReAllocBuffer();
        }

        const int emptySize = _recvBuffer->GetBuffer(pBuffer);
        const int dataSize = ::recv(_socket, pBuffer, emptySize, 0);
        if (dataSize > 0)
        {
            //std::cout << "recv size:" << size << std::endl;
            _recvBuffer->FillDate(dataSize);   //读入数据，移动end指针
        }
        else if (dataSize == 0)
        {
            //std::cout << "recv size:" << dataSize << " error:" << _sock_err() << std::endl;
            break;
        }
        else
        {
            const auto socketError = _sock_err();
            if (socketError == EINTR || socketError == EWOULDBLOCK || socketError == EAGAIN)
            {
                isRs = true;
            }
            //std::cout << "recv size:" << dataSize << " error:" << socketError << std::endl;
            break;
        }
    }

    if (isRs)
    {
        while (true) {
            const auto pPacket = _recvBuffer->GetPacket();
            if (pPacket == nullptr)
                break;

            ThreadMgr::GetInstance()->AddPacket(pPacket);
        }
    }

    return isRs;
}

bool ConnectObj::HasSendData() const
{
    return _sendBuffer->HasData();
}

/* 协议数据加入用户发送缓冲区 */
void ConnectObj::SendPacket(Packet* pPacket) const
{
    _sendBuffer->AddPacket(pPacket); //头部head + 协议版本msgid + 数据data，并拷贝到buffer
}

bool ConnectObj::Send() const
{
    while (true) {
        char *pBuffer = nullptr;
        const int needSendSize = _sendBuffer->GetBuffer(pBuffer);

        // 没有数据可发送
        if (needSendSize <= 0)
        {
            return true;
        }

        const int size = ::send(_socket, pBuffer, needSendSize, 0);
        /* 发送成功，将数据从缓冲区种移除 */
        if (size > 0)
        {
            //std::cout << "send size:" << size << std::endl;
            _sendBuffer->removedata(size);

            // 下一帧再发送
            if (size < needSendSize)
            {
                return true;
            }
        }

        if (size == -1)
        {
            const auto socketError = _sock_err();
            std::cout << "needSendSize:" << needSendSize << " error:" << socketError << std::endl;
            return false;
        }
    }
}

