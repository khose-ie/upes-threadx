
#include <fx_api.h>
#include <sam-filex.h>
#include <sam-media.h>
#include <tx_byte_pool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* Forward declaration for FileX internal partition helper (not exposed in fx_api.h).
    Declared here to avoid implicit function declaration errors when calling it. */
UINT _fx_partition_offset_calculate(void* partition_sector, UINT partition, ULONG* partition_start,
                                    ULONG* partition_size);

#ifndef SAM_FX_MEDIA_COUNT
#define MEDIA_COUNT (1)
#else // SAM_FX_MEDIA_COUNT
#define MEDIA_COUNT (SAM_FX_MEDIA_COUNT)
#endif // SAM_FX_MEDIA_COUNT

#ifndef SAM_FX_MEDIA_WORK_SIZE
#define MEDIA_WORK_SIZE (4096)
#else // SAM_FX_MEDIA_WORK_SIZE
#define MEDIA_WORK_SIZE align32down(SAM_FX_MEDIA_WORK_SIZE)
#endif // SAM_FX_MEDIA_WORK_SIZE

#ifndef SAM_FX_MAX_OPEN_ENTITIES
#define MAX_OPEN_ENTITIES (8)
#else // SAM_FX_MAX_OPEN_ENTITIES
#define MAX_OPEN_ENTITIES (SAM_FX_MAX_OPEN_ENTITIES)
#endif // SAM_FX_MAX_OPEN_ENTITIES

/// @brief FS stack memory configuration
/// @details This section defines the memory configuration for the FS stack, including whether
///          external memory is used and the corresponding memory pool variable.
#ifndef SAM_FX_STACK_EX_MEM
#define EXT1            static
#define FILEX_STACK_MEM _stack_mem
#else // SAM_FX_STACK_EX_MEM
#define EXT1            extern
#define FILEX_STACK_MEM (SAM_FX_STACK_EX_MEM)
#endif // SAM_FX_STACK_EX_MEM

/// @brief Size of memory header for ThreadX block pool allocations
/// @details This constant defines the size of the memory header
///          used by ThreadX for block pool allocations.
#define TX_MEM_HEAD_SIZE sizeof(UCHAR*)

/// @brief Media management requested memory size calculation
/// @details This macro calculates the total memory size required for media management,
///          including the size of the media structures and the associated mutexes,
#define FILEX_MEDIA_SIZE (MEDIA_COUNT * (align32up(sizeof(filexMediaStack_t)) + TX_MEM_HEAD_SIZE))

/// @brief Open handle requested memory size calculation
/// @details This macro calculates the total memory size required for open file and directory
/// handles,
///          including the size of the file and directory items.
#define FILEX_OPEN_HANDLE_SIZE                                                                     \
    (MAX_OPEN_ENTITIES * (align32up(sizeof(filexEntity_t)) + TX_MEM_HEAD_SIZE))

/// @brief Total requested memory size calculation
/// @details This macro calculates the total memory size required for the entire FS stack,
///          including media management, media work, and open handle memory requirements.
#define FILEX_STACK_MEM_REQUIRED_SIZE (FILEX_MEDIA_SIZE + FILEX_OPEN_HANDLE_SIZE)

/// @brief FS stack memory size configuration
/// @details This section defines the size of the FS stack memory, taking into account whether
///          external memory is used and the corresponding memory size macro.
#ifndef SAM_FX_STACK_EX_MEM_SIZE
#define FILEX_STACK_MEM_SIZE (FILEX_STACK_MEM_REQUIRED_SIZE)
#else // SAM_FX_STACK_EX_MEM_SIZE
#define FILEX_STACK_MEM_SIZE align32down(SAM_FX_STACK_EX_MEM_SIZE)
#endif // SAM_FX_STACK_EX_MEM_SIZE

