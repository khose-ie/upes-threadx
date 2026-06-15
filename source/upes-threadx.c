/// @file    threadx.c
/// @brief   UPES OS abstraction layer implementation for ThreadX.
/// @details This source file provides the implementation of the UPES OS abstraction layer
///          using the ThreadX RTOS as the underlying operating system. It includes functions
///          for task management, event handling, message queues, memory pools, mutexes,
///          semaphores, and timers.
/// @author  Khose-ie<khose-ie@outlook.com>
/// @date    2024-06-10

#include <stdlib.h>
#include <tx_api.h>
#include <tx_block_pool.h>
#include <tx_byte_pool.h>
#include <tx_event_flags.h>
#include <tx_mutex.h>
#include <tx_queue.h>
#include <tx_semaphore.h>
#include <tx_thread.h>
#include <tx_timer.h>
#include <upes-os.h>
#include <upes-threadx.h>

#ifndef UPES_THREADX_OS_STACK_SIZE
#define UPES_THREADX_OS_STACK_SIZE (1024 * 60)
#endif // UPES_THREADX_OS_STACK_SIZE

/// @brief Size of the OS memory pool 1 blocks
/// @details This constant defines the size (in bytes) of each memory block
#ifndef UPES_THREADX_OS_MEM_POOL_BKSZ_1
#define UPES_THREADX_OS_MEM_POOL_BKSZ_1 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKSZ_1

/// @brief Count of the OS memory pool 1 blocks
/// @details This constant defines the number of memory blocks in the OS memory pool 1.
#ifndef UPES_THREADX_OS_MEM_POOL_BKCT_1
#define UPES_THREADX_OS_MEM_POOL_BKCT_1 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKCT_1

/// @brief Size of the OS memory pool 2 blocks
/// @details This constant defines the size (in bytes) of each memory block
#ifndef UPES_THREADX_OS_MEM_POOL_BKSZ_2
#define UPES_THREADX_OS_MEM_POOL_BKSZ_2 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKSZ_2

/// @brief Count of the OS memory pool 2 blocks
/// @details This constant defines the number of memory blocks in the OS memory pool 2.
#ifndef UPES_THREADX_OS_MEM_POOL_BKCT_2
#define UPES_THREADX_OS_MEM_POOL_BKCT_2 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKCT_2

/// @brief Size of the OS memory pool 3 blocks
/// @details This constant defines the size (in bytes) of each memory block
#ifndef UPES_THREADX_OS_MEM_POOL_BKSZ_3
#define UPES_THREADX_OS_MEM_POOL_BKSZ_3 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKSZ_3

/// @brief Count of the OS memory pool 3 blocks
/// @details This constant defines the number of memory blocks in the OS memory pool 3.
#ifndef UPES_THREADX_OS_MEM_POOL_BKCT_3
#define UPES_THREADX_OS_MEM_POOL_BKCT_3 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKCT_3

/// @brief Size of the OS memory pool 4 blocks
/// @details This constant defines the size (in bytes) of each memory block
#ifndef UPES_THREADX_OS_MEM_POOL_BKSZ_4
#define UPES_THREADX_OS_MEM_POOL_BKSZ_4 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKSZ_4

/// @brief Count of the OS memory pool 4 blocks
/// @details This constant defines the number of memory blocks in the OS memory pool 4.
#ifndef UPES_THREADX_OS_MEM_POOL_BKCT_4
#define UPES_THREADX_OS_MEM_POOL_BKCT_4 (0)
#endif // UPES_THREADX_OS_MEM_POOL_BKCT_4

/// @brief OS stack definition
/// @details This variable defines the byte pool used for the OS stack.
#if !defined(UPES_THREADX_OS_STACK_EX_MEM) || !defined(UPES_THREADX_OS_STACK_EX_MEM_SIZE)
#define EXT1              static
#define OS_STACK_MEM      (_os_stack_mem)
#define OS_STACK_MEM_SIZE (align32up(sizeof(threadxStack_t)))
#else // UPES_THREADX_OS_STACK_EX_MEM && UPES_THREADX_OS_STACK_EX_MEM_SIZE
#define EXT1              extern
#define OS_STACK_MEM      UPES_THREADX_OS_STACK_EX_MEM
#define OS_STACK_MEM_SIZE (UPES_THREADX_OS_STACK_EX_MEM_SIZE)
#endif // !defined(UPES_THREADX_OS_STACK_EX_MEM) || !defined(UPES_THREADX_OS_STACK_EX_MEM_SIZE)

/// @brief OS memory pool definition
/// @details This section defines the memory used for the OS memory pool.
#ifndef UPES_THREADX_OS_MEM_POOL_EX_MEM
#define EXT2            static
#define OS_MEM_POOL_MEM (_os_mem_pool_mem)
#else // UPES_THREADX_OS_MEM_POOL_EX_MEM
#define EXT2            extern
#define OS_MEM_POOL_MEM UPES_THREADX_OS_MEM_POOL_EX_MEM
#endif // UPES_THREADX_OS_MEM_POOL_EX_MEM

/// @brief Default time slice for tasks
/// @details This constant defines the default time slice (in ticks) for tasks.
#define OS_DEFAULT_TIME_SLICE (4)

/// @brief OS stack size
/// @details This constant defines the size (in bytes) of the OS stack.
#define OS_STACK_SIZE (UPES_THREADX_OS_STACK_SIZE)

/// @brief Required size of the OS stack
/// @details This constant defines the required size (in bytes) of the OS stack, calculated as the
/// aligned size of the threadxStack_t structure.
#define OS_STACK_REQUIRED_SIZE (align32up(sizeof(threadxStack_t)))

/// @brief OS memory pool indices
/// @details These constants define the indices for the OS memory pools.
#define OS_MEM_POOL_1   (0)
#define OS_MEM_POOL_2   (1)
#define OS_MEM_POOL_3   (2)
#define OS_MEM_POOL_4   (3)
#define OS_MEM_POOL_MAX (4)

/// @brief Sizes of individual OS memory pools
/// @details These constants define the sizes (in bytes) of each individual OS memory pool.
#define OS_MEM_POOL_MEM1_SIZE (UPES_THREADX_OS_MEM_POOL_BKSZ_1 * UPES_THREADX_OS_MEM_POOL_BKCT_1)
#define OS_MEM_POOL_MEM2_SIZE (UPES_THREADX_OS_MEM_POOL_BKSZ_2 * UPES_THREADX_OS_MEM_POOL_BKCT_2)
#define OS_MEM_POOL_MEM3_SIZE (UPES_THREADX_OS_MEM_POOL_BKSZ_3 * UPES_THREADX_OS_MEM_POOL_BKCT_3)
#define OS_MEM_POOL_MEM4_SIZE (UPES_THREADX_OS_MEM_POOL_BKSZ_4 * UPES_THREADX_OS_MEM_POOL_BKCT_4)

/// @brief Total size of the OS memory pool
/// @details This constant defines the total size (in bytes) of the OS memory pool, calculated as
/// the sum of the sizes of all memory pool blocks.
#define OS_MEM_POOL_SIZE                                                                           \
    (OS_MEM_POOL_MEM1_SIZE + OS_MEM_POOL_MEM2_SIZE + OS_MEM_POOL_MEM3_SIZE + OS_MEM_POOL_MEM4_SIZE)

