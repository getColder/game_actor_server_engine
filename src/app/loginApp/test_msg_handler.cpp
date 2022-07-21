#include "test_msg_handler.h"
#include "libserver/common.h"
#include "libserver/protobuf/proto_id.pb.h"
#include "libserver/protobuf/msg.pb.h"
#include "libserver/packet.h"

bool TestMsgHandler::Init()
{
    return true;
}
/* 注册protobuf的msgid为TestMSG的packet */
void TestMsgHandler::RegisterMsgFunction()
{
    RegisterFunction(Proto::MsgId::MI_TestMsg, BindFunP1(this, &TestMsgHandler::HandleMsg));    //成员函数转换为全局回调函数
}

/* 虚函数重写 ，登录服务器不需要更新帧函数*/
void TestMsgHandler::Update()
{
}

/* 模拟登录，打印用户信息 */
void TestMsgHandler::HandleMsg(Packet* pPacket)
{
    auto protoObj = pPacket->ParseToProto<Proto::TestMsg>(); //解析用户信息
    std::cout << protoObj.msg().c_str() << std::endl;
}