/// @brief Media information structure
/// @details This structure holds the FX_MEDIA and associated mutex for thread-safe operations.
typedef struct
{
    FX_MEDIA media;
    TX_MUTEX mutex;
    mediaDiskIO_t diskio;
    uint8_t workspace[MEDIA_WORK_SIZE];
} filexMediaStack_t;

/// @brief File information structure
/// @details This typedef represents a file in the FileX file system.
typedef FX_FILE filexFile_t;

/// @brief Directory information structure
/// @details This structure holds information about a directory, including its media pointer,
///          path, and current entry index.
typedef struct
{
    filexMediaStack_t* media;
    char path[FX_MAX_LONG_NAME_LEN];
    uint32_t entry_index;
} filexDir_t;

/// @brief Disk information structure
/// @details This structure holds information about a disk, including its disk number.
typedef struct
{
    uint32_t disk_num;
} diskInfo_t;

/// @brief SCES file item union
/// @details This union represents a file item in the SCES file system,
///          which can be either a file or a directory.
typedef union
{
    filexFile_t file;
    filexDir_t dir;
    uint8_t raw[TX_BYTE_BLOCK_MIN];
} filexEntity_t;

/// @brief Open entity stack structure
/// @details This structure holds the block pool and memory for open file and directory entities.
typedef struct
{
    TX_BLOCK_POOL stack;
    uint8_t stack_mem[align32up(sizeof(filexEntity_t)) * MAX_OPEN_ENTITIES];
} filexEntityStack_t;

/// @brief FileX stack structure
/// @details This structure holds the media and open entity arrays for the FileX file system.
typedef struct
{
    filexMediaStack_t media[MEDIA_COUNT];
    filexEntityStack_t entities;
} filexStack_t;

/// @brief FS stack memory
/// @details This array serves as the memory pool for the FS stack, which includes media management,
///          media work, and open handle memory. The size of this array is defined by the
///          FILEX_STACK_MEM_SIZE macro, which can be configured based on the requirements of
///          the file system and the number of media and open handles needed.
EXT1 uint8_t FILEX_STACK_MEM[FILEX_STACK_MEM_SIZE];

/// @brief FileX control block pointer
/// @details This pointer points to the control block of the FileX file system, which is
///          allocated from the FS stack memory.
static filexStack_t* _filex = (filexStack_t*)FILEX_STACK_MEM;

/// @brief Allocate a memory block for media operations
/// @details This function allocates a memory block for media operations
///          from the media stack block pool.
/// @return Pointer to the allocated memory block, or NULL on failure
static uint8_t* mem_block_alloc(TX_BLOCK_POOL* mem_pool)
{
    uint8_t* mem = NULL;

    if (tx_block_allocate(mem_pool, (VOID**)&mem, TX_NO_WAIT) != TX_SUCCESS)
    {
        return NULL;
    }

    return mem;
}

/// @brief Free a memory block back to the media stack block pool
/// @details This function frees a previously allocated memory block
///          back to the media stack block pool.
/// @param mem Pointer to the memory block to free
static void mem_block_free(uint8_t* mem)
{
    if (mem != NULL)
    {
        tx_block_release((VOID*)mem);
    }
}