/// @brief ThreadX control block
/// @details This structure holds the state and stack for the ThreadX OS.
typedef struct
{
    osState_t state;
    TX_BYTE_POOL stack;
    uint8_t stack_mem[OS_STACK_SIZE];
} threadxControlBlock_t;

/// @brief ThreadX memory pool control block
/// @details This structure holds the memory pool and its associated memory for the ThreadX OS.
typedef struct
{
    TX_BLOCK_POOL stack;
    uint8_t* mem;
} threadxMemPoolControlBlock_t;

/// @brief ThreadX stack structure
/// @details This structure holds the control block, memory pool control blocks, and stack memory
/// for the ThreadX OS.
typedef struct
{
    threadxControlBlock_t os;
    threadxMemPoolControlBlock_t mem_pool[OS_MEM_POOL_MAX];
} threadxStack_t;

/// @brief OS stack memory
/// @details This array defines the memory used for the OS stack.
EXT1 uint8_t OS_STACK_MEM[OS_STACK_REQUIRED_SIZE];

/// @brief OS memory pool memory
/// @details This array defines the memory used for the OS memory pool.
EXT2 uint8_t OS_MEM_POOL_MEM[OS_MEM_POOL_SIZE];

/// @brief ThreadX stack instance
/// @details This static pointer points to the instance of the ThreadX stack structure.
static threadxStack_t* _threadx = (threadxStack_t*)OS_STACK_MEM;

/// @brief Allocate memory from the OS stack byte pool
/// @details This function allocates a block of memory of the specified size
///          from the OS stack byte pool.
/// @param size Size of the memory block to allocate in bytes
/// @return Pointer to the allocated memory block, or NULL on failure
static uint8_t* _mem_alloc(uint32_t size)
{
    uint8_t* mem        = NULL;
    uint32_t alloc_size = size;

    if (size == 0)
    {
        return NULL;
    }

    if (size < TX_BYTE_BLOCK_MIN)
    {
        alloc_size = TX_BYTE_BLOCK_MIN;
    }

    if (tx_byte_allocate(&_threadx->os.stack, (VOID**)&mem, alloc_size, TX_NO_WAIT) != TX_SUCCESS)
    {
        return NULL;
    }

    return mem;
}

/// @brief Free memory back to the OS stack byte pool
/// @details This function frees a previously allocated block of memory
///          back to the OS stack byte pool.
/// @param mem Pointer to the memory block to free
static void _mem_free(uint8_t* mem)
{
    if (mem != NULL)
    {
        tx_byte_release((VOID*)mem);
    }
}

/// @brief Convert ThreadX task state to UPES task state
/// @param threadx_state ThreadX task state
/// @return Corresponding UPES task state
static osTaskState_t _convert_threadx_task_state(UINT threadx_state)
{
    osTaskState_t converted_state;

    switch (threadx_state)
    {
    case TX_READY:
        converted_state = OS_TASK_STATE_READY;
        break;

    case TX_COMPLETED:
    case TX_TERMINATED:
        converted_state = OS_TASK_STATE_TERMINATED;
        break;

    case TX_SUSPENDED:
    case TX_SLEEP:
    case TX_QUEUE_SUSP:
    case TX_SEMAPHORE_SUSP:
    case TX_EVENT_FLAG:
    case TX_BLOCK_MEMORY:
    case TX_BYTE_MEMORY:
    case TX_IO_DRIVER:
    case TX_FILE:
    case TX_TCP_IP:
    case TX_MUTEX_SUSP:
    case TX_PRIORITY_CHANGE:
        converted_state = OS_TASK_STATE_BLOCKED;
        break;

    default:
        return OS_TASK_STATE_UNKNOWN;
    }

    return converted_state;
}

/// @brief Convert ThreadX task priority to UPES task priority
/// @param threadx_priority ThreadX task priority
/// @return Corresponding UPES task priority
static osTaskPriority_t _convert_threadx_task_priority(UINT threadx_priority)
{
    if (threadx_priority > OS_TASK_PRIORITY_MAX)
    {
        return OS_TASK_PRIORITY_NONE;
    }

    return (osTaskPriority_t)(OS_TASK_PRIORITY_MAX - threadx_priority);
}

/// @brief Initialize the OS abstraction layer
/// @details This function initializes the OS abstraction layer by creating
///          the OS stack byte pool and setting the initial OS state.
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_initialize(void)
{
    if (OS_STACK_SIZE < OS_STACK_REQUIRED_SIZE)
    {
        return RET_VALUE_MEM_ALLOC_FAILURE;
    }

    memset(_threadx, 0, sizeof(*_threadx));

    _threadx->os.state = OS_STATE_INITIALIZING;

    if (tx_byte_pool_create(&_threadx->os.stack, "OS Stack", _threadx->os.stack_mem,
                            OS_STACK_SIZE) != TX_SUCCESS)
    {
        _threadx->os.state = OS_STATE_ERR_INIT_MEM;
        return RET_VALUE_OS_MEM_POOL_ERR;
    }

    _threadx->os.state = OS_STATE_RUNNING;
    return RET_VALUE_OK;
}

/// @brief Initialize the OS memory pool
/// @details This function initializes the OS memory pool used for dynamic
///          memory allocations within the OS abstraction layer.
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_initialize_mem_pool(void)
{
    threadxMemPoolControlBlock_t* mem_pool = NULL;

    memset(OS_MEM_POOL_MEM, 0, OS_MEM_POOL_SIZE);

#if OS_MEM_POOL_MEM1_SIZE != 0

    mem_pool      = &_threadx->mem_pool[OS_MEM_POOL_1];
    mem_pool->mem = OS_MEM_POOL_MEM;

    if (os_mem_pool_create_static("OS MemPool 1", &mem_pool->stack, sizeof(mem_pool->stack),
                                  mem_pool->mem, UPES_THREADX_OS_MEM_POOL_BKSZ_1,
                                  UPES_THREADX_OS_MEM_POOL_BKCT_1) == NULL)
    {
        return RET_VALUE_OS_MEM_POOL_ERR;
    }

#endif // OS_MEM_POOL_MEM1_SIZE != 0

#if OS_MEM_POOL_MEM2_SIZE != 0

    mem_pool      = &_threadx->mem_pool[OS_MEM_POOL_2];
    mem_pool->mem = OS_MEM_POOL_MEM + OS_MEM_POOL_MEM1_SIZE;

    if (os_mem_pool_create_static("OS MemPool 2", &mem_pool->stack, sizeof(mem_pool->stack),
                                  mem_pool->mem, UPES_THREADX_OS_MEM_POOL_BKSZ_2,
                                  UPES_THREADX_OS_MEM_POOL_BKCT_2) == NULL)
    {
        return RET_VALUE_OS_MEM_POOL_ERR;
    }

#endif // OS_MEM_POOL_MEM2_SIZE != 0

#if OS_MEM_POOL_MEM3_SIZE != 0

    mem_pool      = &_threadx->mem_pool[OS_MEM_POOL_3];
    mem_pool->mem = OS_MEM_POOL_MEM + OS_MEM_POOL_MEM1_SIZE + OS_MEM_POOL_MEM2_SIZE;

    if (os_mem_pool_create_static("OS MemPool 3", &mem_pool->stack, sizeof(mem_pool->stack),
                                  mem_pool->mem, UPES_THREADX_OS_MEM_POOL_BKSZ_3,
                                  UPES_THREADX_OS_MEM_POOL_BKCT_3) == NULL)
    {
        return RET_VALUE_OS_MEM_POOL_ERR;
    }

#endif // OS_MEM_POOL_MEM3_SIZE != 0

#if OS_MEM_POOL_MEM4_SIZE != 0

    mem_pool = &_threadx->mem_pool[OS_MEM_POOL_4];
    mem_pool->mem =
        OS_MEM_POOL_MEM + OS_MEM_POOL_MEM1_SIZE + OS_MEM_POOL_MEM2_SIZE + OS_MEM_POOL_MEM3_SIZE;

    if (os_mem_pool_create_static("OS MemPool 4", &mem_pool->stack, sizeof(mem_pool->stack),
                                  mem_pool->mem, UPES_THREADX_OS_MEM_POOL_BKSZ_4,
                                  UPES_THREADX_OS_MEM_POOL_BKCT_4) == NULL)
    {
        return RET_VALUE_OS_MEM_POOL_ERR;
    }

#endif // OS_MEM_POOL_MEM4_SIZE != 0

    return RET_VALUE_OK;
}

