#pragma once

#include <mutex>

#include "common.h"
#include "thread.h"
#include "singleton.h"
#include <map>

class Packet;
class ThreadObject;

//继承singleton，作为全局单例
class ThreadMgr :public Singleton<ThreadMgr>
{
public:
    ThreadMgr();
    void StartAllThread();
    bool IsGameLoop();
    void NewThread();
    void AddObjToThread(ThreadObject* obj);

    // message
    void AddPacket(Packet* pPacket);

    void Dispose();

private:
    uint64 _lastThreadSn{ 0 }; // 维护上一次加入对象的线程，使新对象均匀分摊到下一个线程

    std::mutex _thread_lock;
    std::map<uint64, Thread*> _threads;
};

