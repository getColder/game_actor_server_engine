#pragma once

#include "disposable.h"
#include "message_list.h"

/* 
* 线程调用对象 - 抽象基类
* actor模型 - 每个线程调用对象继承自MessageList，方便订阅关心消息 
* 派生类重写函数:
    1.初始化
    2.消息注册函数
    3.update帧函数
* 继承邮箱的protected成员：
    1.msglist 存放packet的消息列表邮箱
    2._callbackHandle 回调函数map表
    3.void addPacket(*packet) 接收关心的packet  (由thread分发)  
    4.void processPacket() 处理邮箱中的所有列表 

*/
class ThreadObject : public IDisposable, public MessageList
{
public:
    virtual bool Init() = 0;        
    virtual void RegisterMsgFunction() = 0;   
    virtual void Update() = 0;

    bool IsActive() const;
    virtual void Dispose() override;
protected:
    bool _active{ true };
};