/// @brief Get the current state of the OS
/// @return Current OS state
osState_t os_state(void)
{
    return _threadx->os.state;
}

/// @brief Get the current OS tick count
/// @return Current OS tick count
uint32_t os_tick_count(void)
{
    return (uint32_t)tx_time_get();
}

/// @brief Get the current number of tasks
/// @return Current number of tasks
uint32_t os_task_count(void)
{
    uint32_t count        = 0;
    TX_THREAD* thread_ptr = _tx_thread_created_ptr;

    while (thread_ptr != NULL)
    {
        if (thread_ptr->tx_thread_id != TX_THREAD_ID)
        {
            count = 0;
            break;
        }

        if ((thread_ptr->tx_thread_created_next == thread_ptr) ||
            (thread_ptr->tx_thread_created_next == _tx_thread_created_ptr))
        {
            break;
        }

        count++;
        thread_ptr = thread_ptr->tx_thread_created_next;
    }

    return count;
}

/// @brief Get the handle of the current task
/// @return Handle of the current task
osTaskHandle_t os_current_task(void)
{
    return (osTaskHandle_t)tx_thread_identify();
}

/// @brief Delay the current task for a specified number of ticks
/// @details This function puts the current task into a blocked state for
///          the specified number of system ticks.
/// @param ticks Number of system ticks to delay
void os_delay(uint32_t ticks)
{
    if (ticks != 0)
    {
        tx_thread_sleep((ULONG)ticks);
    }
}

/// @brief Delay the current task until a specified time increment has passed
/// @details This function delays the current task until the specified time
///          increment has passed since the previous wake time.
/// @param ticks     Time increment in system ticks
void os_delay_interval(uint32_t ticks)
{
    os_delay(ticks - os_tick_count());
}

/// @brief Yield the processor to another ready task
/// @details This function allows the current task to yield the processor,
///          allowing other ready tasks to run.
void os_switch_task(void)
{
    tx_thread_relinquish();
}

/// @brief  Exit the current task
/// @details This function terminates the execution of the current task.
void os_exit_task(void)
{
    os_task_delete(os_current_task());
    while (1);
}

/// @brief  Exit the current task created with static stack allocation
/// @details This function terminates the execution of the current task created with static stack
void os_exit_task_static(void)
{
    os_task_delete_static(os_current_task());
}

/// @brief  Allocate memory from the OS memory pool
/// @details This function allocates a block of memory of the specified size from the OS memory
///          pool.
///.         The os implementation will choice the best fit memory block from the pool.
/// @param size Size of memory want to allocate in bytes
/// @return Pointer to the allocated memory block, or NULL on failure
void* os_malloc(uint32_t size)
{
    void* mem = NULL;

    if ((_threadx->mem_pool[OS_MEM_POOL_1].mem != NULL) &&
        (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_1))
    {
        mem = os_mem_pool_alloc(&_threadx->mem_pool[OS_MEM_POOL_1].stack, size);
    }
    else if ((_threadx->mem_pool[OS_MEM_POOL_2].mem != NULL) &&
             (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_2))
    {
        mem = os_mem_pool_alloc(&_threadx->mem_pool[OS_MEM_POOL_2].stack, size);
    }
    else if ((_threadx->mem_pool[OS_MEM_POOL_3].mem != NULL) &&
             (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_3))
    {
        mem = os_mem_pool_alloc(&_threadx->mem_pool[OS_MEM_POOL_3].stack, size);
    }
    else if ((_threadx->mem_pool[OS_MEM_POOL_4].mem != NULL) &&
             (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_4))
    {
        mem = os_mem_pool_alloc(&_threadx->mem_pool[OS_MEM_POOL_4].stack, size);
    }

    return mem;
}

/// @brief  Free memory back to the OS memory pool
/// @details This function frees a previously allocated block of memory back to the OS memory pool.
/// @param mem Pointer to the memory block to free
/// @param size Size of the memory to free in bytes
void os_free(void* mem, uint32_t size)
{
    if (mem != NULL)
    {
        if ((_threadx->mem_pool[OS_MEM_POOL_1].mem != NULL) &&
            (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_1))
        {
            os_mem_pool_free(&_threadx->mem_pool[OS_MEM_POOL_1].stack, mem);
        }
        else if ((_threadx->mem_pool[OS_MEM_POOL_2].mem != NULL) &&
                 (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_2))
        {
            os_mem_pool_free(&_threadx->mem_pool[OS_MEM_POOL_2].stack, mem);
        }
        else if ((_threadx->mem_pool[OS_MEM_POOL_3].mem != NULL) &&
                 (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_3))
        {
            os_mem_pool_free(&_threadx->mem_pool[OS_MEM_POOL_3].stack, mem);
        }
        else if ((_threadx->mem_pool[OS_MEM_POOL_4].mem != NULL) &&
                 (size <= UPES_THREADX_OS_MEM_POOL_BKSZ_4))
        {
            os_mem_pool_free(&_threadx->mem_pool[OS_MEM_POOL_4].stack, mem);
        }
    }
}

/// @brief  Create a new event object
/// @details This function creates a new event object with the specified name.
/// @param name Name of the event object
/// @return Handle to the created event object or NULL on failure
osEventHandle_t os_event_create(const char* name)
{
    TX_EVENT_FLAGS_GROUP* event = (TX_EVENT_FLAGS_GROUP*)_mem_alloc(sizeof(TX_EVENT_FLAGS_GROUP));

    if (event == NULL)
    {
        return NULL;
    }

    if (tx_event_flags_create(event, (CHAR*)name) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)event);
        return NULL;
    }

    return (osEventHandle_t)event;
}

/// @brief  Delete an event object
/// @details This function deletes the specified event object and frees its resources.
/// @param event Handle to the event object to be deleted
void os_event_delete(osEventHandle_t event)
{
    if (event != NULL)
    {
        tx_event_flags_delete((TX_EVENT_FLAGS_GROUP*)event);
        _mem_free((uint8_t*)event);
    }
}

