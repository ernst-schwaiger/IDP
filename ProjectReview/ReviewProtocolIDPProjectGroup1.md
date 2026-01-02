# Review ip-2025-gruppe-1

## Build Process

### Gattlib dependency to libpcre

when running `cmake ..` in gattlib, we got an error:

```bash
CMake Error at /usr/share/cmake-3.31/Modules/FindPkgConfig.cmake:938 (message):
  None of the required 'libpcre' found                                                                           
Call Stack (most recent call first):                                                                             
  examples/read_write/CMakeLists.txt:27 (pkg_search_module)
```

Since the dependency only included non-essential example sub projects, `cmake -DGATTLIB_BUILD_EXAMPLES=NO ..` can be used to resolve the issue.
Alternatively, `sudo apt install libpcre3-dev` resolves the issue as well. This, however, did only work in Raspbian 12/Bookworm, not in the current Raspbian 13/Trixie. See https://packages.debian.org/search?keywords=libpcre3.

### Wolfssl

The projects CMakeLists.txt contains the statement `add_subdirectory(./wolfssl)`, which caused an error invoking `cmake` since `wolfssl` was checked out aside `ip-2025-gruppe-1`. Moving `wolfssl` below the `ip-2025-gruppe-1` folder fixed the problem.

### curl/curl.h missing

The initial compilation failed since `ip-telegraf.c` includes `curl/curl.h`, which was not present on the build system.
`sudo apt install libcurl4-openssl-dev` fixed the issue, the project could then be compiled without problems.

### Increase Warning Level, add required C Standard

Since the CMakeLists.txt file did not add additional warning flags, and no C standard was mandated, the corresponding entries were added to the `CMakeLists.txt`:

```
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_STANDARD 99)
```

