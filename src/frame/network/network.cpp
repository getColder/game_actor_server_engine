#include "network.h"
#include "connect_obj.h"

#include <iostream>

void Network::Dispose()
{
    for (auto iter = _connects.begin(); iter != _connects.end(); ++iter)
    {
        auto pObj = iter->second;
        pObj->Dispose();
        delete pObj;
    }
    _connects.clear();

#ifdef EPOLL
    ::close(_epfd);
#endif

    //std::cout << "network dispose. close socket:" << _socket << std::endl;
    _sock_close(_masterSocket);
    _masterSocket = -1;
}


#define SetsockOptType const char *

void Network::SetSocketOpt(SOCKET socket)
{
    // 1.复用端口，TIMEW_WAIT后马上重新启用
    bool isReuseaddr = true;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (SetsockOptType)&isReuseaddr, sizeof(isReuseaddr));

    // 2.发送、接收timeout
    int netTimeout = 3000; // 1000 = 1秒
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));

    // 3.非阻塞
    _sock_nonblock(socket);
}

SOCKET Network::CreateSocket()
{
    _sock_init();
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
    {
        std::cout << "::socket failed. err:" << _sock_err() << std::endl;
        return socket;
    }

    SetSocketOpt(socket);
    return socket;
}

void Network::CreateConnectObj(SOCKET socket)
{
    ConnectObj* pConnectObj = new ConnectObj(this, socket);
    _connects.insert(std::make_pair(socket, pConnectObj));

#ifdef EPOLL
    AddEvent(_epfd, socket, EPOLLIN | EPOLLRDHUP);
#endif
}

#ifdef EPOLL

void Network::AddEvent(int epollfd, int fd, int flag)
{
    struct epoll_event ev;
    ev.events = flag;
    ev.data.ptr = nullptr;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void Network::ModifyEvent(int epollfd, int fd, int flag)
{
    struct epoll_event ev;
    ev.events = flag;
    ev.data.ptr = nullptr;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void Network::DeleteEvent(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
}

void Network::InitEpoll()
{
    _epfd = epoll_create(MAX_CLIENT);
    AddEvent(_epfd, _masterSocket, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
}

void Network::Epoll()
{
    _mainSocketEventIndex = -1;
    /* 有数据的写的fd，修改事件 */
    for (auto iter = _connects.begin(); iter != _connects.end(); ++iter)
    {
        ConnectObj* pObj = iter->second;
        if (pObj->HasSendData())
        {
            ModifyEvent(_epfd, iter->first, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
        }
    }
    /* wait后，遍历有事件发生的fd */
    int nfds = epoll_wait(_epfd, _events, MAX_EVENT, 50);
    for (int index = 0; index < nfds; index++)
    {
        int fd = _events[index].data.fd;

        if (fd == _masterSocket)
        {
            _mainSocketEventIndex = index;
        }

        auto iter = _connects.find(fd);
        if (iter == _connects.end())
        {
            continue;
        }
        /* 需要挂断socket的fd事件 */
        if (_events[index].events & EPOLLRDHUP || _events[index].events & EPOLLERR || _events[index].events & EPOLLHUP)
        {
            iter->second->Dispose();
            delete iter->second;
            iter = _connects.erase(iter);
            DeleteEvent(_epfd, fd);
            continue;
        }
        /* 读事件fd */
        if (_events[index].events & EPOLLIN)
        {
            if (!iter->second->Recv())
            {
                iter->second->Dispose();
                delete iter->second;
                iter = _connects.erase(iter);
                DeleteEvent(_epfd, fd);
                continue;
            }
        }
        /* 水平触发，写完移除fd写事件 */
        if (_events[index].events & EPOLLOUT)
        {
            iter->second->Send();
            ModifyEvent(_epfd, iter->first, EPOLLIN | EPOLLRDHUP);
        }
    }

}
#else

void Network::Select()
{
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    FD_SET(_masterSocket, &readfds);
    FD_SET(_masterSocket, &writefds);
    FD_SET(_masterSocket, &exceptfds);

    SOCKET fdmax = _masterSocket;
    ;
    for (auto iter = _connects.begin(); iter != _connects.end(); ++iter)
    {
        if (iter->first > fdmax)
            fdmax = iter->first;

        FD_SET(iter->first, &readfds);
        FD_SET(iter->first, &exceptfds);

        if (iter->second->HasSendData()) {
            FD_SET(iter->first, &writefds);
        }
        else
        {
            if (_masterSocket == iter->first)
                FD_CLR(_masterSocket, &writefds);
        }
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50 * 1000;
    const int nfds = ::select(fdmax + 1, &readfds, &writefds, &exceptfds, &timeout);
    if (nfds <= 0)
        return;

    auto iter = _connects.begin();
    while (iter != _connects.end())
    {
        if (FD_ISSET(iter->first, &exceptfds))
        {
            std::cout << "socket except!! socket:" << iter->first << std::endl;

            iter->second->Dispose();
            delete iter->second;
            iter = _connects.erase(iter);
            continue;
        }

        if (FD_ISSET(iter->first, &readfds))
        {
            if (!iter->second->Recv())
            {
                iter->second->Dispose();
                delete iter->second;
                iter = _connects.erase(iter);
                continue;
            }
        }

        if (FD_ISSET(iter->first, &writefds))
        {
            if (!iter->second->Send())
            {
                iter->second->Dispose();
                delete iter->second;
                iter = _connects.erase(iter);
                continue;
            }
        }

        ++iter;
    }
}

#endif