/// @brief Lock a mutex
/// @details This function locks the specified mutex.
/// @param mutex Pointer to the mutex to lock
/// @return RET_VALUE_OK on success, error code otherwise
static RetValue_t mutex_lock(TX_MUTEX* mutex)
{
#ifndef SINGLE_THREAD

    if (tx_mutex_get(mutex, 500) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

#endif // SINGLE_THREAD

    return RET_VALUE_OK;
}

/// @brief Unlock a mutex
/// @details This function unlocks the specified mutex.
/// @param mutex Pointer to the mutex to unlock
/// @return RET_VALUE_OK on success, error code otherwise
static RetValue_t mutex_unlock(TX_MUTEX* mutex)
{
#ifndef SINGLE_THREAD

    if (tx_mutex_put(mutex) != TX_SUCCESS)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

#endif // SINGLE_THREAD

    return RET_VALUE_OK;
}

/// @brief Disk I/O driver function
/// @details This function serves as the disk I/O driver for FileX.
/// @param media Pointer to the FX_MEDIA structure
static void diskio_main(FX_MEDIA* media)
{
    uint32_t status;
    uint32_t partition_start;
    uint32_t partition_size;

    mediaDiskIO_t* diskio = (mediaDiskIO_t*)media->fx_media_driver_info;

    if (diskio == NULL)
    {
        media->fx_media_driver_status = FX_IO_ERROR;
        return;
    }

    if (diskio->status() != RET_VALUE_OK)
    {
        media->fx_media_driver_status = FX_IO_ERROR;
        return;
    }

    media->fx_media_driver_status = FX_SUCCESS;

    switch (media->fx_media_driver_request)
    {
    case FX_DRIVER_READ:
    {
        if (diskio->read(media->fx_media_hidden_sectors + media->fx_media_driver_logical_sector,
                         media->fx_media_driver_sectors,
                         media->fx_media_driver_buffer) != RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_WRITE:
    {
        if (diskio->write(media->fx_media_hidden_sectors + media->fx_media_driver_logical_sector,
                          media->fx_media_driver_sectors,
                          media->fx_media_driver_buffer) != RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_FLUSH:
    {
        if (diskio->flush() != RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_ABORT:
    {
        break;
    }

    case FX_DRIVER_INIT:
    {
        break;
    }

    case FX_DRIVER_BOOT_READ:
    {
        if (diskio->read(0, media->fx_media_driver_sectors, media->fx_media_driver_buffer) !=
            RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
            break;
        }

        partition_start = 0;

        status = _fx_partition_offset_calculate(media->fx_media_driver_buffer, 0, &partition_start,
                                                &partition_size);

        if (status)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
            break;
        }

        if (partition_start == 0)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
            break;
        }

        if (diskio->read(partition_start, media->fx_media_driver_sectors,
                         media->fx_media_driver_buffer) != RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
            break;
        }

        break;
    }

    case FX_DRIVER_RELEASE_SECTORS:
    {
        break;
    }

    case FX_DRIVER_BOOT_WRITE:
    {
        if (diskio->write(0, media->fx_media_driver_sectors, media->fx_media_driver_buffer) !=
            RET_VALUE_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_UNINIT:
    {
        break;
    }

    default:
        media->fx_media_driver_status = FX_IO_ERROR;
        break;
    }
}

static RetValue_t check_fs_feasibility(const mediaState_t* state, const mediaDiskIO_t* diskio)
{
    RetValue_t value       = RET_VALUE_PARAM_ERR;
    uint32_t cluster_count = (diskio->sector_count() - 1) / state->sectors_per_cluster;

    if ((state->sectors_per_cluster == 0) || (state->num_of_fats > 2))
    {
        return RET_VALUE_PARAM_ERR;
    }

    switch (state->kind)
    {
    case MEDIA_FORMAT_AUTO:
    {
        value = RET_VALUE_PARAM_ERR;
        break;
    }

    case MEDIA_FORMAT_FAT12:
    {
        if (cluster_count > 4084)
        {
            break;
        }

        value = RET_VALUE_OK;
        break;
    }

    case MEDIA_FORMAT_FAT16:
    {
        if ((cluster_count < 4085) || (cluster_count > 65524))
        {
            break;
        }

        if (state->directory_entries == 0)
        {
            break;
        }

        value = RET_VALUE_OK;
        break;
    }

    case MEDIA_FORMAT_FAT32:
    {
        if (cluster_count < 65525)
        {
            break;
        }

        value = RET_VALUE_OK;
        break;
    }

    case MEDIA_FORMAT_EXFAT:
    {
        if ((diskio->sector_size() < 512) || (diskio->sector_size() & (diskio->sector_size() - 1)))
        {
            break;
        }

        value = RET_VALUE_OK;
        break;
    }
    }

    return value;
}

/// @brief Initialize file system subsystem
/// @details This function initializes the global file system subsystem.
///     It must be called once before any sces_fs_mount() operation. For some file systems (e.g.
///     FileX), this function performs underlying system-level initialization.
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_initialize(void)
{
    if (FILEX_STACK_MEM_SIZE < FILEX_STACK_MEM_REQUIRED_SIZE)
    {
        return RET_VALUE_STACK_OVERFLOW;
    }

    memset(_filex, 0, sizeof(*_filex));

    if (tx_block_pool_create(&_filex->entities.stack, "FS Entities Pool",
                             align32up(sizeof(filexEntity_t)), _filex->entities.stack_mem,
                             sizeof(_filex->entities.stack_mem)) != TX_SUCCESS)
    {
        return RET_VALUE_MEM_ALLOC_FAILURE;
    }

    fx_system_initialize();

    return RET_VALUE_OK;
}

/// @brief Set disk I/O driver function
/// @details This function sets the disk I/O driver function for the specified disk number.
/// @param disk_num Disk number for which to set the disk I/O driver function
/// @param diskio Pointer to the disk I/O driver structure
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_set_diskio(uint32_t disk_num, const mediaDiskIO_t* diskio)
{
    if (disk_num >= MEDIA_COUNT || diskio == NULL)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (diskio->read == NULL || diskio->write == NULL || diskio->flush == NULL ||
        diskio->trim == NULL || diskio->status == NULL || diskio->sector_count == NULL ||
        diskio->sector_size == NULL || diskio->block_size == NULL)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (_filex->media[disk_num].diskio.status != NULL)
    {
        return RET_VALUE_INSTANCE_DUPLICATE;
    }

    _filex->media[disk_num].diskio = *diskio;

    return RET_VALUE_OK;
}

/// @brief Format a file system
/// @details This function formats a file system with the specified parameters.
/// @param name   Name of the file system
/// @param disk_num Disk number to format
/// @param info   Information about the file system to format
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_format(const char* name, uint32_t disk_num, const mediaState_t* info)
{
    filexMediaStack_t* media = &_filex->media[disk_num];

    if (media->diskio.status == NULL)
    {
        return RET_VALUE_FS_DISK_NOT_READY;
    }

    if (check_fs_feasibility(info, &media->diskio) != RET_VALUE_OK)
    {
        return RET_VALUE_PARAM_ERR;
    }

    memset(media->workspace, 0, MEDIA_WORK_SIZE);

    if (fx_media_format(&media->media, diskio_main, &media->diskio, media->workspace,
                        MEDIA_WORK_SIZE, (CHAR*)name, info->num_of_fats, info->directory_entries, 0,
                        media->diskio.sector_count(), media->diskio.sector_size(),
                        info->sectors_per_cluster, 0, 0) != FX_SUCCESS)
    {
        return RET_VALUE_FS_FORMAT_FAILURE;
    }

    return RET_VALUE_OK;
}

/// @brief Mount a file system
/// @details This function mounts a file system with the specified parameters.
/// @param name          Name of the file system
/// @param disk_num      Disk number to mount
/// @return Handle to the mounted file system, or NULL on failure
mediaHandle_t media_mount(const char* name, uint32_t disk_num)
{
    filexMediaStack_t* media = &_filex->media[disk_num];

    memset(&media->media, 0, sizeof(media->media));
    memset(&media->mutex, 0, sizeof(media->mutex));
    memset(media->workspace, 0, MEDIA_WORK_SIZE);

    if (tx_mutex_create(&media->mutex, "", TX_INHERIT) != TX_SUCCESS)
    {
        return NULL;
    }

    if (fx_media_open(&media->media, (CHAR*)name, diskio_main, &media->diskio, media->workspace,
                      MEDIA_WORK_SIZE) != FX_SUCCESS)
    {
        tx_mutex_delete(&media->mutex);
        return NULL;
    }

    return (mediaHandle_t)media;
}

/// @brief Unmount a file system
/// @details This function unmounts the specified file system.
/// @param fs Handle to the file system to unmount
void media_unmount(mediaHandle_t media)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    fx_media_close(&xmedia->media);
    tx_mutex_delete(&xmedia->mutex);
}

/// @brief Get the name of the file system
/// @details This function retrieves the name of the specified file system.
/// @param fs Handle to the file system
/// @return Pointer to the file system's name string
const char* media_name(mediaHandle_t media)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;
    return (const char*)xmedia->media.fx_media_name;
}

/// @brief Synchronize the file system
/// @details This function synchronizes the specified file system, ensuring that all pending
///     changes are written to the underlying storage.
/// @param fs Handle to the file system
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_sync(mediaHandle_t media)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_media_flush(&xmedia->media) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);

    return RET_VALUE_OK;
}

/// @brief Create an empty file
/// @details Creates an empty file at the specified path.
///     If the file already exists, an error is returned.
/// @param fs Handle to the file system
/// @param path Path of the file to create
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_create_file(mediaHandle_t media, const char* path)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_file_create(&xmedia->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_INSTANCE_CREATE_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);
    return RET_VALUE_OK;
}

/// @brief Remove a file
/// @details Removes the file at the specified path.
/// @param fs Handle to the file system
/// @param path Path of the file to remove
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_remove_file(mediaHandle_t media, const char* path)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_file_delete(&xmedia->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);

    return RET_VALUE_OK;
}

/// @brief Create a directory
/// @details Creates a directory at the specified path.
///     If the directory already exists, an error is returned.
/// @param fs   Handle to the file system
/// @param path Path of the directory to create
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_create_dir(mediaHandle_t media, const char* path)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_directory_create(&xmedia->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_INSTANCE_CREATE_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);
    return RET_VALUE_OK;
}

