#pragma once

#include <thread>
#include <list>

#include "thread_obj.h"
#include "sn_object.h"

class Packet;

/* 继承自全局SNObj，可以为对象产生唯一的SN码 */
class Thread : public SnObject, IDisposable {
public:
    Thread( );
    void AddThreadObj( ThreadObject* _obj );

    void Start( );
    void Stop( );
    void Update( );
    bool IsRun( ) const;
    void Dispose( ) override;
    
    void AddPacket(Packet* pPacket);

private:

    std::list<ThreadObject*> _objlist;  //线程调用对象列表
    std::list<ThreadObject*> _tmpObjs;  //列表拷贝，以减少加锁时间
    std::mutex _thread_lock;

    bool _isRun;
    std::thread _thread;
};

