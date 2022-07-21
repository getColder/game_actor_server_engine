#pragma once

#include "base_buffer.h"
#include "common.h"

#pragma pack(push)
#pragma pack(4)

struct PacketHead {
    unsigned short MsgId;
};

#pragma pack(pop)


#define DEFAULT_PACKET_BUFFER_SIZE	1024 * 10

/* 继承于基类buffer,拥有环形缓冲区并支持扩容 */
class Packet : public Buffer {
public:
    Packet(const int msgId, SOCKET socket);     //Packet()构造，需要msgID与socket;
    ~Packet();

/* 从packet缓冲区读入数据,解析构造为协议对象，*/
    template<class ProtoClass>
    ProtoClass ParseToProto()
    {
        ProtoClass proto;
        proto.ParsePartialFromArray(GetBuffer(), GetDataLength());
        return proto;
    }
    /* 序列化给定协议，存入packet缓冲区*/
    template<class ProtoClass>
    void SerializeToBuffer(ProtoClass& protoClase)
    {
        auto total = protoClase.ByteSizeLong();
        while (GetEmptySize() < total)
        {
            /* 剩余缓冲区数据是否够用？ */
            ReAllocBuffer();
        }

        protoClase.SerializePartialToArray(GetBuffer(), total);
        FillData(total);
    }

    void Dispose() override;
    void CleanBuffer();

    char* GetBuffer() const;
    unsigned short GetDataLength() const;
    int GetMsgId() const;
    void FillData(unsigned int size);
    void ReAllocBuffer();
    SOCKET GetSocket() const;

private:
    int _msgId;
    SOCKET _socket;
};