/// @brief Remove a directory
/// @details Removes the directory at the specified path.
///     The directory must be empty to be removed successfully.
/// @param fs   Handle to the file system
/// @param path Path of the directory to remove
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_remove_dir(mediaHandle_t media, const char* path)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_directory_delete(&xmedia->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);
    return RET_VALUE_OK;
}

/// @brief  Move or rename a file or directory
/// @details This function moves or renames a file or directory from old_path to new_path.
///     It do the same operation for both files and directories, and like the "rename" function
///     in standard C library. It can't move files or directories across different mounted file
///     systems.
/// @param fs        Handle to the file system
/// @param old_path  Current path of the file or directory
/// @param new_path  New path of the file or directory
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_entity_move(mediaHandle_t media, const char* old_path, const char* new_path)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;
    UINT dir_test_value       = fx_directory_name_test(&xmedia->media, (CHAR*)old_path);

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (dir_test_value == FX_SUCCESS)
    {
        if (fx_directory_rename(&xmedia->media, (CHAR*)old_path, (CHAR*)new_path) != FX_SUCCESS)
        {
            mutex_unlock(&xmedia->mutex);
            return RET_VALUE_LOW_LEVEL_FAILURE;
        }
    }
    else if (dir_test_value == FX_NOT_DIRECTORY)
    {
        if (fx_file_rename(&xmedia->media, (CHAR*)old_path, (CHAR*)new_path) != FX_SUCCESS)
        {
            mutex_unlock(&xmedia->mutex);
            return RET_VALUE_LOW_LEVEL_FAILURE;
        }
    }
    else
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&xmedia->mutex);
    return RET_VALUE_OK;
}