/// @brief  Get the name of an event object
/// @details This function retrieves the name of the specified event object.
/// @param event Handle to the event object
/// @return Pointer to the event object's name string
const char* os_event_name(osEventHandle_t event)
{
    char* name = NULL;

    if (event == NULL)
    {
        return NULL;
    }

    if (tx_event_flags_info_get((TX_EVENT_FLAGS_GROUP*)event, (CHAR**)&name, NULL, NULL, NULL,
                                NULL) != TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the current state of an event object
/// @details This function retrieves the current state (flags) of the specified event object.
/// @param event Handle to the event object
/// @return Current state (flags) of the event object
uint32_t os_event_state(osEventHandle_t event)
{
    uint32_t events_state = 0;

    if (event == NULL)
    {
        return 0;
    }

    if (((TX_EVENT_FLAGS_GROUP*)event)->tx_event_flags_group_id != TX_EVENT_FLAGS_ID)
    {
        return 0;
    }

    if (tx_event_flags_info_get((TX_EVENT_FLAGS_GROUP*)event, NULL, &events_state, NULL, NULL,
                                NULL) != TX_SUCCESS)
    {
        return 0;
    }

    return events_state;
}

/// @brief  Put event flags
/// @details This function puts the specified flags in the event object.
/// @param event Handle to the event object
/// @param flags Flags to be putted
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_event_put(osEventHandle_t event, uint32_t flags)
{
    uint32_t events_state;

    if (event == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_EVENT_FLAGS_GROUP*)event)->tx_event_flags_group_id != TX_EVENT_FLAGS_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_event_flags_set((TX_EVENT_FLAGS_GROUP*)event, (ULONG)flags, TX_OR) != TX_SUCCESS)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    if (tx_event_flags_info_get((TX_EVENT_FLAGS_GROUP*)event, NULL, &events_state, NULL, NULL,
                                NULL) != TX_SUCCESS)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Wait for event flags
/// @details This function waits for the specified flags to be set in the event object.
/// @param event        Handle to the event object
/// @param events_value Flags to wait for
/// @param timeout      Timeout in milliseconds to wait (0 for no wait, OS_WAIT_FOREVER for
/// infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_event_wait(osEventHandle_t event, uint32_t events_value, uint32_t timeout)
{
    ULONG event_value;

    if ((event == NULL) || (events_value == OS_EVENT_NONE))
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_EVENT_FLAGS_GROUP*)event)->tx_event_flags_group_id != TX_EVENT_FLAGS_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_event_flags_get((TX_EVENT_FLAGS_GROUP*)event, (ULONG)events_value, TX_OR, &event_value,
                           (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    if ((event_value & events_value) == OS_EVENT_NONE)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Clear event flags
/// @details This function clears the specified flags in the event object.
/// @param events_value Handle to the event object
/// @param flags Flags to be cleared
void os_event_clear(osEventHandle_t event, uint32_t events_value)
{
    if ((event != NULL) && (events_value != OS_EVENT_NONE) &&
        (((TX_EVENT_FLAGS_GROUP*)event)->tx_event_flags_group_id == TX_EVENT_FLAGS_ID))
    {
        tx_event_flags_set((TX_EVENT_FLAGS_GROUP*)event, (ULONG)events_value, TX_AND);
    }
}

/// @brief  Wait for event flags and clear them
/// @details This function waits for the specified flags to be set in the event object and clears
/// them upon wakeup.
/// @param event             Handle to the event object
/// @param events_value      Flags to wait for
/// @param out_events_value  Pointer to store the flags that caused the wakeup
/// @param timeout           Timeout in milliseconds to wait (0 for no wait, OS_WAIT_FOREVER
/// for infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_event_wait_and_clear(osEventHandle_t event, uint32_t events_value,
                                   uint32_t* out_events_value, uint32_t timeout)
{
    ULONG event_value;

    if ((event == NULL) || (events_value == OS_EVENT_NONE))
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_EVENT_FLAGS_GROUP*)event)->tx_event_flags_group_id != TX_EVENT_FLAGS_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_event_flags_get((TX_EVENT_FLAGS_GROUP*)event, (ULONG)events_value, TX_OR, &event_value,
                           (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    if ((event_value & events_value) == OS_EVENT_NONE)
    {
        return RET_VALUE_OS_EVENT_ERR;
    }

    if (out_events_value != NULL)
    {
        *out_events_value = (uint32_t)(event_value & events_value);
    }

    os_event_clear(event, *out_events_value);

    return RET_VALUE_OK;
}

/// @brief  Create a new message queue
/// @details This function creates a new message queue with the specified parameters.
/// @param name          Name of the message queue
/// @param message_size  Size of each message in bytes
/// @param message_count Maximum number of messages the queue can hold
/// @return Handle to the created message queue or NULL on failure
osMessageQueueHandle_t os_message_queue_create(const char* name, uint32_t message_size,
                                               uint32_t message_count)
{
    TX_QUEUE* queue               = (TX_QUEUE*)_mem_alloc(sizeof(TX_QUEUE));
    uint32_t aligned_message_size = message_size % sizeof(ULONG) == 0
                                        ? message_size
                                        : ((message_size / sizeof(ULONG)) + 1) * sizeof(ULONG);

    if (queue == NULL)
    {
        return NULL;
    }

    VOID* queue_start = (VOID*)_mem_alloc(aligned_message_size * message_count);

    if (queue_start == NULL)
    {
        _mem_free((uint8_t*)queue);
        return NULL;
    }

    if (tx_queue_create(queue, (CHAR*)name, aligned_message_size / sizeof(ULONG), queue_start,
                        message_count) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)queue);
        _mem_free((uint8_t*)queue_start);
        return NULL;
    }

    return (osMessageQueueHandle_t)queue;
}

/// @brief  Create a new message queue with static buffer
/// @details This function creates a new message queue with the specified parameters,
///     using a statically allocated message buffer.
/// @param name           Name of the message queue
/// @param message_buffer Pointer to the statically allocated message buffer
/// @param message_size   Size of each message in bytes
/// @param message_count  Maximum number of messages the queue can hold
/// @return Handle to the created message queue
osMessageQueueHandle_t os_message_queue_create_static(const char* name, uint8_t* message_buffer,
                                                      uint32_t message_size, uint32_t message_count)
{
    TX_QUEUE* queue = (TX_QUEUE*)_mem_alloc(sizeof(TX_QUEUE));

    if (queue == NULL)
    {
        return NULL;
    }

    if (tx_queue_create(queue, (CHAR*)name, message_size, (VOID*)message_buffer, message_count) !=
        TX_SUCCESS)
    {
        _mem_free((uint8_t*)queue);
        return NULL;
    }

    return (osMessageQueueHandle_t)queue;
}

/// @brief  Delete a message queue
/// @details This function deletes the specified message queue and frees its resources.
/// @param queue Handle to the message queue to be deleted
void os_message_queue_delete(osMessageQueueHandle_t queue)
{
    if (queue != NULL)
    {
        tx_queue_delete((TX_QUEUE*)queue);
        _mem_free((uint8_t*)queue);
        _mem_free((uint8_t*)((TX_QUEUE*)queue)->tx_queue_start);
    }
}

/// @brief  Delete a message queue created with static buffer
/// @details This function deletes the specified message queue created with a static buffer.
/// @param queue Handle to the message queue to be deleted
void os_message_queue_delete_static(osMessageQueueHandle_t queue)
{
    if (queue != NULL)
    {
        tx_queue_delete((TX_QUEUE*)queue);
        _mem_free((uint8_t*)queue);
    }
}

