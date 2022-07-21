#pragma once
#include "disposable.h"
#include "thread_mgr.h"
#include "common.h"

//创建特定功能的server app，并启动相关线程，进入游戏循环
template<class APPClass>
inline int MainTemplate()
{
    APPClass* pApp = new APPClass();
    pApp->InitApp();
    pApp->StartAllThread();
    pApp->Run();
    pApp->Dispose();
    delete pApp;
    return 0;
}

//app抽象类，托管于资源回收类
class ServerApp : public IDisposable
{
public:
    ServerApp(APP_TYPE  appType);
    ~ServerApp();

    virtual void InitApp() = 0;
    void Dispose() override;

    void StartAllThread() const;
    void Run() const;
    void UpdateTime() const;

    bool AddListenerToThread(std::string ip, int port) const;

protected:
    ThreadMgr * _pThreadMgr;    //线程管理器，全局单例
    APP_TYPE _appType;          //APP类型标识符
};

