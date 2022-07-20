#pragma once

#include "base_buffer.h"
#include "common.h"

#pragma pack(push)
#pragma pack(4)

struct PacketHead {
    unsigned short MsgId;
};

#pragma pack(pop)

#if TestNetwork
#define DEFAULT_PACKET_BUFFER_SIZE	10
#else
// Ĭ�ϴ�С 10KB
#define DEFAULT_PACKET_BUFFER_SIZE	1024 * 10
#endif

class Packet : public Buffer {
public:
    //Packet();
    Packet(const int msgId, SOCKET socket);
    ~Packet();

    template<class ProtoClass>
    //解析protobuf到buffer
    ProtoClass ParseToProto()
    {
        ProtoClass proto;
        proto.ParsePartialFromArray(GetBuffer(), GetDataLength());
        return proto;
    }
    //从buffer序列化结构体为protobuf
    template<class ProtoClass>
    void SerializeToBuffer(ProtoClass& protoClase)
    {
        auto total = protoClase.ByteSizeLong();
        while (GetEmptySize() < total)
        {
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