/// @brief Get the state of a file or directory
/// @details This function retrieves the state information of a file or directory at the specified
///          path.
/// @param fs    Handle to the file system
/// @param path  Path of the file or directory
/// @param state Pointer to a mediaEntityState_t structure to receive the state information
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_entity_state(mediaHandle_t media, const char* path, mediaEntityState_t* state)
{
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (state == NULL)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_directory_information_get(
            &xmedia->media, (CHAR*)path, (UINT*)&state->attributes, (ULONG*)&state->size, (UINT*)&state->time_stamp.year,
            (UINT*)&state->time_stamp.month, (UINT*)&state->time_stamp.day, (UINT*)&state->time_stamp.hour,
            (UINT*)&state->time_stamp.minute, (UINT*)&state->time_stamp.second) != FX_SUCCESS)
    {
        mutex_unlock(&xmedia->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    state->kind = (state->attributes & FX_DIRECTORY) ? MEDIA_ENTITY_DIR : MEDIA_ENTITY_FILE;

    mutex_unlock(&xmedia->mutex);
    return RET_VALUE_OK;
}

//===============================
//  File operations
//===============================

/// @brief Open a file
/// @details This function opens a file at the specified path with the given mode.
/// @param media Handle to the file system
/// @param path  Path of the file to open
/// @param mode  File open mode mask
/// @return Handle to the opened file, or NULL on failure
mediaFileHandle_t media_file_open(mediaHandle_t media, const char* path, mediaFileOpenMode_t mode)
{
    FX_FILE* file;
    uint32_t fx_mode          = 0;
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mode & MEDIA_FILE_READ)
    {
        fx_mode |= FX_OPEN_FOR_READ;
    }

    if ((mode & MEDIA_FILE_WRITE) || (mode & MEDIA_FILE_APPEND))
    {
        fx_mode |= FX_OPEN_FOR_WRITE;
    }

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return NULL;
    }

    file = (FX_FILE*)mem_block_alloc(&_filex->entities.stack);

    if (file == NULL)
    {
        mutex_unlock(&xmedia->mutex);
        return NULL;
    }

    if (fx_file_open(&xmedia->media, file, (CHAR*)path, fx_mode) != FX_SUCCESS)
    {
        mem_block_free((uint8_t*)file);
        mutex_unlock(&xmedia->mutex);
        return NULL;
    }

    if (mode & MEDIA_FILE_APPEND)
    {
        if (fx_file_seek(file, file->fx_file_current_file_size) != FX_SUCCESS)
        {
            fx_file_close(file);
            mem_block_free((uint8_t*)file);
            mutex_unlock(&xmedia->mutex);
            return NULL;
        }
    }

    mutex_unlock(&xmedia->mutex);
    return (mediaFileHandle_t)file;
}

