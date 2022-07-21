#pragma once
#include <mutex>
#include <map>
#include <list>
#include <functional>

class Packet;

/* acotr模型 - 消息邮箱，订阅相关消息执行回调函数 */
typedef
 std::function<void(Packet*)> HandleFunction;

class MessageList
{
public:
    void RegisterFunction(int msgId, HandleFunction function);  //根据msgID，处理packet
    bool IsFollowMsgId(int msgId);
    void ProcessPacket();
    void AddPacket(Packet* pPacket);

protected:
    std::mutex _msgMutex;
    std::list<Packet*> _msgList;    //消息列表
    std::map<int, HandleFunction> _callbackHandle;  //注册的回调函数表 ,键为msgid
};