/// @brief  Get the name of a message queue
/// @param queue Handle to the message queue
/// @return Pointer to the message queue's name string
const char* os_message_queue_name(osMessageQueueHandle_t queue)
{
    char* name = NULL;

    if (queue == NULL)
    {
        return NULL;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return NULL;
    }

    if (tx_queue_info_get((TX_QUEUE*)queue, (CHAR**)&name, NULL, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the size of a message in the queue
/// @param queue Handle to the message queue
/// @return Size of each message in bytes
uint32_t os_message_queue_message_size(osMessageQueueHandle_t queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return 0;
    }

    return ((TX_QUEUE*)queue)->tx_queue_message_size * sizeof(ULONG);
}

/// @brief  Get the number of messages currently in the queue
/// @param queue Handle to the message queue
/// @return Number of messages currently in the queue
uint32_t os_message_queue_message_count(osMessageQueueHandle_t queue)
{
    ULONG queued_messages_number = 0U;

    if (queue == NULL)
    {
        return 0;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return 0;
    }

    if (tx_queue_info_get((TX_QUEUE*)queue, NULL, &queued_messages_number, NULL, NULL, NULL,
                          NULL) != TX_SUCCESS)
    {
        return 0;
    }

    return (uint32_t)queued_messages_number;
}

/// @brief  Get the maximum number of messages the queue can hold
/// @param queue Handle to the message queue
/// @return Maximum number of messages the queue can hold
uint32_t os_message_queue_max_message_count(osMessageQueueHandle_t queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return 0;
    }

    return (uint32_t)((TX_QUEUE*)queue)->tx_queue_capacity;
}

/// @brief  Receive a message from the queue
/// @details This function receives a message from the specified message queue.
/// @param queue    Handle to the message queue
/// @param message  Pointer to the buffer to store the received message
/// @param timeout  Timeout in milliseconds to wait for a message (0 for no wait, UINT32_MAX for
/// infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_message_queue_send(osMessageQueueHandle_t queue, const void* message,
                                 uint32_t timeout)
{
    if ((queue == NULL) || (message == NULL))
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_queue_send((TX_QUEUE*)queue, (VOID*)message, (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MQ_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Receive a message from the queue
/// @details This function receives a message from the specified message queue.
/// @param queue    Handle to the message queue
/// @param message  Pointer to the buffer to store the received message
/// @param timeout  Timeout in milliseconds to wait for a message (0 for no wait,
/// OS_WAIT_FOREVER for infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_message_queue_receive(osMessageQueueHandle_t queue, void* message, uint32_t timeout)
{
    if ((queue == NULL) || (message == NULL))
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_QUEUE*)queue)->tx_queue_id != TX_QUEUE_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_queue_receive((TX_QUEUE*)queue, message, (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MQ_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Clear all messages from the queue
/// @details This function clears all messages currently in the specified message queue.
/// @param queue Handle to the message queue
void os_message_queue_clear(osMessageQueueHandle_t queue)
{
    if ((queue != NULL) && (((TX_QUEUE*)queue)->tx_queue_id == TX_QUEUE_ID))
    {
        tx_queue_flush((TX_QUEUE*)queue);
    }
}

/// @brief  Create a new memory pool
/// @details This function creates a new memory pool with the specified parameters.
/// @param name        Name of the memory pool
/// @param block_size  Size of each memory block in bytes
/// @param block_count Number of memory blocks in the pool
/// @return Handle to the created memory pool or NULL on failure
osMemPoolHandle_t os_mem_pool_create(const char* name, uint32_t block_size, uint32_t block_count)
{
    uint32_t aligned_block_size = block_size % sizeof(ULONG) == 0
                                      ? block_size
                                      : ((block_size / sizeof(ULONG)) + 1) * sizeof(ULONG);

    TX_BLOCK_POOL* pool = (TX_BLOCK_POOL*)_mem_alloc(sizeof(TX_BLOCK_POOL));

    if (pool == NULL)
    {
        return NULL;
    }

    VOID* pool_zone = _mem_alloc(aligned_block_size * block_count);

    if (pool_zone == NULL)
    {
        _mem_free((uint8_t*)pool);
        return NULL;
    }

    if (tx_block_pool_create(pool, (CHAR*)name, aligned_block_size, pool_zone,
                             aligned_block_size * block_count) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)pool);
        _mem_free((uint8_t*)pool_zone);
        return NULL;
    }

    return (osMemPoolHandle_t)pool;
}

/// @brief  Create a new memory pool with static buffer
/// @details This function creates a new memory pool with the specified parameters,
///     using a statically allocated pool buffer.
/// @param name        Name of the memory pool
/// @param pool_buffer Pointer to the statically allocated pool buffer
/// @param block_size  Size of each memory block in bytes
/// @param block_count Number of memory blocks in the pool
/// @return Handle to the created memory pool or NULL on failure
osMemPoolHandle_t os_mem_pool_create_static(const char* name, void* pool, uint32_t pool_size,
                                            uint8_t* pool_mem, uint32_t block_size,
                                            uint32_t block_count)
{
    if ((pool == NULL) || (pool_size < sizeof(TX_BLOCK_POOL)) || (pool_mem == NULL) ||
        (block_size == 0) || (block_size % sizeof(ULONG) != 0) || (block_count == 0))
    {
        return NULL;
    }

    if (tx_block_pool_create((TX_BLOCK_POOL*)pool, (CHAR*)name, block_size, (VOID*)pool_mem,
                             block_size * block_count) != TX_SUCCESS)
    {
        return NULL;
    }

    return (osMemPoolHandle_t)pool;
}

/// @brief  Delete a memory pool
/// @details This function deletes the specified memory pool and frees its resources.
/// @param pool Handle to the memory pool to be deleted
void os_mem_pool_delete(osMemPoolHandle_t pool)
{
    VOID* pool_zone = ((TX_BLOCK_POOL*)pool)->tx_block_pool_start;

    if ((pool != NULL) && (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID))
    {
        tx_block_pool_delete((TX_BLOCK_POOL*)pool);
        _mem_free((uint8_t*)pool);
        _mem_free((uint8_t*)pool_zone);
    }
}

/// @brief  Delete a memory pool created with static buffer
/// @details This function deletes the specified memory pool created with a static buffer.
/// @param pool Handle to the memory pool to be deleted
void os_mem_pool_delete_static(osMemPoolHandle_t pool)
{
    if ((pool != NULL) && (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID))
    {
        tx_block_pool_delete((TX_BLOCK_POOL*)pool);
    }
}

/// @brief  Get the name of a memory pool
/// @param pool Handle to the memory pool
/// @return Pointer to the memory pool's name string
const char* os_mem_pool_name(osMemPoolHandle_t pool)
{
    char* name = NULL;

    if (pool == NULL)
    {
        return NULL;
    }

    if (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID)
    {
        return NULL;
    }

    if (tx_block_pool_info_get((TX_BLOCK_POOL*)pool, (CHAR**)&name, NULL, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the size of each memory block in the pool
/// @param pool Handle to the memory pool
/// @return Size of each memory block in bytes
uint32_t os_mem_pool_block_size(osMemPoolHandle_t pool)
{
    if (pool == NULL)
    {
        return 0;
    }

    if (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID)
    {
        return 0;
    }

    return (uint32_t)((TX_BLOCK_POOL*)pool)->tx_block_pool_block_size;
}

/// @brief  Get the number of used memory blocks in the pool
/// @param pool Handle to the memory pool
/// @return Number of memory blocks in the pool
uint32_t os_mem_pool_block_count(osMemPoolHandle_t pool)
{
    ULONG available_blocks = 0U;
    ULONG total_blocks     = 0U;

    if (pool == NULL)
    {
        return 0;
    }

    if (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID)
    {
        return 0;
    }

    if (tx_block_pool_info_get((TX_BLOCK_POOL*)pool, NULL, &available_blocks, &total_blocks, NULL,
                               NULL, NULL) != TX_SUCCESS)
    {
        return 0;
    }

    return (uint32_t)(total_blocks - available_blocks);
}

/// @brief  Get the maximum number of memory blocks in the pool
/// @param pool Handle to the memory pool
/// @return Maximum number of memory blocks in the pool
uint32_t os_mem_pool_max_block_count(osMemPoolHandle_t pool)
{
    ULONG total_blocks = 0U;

    if (pool == NULL)
    {
        return 0;
    }

    if (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID)
    {
        return 0;
    }

    if (tx_block_pool_info_get((TX_BLOCK_POOL*)pool, NULL, NULL, &total_blocks, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return 0;
    }

    return (uint32_t)total_blocks;
}

/// @brief  Allocate a memory block from the pool
/// @details This function allocates a memory block from the specified memory pool.
/// @param pool    Handle to the memory pool
/// @param timeout Timeout in milliseconds to wait for a memory block (0 for no wait,
/// OS_WAIT_FOREVER for infinite wait)
/// @return Pointer to the allocated memory block, or NULL on failure
void* os_mem_pool_alloc(osMemPoolHandle_t pool, uint32_t timeout)
{
    VOID* block = NULL;

    if (pool == NULL)
    {
        return NULL;
    }

    if (((TX_BLOCK_POOL*)pool)->tx_block_pool_id != TX_BLOCK_POOL_ID)
    {
        return NULL;
    }

    if (tx_block_allocate((TX_BLOCK_POOL*)pool, &block, (ULONG)timeout) != TX_SUCCESS)
    {
        return NULL;
    }

    return block;
}

/// @brief  Free a memory block back to the pool
/// @details This function frees a previously allocated memory block back to the specified memory
/// pool.
/// @param pool  Handle to the memory pool
/// @param block Pointer to the memory block to be freed
void os_mem_pool_free(osMemPoolHandle_t pool, void* block)
{
    if ((pool != NULL) && (block != NULL) &&
        (((TX_BLOCK_POOL*)pool)->tx_block_pool_id == TX_BLOCK_POOL_ID))
    {
        tx_block_release(block);
    }
}

/// @brief  Create a new mutex
/// @details This function creates a new mutex with the specified name.
/// @param name Name of the mutex
/// @return Handle to the created mutex or NULL on failure
osMutexHandle_t os_mutex_create(const char* name)
{
    TX_MUTEX* mutex = (TX_MUTEX*)_mem_alloc(sizeof(TX_MUTEX));

    if (mutex == NULL)
    {
        return NULL;
    }

    if (tx_mutex_create(mutex, (CHAR*)name, TX_INHERIT) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)mutex);
        return NULL;
    }

    return (osMutexHandle_t)mutex;
}

/// @brief  Delete a mutex
/// @details This function deletes the specified mutex and frees its resources.
/// @param mutex Handle to the mutex to be deleted
void os_mutex_delete(osMutexHandle_t mutex)
{
    if (mutex != NULL)
    {
        tx_mutex_delete((TX_MUTEX*)mutex);
        _mem_free((uint8_t*)mutex);
    }
}

/// @brief  Get the name of a mutex
/// @param mutex Handle to the mutex
/// @return Pointer to the mutex's name string
const char* os_mutex_name(osMutexHandle_t mutex)
{
    char* name = NULL;

    if (mutex == NULL)
    {
        return NULL;
    }

    if (((TX_MUTEX*)mutex)->tx_mutex_id != TX_MUTEX_ID)
    {
        return NULL;
    }

    if (tx_mutex_info_get((TX_MUTEX*)mutex, (CHAR**)&name, NULL, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the owner of a mutex
/// @details This function retrieves the handle of the task that currently owns the specified
/// mutex.
/// @param mutex Handle to the mutex
/// @return Handle to the task that currently owns the mutex
osTaskHandle_t os_mutex_owner(osMutexHandle_t mutex)
{
    TX_THREAD* owner = NULL;

    if (mutex == NULL)
    {
        return NULL;
    }

    if (((TX_MUTEX*)mutex)->tx_mutex_id != TX_MUTEX_ID)
    {
        return NULL;
    }

    if (tx_mutex_info_get((TX_MUTEX*)mutex, NULL, NULL, &owner, NULL, NULL, NULL) != TX_SUCCESS)
    {
        return NULL;
    }

    return (osTaskHandle_t)owner;
}

/// @brief  Lock a mutex
/// @details This function locks the specified mutex, blocking the calling task if necessary.
/// @param mutex   Handle to the mutex
/// @param timeout Timeout in milliseconds to wait for the mutex (0 for no wait,
/// OS_WAIT_FOREVER for infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_mutex_lock(osMutexHandle_t mutex, uint32_t timeout)
{
    if (mutex == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_MUTEX*)mutex)->tx_mutex_id != TX_MUTEX_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_mutex_get((TX_MUTEX*)mutex, (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Unlock a mutex
/// @details This function unlocks the specified mutex.
/// @param mutex Handle to the mutex
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_mutex_unlock(osMutexHandle_t mutex)
{
    if (mutex == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_MUTEX*)mutex)->tx_mutex_id != TX_MUTEX_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_mutex_put((TX_MUTEX*)mutex) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Create a new semaphore
/// @details This function creates a new semaphore with the specified parameters.
/// @param name          Name of the semaphore
/// @param max_count     Maximum count of the semaphore
/// @return Handle to the created semaphore, or NULL on failure
osSemaphoreHandle_t os_semaphore_create(const char* name, uint32_t max_count)
{
    TX_SEMAPHORE* semaphore = (TX_SEMAPHORE*)_mem_alloc(sizeof(TX_SEMAPHORE));

    if (semaphore == NULL)
    {
        return NULL;
    }

    if (tx_semaphore_create(semaphore, (CHAR*)name, (ULONG)max_count) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)semaphore);
        return NULL;
    }

    return (osSemaphoreHandle_t)semaphore;
}

/// @brief  Delete a semaphore
/// @details This function deletes the specified semaphore and frees its resources.
/// @param semaphore Handle to the semaphore to be deleted
void os_semaphore_delete(osSemaphoreHandle_t semaphore)
{
    if (semaphore != NULL)
    {
        tx_semaphore_delete((TX_SEMAPHORE*)semaphore);
        _mem_free((uint8_t*)semaphore);
    }
}

/// @brief  Get the name of a semaphore
/// @param semaphore Handle to the semaphore
/// @return Pointer to the semaphore's name string
const char* os_semaphore_name(osSemaphoreHandle_t semaphore)
{
    char* name = NULL;

    if (semaphore == NULL)
    {
        return NULL;
    }

    if (((TX_SEMAPHORE*)semaphore)->tx_semaphore_id != TX_SEMAPHORE_ID)
    {
        return NULL;
    }

    if (tx_semaphore_info_get((TX_SEMAPHORE*)semaphore, (CHAR**)&name, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the current count of a semaphore
/// @param semaphore Handle to the semaphore
/// @return Current count of the semaphore
uint32_t os_semaphore_count(osSemaphoreHandle_t semaphore)
{
    ULONG current_count = 0U;

    if (semaphore == NULL)
    {
        return 0;
    }

    if (((TX_SEMAPHORE*)semaphore)->tx_semaphore_id != TX_SEMAPHORE_ID)
    {
        return 0;
    }

    if (tx_semaphore_info_get((TX_SEMAPHORE*)semaphore, NULL, &current_count, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return 0;
    }

    return (uint32_t)current_count;
}

/// @brief  Take (decrement) a semaphore
/// @details This function takes (decrements) the specified semaphore, blocking the calling task
/// if necessary.
/// @param semaphore Handle to the semaphore
/// @param timeout   Timeout in milliseconds to wait for the semaphore (0 for no wait,
/// OS_WAIT_FOREVER for infinite wait)
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_semaphore_take(osSemaphoreHandle_t semaphore, uint32_t timeout)
{
    if (semaphore == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_SEMAPHORE*)semaphore)->tx_semaphore_id != TX_SEMAPHORE_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_semaphore_get((TX_SEMAPHORE*)semaphore, (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_SEMAPHORE_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Release (increment) a semaphore
/// @details This function releases (increments) the specified semaphore.
/// @param semaphore Handle to the semaphore
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_semaphore_release(osSemaphoreHandle_t semaphore)
{
    if (semaphore == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_SEMAPHORE*)semaphore)->tx_semaphore_id != TX_SEMAPHORE_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (tx_semaphore_put((TX_SEMAPHORE*)semaphore) != TX_SUCCESS)
    {
        return RET_VALUE_OS_SEMAPHORE_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Create a new task
/// @details This function creates a new task with the specified parameters.
///     The task will start executing the provided main function with the given argument.
/// @param name        Name of the task
/// @param main        Pointer to the task's main function
/// @param arg         Argument to be passed to the task's main function
/// @param stack_size  Size of the task's stack in bytes
/// @param priority    Priority of the task (use OS_TASK_PRIORITY_* constants)
/// @return Handle to the created task, or NULL on failure
osTaskHandle_t os_task_create(const char* name, void (*main)(void*), void* arg, uint32_t stack_size,
                              osTaskPriority_t priority)
{
    if ((main == NULL) || (stack_size == 0) || (stack_size < TX_MINIMUM_STACK))
    {
        return NULL;
    }

    TX_THREAD* thread = (TX_THREAD*)_mem_alloc(sizeof(TX_THREAD));

    if (thread == NULL)
    {
        return NULL;
    }

    VOID* stack = _mem_alloc(stack_size);

    if (stack == NULL)
    {
        _mem_free((uint8_t*)thread);
        return NULL;
    }

    UINT converted_priority = (UINT)(OS_TASK_PRIORITY_MAX - priority);

    if (tx_thread_create(thread, (CHAR*)name, (void (*)(ULONG))main, (ULONG)arg, stack, stack_size,
                         (UINT)converted_priority, converted_priority, OS_DEFAULT_TIME_SLICE,
                         TX_AUTO_START) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)stack);
        _mem_free((uint8_t*)thread);
        return NULL;
    }

    return (osTaskHandle_t)thread;
}

/// @brief  Create a new task with static stack allocation
/// @details This function creates a new task with the specified parameters.
///     The task will start executing the provided main function with the given argument.
/// @param name        Name of the task
/// @param main        Pointer to the task's main function
/// @param arg         Argument to be passed to the task's main function
/// @param stack       Pointer to the static stack buffer
/// @param stack_size  Size of the task's stack in bytes
/// @param priority    Priority of the task (use OS_TASK_PRIORITY_* constants)
/// @return Handle to the created task, or NULL on failure
osTaskHandle_t os_task_create_static(const char* name, void (*main)(void*), void* arg,
                                     uint8_t* stack, uint32_t stack_size, osTaskPriority_t priority)
{
    if ((main == NULL) || (stack == NULL) || (stack_size == 0) || (stack_size < TX_MINIMUM_STACK))
    {
        return NULL;
    }

    TX_THREAD* thread = (TX_THREAD*)_mem_alloc(sizeof(TX_THREAD));

    if (thread == NULL)
    {
        return NULL;
    }

    UINT converted_priority = (UINT)(OS_TASK_PRIORITY_MAX - priority);

    if (tx_thread_create(thread, (CHAR*)name, (void (*)(ULONG))main, (ULONG)arg, (VOID*)stack,
                         stack_size, (UINT)converted_priority, converted_priority,
                         OS_DEFAULT_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)thread);
        return NULL;
    }

    return (osTaskHandle_t)thread;
}

/// @brief  Delete a task
/// @details This function deletes the specified task and frees its resources.
/// @param task Handle to the task to be deleted
void os_task_delete(osTaskHandle_t task)
{
    VOID* stack = ((TX_THREAD*)task)->tx_thread_stack_start;

    if ((task != NULL) && (((TX_THREAD*)task)->tx_thread_id == TX_THREAD_ID))
    {
        if (tx_thread_terminate((TX_THREAD*)task) == TX_SUCCESS)
        {
            tx_thread_delete((TX_THREAD*)task);
            _mem_free((uint8_t*)stack);
            _mem_free((uint8_t*)task);
        }
    }
}

/// @brief  Delete a task created with static stack allocation
/// @details This function deletes the specified task created with static stack allocation.
/// @param task Handle to the task to be deleted
void os_task_delete_static(osTaskHandle_t task)
{
    if ((task != NULL) && (((TX_THREAD*)task)->tx_thread_id == TX_THREAD_ID))
    {
        if (tx_thread_terminate((TX_THREAD*)task) == TX_SUCCESS)
        {
            tx_thread_delete((TX_THREAD*)task);
            _mem_free((uint8_t*)task);
        }
    }
}

/// @brief  Get the name of a task
/// @param task Handle to the task control block
/// @return Pointer to the task's name string
const char* os_task_name(osTaskHandle_t task)
{
    char* name = NULL;

    if (task == NULL)
    {
        return NULL;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return NULL;
    }

    if (tx_thread_info_get((TX_THREAD*)task, (CHAR**)&name, NULL, NULL, NULL, NULL, NULL, NULL,
                           NULL) != TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the current state of a task
/// @param task Handle to the task control block
/// @return Current state of the task
uint32_t os_task_stack_size(osTaskHandle_t task)
{
    if (task == NULL)
    {
        return 0;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return 0;
    }

    return (uint32_t)((TX_THREAD*)task)->tx_thread_stack_size;
}

/// @brief  Get the priority of a task
/// @param task Handle to the task control block
/// @return Priority of the task
osTaskPriority_t os_task_priority(osTaskHandle_t task)
{
    if (task == NULL)
    {
        return OS_TASK_PRIORITY_NONE;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return OS_TASK_PRIORITY_NONE;
    }

    UINT priority = OS_TASK_PRIORITY_NONE;

    if (tx_thread_info_get((TX_THREAD*)task, NULL, NULL, NULL, &priority, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return OS_TASK_PRIORITY_NONE;
    }

    return _convert_threadx_task_priority(priority);
}

/// @brief  Get the current state of a task
/// @param task Handle to the task control block
/// @return Current state of the task
osTaskState_t os_task_state(osTaskHandle_t task)
{
    UINT state;

    if (task == NULL)
    {
        return OS_TASK_STATE_UNKNOWN;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return OS_TASK_STATE_UNKNOWN;
    }

    if (os_current_task() == task)
    {
        return OS_TASK_STATE_RUNNING;
    }

    if (tx_thread_info_get((TX_THREAD*)task, NULL, &state, NULL, NULL, NULL, NULL, NULL, NULL) !=
        TX_SUCCESS)
    {
        return OS_TASK_STATE_UNKNOWN;
    }

    return _convert_threadx_task_state(state);
}

/// @brief  Set the priority of a task
/// @param task     Handle to the task control block
/// @param priority New priority to be set for the task
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_task_set_priority(osTaskHandle_t task, osTaskPriority_t priority)
{
    UINT current_priority;
    UINT converted_priority;

    if (task == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return RET_VALUE_INSTANCE_UNAVAILABLE;
    }

    converted_priority = (UINT)(OS_TASK_PRIORITY_MAX - priority);

    if (tx_thread_priority_change((TX_THREAD*)task, converted_priority, &current_priority) !=
        TX_SUCCESS)
    {
        return RET_VALUE_OS_TASK_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Suspend a task
/// @param task Handle to the task control block to be suspended
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_task_suspend(osTaskHandle_t task)
{
    if (task == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return RET_VALUE_INSTANCE_UNAVAILABLE;
    }

    if (tx_thread_suspend((TX_THREAD*)task) != TX_SUCCESS)
    {
        return RET_VALUE_OS_TASK_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Resume a suspended task
/// @param task Handle to the task control block to be resumed
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_task_resume(osTaskHandle_t task)
{
    if (task == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_THREAD*)task)->tx_thread_id != TX_THREAD_ID)
    {
        return RET_VALUE_INSTANCE_UNAVAILABLE;
    }

    if (tx_thread_resume((TX_THREAD*)task) != TX_SUCCESS)
    {
        return RET_VALUE_OS_TASK_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Create a new timer
/// @details This function creates a new timer with the specified parameters.
/// @param name        Name of the timer
/// @param callback    Pointer to the timer's callback function
/// @param arg         Argument to be passed to the timer's callback function
/// @return Handle to the created timer, or NULL on failure
osTimerHandle_t os_timer_create_once(const char* name, void (*callback)(void*), void* arg)
{
    if (callback == NULL)
    {
        return NULL;
    }

    TX_TIMER* timer = (TX_TIMER*)_mem_alloc(sizeof(TX_TIMER));

    if (timer == NULL)
    {
        return NULL;
    }

    if (tx_timer_create(timer, (CHAR*)name, (void (*)(ULONG))callback, (ULONG)arg, 1, 0,
                        TX_NO_ACTIVATE) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)timer);
        return NULL;
    }

    return (osTimerHandle_t)timer;
}

/// @brief  Create a new periodic timer
/// @details This function creates a new periodic timer with the specified parameters.
/// @param name        Name of the timer
/// @param callback    Pointer to the timer's callback function
/// @param arg         Argument to be passed to the timer's callback function
osTimerHandle_t os_timer_create_periodic(const char* name, void (*callback)(void*), void* arg)
{
    if (callback == NULL)
    {
        return NULL;
    }

    TX_TIMER* timer = (TX_TIMER*)_mem_alloc(sizeof(TX_TIMER));

    if (timer == NULL)
    {
        return NULL;
    }

    if (tx_timer_create(timer, (CHAR*)name, (void (*)(ULONG))callback, (ULONG)arg, 1, 1,
                        TX_NO_ACTIVATE) != TX_SUCCESS)
    {
        _mem_free((uint8_t*)timer);
        return NULL;
    }

    return (osTimerHandle_t)timer;
}

/// @brief  Delete a timer
/// @details This function deletes the specified timer and frees its resources.
/// @param timer Handle to the timer to be deleted
void os_timer_delete(osTimerHandle_t timer)
{
    if (timer != NULL)
    {
        tx_timer_delete((TX_TIMER*)timer);
        _mem_free((uint8_t*)timer);
    }
}

/// @brief  Get the name of a timer
/// @param timer Handle to the timer
/// @return Pointer to the timer's name string
const char* os_timer_name(osTimerHandle_t timer)
{
    char* name = NULL;

    if (timer == NULL)
    {
        return NULL;
    }

    if (((TX_TIMER*)timer)->tx_timer_id != TX_TIMER_ID)
    {
        return NULL;
    }

    if (tx_timer_info_get((TX_TIMER*)timer, (CHAR**)&name, NULL, NULL, NULL, NULL) != TX_SUCCESS)
    {
        return NULL;
    }

    return name;
}

/// @brief  Get the current state of a timer
/// @param timer Handle to the timer
/// @return Current state of the timer
osTimerState_t os_timer_state(osTimerHandle_t timer)
{
    UINT active;

    if (timer == NULL)
    {
        return OS_TIMER_STATE_UNKNOWN;
    }

    if (((TX_TIMER*)timer)->tx_timer_id != TX_TIMER_ID)
    {
        return OS_TIMER_STATE_UNKNOWN;
    }

    if (tx_timer_info_get((TX_TIMER*)timer, NULL, &active, NULL, NULL, NULL) != TX_SUCCESS)
    {
        return OS_TIMER_STATE_UNKNOWN;
    }

    if (!active)
    {
        return OS_TIMER_STATE_IDLE;
    }

    return OS_TIMER_STATE_ACTIVE;
}

/// @brief  Start a timer
/// @details This function starts the specified timer with the given timeout.
/// @param timer   Handle to the timer
/// @param timeout Timeout in OS count for the timer
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t os_timer_start(osTimerHandle_t timer, uint32_t timeout)
{
    if (timer == NULL)
    {
        return RET_VALUE_NULL_REF;
    }

    if (((TX_TIMER*)timer)->tx_timer_id != TX_TIMER_ID)
    {
        return RET_VALUE_PARAM_ERR;
    }

    UINT active;

    if (tx_timer_info_get((TX_TIMER*)timer, NULL, &active, NULL, NULL, NULL) != TX_SUCCESS)
    {
        return RET_VALUE_OS_TIMER_ERR;
    }

    if (active)
    {
        if (tx_timer_deactivate((TX_TIMER*)timer) != TX_SUCCESS)
        {
            return RET_VALUE_OS_TIMER_ERR;
        }
    }

    if (tx_timer_change((TX_TIMER*)timer, (ULONG)timeout, (ULONG)timeout) != TX_SUCCESS)
    {
        return RET_VALUE_OS_TIMER_ERR;
    }

    if (tx_timer_activate((TX_TIMER*)timer) != TX_SUCCESS)
    {
        return RET_VALUE_OS_TIMER_ERR;
    }

    return RET_VALUE_OK;
}

/// @brief  Stop a timer
/// @details This function stops the specified timer.
/// @param timer Handle to the timer
void os_timer_stop(osTimerHandle_t timer)
{
    UINT active;

    if ((timer != NULL) && (((TX_TIMER*)timer)->tx_timer_id == TX_TIMER_ID))
    {
        if (tx_timer_info_get((TX_TIMER*)timer, NULL, &active, NULL, NULL, NULL) == TX_SUCCESS)
        {
            if (active)
            {
                tx_timer_deactivate((TX_TIMER*)timer);
            }
        }
    }
}