Recompilation with the added flags revealed the warnings listed in section [gcc warnings](#gcc-warnings). The more relevant ones are discussed below:

#### Warning: Unused variables

```
...
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:163:16: warning: ‘alarm_state’ defined but not used [-Wunused-variable]
  163 | static uint8_t alarm_state = ALARM_STATE_OFF;
      |                ^~~~~~~~~~~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:161:18: warning: ‘ring_alarm_thread’ defined but not used [-Wunused-variable]
  161 | static pthread_t ring_alarm_thread = THREAD_UNINITIALIZED;
      |                  ^~~~~~~~~~~~~~~~~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:87:17: warning: ‘devices’ defined but not used [-Wunused-variable]
   87 | static device_t devices[MAX_DEVICES] =
      |                 ^~~~~~~
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:13:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:24:16: warning: ‘encryption_key’ defined but not used [-Wunused-variable]
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:24:16: warning: ‘encryption_key’ defined but not used [-Wunused-variable]
...
```

`ip-bluetooth.h` *defines* several static variables. As a consequence, every .c file which is directly/indirectly including that
header will contain these variables, even if they are not used in the .c file. In the current setting, `ip-bluetooth.c` and `ip.c`
include that header, and the compiler issues warnings about the unused variables.

The issue can be fixed by moving the variable definitions into the respective .c files. Since all variables are "static", i.e. not
shared between multiple modules, no "extern" variable declarations are required.

The same warning is reported for `ip-wolfcrypt.h`, which is included by `ip-wolfcrypt.c` and, indirectly via `ip-bluetooth,h` by `ip.c`
and by `ip-bluetooth.c`. The header file defines a static variable `encryption_key`, which is only used in `ip-wolfcrypt.c`, hence
causes the warnings in `ip.c`, and in `ip-bluetooth.c`.

#### Warning: Empty initializer lists for arrays

```
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip.c:1:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:104:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  104 |                                         .temperatures = {},
```

The project defined ring-buffers for storing the most recent temperature measurements, however the entries in the ring-buffers were initialized
with an empty initializer list, like below for `temperatures`:

```c
/* ... */
static device_t devices[MAX_DEVICES] =
        {
                {
                        .mac_address = CLIENT_DEVICE_ONE_MAC,
                        /* ... */
                        .accepted_recent_five =
                                {
                                        .temperatures = {},
                                        .head = TEMPERATURE_ARRAY_INIT_VALUE_HEAD,
                                        .tail = TEMPERATURE_ARRAY_INIT_VALUE_TAIL,
                                        .size = TEMPERATURE_ARRAY_INIT_VALUE_SIZE
                                },
                },
                /* ... */
        }
```
This is not allowed in ISO C90, or in ISO C99. gcc allows this, probably clang as well. 
The recommendation here is to replace the assignment by: `.temperatures = { 0 },` in the same manner as it was done for other fields of the same
structure.

A similar initialization code is also found in the ESP32 `read_temperature()` and `decrypt()` functions. In ESP32, however, we have a C++ file, and (assuming C++14 at least), **empty braced initializers are allowed in C++**; the following code is OK and initializes the array with zeros.

```cpp
void read_temperature() {
    /* ... */
    /* OK, initialize decrypted with 0s */
    td_uint8_t decrypted[COMMAND_SIZE + STRING_TERMINATOR_OFFSET] = {};
    uint8_t *message = device.alarm_chara->getData();

    td_int32_t ret = decrypt(message, decrypted);
    /* ... */
}

td_int32_t decrypt(uint8_t *message, td_uint8_t *dest) {
    /* OK, initialize buffers with 0s */
    uint8_t ciphertext[COMMAND_SIZE] = {};
    uint8_t iv[IV_SIZE] = {};
    uint8_t tag[TAG_SIZE] = {};
    uint8_t plaintext[COMMAND_SIZE] = {};
}
```

#### Warning: comparison will always evaluate as ‘false’

```
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c: In function ‘get_device_info_by_mac’:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:12:25: warning: the comparison will always evaluate as ‘false’ for the address of ‘devices’ will never be NULL [-Waddress]
   12 |         if (&devices[i] == NULL) {
      |                         ^~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:87:17: note: ‘devices’ declared here
   87 | static device_t devices[MAX_DEVICES] =
```

```c
static device_t *get_device_info_by_mac(td_const_uint8_t *mac)
{
    for (int16_t i = 0; i < MAX_DEVICES; i++) {
        if (&devices[i] == NULL) {
            printf("[GET DEVICE INFO] Device at index %d is NULL\n", i);
            continue;
        }

        if (strcmp(mac, devices[i].mac_address) == 0) {
            return &devices[i];
        }
    }
    return NULL;
}
```

Since the `devices` global variable is defined, the expression `&devices[i]` won't be `NULL` for all value of `i` in the range `[0..MAX_DEVICES-1]`.
Recommendation: Remove the if block completely, as it has no effect.

#### Warning: comparison of integer expressions of different signedness

```
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c: In function ‘read_temperature_loop’:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:454:27: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  454 |             if (nextValue == QUEUE_SIZE_INVALID) {
```

ip-fifo.h:
```c
#define QUEUE_SIZE_INVALID (-1)
```

ip-fifo.c:
```c
uint32_t dequeue(temperature_queue *queue)
{
    if (queue->size == 0) {
        return QUEUE_SIZE_INVALID;
    }
    uint32_t val = queue->temperatures[queue->head];
    queue->head = (queue->head + 1) % TEMPERATURE_VALIDATION_SIZE;
    queue->size--;
    return val;
}
```

ip-bluetooth.c:
```c
    /* in ip-fifo.h: #define QUEUE_SIZE_INVALID (-1) */

    /* ... */
    uint32_t nextValue;
    /* ... */
    nextValue = dequeue(&device->next_five_temp_to_evaluate);

    if (nextValue == QUEUE_SIZE_INVALID) {
        /* ... */
    }
    /* ... */
```

Since `dequeue()` returns an `uint32_t`, which is unsigned, it won't return `-1` as `QUEUE_SIZE_INVALID` indicates, but `4294967295` instead.
The comparison in `ip-bluetooth.c`, however, still works.

Recommend to change `QUEUE_SIZE_INVALID` to `#define QUEUE_SIZE_INVALID 4294967295U`

#### Warning: format ‘%s’ expects argument of type ‘char *’, but argument 2 has type ‘unsigned int’

```
/home/ernst/projects/ip-2025-gruppe-1/ip-telegraf.c: In function ‘log_data’:
/home/ernst/projects/ip-2025-gruppe-1/ip-telegraf.c:39:66: warning: format ‘%s’ expects argument of type ‘char *’, but argument 2 has type ‘unsigned int’ [-Wformat=]
   39 |             printf("[LOG DATA] Failed to send data. Error Code: %s\n", res);
      |                                                                 ~^     ~~~
      |                                                                  |     |
      |                                                                  |     unsigned int
      |                                                                  char *
      |                                                                 %d
```

```c
void log_data(td_const_uint8_t* device_name, uint8_t status, uint32_t value) {
    /* ... */
    CURLcode res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        printf("[LOG DATA] Failed to send data. Error Code: %s\n", res);
    }    
    /* ... */
}
```

If the curl command ever fails, it will likely cause a segfault.
Recommendation: Turn on compiler warnings to detect these kinds of issues.

We did not compile the ESP32 code, as we did not get a compilable project. We, however, examined the file `ESP32-DHT11-vf2.ino` containing the logic which is executed in the ESP32 nodes.

### MISRA checks

Running `cppcheck` on the sources did not show any findings regarding the mandatory MISRA rules, a few other rule violations were detected, listed in section [misra warnings](#misra-warnings). We continued with manually checking the mandatory MISRA rules:

#### Dir 2.1 All source files shall compile without any compilation errors

In the original setting, the project compiles without errors using gcc. In an environment with compiler settings that enforce
ISO C90 or ISO C99, a compilation error would have occurred due to the empty initializer lists.

#### Dir 3.1 All code shall be traceable to documented requirements 

Requirements and where they are documented in the Code:   

GEN-01: this requirement is not relevant for the code

GEN-02: ip-bluetooth.c Row 107 ff and ESP32-DHT11-vf2.ino Row 184    
GEN-03: ip-bluetooth.h Row 82 ff    
GEN-04: ip-bluetooth.h Row 108 ff     
GEN-05: ip-telegraf.c  Row 9 ff     

SAF-01: ESP32-DHT11-vf2.ino Row 112    
SAF-02: not found in Code    
SAF-03: ip-bluetooth.c Row 362 ff    
SAF-04: ip-bluetooth.c Row 429 ff    
SAF-05: ip-bluetooth.c Row 362 ff    
SAF-06: ESP32-DHT11-vf2.ino Row 346   
SAF-07.1: ip-bluetooth.c Row 107 ff    
SAF-07.2: ip-bluetooth.c Row 429 ff    
SAF-08: this requirement is not relevant for the code    
SAF-09: this requirement is not relevant for the code    

SEC-01: ip-wolfcrypt.h Row 24 and ESP32-DHT11-vf2.ino Row 82   
SEC-02: ip-wolfcrypt.c Row 12    
SEC-03: this requirement is not relevant for the code

#### Dir 4.1 Run-time failures shall be minimized

The unnecessary check on `&devices[i]` against `NULL` was already discussed in the section above: "Warning: comparison will always evaluate as ‘false’"

In `ip-bluetooth.c`, several functions are passed a pointer to `device_t`, and check the validity of that pointer, which in our
humble opinion is not necessary since the function callers (partly in the used gattlib BLE framework) ensure the validity of the pointer:

```c
void disconnect_device(device_t *device)
{
    /* not required */
    if (device == NULL) {
        printf("[DISCONNECT DEVICE] Device is NULL\n");
        return;
    }

    /* ... */
}

void *bluetooth_connect_device(void *arg)
{
    device_t *connection = (device_t *) arg;
    /* not required */
    if (connection == NULL) {
        printf("[CONNECT DEVICE] Cannot read device\n");
        return NULL;
    }
    /* ... */
}

uint32_t read_temperature(device_t *device)
{

    /* not required */
    if (device == NULL) {
        printf("[READ TEMP] Cannot read device\n");
        return READ_TEMPERATURE_FAILED;
    }
    /* ... */
}

void *read_temperature_loop(void *arg)
{
    device_t *device = (device_t *) arg;

    /* not required */
    if (device == NULL) {
        printf("[READ TEMP LOOP] Cannot read device\n");
        return NULL;
    }
    /* ... */
}

bluetooth_on_device_connected(gattlib_adapter_t *adapter, td_const_uint8_t *dst, gattlib_connection_t *connection, td_int32_t error,
                              void *user_data)
{
    /* ... */

    device_t *device = (device_t *) user_data;
    /* not required */
    if (device == NULL) {
        printf("[ON DEVICE CONNECTED] Cannot read device\n");
        return;
    }
    /* ... */
}

void *reconnect_thread(void *arg)
{
    device_t *device = (device_t *) arg;
    /* not required */
    if (device == NULL) {
        printf("[RECONNECT] Cannot read device\n");
        return NULL;
    }
    /* ... */
}

void bluetooth_on_device_disconnected(gattlib_connection_t *connection, void *user_data)
{
   /* ... */

    /* not required */
    if (device == NULL) {
        printf("[DISCONNECT] Cannot read device\n");
        return;
    }

  /* ... */
}
```

In `ip-bluetooth.c`, all `enqueue()`, `dequeue()` calls are invoked in the context one thread, i.e. there can't be race conditions accessing the queues the thread operates on. However, the validity of the `dequeue()` calls is checked twice:

`dequeue(&device->next_five_temp_to_evaluate)` only returns `QUEUE_SIZE_INVALID` if `(device->next_five_temp_to_evaluate.size > 0)` does not hold. One of the two checks below can be removed.

```c
/* In ip-fifo.c: */
uint32_t dequeue(temperature_queue *queue)
{
    if (queue->size == 0) {
        return QUEUE_SIZE_INVALID;
    }
    uint32_t val = queue->temperatures[queue->head];
    queue->head = (queue->head + 1) % TEMPERATURE_VALIDATION_SIZE;
    queue->size--;
    return val;
}

/* In ip-bluetooth.c: */
void *read_temperature_loop(void *arg)
{
    /* ... */
    if (device->next_five_temp_to_evaluate.size > 0) {
        nextValue = dequeue(&device->next_five_temp_to_evaluate);
        /* not required */
        if (nextValue == QUEUE_SIZE_INVALID) {
            printf("[READ TEMP LOOP] Failed to dequeue value for evaluation %u.\n", nextValue);
            continue;
        }
        skip_sleep = SKIP_SLEEP_YES;
    }
    /* ... */
}
```

In `ip-fifo.c`, both `enqueue()` and `enqueue_accepted_values()` always return `ENQUEUE_SUCCESS`, the error handling code in the client is not required (and there is also no need for `enqueue()`, or `enqueue_accepted_values()` to return a status value):

```c
td_int32_t enqueue(temperature_queue *queue, uint32_t value)
{
    if (queue->size == TEMPERATURE_VALIDATION_SIZE) {
        queue->head = (queue->head + 1) % TEMPERATURE_VALIDATION_SIZE;
    } else {
        queue->size++;
    }
    queue->tail = (queue->tail + 1) % TEMPERATURE_VALIDATION_SIZE;
    queue->temperatures[queue->tail] = value;
    return ENQUEUE_SUCCESS;
}

td_int32_t enqueue_accepted_values(accepted_temperature_queue *queue, uint32_t value)
{
    if (queue->size == TEMPERATURE_ACCEPTED_SIZE) {
        queue->head = (queue->head + 1) % TEMPERATURE_ACCEPTED_SIZE;
    } else {
        queue->size++;
    }
    queue->tail = (queue->tail + 1) % TEMPERATURE_ACCEPTED_SIZE;
    queue->temperatures[queue->tail] = value;
    return ENQUEUE_SUCCESS;
}

void *read_temperature_loop(void *arg)
{
    /* ... */
    td_int32_t ret = enqueue_accepted_values(&device->accepted_recent_five, nextValue);
    /* not required */
    if (ret != ENQUEUE_SUCCESS) {
        printf("[READ TEMP LOOP] Failed to accept value %u.\n", nextValue);
    }
    /* ... */
}
```

in `ESP32-DHT11-vf2.ino`, the decrypt function checks `dest`, which is not required, since the only caller provides a valid buffer. For `message`, a length of the buffer should be passed to ensure the decrypt function does not read off the boundaries:

```cpp
td_int32_t decrypt(uint8_t *message, td_uint8_t *dest) {
    if (message == NULL) {
        Serial.println("[DECRYPT] Error: Message pointer is NULL");
        return DECRYPTION_FAILED;
    }
    /* not required */
    if (dest == NULL) {
        Serial.println("[DECRYPT] Error: Destination pointer is NULL");
        return DECRYPTION_FAILED;
    }
    /* ... * /
}

/* ... */
void read_temperature()
{
    /* ... */
    temp = ALARM_STATE_TEMPERATURE_VALUE;

    td_uint8_t decrypted[COMMAND_SIZE + STRING_TERMINATOR_OFFSET] = {};
    uint8_t *message = device.alarm_chara->getData();

    td_int32_t ret = decrypt(message, decrypted);
    /* ... */
}
```

#### Dir 4.6 typedefs that indicate size and signedness should be used in place of the basic numerical types

No basic numerical types were found, however in `ip-typedefs.h`, the project defined some types itself:

```c
#ifndef IP_TYPEDEFS
#define IP_TYPEDEFS

typedef const char td_const_uint8_t;
typedef char td_uint8_t;
typedef int td_int32_t;

#endif
```

- The C standard does define whether `char` is signed or unsigned. On the Raspbian/ARM platform (also on X86_64, PowerPC, ...) `char` is *signed*. 
Hence, the type name `td_uint8_t` is misleading.
- For `int` is always signed, but may have 16, 32, or (in obscure/obsolete architectures) even 64 bit.

Recommendation: use `uint8_t`, `int32_t` from `stddef.h`. If libraries like wolfssl require `char` pointers to be passed, cast the buffer pointers to that type in the call to the function.


#### Dir 4.7 If a function returns error information, then that error information shall be tested

For the following functions providing return values, calls were found where the return value has not been checked.

|Function|Ignoring return value OK?|
|-|-|
|`curl_global_init()`||
|`gattlib_register_on_disconnect()`||
|`gpioDelay()`|OK, perhaps use macro which checks return value in debug mode|
|`gpioPWM()`|OK, perhaps use macro which checks return value in debug mode|
|`gpioSetMode()`|OK, perhaps use macro which checks return value in debug mode|
|`gpioSetPWMfrequency()`|OK, perhaps use macro which checks return value in debug mode|
|`printf()`|OK, perhaps explicitly cast calls as void|
|`sprintf()`|Check the return value to ensure you have a proper string in the buffer|
|`pthread_cancel()`||
|`pthread_cond_wait()`|Call likely not needed, see below|
|`pthread_detach()`||
|`pthread_mutex_lock()`|Call likely not needed, see below|
|`pthread_mutex_unlock()`|Call likely not needed, see below|
|`sleep()`|OK, perhaps explicitly cast calls as void|
|`wc_AesInit()`|OK, only returns error on incorrect input parameters|

#### Dir 4.10 Precautions shall be taken in order to prevent the contents of a header file being included more than once
OK

#### Dir 4.11 The validity of values passed to library functions shall be checked

In ip-bluetooth.c, the `reader_thread` is canceled and detached, however the incorrect handle is checked upfront.
Moreover, `THREAD_UNINITIALIZED` is #defined as 0, which might be a valid thread handle. Suggest to either use a dedicated field to indicate a valid thread, or to use a datatype which is bigger than `pthread_t` and use a value which `pthread_t` can't hold as `THREAD_UNINITIALIZED`

```c
void handle_sigterm(td_int32_t sig)
{
    (void) sig;

    printf("[SIGNAL HANDLER] SIGTERM executed.\n");

    for (int16_t i = 0; i < MAX_DEVICES; i++) {
        disconnect_device(&devices[i]);

        if (devices[i].thread != THREAD_UNINITIALIZED) {
            pthread_cancel(devices[i].thread);
            pthread_detach(devices[i].thread);
        }
        /* (devices[i].reader_thread != THREAD_UNINITIALIZED) ? */
        if (devices[i].thread != THREAD_UNINITIALIZED) {
            pthread_cancel(devices[i].reader_thread);
            pthread_detach(devices[i].reader_thread);
        }
    }

    if (ring_alarm_thread != THREAD_UNINITIALIZED) {
        pthread_cancel(ring_alarm_thread);
        pthread_detach(ring_alarm_thread);
    }

    exit(0);
}
```

TimeOfCheck - TimeOfUse TOCTOU issue in `read_temperature_loop()`. Between validity check  `while (device->connection != NULL)` and its use in `read_temperature(device)`, `device->connection` may have been set to `NULL` in `bluetooth_on_device_disconnected()` which is executed by a concurrent thread. Moreover, the `device->connection` should be declared `volatile` to prevent the compiler from removing code accessing that fields value.

```c
void *read_temperature_loop(void *arg)
{
    /* ... */

    while (device->connection != NULL) {

        /* lots of code executed here */

        nextValue = read_temperature(device);
        /* ... */
    }
    /* ... */
}

uint32_t read_temperature(device_t *device)
{
    /* ... no check for validity of device->connection */
    uint32_t ret = gattlib_read_char_by_uuid(device->connection, &device->temperature_uuid, (void **) &buffer, &len);
}

void bluetooth_on_device_disconnected(gattlib_connection_t *connection, void *user_data)
{
    /* reader thread is cancelled *after* device->connection is invalidated */
    /* ... */
    device->connection = NULL;
    /* ... */
    if (device->reader_thread != THREAD_UNINITIALIZED) {
        pthread_cancel(device->reader_thread);
        pthread_detach(device->reader_thread);
    }
}
```

In `bluetooth_on_device_connected()`, there is a logic bug in the error handling code. If `connection == NULL`, it is passed to `bluetooth_on_device_disconnected()` as first parameter.

```c
void
bluetooth_on_device_connected(gattlib_adapter_t *adapter, td_const_uint8_t *dst, gattlib_connection_t *connection, td_int32_t error,
                              void *user_data)
{
    (void) adapter;

    if ((error != 0) || (connection == NULL)) {
        printf("[ON DEVICE CONNECTED] Failed to connect to %s, error: %d\n", dst, error);
        bluetooth_on_device_disconnected(connection, user_data);
        return;
    }
    /* ... */
}
```

#### Rule 14.2 A for loop shall be well-formed
OK

#### Rule 15.1 The goto statement should not be used
OK

### Manual Checks:

#### Recommend to use stderr
All messages were written to `stdout`. Suggest to write info messages to `stdout`, and error messages to `stderr`.

#### Lock statements without effect

in `ip-bluetooth.c`, there are mutex lock/unlock operations which do not have an effect, as the lock variables are local to the function (i.e. `pthread_mutex_lock()` below will never block).

`pthread_cond_wait()`, however, blocks the thread indefinitely. We assume that was done to prevent the application from terminating. Probably, the `read_temperature_loop()` could be executed here.

```c
void *bluetooth_scan_devices(void *args)
{
    /* ... */
    printf("[MAIN] End scanning devices..\n");

    // This is just to block this thread while other threads are working...
    // see example: https://github.com/labapart/gattlib/blob/master/examples/read_write/read_write.c

    pthread_mutex_t m_connection_terminated_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_connection_terminated = PTHREAD_COND_INITIALIZER;

    pthread_mutex_lock(&m_connection_terminated_lock); /* will never block */
    pthread_cond_wait(&m_connection_terminated, &m_connection_terminated_lock); /* blocks indefinitively */
    pthread_mutex_unlock(&m_connection_terminated_lock); /* won't be reached */

    return NULL;
}
```

The locking/unlocking of the mutexes has no effect here as well, since the mutexes are created locally on the stack.

```c
void bluetooth_on_device_disconnected(gattlib_connection_t *connection, void *user_data)
{
    (void) connection;

    pthread_mutex_t device_connection_lock = PTHREAD_MUTEX_INITIALIZER;

    printf("[DISCONNECT] Device got disconnected...\n");
    pthread_mutex_lock(&device_connection_lock); /* will never block */
    /* ... critical section here? ... */
    pthread_mutex_unlock(&device_connection_lock);

    /* ... */
}
```

#### Potential resource/memory leaks

in ip-wolfcrypt.c, `encrypt()` may cause resource leaks, as `wc_AesInit()` and `wc_InitRng()` *may* allocate resources which must be freed with `wc_AesFree()` and `wc_FreeRng()`. The error handling blocks, however, immediately return without calling the free functions, which may cause resource/memory leaks.

```c
td_int32_t encrypt(int8_t *dest)
{
    /* ... */
    wc_AesInit(&aes, NULL, INVALID_DEVID);

    // Initialize the random number generator
    uint32_t ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("[ENCRYPT] RNG initialization failed! Error: %d\n", ret);
        /* probably resource leak here */
        return ENCRYPTION_FAILED;
    }

    // Generate a random IV (12 bytes)
    ret = wc_RNG_GenerateBlock(&rng, iv, IV_SIZE);
    if (ret != 0) {
        printf("[ENCRYPT] IV generation failed! Error: %d\n", ret);
        /* probably resource leak here */
        return ENCRYPTION_FAILED;
    }

    // Set up AES key in the AES context
    ret = wc_AesGcmSetKey(&aes, encryption_key, KEY_SIZE);
    if (ret != 0) {
        printf("[ENCRYPT] AES key setup failed! Error: %d\n", ret);
        /* probably resource leak here */
        return ENCRYPTION_FAILED;
    }

    ret = wc_AesGcmEncrypt(&aes, ciphertext, COMMAND, COMMAND_SIZE, iv, IV_SIZE, auth_tag, TAG_SIZE, iv, IV_SIZE);
    if (ret != 0) {
        printf("[ENCRYPT] Encryption failed! Error: %d\n", ret);
        /* probably resource leak here */
        return ENCRYPTION_FAILED;
    }

    /* ... */

    wc_AesFree(&aes);
    wc_FreeRng(&rng);
    /* no resource leaks here */
    return ENCRYPTION_SUCCESS;
}
```

#### Missing reset of handle variable?

By sending the `SIGUSR1` and/or `SIGUSR2` to the running process, the bluetooth connection to the operators 1 and/or 2 can be reset, and new bluetooth connections will be set up again. In the context of resetting the connection, `bluetooth_on_device_disconnected()` is called, canceling the `reader_thread`, which processes the incoming temperature values from the respective operator. However, the field holding the thread handle is not reset, which prevents a recreation of the `reader_thread` in `bluetooth_on_device_connected()` when the new connection is set up, and no temperature values will be processed from the respective operator again:

```c
void bluetooth_on_device_disconnected(gattlib_connection_t *connection, void *user_data)
{
    /* ... */
    if (device->reader_thread != THREAD_UNINITIALIZED) {
        pthread_cancel(device->reader_thread);
        pthread_detach(device->reader_thread);
        /* device->reader_thread = THREAD_UNINITIALIZED; // missing? */
    }
    /* ... */
}

/* ... */
void
bluetooth_on_device_connected(gattlib_adapter_t *adapter, td_const_uint8_t *dst, gattlib_connection_t *connection, td_int32_t error, void *user_data)
{
    /* ... */
    /* since the thread handle of the cancelled thread is still in device->reader_thread, the
       error message is printed, and no new `reader_thread` is created */
    if(device->reader_thread != THREAD_UNINITIALIZED) {
        printf("[ON DEVICE CONNECTED] Cannot create reader thread.\n");
        return;
    }

    if (pthread_create(&device->reader_thread, NULL, read_temperature_loop, device) != 0) {
        printf("[ON DEVICE CONNECTED] Failed to create reader thread.\n");
        return;
    }
}
```

### gcc warnings

The following warning messages were obtained using gcc 12.2 with the options `-Wall -Wextra -Wpedantic` turned on.

```
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip.c:1:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:104:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  104 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:111:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  111 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:118:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  118 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:139:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  139 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:146:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  146 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:153:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  153 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:163:16: warning: ‘alarm_state’ defined but not used [-Wunused-variable]
  163 | static uint8_t alarm_state = ALARM_STATE_OFF;
      |                ^~~~~~~~~~~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:161:18: warning: ‘ring_alarm_thread’ defined but not used [-Wunused-variable]
  161 | static pthread_t ring_alarm_thread = THREAD_UNINITIALIZED;
      |                  ^~~~~~~~~~~~~~~~~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:87:17: warning: ‘devices’ defined but not used [-Wunused-variable]
   87 | static device_t devices[MAX_DEVICES] =
      |                 ^~~~~~~
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:13:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:24:16: warning: ‘encryption_key’ defined but not used [-Wunused-variable]
   24 | static uint8_t encryption_key[KEY_SIZE] = {
      |                ^~~~~~~~~~~~~~
[ 28%] Building C object CMakeFiles/ip.dir/ip-buzzer.c.o
[ 29%] Building C object CMakeFiles/ip.dir/ip-bluetooth.c.o
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:1:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:104:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  104 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:111:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  111 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:118:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  118 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:139:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  139 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:146:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  146 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:153:57: warning: ISO C forbids empty initializer braces [-Wpedantic]
  153 |                                         .temperatures = {},
      |                                                         ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c: In function ‘get_device_info_by_mac’:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:12:25: warning: the comparison will always evaluate as ‘false’ for the address of ‘devices’ will never be NULL [-Waddress]
   12 |         if (&devices[i] == NULL) {
      |                         ^~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:87:17: note: ‘devices’ declared here
   87 | static device_t devices[MAX_DEVICES] =
      |                 ^~~~~~~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c: In function ‘alarm_deactivate’:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:118:23: warning: left-hand operand of comma expression has no effect [-Wunused-value]
  118 |     (void) gpio, level, tick;
      |                       ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:118:5: warning: statement with no effect [-Wunused-value]
  118 |     (void) gpio, level, tick;
      |     ^
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c: In function ‘read_temperature_loop’:
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:454:27: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  454 |             if (nextValue == QUEUE_SIZE_INVALID) {
      |                           ^~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:465:23: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  465 |         if (nextValue == READ_TEMPERATURE_FAILED) {
      |                       ^~
/home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.c:521:31: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  521 |                 if (collected == QUEUE_SIZE_INVALID) {
      |                               ^~
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-bluetooth.h:13:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h: At top level:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:24:16: warning: ‘encryption_key’ defined but not used [-Wunused-variable]
   24 | static uint8_t encryption_key[KEY_SIZE] = {
      |                ^~~~~~~~~~~~~~
[ 30%] Building C object CMakeFiles/ip.dir/ip-fifo.c.o
/home/ernst/projects/ip-2025-gruppe-1/ip-fifo.c: In function ‘is_value_within_accepted_temperature_average’:
/home/ernst/projects/ip-2025-gruppe-1/ip-fifo.c:116:13: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  116 |     if (avg == QUEUE_SIZE_INVALID) {
      |             ^~
/home/ernst/projects/ip-2025-gruppe-1/ip-fifo.c: In function ‘is_value_within_next_five_average’:
/home/ernst/projects/ip-2025-gruppe-1/ip-fifo.c:137:13: warning: comparison of integer expressions of different signedness: ‘uint32_t’ {aka ‘unsigned int’} and ‘int’ [-Wsign-compare]
  137 |     if (avg == QUEUE_SIZE_INVALID) {
      |             ^~
[ 30%] Building C object CMakeFiles/ip.dir/ip-telegraf.c.o
/home/ernst/projects/ip-2025-gruppe-1/ip-telegraf.c: In function ‘log_data’:
/home/ernst/projects/ip-2025-gruppe-1/ip-telegraf.c:39:66: warning: format ‘%s’ expects argument of type ‘char *’, but argument 2 has type ‘unsigned int’ [-Wformat=]
   39 |             printf("[LOG DATA] Failed to send data. Error Code: %s\n", res);
      |                                                                 ~^     ~~~
      |                                                                  |     |
      |                                                                  |     unsigned int
      |                                                                  char *
      |                                                                 %d
[ 31%] Building C object CMakeFiles/ip.dir/ip-wolfcrypt.c.o
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.c:1:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.c: In function ‘encrypt’:
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:16:17: warning: pointer targets in passing argument 3 of ‘wc_AesGcmEncrypt’ differ in signedness [-Wpointer-sign]
   16 | #define COMMAND "RESET"
      |                 ^~~~~~~
      |                 |
      |                 char *
/home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.c:49:46: note: in expansion of macro ‘COMMAND’
   49 |     ret = wc_AesGcmEncrypt(&aes, ciphertext, COMMAND, COMMAND_SIZE, iv, IV_SIZE, auth_tag, TAG_SIZE, iv, IV_SIZE);
      |                                              ^~~~~~~
In file included from /home/ernst/projects/ip-2025-gruppe-1/ip-wolfcrypt.h:8:
/home/ernst/projects/ip-2025-gruppe-1/./wolfssl/wolfssl/wolfcrypt/aes.h:617:48: note: expected ‘const byte *’ {aka ‘const unsigned char *’} but argument is of type ‘char *’
  617 |                                    const byte* in, word32 sz,
      |
```


### misra warnings

The following MISRA warnings were obtained using Cppcheck 2.13.0, and the following `checkMISRA.sh` script:

```bash
#!/bin/bash
CPPCHECK=cppcheck
# building cppcheck from github does not increase findings.
#CPPCHECK=${HOME}/projects/cppcheck/build/bin/cppcheck
CPPCHECKOPTS="--cppcheck-build-dir=cppcheck-build --addon=misra --enable=all --inconclusive --safety --platform=unix64 --checkers-report=report.txt --suppress=misra-config --suppress=missingIncludeSystem"
INC_PATH_ARGS="-I."
CPPCHECKFILES="ip-bluetooth.c ip-buzzer.c ip-fifo.c ip-telegraf.c ip-wolfcrypt.c ip.c"

mkdir -p cppcheck-build

#
# Runs cppcheck including MISRA rules
#
${CPPCHECK} --language=c --std=c99 ${CPPCHECKOPTS} ${INC_PATH_ARGS} ${CPPCHECKFILES} ${HDRCHECKFILES}
```

```
ernst@StudiNotebook:~/projects/InterDisziplinaeresProjekt/ip-2025-gruppe-1$ ./checkMISRA.sh 
Checking ip-bluetooth.c ...
ip-bluetooth.h:5:0: information: Include file: "gattlib.h" not found. [missingInclude]
#include "gattlib.h"
^
ip-bluetooth.c:12:25: style: Condition '&devices[i]==NULL' is always false [knownConditionTrueFalse]
        if (&devices[i] == NULL) {
                        ^
ip-bluetooth.c:125:20: style: Condition 'device==NULL' is always false [knownConditionTrueFalse]
        if (device == NULL) {
                   ^
ip-bluetooth.c:201:20: style: Condition 'device==NULL' is always false [knownConditionTrueFalse]
        if (device == NULL) {
                   ^
ip-bluetooth.c:347:9: warning: %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
        printf("[READ TEMP] Failed reading from: %s - error code: %d'\n", device->mac_address, ret);
        ^
ip-bluetooth.c:30:6: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void disconnect_device(device_t *device)
     ^
ip-bluetooth.c:56:6: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void handle_sigusr1(td_int32_t sig)
     ^
ip-bluetooth.c:68:6: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void handle_sigusr2(td_int32_t sig)
     ^
ip-bluetooth.c:80:6: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void handle_sigterm(td_int32_t sig)
     ^
ip-bluetooth.c:116:6: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void alarm_deactivate(td_int32_t gpio, td_int32_t level, uint32_t tick)
     ^
ip-bluetooth.c:158:7: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void *ring_alarm(void *arg)
      ^
ip-bluetooth.c:334:10: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
uint32_t read_temperature(device_t *device)
         ^
ip-bluetooth.c:371:7: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void *read_temperature_loop(void *arg)
      ^
ip-bluetooth.c:587:7: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.4]
void *reconnect_thread(void *arg)
      ^
ip-bluetooth.h:87:17: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-9.2]
static device_t devices[MAX_DEVICES] =
                ^
ip-bluetooth.c:354:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.3]
        value |= ((uint32_t) buffer[i]) << (8 * i);
              ^
ip-bluetooth.c:433:37: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.3]
            uint8_t override_active =
                                    ^
ip-bluetooth.c:436:34: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.3]
            uint8_t alarm_active =
                                 ^
ip-bluetooth.c:207:76: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                                   strlen(device->alarm_reset_uuid_string) + 1,
                                                                           ^
ip-bluetooth.c:279:113: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (gattlib_string_to_uuid(connection->temperature_uuid_string, strlen(connection->temperature_uuid_string) + 1,
                                                                                                                ^
ip-bluetooth.c:390:24: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
        if (skip_sleep == SKIP_SLEEP_NO) {
                       ^
ip-bluetooth.c:434:40: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                    (device_status_one == OPERATOR_OVERRIDE) || (device_status_two == OPERATOR_OVERRIDE);
                                       ^
ip-bluetooth.c:437:42: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                    (((device_status_one == OPERATOR_ALARM) && (device_status_two == OPERATOR_WARNING)) ||
                                         ^
ip-bluetooth.c:438:42: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                     ((device_status_one == OPERATOR_WARNING) && (device_status_two == OPERATOR_ALARM))) ||
                                         ^
ip-bluetooth.c:439:42: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                    (((device_status_one == OPERATOR_ALARM) && (device_status_two == OPERATOR_OFFLINE)) ||
                                         ^
ip-bluetooth.c:440:42: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                     ((device_status_one == OPERATOR_OFFLINE) && (device_status_two == OPERATOR_ALARM))) ||
                                         ^
ip-bluetooth.c:441:41: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                    ((device_status_one == OPERATOR_ALARM) && (device_status_two == OPERATOR_ALARM));
                                        ^
ip-bluetooth.c:454:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
            if (nextValue == QUEUE_SIZE_INVALID) {
                          ^
ip-bluetooth.c:465:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
        if (nextValue == READ_TEMPERATURE_FAILED) {
                      ^
ip-bluetooth.c:521:31: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
                if (collected == QUEUE_SIZE_INVALID) {
                              ^
ip-bluetooth.c:300:28: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-11.5]
    device_t *connection = (device_t *) arg;
                           ^
ip-bluetooth.c:373:24: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-11.5]
    device_t *device = (device_t *) arg;
                       ^
ip-bluetooth.c:562:24: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-11.5]
    device_t *device = (device_t *) user_data;
                       ^
ip-bluetooth.c:589:24: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-11.5]
    device_t *device = (device_t *) arg;
                       ^
ip-bluetooth.c:631:24: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-11.5]
    device_t *device = (device_t *) user_data;
                       ^
ip-bluetooth.c:346:32: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-12.1]
    if (ret != GATTLIB_SUCCESS || buffer == NULL) {
                               ^
ip-bluetooth.c:18:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
            return &devices[i];
            ^
ip-bluetooth.c:34:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:39:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:185:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:190:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:195:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:216:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:225:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:234:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:268:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:282:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:287:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:304:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:310:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:339:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return READ_TEMPERATURE_FAILED;
        ^
ip-bluetooth.c:348:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return READ_TEMPERATURE_FAILED;
        ^
ip-bluetooth.c:377:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:557:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:565:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:572:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:577:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:592:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return NULL;
        ^
ip-bluetooth.c:635:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:639:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:661:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return;
        ^
ip-bluetooth.c:421:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.7]
            }
            ^
ip-bluetooth.c:43:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (gattlib_disconnect(device->connection, true) != GATTLIB_SUCCESS) {
        ^
ip-bluetooth.c:206:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
        if (gattlib_string_to_uuid(device->alarm_reset_uuid_string,
            ^
ip-bluetooth.c:214:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (pthread_create(&ring_alarm_thread, NULL, ring_alarm, NULL) != 0) {
        ^
ip-bluetooth.c:285:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (pthread_create(&connection->thread, NULL, bluetooth_connect_device, connection) != 0) {
        ^
ip-bluetooth.c:575:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (pthread_create(&device->reader_thread, NULL, read_temperature_loop, device) != 0) {
        ^
ip-bluetooth.c:659:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (pthread_create(&tid, NULL, reconnect_thread, device) != 0) {
        ^
ip-bluetooth.c:13:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[GET DEVICE INFO] Device at index %d is NULL\n", i);
                  ^
ip-bluetooth.c:33:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT DEVICE] Device is NULL\n");
              ^
ip-bluetooth.c:38:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT DEVICE] No connection found for device with mac address %s\n", device->mac_address);
              ^
ip-bluetooth.c:42:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[DISCONNECT DEVICE] Disconnecting device with mac address %s\n", device->mac_address);
          ^
ip-bluetooth.c:44:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT DEVICE] Connection could not be disconnected for device with mac address %s\n",
              ^
ip-bluetooth.c:47:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT DEVICE] Successfully disconnected device with mac address %s\n", device->mac_address);
              ^
ip-bluetooth.c:59:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[SIGNAL HANDLER] SIGUSR1 executed.\n");
          ^
ip-bluetooth.c:71:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[SIGNAL HANDLER] SIGUSR2 executed.\n");
          ^
ip-bluetooth.c:84:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[SIGNAL HANDLER] SIGTERM executed.\n");
          ^
ip-bluetooth.c:126:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[ALARM DEACTIVATE] Cannot read device\n");
                  ^
ip-bluetooth.c:131:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[ALARM DEACTIVATE] Could not find connection for device %s. Skipping..\n", device->mac_address);
                  ^
ip-bluetooth.c:139:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[ALARM DEACTIVATE] Encryption for alarm deactivation command failed. Error code: %d\n", ret);
                  ^
ip-bluetooth.c:144:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                printf("[ALARM DEACTIVATE] Failed to write to the characteristic. Error code: %d\n", ret);
                      ^
ip-bluetooth.c:146:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                printf("[ALARM DEACTIVATE] Successful write to the characteristic.\n");
                      ^
ip-bluetooth.c:184:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[INIT] Failed to set SIGUSR1 handler\n");
              ^
ip-bluetooth.c:189:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[INIT] Failed to set SIGUSR2 handler\n");
              ^
ip-bluetooth.c:194:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[INIT] Failed to set SIGTERM handler\n");
              ^
ip-bluetooth.c:202:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[INIT] Cannot read device\n");
                  ^
ip-bluetooth.c:209:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[INIT] Failed to set Alarm Reset UUID.\n");
                  ^
ip-bluetooth.c:215:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[INIT] Failed to create BLE connection thread.\n");
              ^
ip-bluetooth.c:219:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[MAIN] Opening Bluetooth adapter..\n");
          ^
ip-bluetooth.c:224:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[MAIN] Failed to open Bluetooth adapter.\n");
              ^
ip-bluetooth.c:228:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[MAIN] Successfully opened Bluetooth adapter.\n");
          ^
ip-bluetooth.c:230:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[MAIN] Start scanning devices..\n");
          ^
ip-bluetooth.c:233:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[MAIN] Failed scanning devices.\n");
              ^
ip-bluetooth.c:237:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[MAIN] End scanning devices..\n");
          ^
ip-bluetooth.c:272:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("Discovered %s - '%s'\n", addr, name);
              ^
ip-bluetooth.c:274:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("Discovered %s\n", addr);
              ^
ip-bluetooth.c:281:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCOVERED DEVICE] Failed to set Temperature UUID.\n");
              ^
ip-bluetooth.c:286:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCOVERED DEVICE] Failed to create BLE connection thread.\n");
              ^
ip-bluetooth.c:303:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[CONNECT DEVICE] Cannot read device\n");
              ^
ip-bluetooth.c:308:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[CONNECT DEVICE] Already reconnecting to device: %s - '%s'\n", connection->mac_address,
              ^
ip-bluetooth.c:315:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[CONNECT DEVICE] Connecting to: %s - '%s'\n", connection->mac_address, connection->name);
              ^
ip-bluetooth.c:319:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[CONNECT DEVICE] Successfully connected to: %s - '%s'\n", connection->mac_address, connection->name);
                  ^
ip-bluetooth.c:321:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[CONNECT DEVICE] Failed connecting or reading temp to: %s - '%s'\n", connection->mac_address, connection->name);
                  ^
ip-bluetooth.c:338:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[READ TEMP] Cannot read device\n");
              ^
ip-bluetooth.c:347:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[READ TEMP] Failed reading from: %s - error code: %d'\n", device->mac_address, ret);
              ^
ip-bluetooth.c:376:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[READ TEMP LOOP] Cannot read device\n");
              ^
ip-bluetooth.c:383:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[READ TEMP LOOP] Start reading from: %s with UUID '%s'\n", device->mac_address,
          ^
ip-bluetooth.c:423:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[READ TEMP LOOP] Operator %s is %hhu\n", device->name, status);
                  ^
ip-bluetooth.c:455:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                printf("[READ TEMP LOOP] Failed to dequeue value for evaluation %u.\n", nextValue);
                      ^
ip-bluetooth.c:466:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[READ TEMP LOOP] Failed to read temperature for device %s.\n", device->mac_address);
                  ^
ip-bluetooth.c:474:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                    printf("[READ TEMP LOOP] Failed to accept value %u.\n", nextValue);
                          ^
ip-bluetooth.c:484:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                    printf("[READ TEMP LOOP] Failed to accept value %u.\n", nextValue);
                          ^
ip-bluetooth.c:490:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                printf("[READ TEMP LOOP] Detected suspicious value: %u on device %s\n", nextValue, device->name);
                      ^
ip-bluetooth.c:498:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[READ TEMP LOOP] Failed to add evaluation value %u.\n", nextValue);
                  ^
ip-bluetooth.c:507:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                    printf("[READ TEMP LOOP] Failed to accept value %u.\n", nextValue);
                          ^
ip-bluetooth.c:513:23: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                printf("Rejected value: %u on device %s\n", suspectValue, device->name);
                      ^
ip-bluetooth.c:522:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                    printf("[READ TEMP LOOP] Failed to dequeue value for next evaluation %u.\n", nextValue);
                          ^
ip-bluetooth.c:529:27: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
                    printf("[READ TEMP LOOP] Failed to queue value for next evaluation %u.\n", nextValue);
                          ^
ip-bluetooth.c:555:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ON DEVICE CONNECTED] Failed to connect to %s, error: %d\n", dst, error);
              ^
ip-bluetooth.c:564:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ON DEVICE CONNECTED] Cannot read device\n");
              ^
ip-bluetooth.c:571:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ON DEVICE CONNECTED] Cannot create reader thread.\n");
              ^
ip-bluetooth.c:576:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ON DEVICE CONNECTED] Failed to create reader thread.\n");
              ^
ip-bluetooth.c:591:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[RECONNECT] Cannot read device\n");
              ^
ip-bluetooth.c:597:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("Attempting to reconnect in 5 seconds...\n");
          ^
ip-bluetooth.c:600:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("Connecting to: %s - '%s'\n", device->mac_address, device->name);
              ^
ip-bluetooth.c:604:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("Successfully reconnected to: %s - '%s'\n", device->mac_address, device->name);
                  ^
ip-bluetooth.c:607:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("Reconnect failed, retrying in 5 seconds...\n");
                  ^
ip-bluetooth.c:628:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[DISCONNECT] Device got disconnected...\n");
          ^
ip-bluetooth.c:634:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT] Cannot read device\n");
              ^
ip-bluetooth.c:660:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[DISCONNECT] Failed to create reconnect thread.\n");
              ^
ip-bluetooth.h:8:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.5]
#include <signal.h>
^
ip-bluetooth.h:4:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.6]
#include <stdio.h>
^
ip-bluetooth.c:104:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.8]
    exit(0);
        ^
ip-telegraf.h:8:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.10]
#include <time.h>
^
1/6 files checked 69% done
Checking ip-buzzer.c ...
ip-buzzer.h:15:35: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.2]
extern void buzzer_initialize_gpio(void);
                                  ^
ip-buzzer.c:11:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (gpioInitialise() < BUZZER_INITIALIZE_SUCCESS_CODE) {
        ^
ip-buzzer.c:12:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[BUZZER] Failed to initialise GPIO pin\n");
              ^
ip-buzzer.c:15:11: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    printf("[BUZZER] Successfully initialised GPIO pin. Using pin %d\n", BUZZER_GPIO);
          ^
ip-buzzer.c:4:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.6]
#include <stdio.h>
^
ip-buzzer.c:13:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.8]
        exit(BUZZER_INITIALIZE_ERROR_CODE);
            ^
2/6 files checked 72% done
Checking ip-fifo.c ...
ip-fifo.c:79:17: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    return (sum / queue->size);
                ^
ip-fifo.c:101:17: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    return (sum / QUEUE_SIZE_FOR_CALCULATION);
                ^
ip-fifo.c:116:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (avg == QUEUE_SIZE_INVALID) {
            ^
ip-fifo.c:120:31: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    uint32_t lowerBound = avg * 9 / 10;
                              ^
ip-fifo.c:121:31: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    uint32_t upperBound = avg * 11 / 10;
                              ^
ip-fifo.c:137:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (avg == QUEUE_SIZE_INVALID) {
            ^
ip-fifo.c:141:31: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    uint32_t lowerBound = avg * 9 / 10;
                              ^
ip-fifo.c:142:31: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    uint32_t upperBound = avg * 11 / 10;
                              ^
ip-fifo.c:53:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return QUEUE_SIZE_INVALID;
        ^
ip-fifo.c:71:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return QUEUE_SIZE_INVALID;
        ^
ip-fifo.c:92:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return QUEUE_SIZE_INVALID;
        ^
ip-fifo.c:117:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return VALUE_NOT_WITHIN_AVERAGE;
        ^
ip-fifo.c:138:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return VALUE_NOT_WITHIN_AVERAGE;
        ^
ip-fifo.h:5:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.6]
#include <stdio.h>
^
3/6 files checked 87% done
Checking ip-telegraf.c ...
ip-telegraf.c:19:5: warning: %d in format string (no. 3) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
    sprintf(data, "temperature,device_id=%s,status=%d value=%d", device_name, status, value);
    ^
ip-telegraf.c:19:12: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
    sprintf(data, "temperature,device_id=%s,status=%d value=%d", device_name, status, value);
           ^
ip-telegraf.c:39:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[LOG DATA] Failed to send data. Error Code: %s\n", res);
                  ^
ip-telegraf.c:41:19: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
            printf("[LOG DATA] Sent log data.\n");
                  ^
ip-telegraf.c:1:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.6]
#include <stdio.h>
^
ip-telegraf.c:6:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.10]
#include <time.h>
^
4/6 files checked 92% done
Checking ip-wolfcrypt.c ...
ip-wolfcrypt.c:31:9: warning: %d in format string (no. 1) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
        printf("[ENCRYPT] RNG initialization failed! Error: %d\n", ret);
        ^
ip-wolfcrypt.c:38:9: warning: %d in format string (no. 1) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
        printf("[ENCRYPT] IV generation failed! Error: %d\n", ret);
        ^
ip-wolfcrypt.c:45:9: warning: %d in format string (no. 1) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
        printf("[ENCRYPT] AES key setup failed! Error: %d\n", ret);
        ^
ip-wolfcrypt.c:51:9: warning: %d in format string (no. 1) requires 'int' but the argument type is 'unsigned int'. [invalidPrintfArgType_sint]
        printf("[ENCRYPT] Encryption failed! Error: %d\n", ret);
        ^
ip-wolfcrypt.h:24:16: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.9]
static uint8_t encryption_key[KEY_SIZE] = {
               ^
ip-wolfcrypt.c:30:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (ret != 0) {
            ^
ip-wolfcrypt.c:37:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (ret != 0) {
            ^
ip-wolfcrypt.c:44:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (ret != 0) {
            ^
ip-wolfcrypt.c:50:13: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    if (ret != 0) {
            ^
ip-wolfcrypt.c:57:26: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    for (size_t i = 0; i < COMMAND_SIZE; i++) {
                         ^
ip-wolfcrypt.c:61:26: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    for (size_t i = 0; i < IV_SIZE; i++) {
                         ^
ip-wolfcrypt.c:65:26: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-10.4]
    for (size_t i = 0; i < TAG_SIZE; i++) {
                         ^
ip-wolfcrypt.c:58:21: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-13.3]
        dest[idx++] = ciphertext[i];
                    ^
ip-wolfcrypt.c:62:21: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-13.3]
        dest[idx++] = iv[i];
                    ^
ip-wolfcrypt.c:66:21: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-13.3]
        dest[idx++] = auth_tag[i];
                    ^
ip-wolfcrypt.c:16:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return ENCRYPTION_FAILED;
        ^
ip-wolfcrypt.c:32:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return ENCRYPTION_FAILED;
        ^
ip-wolfcrypt.c:39:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return ENCRYPTION_FAILED;
        ^
ip-wolfcrypt.c:46:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return ENCRYPTION_FAILED;
        ^
ip-wolfcrypt.c:52:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-15.5]
        return ENCRYPTION_FAILED;
        ^
ip-wolfcrypt.c:15:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ENCRYPT] Destination pointer is NULL\n");
              ^
ip-wolfcrypt.c:31:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ENCRYPT] RNG initialization failed! Error: %d\n", ret);
              ^
ip-wolfcrypt.c:38:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ENCRYPT] IV generation failed! Error: %d\n", ret);
              ^
ip-wolfcrypt.c:45:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ENCRYPT] AES key setup failed! Error: %d\n", ret);
              ^
ip-wolfcrypt.c:51:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[ENCRYPT] Encryption failed! Error: %d\n", ret);
              ^
ip-wolfcrypt.h:4:0: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-21.6]
#include <stdio.h>
^
5/6 files checked 98% done
Checking ip.c ...
ip.c:3:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-8.2]
int main()
        ^
ip.c:12:9: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.3]
    if (gattlib_mainloop(bluetooth_scan_devices, NULL) != GATTLIB_SUCCESS) {
        ^
ip.c:13:15: style: misra violation (use --rule-texts=<file> to get proper output) [misra-c2012-17.7]
        printf("[MAIN] Failed starting application.\n");
              ^
6/6 files checked 100% done
nofile:0:0: information: Unmatched suppression: misra-config [unmatchedSuppression]

nofile:0:0: information: Unmatched suppression: missingIncludeSystem [unmatchedSuppression]

nofile:0:0: information: Active checkers: 178/792 (use --checkers-report=<filename> to see details) [checkersReport]

Checking ESP32-DHT11-vf2.ino ...
ESP32-DHT11-vf2.ino:251:1: error: Code 'classIPServerCallbacks:' is invalid C code. Use --std or --language to configure the language. [syntaxError]
class IPServerCallbacks : public BLEServerCallbacks {
^
nofile:0:0: information: Unmatched suppression: misra-config [unmatchedSuppression]

nofile:0:0: information: Unmatched suppression: missingIncludeSystem [unmatchedSuppression]

nofile:0:0: information: Active checkers: There was critical errors (use --checkers-report=<filename> to see details) [checkersReport]
```