/// @brief Close a file
/// @details This function closes the specified file and releases its resources.
/// @param file Handle to the file to be closed
void media_file_close(mediaFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    fx_file_close(fx_file);
    mem_block_free((uint8_t*)fx_file);
}

/// @brief Read data from a file
/// @details This function reads data from the specified file into the provided buffer.
/// @param file   Handle to the file to read from
/// @param buffer Pointer to the buffer to receive the data
/// @param size   Number of bytes to read
/// @param read_size Pointer to a variable to receive the number of bytes actually read
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_file_read(mediaFileHandle_t file, void* buffer, uint32_t size, uint32_t* read_size)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_read(fx_file, buffer, size, read_size) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    return RET_VALUE_OK;
}

/// @brief Write data to a file
/// @details This function writes data to the specified file from the provided buffer.
/// @param file   Handle to the file to write to
/// @param buffer Pointer to the buffer containing the data to write
/// @param size   Number of bytes to write
/// @param written_size Pointer to a variable to receive the number of bytes actually written
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_file_write(mediaFileHandle_t file, const void* buffer, uint32_t size,
                            uint32_t* written_size)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_write(fx_file, (VOID*)buffer, size) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    if (written_size != NULL)
    {
        *written_size = size;
    }

    return RET_VALUE_OK;
}

