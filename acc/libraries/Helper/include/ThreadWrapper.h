#pragma once

namespace acc
{
template<typename T>
class ThreadWrapper
{
public:
    explicit ThreadWrapper(T *pThreadArg) : m_pthreadArg(pThreadArg), m_terminate(false) {}
    
    void run()
    {
        while (!m_terminate)
        {
            threadLoop();
        }
    }

    virtual void threadLoop() = 0;

    virtual ~ThreadWrapper()
    {
        // cleanup code goes here
    }

protected:
    T *m_pthreadArg;

private:
    bool m_terminate;
};

} // namespace acc
