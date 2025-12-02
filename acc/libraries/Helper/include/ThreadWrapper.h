#pragma once

namespace acc
{
// Wraps a thread function passed a parameter of type T*, provides an API to detect if the thread
// shall exit its loop since the application is being terminated
template<typename T>
class ThreadWrapper
{
public:
    // Initialize thread wrapper
    explicit ThreadWrapper(bool &terminateApp, T *pThreadArg) : m_pthreadArg(pThreadArg), m_bterminateApp(terminateApp) {}
    // Cleanup thread wrapper
    virtual ~ThreadWrapper(void) {};
    // Execute the thread function
    virtual void run(void) = 0;

protected:
    // returns whether the application is being terminated
    [[ nodiscard ]] bool terminateApp() const
    {
        return m_bterminateApp;
    }

    T *m_pthreadArg; // arg passed to the thread function

private:
    bool &m_bterminateApp; // reference to the global "terminate app" flag
};

} // namespace acc
