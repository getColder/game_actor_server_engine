#include "thread_mgr.h"
#include "common.h"
#include "packet.h"

#include <iostream>

ThreadMgr::ThreadMgr()
{
}

void ThreadMgr::StartAllThread()
{
    auto iter = _threads.begin();
    while (iter != _threads.end())
    {
        iter->second->Start();
        ++iter;
    }
}

bool ThreadMgr::IsGameLoop()
{
    /* 所有线程不活跃即游戏循环结束 */
    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        if (iter->second->IsRun())
            return true;
    }

    return false;
}

void ThreadMgr::NewThread()
{
    
    std::lock_guard<std::mutex> guard(_thread_lock);   //加锁，互斥访问_threads列表
    auto pThread = new Thread();
    _threads.insert(std::make_pair(pThread->GetSN(), pThread));
}

/* 找到合适线程，加入线程调用对象 */
void ThreadMgr::AddObjToThread(ThreadObject* obj)
{
    std::lock_guard<std::mutex> guard(_thread_lock);

    /* 在加入之前初始化一下  */
    if (!obj->Init())
    {
        std::cout << "AddThreadObj Failed. ThreadObject init failed." << std::endl;
        return;
    }

    /* 找到上一次的线程  */
    auto iter = _threads.begin();
    if (_lastThreadSn > 0)
    {
        iter = _threads.find(_lastThreadSn);
    }

    if (iter == _threads.end())
    {
        std::cout << "AddThreadObj Failed. no thead." << std::endl;
        return;
    }

    /* 取到它的下一个活动线程 */
    do
    {
        ++iter;
        if (iter == _threads.end())
            iter = _threads.begin();
    } while (!(iter->second->IsRun()));

    auto pThread = iter->second;
    pThread->AddThreadObj(obj);         //加入线程调用对象
    _lastThreadSn = pThread->GetSN();   //更新最近使用线程的SN码为添加线程
    //std::cout << "add obj to thread.id:" << pThread->GetSN() << std::endl;
}

void ThreadMgr::Dispose()
{
    /*
    托管对象销毁前处理函数实现：
        * 1.调用存在对象的dispose
        * 2.释放thread对象动态空间
        * 3.遍历_threads
    */
    auto iter = _threads.begin();
    while (iter != _threads.end())
    {
        Thread* pThread = iter->second;
        pThread->Dispose();
        delete pThread;
        ++iter;
    }
}

void ThreadMgr::AddPacket(Packet* pPacket)
{
    /* Actor模型 - 分发机制 ，将packet消息广播到各个线程 */
    std::lock_guard<std::mutex> guard(_thread_lock);    //_threads互斥
    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        Thread* pThread = iter->second;
        pThread->AddPacket(pPacket);
    }
}