#include "packet.h"
#include "network_buffer.h"
#include "connect_obj.h"

#include <cstdlib>
#include <cstring>

NetworkBuffer::NetworkBuffer(const unsigned size, ConnectObj* pConnectObj)
{
    _pConnectObj = pConnectObj;
    _bufferSize = size;
    _beginIndex = 0;
    _endIndex = 0;
    _dataSize = 0;
    _buffer = new char[_bufferSize];
}

NetworkBuffer::~NetworkBuffer()
{
    if (_buffer != nullptr)
        delete[] _buffer;
}

bool NetworkBuffer::HasData() const
{
    if (_dataSize <= 0)
        return false;

    // 至少要有一个协议头
    if (_dataSize < sizeof(PacketHead))
        return false;

    return true;
}

unsigned int NetworkBuffer::GetEmptySize()
{
    return _bufferSize - _dataSize;
}

unsigned int NetworkBuffer::GetWriteSize() const
{
    /* 剩余缓冲区大小 */
    if (_beginIndex <= _endIndex)
    {
        return _bufferSize - _endIndex;
    }
    else
    {
        return _beginIndex - _endIndex;
    }
}

unsigned int NetworkBuffer::GetReadSize() const
{
    if (_dataSize <= 0)
        return 0;
    /* 可用缓冲区 */
    if (_beginIndex < _endIndex)
    {
        return _endIndex - _beginIndex;
    }
    else
    {
        return _bufferSize - _beginIndex;
    }
}

void NetworkBuffer::FillDate(unsigned int  size)
{
    _dataSize += size; //填充后，改变有效字节数

      /* 填充后更新end指针 */
    if ((_bufferSize - _endIndex) <= size)
    {
        size -= _bufferSize - _endIndex;
        _endIndex = 0;
    }

    _endIndex += size;
}

void NetworkBuffer::removedata(unsigned int size)
{
    _dataSize -= size;  //移除后，改变有效字节数
    /* 移除后更新begin指针 */
    if ((_beginIndex + size) >= _bufferSize)
    {   
        /* 环形    [-E..B----] */
        size -= _bufferSize - _beginIndex;  //先尾部 Begin到边界， size减去一部分
        _beginIndex = 0;
    }
    //再根据剩余的size 移动begin
    _beginIndex += size;
}

void NetworkBuffer::ReAllocBuffer()
{
    /* 根据所持数据分配 */
    Buffer::ReAllocBuffer(_dataSize);
}


///////////////////////////////////////////////////////////////////////////////////////////////

RecvNetworkBuffer::RecvNetworkBuffer(const unsigned int size, ConnectObj* pConnectObj) : NetworkBuffer(size, pConnectObj) {

}

void RecvNetworkBuffer::Dispose() {

}

int RecvNetworkBuffer::GetBuffer(char*& pBuffer) const
{
    pBuffer = _buffer + _endIndex;
    return GetWriteSize();
}

Packet* RecvNetworkBuffer::GetPacket()
{
    /* 数据长度不够 */
    if (_dataSize < sizeof(TotalSizeType))
    {
        return nullptr;
    }

    /* 1.读出 整体长度 */
    unsigned short totalSize;
    MemcpyFromBuffer(reinterpret_cast<char*>(&totalSize), sizeof(TotalSizeType));

    /* 协议体长度不够，等待 */
    if (_dataSize < totalSize)
    {
        return nullptr;
    }

    removedata(sizeof(TotalSizeType));

    /* 2.读出 PacketHead 头部  */
    PacketHead head;
    MemcpyFromBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHead));
    removedata(sizeof(PacketHead));

    /* 3.读出 packet数据（由protobuf构成协议） */
    const auto socket = _pConnectObj->GetSocket();
    /* 4.新建一个packet，分配空间，并绑定socket */
    Packet* pPacket = new Packet(head.MsgId, socket);
    const auto dataLength = totalSize - sizeof(PacketHead) - sizeof(TotalSizeType);
    while (pPacket->GetTotalSize() < dataLength)
    {
        pPacket->ReAllocBuffer();
    }
    /* 5.拷贝协议数据  */
    MemcpyFromBuffer(pPacket->GetBuffer(), dataLength);
    /* 6.更新buffer有效下标（endIndex） */
    pPacket->FillData(dataLength);
    /* 拷贝成功，从接收缓冲区移除数据 */
    removedata(dataLength);

    return pPacket;
}

void RecvNetworkBuffer::MemcpyFromBuffer(char* pVoid, const unsigned int size)
{
    const auto readSize = GetReadSize();
    if (readSize < size)
    {
        // 1.copy尾部数据
        ::memcpy(pVoid, _buffer + _beginIndex, readSize);

        // 2.copy头部数据
        ::memcpy(pVoid + readSize, _buffer, size - readSize);
    }
    else
    {
        ::memcpy(pVoid, _buffer + _beginIndex, size);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////

SendNetworkBuffer::SendNetworkBuffer(const unsigned int size, ConnectObj* pConnectObj) : NetworkBuffer(size, pConnectObj)
{

}

void SendNetworkBuffer::Dispose()
{

}

int SendNetworkBuffer::GetBuffer(char*& pBuffer) const
{
    if (_dataSize <= 0)
    {
        pBuffer = nullptr;
        return 0;
    }

    if (_beginIndex < _endIndex)
    {
        pBuffer = _buffer + _beginIndex;
        return _endIndex - _beginIndex;
    }
    else
    {
        pBuffer = _buffer + _beginIndex;
        return _bufferSize - _beginIndex;
    }
}

/* 添加总长度，头部信息，数据部分 */
void SendNetworkBuffer::AddPacket(Packet* pPacket)
{
    const auto dataLength = pPacket->GetDataLength();
    /* 发送缓冲区，每一个tcp包： 总长度 + msgid + 数据 */
    TotalSizeType totalSize = dataLength + sizeof(PacketHead) + sizeof(TotalSizeType);

    // 长度不够，扩容
    while (GetEmptySize() < totalSize) {
        ReAllocBuffer();
    }

    //std::cout << "send buffer::Realloc. _bufferSize:" << _bufferSize << std::endl;

    // 1.整体长度
    MemcpyToBuffer(reinterpret_cast<char*>(&totalSize), sizeof(TotalSizeType)); //逐字节放入buffer

    // 2.头部
    PacketHead head;
    head.MsgId = pPacket->GetMsgId();
    MemcpyToBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHead));

    // 3.数据
    MemcpyToBuffer(pPacket->GetBuffer(), pPacket->GetDataLength());
}

void SendNetworkBuffer::MemcpyToBuffer(char* pVoid, const unsigned int size)
{
    const auto writeSize = GetWriteSize();
    if (writeSize < size)
    {
        // 1.copy到尾部
        ::memcpy(_buffer + _endIndex, pVoid, writeSize);

        // 2.copy到头部
        ::memcpy(_buffer, pVoid + writeSize, size - writeSize);
    }
    else
    {
        ::memcpy(_buffer + _endIndex, pVoid, size);
    }

    FillDate(size);
}
