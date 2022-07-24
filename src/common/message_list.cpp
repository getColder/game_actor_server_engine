#include "message_list.h"
#include <iterator>

#include "packet.h"

/* 注册关心msgid的的回调函数，其参数为packet* */
void MessageList::RegisterFunction(int msgId, HandleFunction function)
{
    std::lock_guard<std::mutex> guard(_msgMutex);
    _callbackHandle[msgId] = function;
}

/* 查看函数表回调函数注册情况，是否存在键msgID */
bool MessageList::IsFollowMsgId(int msgId)
{
    std::lock_guard<std::mutex> guard(_msgMutex);
    return _callbackHandle.find(msgId) != _callbackHandle.end();
}

/* 处理packet */
void MessageList::ProcessPacket()
{
    std::list<Packet*> tmpList;
    _msgMutex.lock();   //消息列表msgList互斥
    std::copy(_msgList.begin(), _msgList.end(), std::back_inserter(tmpList));   //拷贝并清空，减少加锁时间
    _msgList.clear();
    _msgMutex.unlock();

    for (auto packet : tmpList)
    {
        const auto handleIter = _callbackHandle.find(packet->GetMsgId());
        if (handleIter == _callbackHandle.end())
        {
            std::cout << "packet is not hander. msg id;" << packet->GetMsgId() << std::endl;
        }
        else
        {
            handleIter->second(packet);  //执行回调
        }
    }

    tmpList.clear();
}

/* 将packet加入消息列表 */
void MessageList::AddPacket(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_msgMutex);
    _msgList.push_back(pPacket);
}
