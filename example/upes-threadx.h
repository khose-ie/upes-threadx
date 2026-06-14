#ifndef _UPES_THREADX_H_
#define _UPES_THREADX_H_

//============================================================================================
/// @brief OS Stack Configuration
/// @details These constants and macros define the configuration for the OS stack byte pool.
//============================================================================================

/// @brief Size of the OS stack memory zone
/// @details This constant defines the size (in bytes) of the memory zone
///          allocated for the OS stack byte pool.
///          If you always use static stack allocation APIs, you can reduce this size
///          to save memory due to only management blocks are allocated from this pool.
/// @attention This size don't includes the size of memory pool that you set in below marcos.
/// @attention Modify this value according to your application's needs and this value
///            must be defined and aligned to uint32_t.
/// @note If you don't define this macro, the default size will be set to 60KB.
#define UPES_THREADX_OS_STACK_SIZE (1024 * 60)

/// @brief Declaration of the external OS stack memory zone
/// @details This macro declares an external byte array that serves as the
///          memory zone for the OS stack byte pool.
/// @attention The size of this zone must be equal to @ref UPES_THREADX_OS_STACK_SIZE.
/// @note If you don't define this macro, a default internal byte array will be used as the OS stack
/// memory zone.
/// @note If you don't define @ref UPES_THREADX_OS_STACK_SIZE, this macro will be ignored.
// #define UPES_THREADX_OS_STACK_EX_MEM (_os_stack_mem)

//============================================================================================
/// @brief OS Memory Pool Configuration
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
//============================================================================================

/// @brief Memory pool block size configuration #1
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKSZ_1 (256)

/// @brief Memory pool block size configuration #2
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKSZ_2 (512)

/// @brief Memory pool block size configuration #3
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKSZ_3 (1024)

/// @brief Memory pool block size configuration #4
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKSZ_4 (2048)

/// @brief Memory pool block count configuration #1
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKCT_1 (32)

/// @brief Memory pool block count configuration #2
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKCT_2 (16)

/// @brief Memory pool block count configuration #3
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKCT_3 (8)

/// @brief Memory pool block count configuration #4
/// @details These constants define the block sizes and counts for the OS memory pool.
///          Modify these values according to your application's memory allocation patterns.
#define UPES_THREADX_OS_MEM_POOL_BKCT_4 (4)

/// @brief Declaration of the external OS memory pool memory zone
/// @details This macro declares an external byte array that serves as the
///          memory zone for the OS memory pool.
/// @attention The size of this zone must be equal to the total size of all memory pool blocks.
/// @note If you don't define this macro, a default internal byte array will be used as the OS
/// memory pool memory zone.
/// @note The defined memory zone should be large enough to accommodate all memory pool blocks.
// #define UPES_THREADX_OS_MEM_POOL_EX_MEM (_os_mem_pool_mem)

#endif // _UPES_THREADX_H_
