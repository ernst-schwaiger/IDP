#include <iostream>
#include <span>

#include <signal.h>
#include <time.h>

#include <CryptoComm.h>

using namespace std;
using namespace acc;

// Type definitions

typedef void (*task_type) (union sigval);

// Global variables
static char const *BYE = "bye";
static uint32_t gSuccessfulRxMsgs;

static void task_5_ms(union sigval arg)
{
    if (arg.sival_ptr != nullptr)
    {
        array<uint8_t, MAX_MSG_LEN> msgBuf = { 0 };
        BTConnection *conn = reinterpret_cast<BTConnection *>(arg.sival_ptr);
        // just send a dummy byte
        if (conn->sendWithCounterAndMAC(0x01, {&msgBuf[0], 1}) < 0)
        {
            cerr << "Failed to send data from buffer.\n";
        }
        else
        {
            // wait for the response
            uint8_t msgType;
            if (conn->receiveWithCounterAndMAC(msgType, {&msgBuf[0], msgBuf.size()}) < 0)
            {
                cerr << "Failed to receive sensor readings.\n";
            }
            else
            {
                gSuccessfulRxMsgs++;
            }
        }

    }
}

// Cyclic task which 
// - receives the proximity readings, 
// - checks validity of readings
// - calculates new motor speed
// - activates an alarm, if required
static timer_t setup_task(uint16_t period_ms, task_type task, void *task_arg)
{
    struct sigevent sev;
    timer_t timerid;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = task;
    sev.sigev_value.sival_ptr = task_arg;
    sev.sigev_notify_attributes = nullptr;

    timer_create(CLOCK_MONOTONIC, &sev, &timerid);

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1'000'000 * period_ms;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 1'000'000 * period_ms;

    timer_settime(timerid, 0, &its, nullptr);
    return timerid;
}

static void stop_timer(timer_t timerId)
{
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    timer_settime(timerId, 0, &its, NULL);
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
            return -1;
        }

        cout << "Hello, I am node2.\n";

        // Set up a BT connection to node1
        BTConnection conn(argv[1]);

        conn.keyExchangeClient();
        gSuccessfulRxMsgs = 0;
        // Activate our 5ms task
        timer_t timerId = setup_task(5, task_5_ms, &conn);

        sleep(10); // let the task run for 10 secs, ~2000 times
        stop_timer(timerId);
        sleep(1); // be sure the timer does not fire any more

        // send the end token to inform the server we are done
        std::span<const uint8_t> byeSpan(reinterpret_cast<uint8_t const *>(BYE), strlen(BYE) + 1);
        conn.send(byeSpan);
        cout << "Received " << gSuccessfulRxMsgs << " readings successfully\n";
    }
    catch(const runtime_error &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
        return -1;
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
        return -1;
    }

    return 0;
}