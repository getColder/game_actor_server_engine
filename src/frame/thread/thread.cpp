#include "thread.h"
#include "global.h"
#include "packet.h"

#include <iterator>

Thread::Thread()
{
    this->_isRun = true;
    _tmpObjs.clear();
}

void Thread::Stop()
{
    if (!_isRun)
    {   
        _isRun = false;     //停止update
        if (_thread.joinable()) _thread.join();     //阻塞等待，直到线程运行结束
    }
}

/* 进入update循环  */
void Thread::Start()
{
    _isRun = true;
    _thread = std::thread([this]()
    {
        while (_isRun)
        {
            Update();
        }
    });
}

bool Thread::IsRun() const
{
    return _isRun;
}

/* 线程调用对象_objlist列表只作只读操作 -> 拷贝再操作减少加锁时间
 * 为每个对象执行两件事情
     1.处理packet
     2.执行update帧函数
 */
void Thread::Update()
{
    _thread_lock.lock();
    std::copy(_objlist.begin(), _objlist.end(), std::back_inserter(_tmpObjs));
    _thread_lock.unlock();

    for (ThreadObject* pTObj : _tmpObjs)
    {
        pTObj->ProcessPacket();   //actor模型 -> 处理分发email
        pTObj->Update();          //调用对象的update循环帧

        /* 当对象不再活跃，移除列表后处置并销毁 （互斥） */
        if (!pTObj->IsActive())
        {
            _thread_lock.lock();
            _objlist.remove(pTObj);
            _thread_lock.unlock();

            pTObj->Dispose();
            delete pTObj;
        }
    }

    _tmpObjs.clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));    //象征性休眠，进入阻塞队列让出CPU
}

/*actor模型 - 某线程调用对象需要注册自己关心的消息*/
void Thread::AddThreadObj(ThreadObject* obj)
{
    obj->RegisterMsgFunction();

    std::lock_guard<std::mutex> guard(_thread_lock);    //互斥
    _objlist.push_back(obj);
}

/* 释放该线程所有线程调用对象 */
void Thread::Dispose()
{
    std::list<ThreadObject*>::iterator iter = _objlist.begin();
    while (iter != _objlist.end())
    {
        (*iter)->Dispose();
        delete (*iter);
        iter = _objlist.erase(iter);
    }

    _objlist.clear();
}

/* actor模型 - 收到广播消息，发送packet给关心此消息的线程调用对象 */
void Thread::AddPacket(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_thread_lock);    //互斥
    for (auto iter = _objlist.begin(); iter != _objlist.end(); ++iter)
    {
        ThreadObject* pObj = *iter;
        /* protobuf维护字段MsgID */
        if (pObj->IsFollowMsgId(pPacket->GetMsgId()))
        {
            pObj->AddPacket(pPacket);   //将packet加入对象缓冲区
        }
    }
}