/// @brief Seek to a position in a file
/// @details This function sets the file position indicator for the specified file.
/// @param file   Handle to the file
/// @param offset Offset in bytes to seek
/// @return RET_VALUE_OK on success, error code otherwise
/// @note The offset must be within the range of a 32-bit unsigned integer due to FileX limitations.
RetValue_t media_file_seek(mediaFileHandle_t file, uint64_t offset)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (offset > UINT32_MAX)
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (fx_file_seek(fx_file, offset) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    return RET_VALUE_OK;
}

/// @brief Seek to a position in a file from the current position
/// @details This function sets the file position indicator for the specified file,
///     relative to the current position.
/// @param file   Handle to the file
/// @param offset Offset in bytes to seek from the current position
/// @return RET_VALUE_OK on success, error code otherwise
/// @note The resulting position must be within the range of a 32-bit unsigned integer
///       due to FileX limitations.
RetValue_t media_file_seek_from_current(mediaFileHandle_t file, uint64_t offset)
{
    FX_FILE* fx_file      = (FX_FILE*)file;
    uint64_t new_position = fx_file->fx_file_current_file_offset + offset;

    if ((fx_file->fx_file_current_file_offset >= UINT32_MAX) || (offset > UINT32_MAX) ||
        (new_position > UINT32_MAX))
    {
        return RET_VALUE_PARAM_ERR;
    }

    if (fx_file_seek(fx_file, (uint32_t)new_position) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }
    return RET_VALUE_OK;
}

/// @brief Get the current position in a file
/// @details This function retrieves the current file position indicator for the specified file.
/// @param file Handle to the file
/// @return Current position in bytes from the beginning of the file
uint32_t media_file_tell(mediaFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;
    return fx_file->fx_file_current_file_offset;
}

/// @brief Get the size of a file
/// @details This function retrieves the size of the specified file in bytes.
/// @param file Handle to the file
/// @return Size of the file in bytes
uint32_t media_file_size(mediaFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;
    return fx_file->fx_file_current_file_size;
}

/// @brief Truncate a file to the current position
/// @details This function truncates the specified file to the current file position.
/// @param file Handle to the file
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_file_truncate(mediaFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_truncate(fx_file, fx_file->fx_file_current_file_offset) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    return RET_VALUE_OK;
}

