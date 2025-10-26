#pragma once

namespace acc
{
template<typename T>
class ThreadWrapper
{
public:
    explicit ThreadWrapper(bool &terminateApp, T *pThreadArg) : m_pthreadArg(pThreadArg), m_bterminateApp(terminateApp) {}
    virtual ~ThreadWrapper(void) {};
    
    virtual void run(void) = 0;

protected:
    [[ nodiscard ]] bool terminateApp() const
    {
        return m_bterminateApp;
    }

    T *m_pthreadArg;

private:
    bool &m_bterminateApp;
};

} // namespace acc
