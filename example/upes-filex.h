/// @file upes-filex.h
/// @brief UPES Configuration Header Example for File System Abstraction Layer
/// @details This is an example configuration header file for the UPES file system abstraction
/// layer.
///          It defines constants and macros that can be modified to customize the behavior
///          of the file system abstraction layer according to the specific requirements of the
///          target application and underlying operating system.
/// @note This file should be copied and renamed to upes-filex.h and modified as needed.
/// @author Khose-ie<khose-ie@outlook.com>
/// @date 2026-05-14

#ifndef _UPES_FILEX_H_
#define _UPES_FILEX_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///============================================================================================
/// @brief File System Configuration
/// @details These constants and macros define the configuration for the file system abstraction
/// layer.
///          Modify these values according to your application's file system requirements.
///============================================================================================

/// @brief Number of file system media instances
/// @details This constant defines the number of file system media instances supported.
///     Modify this value according to your application's needs.
#define UPES_FILEX_MEDIA_COUNT (1)

/// @brief Size of each media work stack block
/// @details This constant defines the size (in bytes) of each media work stack block.
///     Modify this value according to your application's needs and the requirements
///     of the underlying file system used.
#define UPES_FILEX_MEDIA_WORK_SIZE (4096)

/// @brief Size of the FS stack memory zone
/// @details This constant defines the size (in bytes) of the memory zone
///          allocated for the FS stack byte pool.
///          If you always use static stack allocation APIs, you can reduce this size
///          to save memory due to only management blocks are allocated from this pool.
#define UPES_FILEX_MAX_OPEN_ENTITIES (8)

/// @brief Use external memory for FS stack
/// @details This macro defines an external byte array that serves as the memory pool for the FS
/// stack byte pool.
///     If this macro is not defined, a static byte pool will be created. The size of this memory
///     must be equal to
///     @ref UPES_FILEX_STACK_EX_MEM_SIZE.
///     If don't define this macro, the FS stack will define a static variable as the memory pool,
///     and automatically calculate the size based on the media count and work stack size. The
///     external memory stack should be defined as following style:
///         uint8_t fs_ex_stack_mem[UPES_FILEX_STACK_EX_MEM_SIZE]
// #define UPES_FILEX_STACK_EX_MEM (fs_ex_stack_mem)

/// @brief Size of the external FS stack memory
/// @details This constant defines the size (in bytes) of the external memory allocated for the FS
/// stack byte pool.
///     This value must be defined if the @ref UPES_FILEX_STACK_MEM macro is defined.
///     The requested size of the external memory are different based on the file system you used.
///     Please calculate the required size according to your file system's requirements. If you
///     don't define the @ref UPES_FILEX_STACK_MEM macro, this marco will be ignored.
// #define UPES_FILEX_STACK_EX_MEM_SIZE                                                               \
//     (UPES_FILEX_MEDIA_COUNT * (12416 + UPES_FILEX_MEDIA_WORK_SIZE) +                               \
//      UPES_FILEX_MAX_OPEN_ENTITIES * 1024)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _UPES_FILEX_H_