/// @brief Synchronize a file
/// @details This function synchronizes the specified file, ensuring that all pending changes
///     are written to the underlying storage.
/// @param file Handle to the file
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_file_sync(mediaFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    /* FileX does not expose a file-level flush in the public API; flush the
       underlying media instead. This ensures data is written to the device. */
    if (fx_media_flush(fx_file->fx_file_media_ptr) != FX_SUCCESS)
    {
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    return RET_VALUE_OK;
}

//===============================
//  Directory operations
//===============================

/// @brief Open a directory
/// @details This function opens a directory at the specified path.
/// @param fs   Handle to the file system
/// @param path Path of the directory to open
/// @return Handle to the opened directory, or NULL on failure
mediaDirHandle_t media_dir_open(mediaHandle_t media, const char* path)
{
    filexDir_t* dir;
    filexMediaStack_t* xmedia = (filexMediaStack_t*)media;

    if (mutex_lock(&xmedia->mutex) != RET_VALUE_OK)
    {
        return NULL;
    }

    dir = (filexDir_t*)mem_block_alloc(&_filex->entities.stack);

    if (dir == NULL)
    {
        mutex_unlock(&xmedia->mutex);
        return NULL;
    }

    dir->media       = media;
    dir->entry_index = 0;

    strncpy(dir->path, path, FX_MAX_LONG_NAME_LEN - 1);
    dir->path[FX_MAX_LONG_NAME_LEN - 1] = '\0';

    mutex_unlock(&xmedia->mutex);
    return (mediaDirHandle_t)dir;
}

/// @brief Close a directory
/// @details This function closes the specified directory and releases its resources.
/// @param dir Handle to the directory to be closed
void media_dir_close(mediaDirHandle_t dir)
{
    mem_block_free((uint8_t*)dir);
}

/// @brief Get the next item in a directory
/// @details This function retrieves the next item in the specified directory.
/// @param dir   Handle to the directory
/// @param state Pointer to a mediaEntityState_t structure to receive the item information
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_dir_next_item_state(mediaDirHandle_t dir, mediaEntityState_t* state)
{
    CHAR* default_name;
    CHAR name[FX_MAX_LONG_NAME_LEN];
    filexDir_t* fx_dir = (filexDir_t*)dir;

    if (mutex_lock(&fx_dir->media->mutex) != RET_VALUE_OK)
    {
        return RET_VALUE_OS_MUTEX_ERR;
    }

    if (fx_dir->entry_index == 0)
    {
        if (fx_directory_default_set(&fx_dir->media->media, (CHAR*)fx_dir->path) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return RET_VALUE_LOW_LEVEL_FAILURE;
        }

        if (fx_directory_first_full_entry_find(
                &fx_dir->media->media, name, (UINT*)&state->attributes, (ULONG*)&state->size,
                (UINT*)&state->time_stamp.year, (UINT*)&state->time_stamp.month,
                (UINT*)&state->time_stamp.day, (UINT*)&state->time_stamp.hour,
                (UINT*)&state->time_stamp.minute, (UINT*)&state->time_stamp.second) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return RET_VALUE_LOW_LEVEL_FAILURE;
        }

        state->kind = (state->attributes & FX_DIRECTORY) ? MEDIA_ENTITY_DIR : MEDIA_ENTITY_FILE;

        fx_dir->entry_index = fx_dir->media->media.fx_media_directory_next_full_entry_finds;
        mutex_unlock(&fx_dir->media->mutex);

        return RET_VALUE_OK;
    }

    if (fx_directory_default_get(&fx_dir->media->media, &default_name) != FX_SUCCESS)
    {
        mutex_unlock(&fx_dir->media->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    if (strncmp(fx_dir->path, default_name, FX_MAX_LONG_NAME_LEN) != 0)
    {
        if (fx_directory_default_set(&fx_dir->media->media, (CHAR*)fx_dir->path) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return RET_VALUE_LOW_LEVEL_FAILURE;
        }

        fx_dir->media->media.fx_media_directory_next_full_entry_finds = fx_dir->entry_index;
    }

    if (fx_directory_next_full_entry_find(
            &fx_dir->media->media, name, (UINT*)&state->attributes, (ULONG*)&state->size, (UINT*)&state->time_stamp.year,
            (UINT*)&state->time_stamp.month, (UINT*)&state->time_stamp.day, (UINT*)&state->time_stamp.hour,
            (UINT*)&state->time_stamp.minute, (UINT*)&state->time_stamp.second) != FX_SUCCESS)
    {
        mutex_unlock(&fx_dir->media->mutex);
        return RET_VALUE_LOW_LEVEL_FAILURE;
    }

    state->kind = (state->attributes & FX_DIRECTORY) ? MEDIA_ENTITY_DIR : MEDIA_ENTITY_FILE;

    fx_dir->entry_index = fx_dir->media->media.fx_media_directory_next_full_entry_finds;
    mutex_unlock(&fx_dir->media->mutex);

    return RET_VALUE_OK;
}

/// @brief Rewind a directory
/// @details This function rewinds the specified directory to the beginning.
/// @param dir Handle to the directory
/// @return RET_VALUE_OK on success, error code otherwise
RetValue_t media_dir_rewind(mediaDirHandle_t dir)
{
    filexDir_t* fx_dir = (filexDir_t*)dir;

    fx_dir->entry_index = 0;

    return RET_VALUE_OK;
}

#ifdef __cplusplus
}
#endif // __cplusplus
