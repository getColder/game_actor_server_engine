#pragma once

#include <stdexcept>

template <typename T>
class Singleton {
public:
    template<typename... Args>
    static T* Instance( Args&&... args ) {
        /* 首次使用初始化 */
        if ( m_pInstance == nullptr )
            m_pInstance = new T( std::forward<Args>( args )... );   //完美转发： 还不太懂用处... (传左值处理左值，传右值处理右值)

        return m_pInstance;

    }

    static T* GetInstance( ) {
        if ( m_pInstance == nullptr )
            throw std::logic_error( "the instance is not init, please initialize the instance first" );

        return m_pInstance;
    }

    static void DestroyInstance( ) {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

private:
    static T* m_pInstance;
};

template <class T> T*  Singleton<T>::m_pInstance = nullptr;