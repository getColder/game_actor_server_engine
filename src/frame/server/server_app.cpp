#include "common.h"
#include "server_app.h"
#include "network_listen.h"


ServerApp::ServerApp(APP_TYPE  appType)
{
    /*初始化APP
    * 1.标定APP类型
    * 2.确保全局单例Global和ThreadMgr已初始化
    * 3.同步到全局时间
    * 4.分配线程数
    */
    _appType = appType;

    Global::Instance();
    ThreadMgr::Instance();
    _pThreadMgr = ThreadMgr::GetInstance();
    UpdateTime();

    // 创建线程
    for (int i = 0; i < 3; i++)
    {
        _pThreadMgr->NewThread();
    }
}

ServerApp::~ServerApp()
{
    _pThreadMgr->DestroyInstance();
}

void ServerApp::Dispose()
{
    _pThreadMgr->Dispose();
}

void ServerApp::StartAllThread() const
{
    _pThreadMgr->StartAllThread();
}

/* 更新时间以及判断游戏服务器循环 */
void ServerApp::Run() const
{
    bool isRun = true;
    while (isRun)
    {
        UpdateTime();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        isRun = _pThreadMgr->IsGameLoop(); //线程全部终止代表服务器运行结束
    }
}

/* 更新全局时间 */
void ServerApp::UpdateTime() const
{
    auto timeValue = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    Global::GetInstance()->TimeTick = timeValue.time_since_epoch().count(); //更新服务器全局时间

    auto tt = std::chrono::system_clock::to_time_t(timeValue);
    struct tm tm;
    localtime_s(&tm, &tt);  //通过纪元时间获取结构体 tm
    Global::GetInstance()->YearDay = tm.tm_yday;
}

/* 构建监听服务器对象，加入线程并由线程托管 */
bool ServerApp::AddListenerToThread(const std::string ip, const int port) const
{
    NetworkListen* pListener = new NetworkListen();
    if (!pListener->Listen(ip, port))
    {
        //监听失败，在主线程销毁
        delete pListener;
        return false;
    }

    _pThreadMgr->AddObjToThread(pListener); //加入线程
    return true;
}
