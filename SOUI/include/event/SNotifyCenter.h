﻿#pragma once

#include <core/SSingleton.h>
#include <helper/SCriticalSection.h>

#if _MSC_VER >= 1700 // VS2012
#define ENABLE_RUNONUI
#endif
#ifdef ENABLE_RUNONUI
#include <functional>
// 将 闭包 传递到了 UI线程
// 所以 这里 尽量 将 相同类型的 处理 放到一起 执行  而不是分开调用。

// SendMessage [&] 中的 & 是指 fn里调用的变量 都是 引用拷贝的
#define SRUNONUISYNC(fn) SNotifyCenter::getSingletonPtr()->RunOnUISync([&]() { fn })

// PostMessage [=] 中的 等号 是指 fn里调用的变量 都是 值拷贝的
#define SRUNONUI(fn) SNotifyCenter::getSingletonPtr()->RunOnUIAsync([=]() { fn })

#endif

SNSBEGIN

template <class T>
class TAutoEventMapReg {
    typedef TAutoEventMapReg<T> _thisClass;

  public:
    TAutoEventMapReg()
    {
        registerNotifyCenter();
    }

    ~TAutoEventMapReg()
    {
        unregisterNotifyCenter();
    }

    void registerNotifyCenter()
    {
        SNotifyCenter::getSingleton().RegisterEventMap(Subscriber(&_thisClass::OnEvent, this));
    }
    void unregisterNotifyCenter()
    {
        SNotifyCenter::getSingleton().UnregisterEventMap(Subscriber(&_thisClass::OnEvent, this));
    }

  protected:
    BOOL OnEvent(IEvtArgs *e)
    {
        T *pThis = static_cast<T *>(this);
        return pThis->_HandleEvent(e);
    }
};

struct INotifyCallback
{
    virtual void OnFireEvent(IEvtArgs *e) = 0;
    virtual void OnFireEvts() = 0;
};

class SNotifyReceiver;

class SOUI_EXP SNotifyCenter
    : public SSingleton<SNotifyCenter>
    , public SEventSet
    , protected INotifyCallback {
  public:
    SNotifyCenter(int nIntervel = 20);
    ~SNotifyCenter(void);

    /**
     * FireEventSync
     * @brief    触发一个同步通知事件
     * @param    EventArgs *e -- 事件对象
     * @return
     *
     * Describe  只能在UI线程中调用
     */
    void FireEventSync(IEvtArgs *e);

    /**
     * FireEventAsync
     * @brief    触发一个异步通知事件
     * @param    EventArgs *e -- 事件对象
     * @return
     *
     * Describe  可以在非UI线程中调用，EventArgs
     * *e必须是从堆上分配的内存，调用后使用Release释放引用计数
     */
    void FireEventAsync(IEvtArgs *e);

    /**
     * RegisterEventMap
     * @brief    注册一个处理通知的对象
     * @param    const ISlotFunctor &slot -- 事件处理对象
     * @return
     *
     * Describe
     */
    bool RegisterEventMap(const IEvtSlot &slot);

    /**
     * RegisterEventMap
     * @brief    注销一个处理通知的对象
     * @param    const ISlotFunctor &slot -- 事件处理对象
     * @return
     *
     * Describe
     */
    bool UnregisterEventMap(const IEvtSlot &slot);

  protected:
    virtual void OnFireEvent(IEvtArgs *e);
    virtual void OnFireEvts();

    DWORD m_dwMainTrdID; //主线程ID

    SList<IEvtSlot *> m_evtHandlerMap;

    SNotifyReceiver *m_pReceiver;

    SCriticalSection m_cs;
    SList<IEvtArgs *> m_ayncEvent;
    BOOL m_bRunning;
    int m_nInterval;
#ifdef ENABLE_RUNONUI
    SList<std::function<void(void)> *> m_asyncFuns;

  public:
    void RunOnUISync(std::function<void(void)> fn);
    void RunOnUIAsync(std::function<void(void)> fn);
#endif
};

SNSEND