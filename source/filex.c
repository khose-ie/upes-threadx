
#include <fx_api.h>
#include <sces-conf.h>
#include <sces-fs.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <tx_byte_pool.h>

#ifndef SCES_FS_MEDIA_COUNT
#define SCES_FS_MEDIA_COUNT (1)
#endif // SCES_FS_MEDIA_COUNT

#ifndef SCES_FS_MEDIA_EACH_WORK_SIZE
#define SCES_FS_MEDIA_EACH_WORK_SIZE (4096)
#else // SCES_FS_MEDIA_EACH_WORK_SIZE
#define SCES_FS_MEDIA_EACH_WORK_SIZE sces_aligned_sizeof(SCES_FS_MEDIA_EACH_WORK_SIZE, ALIGN_TYPE)
#endif // SCES_FS_MEDIA_EACH_WORK_SIZE

#ifndef SCES_FS_MAX_OPEN_COUNT
#define SCES_FS_MAX_OPEN_COUNT (8)
#endif // SCES_FS_MAX_OPEN_COUNT

#ifndef SCES_FS_STACK_MEM
#define STATIC            static
#define SCES_FS_STACK_MEM fs_stack_mem
#else // SCES_FS_STACK_MEM
#define STATIC extern
#endif // SCES_FS_STACK_MEM

/// @brief Size of memory header for ThreadX block pool allocations
/// @details This constant defines the size of the memory header
///          used by ThreadX for block pool allocations.
#define TX_MEM_HEAD_SIZE sizeof(UCHAR*)

/// @brief Media management requested memory size calculation
/// @details This macro calculates the total memory size required for media management,
///          including the size of the media structures and the associated mutexes,
#define SCES_FS_MEDIA_SIZE                                                                         \
    (SCES_FS_MEDIA_COUNT * ((sces_aligned_sizeof(SCES_MEDIA, ALIGN_TYPE) + TX_MEM_HEAD_SIZE)))

/// @brief Media work requested memory size calculation
/// @details This macro calculates the total memory size required for media work operations,
///          including the size of the work stacks for each media.
#define SCES_FS_MEDIA_WORK_SIZE                                                                    \
    (SCES_FS_MEDIA_COUNT * (SCES_FS_MEDIA_EACH_WORK_SIZE + TX_MEM_HEAD_SIZE))

/// @brief Open handle requested memory size calculation
/// @details This macro calculates the total memory size required for open file and directory
/// handles,
///          including the size of the file and directory items.
#define SCES_FS_OPEN_HANDLE_SIZE                                                                   \
    (SCES_FS_MAX_OPEN_COUNT * (sces_aligned_sizeof(scesFileItem_t, ALIGN_TYPE) + TX_MEM_HEAD_SIZE))

/// @brief Total requested memory size calculation
/// @details This macro calculates the total memory size required for the entire FS stack,
///          including media management, media work, and open handle memory requirements.
#define SCES_FS_STACK_MEM_REQUIRED_SIZE                                                            \
    (SCES_FS_MEDIA_SIZE + SCES_FS_MEDIA_WORK_SIZE + SCES_FS_OPEN_HANDLE_SIZE)

#ifndef SCES_FS_STACK_MEM_SIZE
#define SCES_FS_STACK_MEM_SIZE SCES_FS_STACK_MEM_REQUIRED_SIZE
#endif // SCES_FS_STACK_MEM_SIZE

/// @brief Media information structure
/// @details This structure holds the FX_MEDIA and associated mutex for thread-safe operations.
typedef struct
{
    FX_MEDIA media;
    TX_MUTEX mutex;
} SCES_MEDIA;

/// @brief Directory information structure
/// @details This structure holds information about a directory, including its media pointer,
///          path, and current entry index.
typedef struct
{
    SCES_MEDIA* media;
    char path[FX_MAX_LONG_NAME_LEN];
    uint32_t entry_index;
} SCES_DIR;

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
    FX_FILE file;
    SCES_DIR dir;
    uint8_t raw[TX_BYTE_BLOCK_MIN];
} scesFileItem_t;

/// @brief FS stack memory
/// @details This array serves as the memory pool for the FS stack, which includes media management,
///          media work, and open handle memory. The size of this array is defined by the
///          SCES_FS_STACK_MEM_SIZE macro, which can be configured based on the requirements of
///          the file system and the number of media and open handles needed.
STATIC uint8_t SCES_FS_STACK_MEM[SCES_FS_STACK_MEM_SIZE];

/// @brief FS stack block pool
/// @details This block pool is used for allocating memory blocks for
/// media operations.
static TX_BLOCK_POOL fs_media;

/// @brief FS stack memory zone
/// @details This memory zone is used for the FS stack, containing FX_MEDIA
/// structures.
const uint8_t* fs_media_mem = SCES_FS_STACK_MEM;

/// @brief FS work block pool
/// @details This block pool is used for allocating memory blocks for media
/// work operations.
static TX_BLOCK_POOL fs_works;

/// @brief FS work memory zone
/// @details This memory zone is used for the FS work, containing work
/// stacks for each media
const uint8_t* fs_works_mem = SCES_FS_STACK_MEM + SCES_FS_MEDIA_SIZE;

/// @brief FS open handle block pool
/// @details This block pool is used for allocating file and directory
/// handles.
static TX_BLOCK_POOL fs_items;

/// @brief FS open handle memory zone
/// @details This memory zone is used for the FS open handles, containing
/// file and directory items
const uint8_t* fs_items_mem = SCES_FS_STACK_MEM + SCES_FS_MEDIA_SIZE + SCES_FS_MEDIA_WORK_SIZE;

/// @brief Disk I/O driver function
/// @details This function serves as the disk I/O driver for FileX.
static scesFileDiskIO_t diskio_table[SCES_FS_MEDIA_COUNT];

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
/// @return SCES_RET_OK on success, error code otherwise
static scesRetVal_t mutex_lock(TX_MUTEX* mutex)
{
#ifndef SINGLE_THREAD

    if (tx_mutex_get(mutex, 500) != TX_SUCCESS)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

#endif // SINGLE_THREAD

    return SCES_RET_OK;
}

/// @brief Unlock a mutex
/// @details This function unlocks the specified mutex.
/// @param mutex Pointer to the mutex to unlock
/// @return SCES_RET_OK on success, error code otherwise
static scesRetVal_t mutex_unlock(TX_MUTEX* mutex)
{
#ifndef SINGLE_THREAD

    if (tx_mutex_put(mutex) != TX_SUCCESS)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

#endif // SINGLE_THREAD

    return SCES_RET_OK;
}

/// @brief Search for the disk I/O driver for a given disk number
/// @details This function searches for the disk I/O driver associated
///          with the specified disk number.
/// @param disk_num Disk number to search for
/// @return Pointer to the disk I/O driver structure, or NULL if not found
static scesFileDiskIO_t* search_diskio(uint32_t disk_num)
{
    if (disk_num >= SCES_FS_MEDIA_COUNT)
    {
        return NULL;
    }

    if (diskio_table[disk_num].status == NULL)
    {
        return NULL;
    }

    return &diskio_table[disk_num];
}

/// @brief Disk I/O driver function
/// @details This function serves as the disk I/O driver for FileX.
/// @param media Pointer to the FX_MEDIA structure
static void diskio_main(FX_MEDIA* media)
{
    uint32_t status;
    uint32_t partition_start;
    uint32_t partition_size;

    diskInfo_t* disk         = (diskInfo_t*)media->fx_media_driver_info;
    scesFileDiskIO_t* diskio = search_diskio(disk->disk_num);

    if (diskio == NULL)
    {
        media->fx_media_driver_status = FX_IO_ERROR;
        return;
    }

    if (diskio->status() != SCES_RET_OK)
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
                         media->fx_media_driver_buffer) != SCES_RET_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_WRITE:
    {
        if (diskio->write(media->fx_media_hidden_sectors + media->fx_media_driver_logical_sector,
                          media->fx_media_driver_sectors,
                          media->fx_media_driver_buffer) != SCES_RET_OK)
        {
            media->fx_media_driver_status = FX_IO_ERROR;
        }

        break;
    }

    case FX_DRIVER_FLUSH:
    {
        if (diskio->flush() != SCES_RET_OK)
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
            SCES_RET_OK)
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
                         media->fx_media_driver_buffer) != SCES_RET_OK)
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
            SCES_RET_OK)
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

static scesRetVal_t check_fs_feasibility(const scesFsInfo_t* info, const scesFileDiskIO_t* diskio)
{
    scesRetVal_t value     = SCES_RET_PARAM_ERR;
    uint32_t cluster_count = (diskio->sector_count() - 1) / info->sectors_per_cluster;

    if ((info->sectors_per_cluster == 0) || (info->num_of_fats > 2))
    {
        return SCES_RET_PARAM_ERR;
    }

    switch (info->kind)
    {
    case SCES_FS_FAT12:
    {
        if (cluster_count > 4084)
        {
            break;
        }

        value = SCES_RET_OK;
        break;
    }

    case SCES_FS_FAT16:
    {
        if ((cluster_count < 4085) || (cluster_count > 65524))
        {
            break;
        }

        if (info->directory_entries == 0)
        {
            break;
        }

        value = SCES_RET_OK;
        break;
    }

    case SCES_FS_FAT32:
    {
        if (cluster_count < 65525)
        {
            break;
        }

        value = SCES_RET_OK;
        break;
    }

    case SCES_FS_EXFAT:
    {
        if ((diskio->sector_size() < 512) || (diskio->sector_size() & (diskio->sector_size() - 1)))
        {
            break;
        }

        value = SCES_RET_OK;
        break;
    }
    }

    return value;
}

/// @brief Initialize file system subsystem
/// @details This function initializes the global file system subsystem.
///     It must be called once before any sces_fs_mount() operation. For some file systems (e.g.
///     FileX), this function performs underlying system-level initialization.
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_initialize(void)
{
    if (SCES_FS_STACK_MEM_SIZE < SCES_FS_STACK_MEM_REQUIRED_SIZE)
    {
        return SCES_RET_STACK_OVERFLOW;
    }

    memset(fs_stack_mem, 0, sizeof(fs_stack_mem));

    if (tx_block_pool_create(&fs_media, "FS Media", fs_media_mem,
                             sces_aligned_sizeof(FX_MEDIA, ALIGN_TYPE),
                             SCES_FS_MEDIA_SIZE) != TX_SUCCESS)
    {
        return SCES_RET_OS_MEM_POOL_ERR;
    }

    if (tx_block_pool_create(&fs_works, "FS Works", fs_works_mem, SCES_FS_WORK_STACK_SIZE,
                             SCES_FS_MEDIA_WORK_SIZE) != TX_SUCCESS)
    {
        return SCES_RET_OS_MEM_POOL_ERR;
    }

    if (tx_block_pool_create(&fs_items, "FS Open Handles", fs_items_mem,
                             sces_aligned_sizeof(scesFileItem_t, ALIGN_TYPE),
                             SCES_FS_OPEN_HANDLE_SIZE) != TX_SUCCESS)
    {
        return SCES_RET_OS_MEM_POOL_ERR;
    }

    fx_system_initialize();

    return SCES_RET_OK;
}

/// @brief Set disk I/O driver function
/// @details This function sets the disk I/O driver function for the specified disk number.
/// @param disk_num Disk number for which to set the disk I/O driver function
/// @param diskio Pointer to the disk I/O driver structure
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_set_diskio(uint32_t disk_num, const scesFileDiskIO_t* diskio)
{
    if (disk_num >= SCES_FS_MEDIA_COUNT || diskio == NULL)
    {
        return SCES_RET_PARAM_ERR;
    }

    if (diskio->read == NULL || diskio->write == NULL || diskio->flush == NULL ||
        diskio->trim == NULL || diskio->status == NULL || diskio->sector_count == NULL ||
        diskio->sector_size == NULL || diskio->block_size == NULL)
    {
        return SCES_RET_PARAM_ERR;
    }

    if (diskio_table[disk_num].status != NULL)
    {
        return SCES_RET_INSTANCE_DUPLICATE;
    }

    diskio_table[disk_num] = *diskio;

    return SCES_RET_OK;
}

/// @brief Format a file system
/// @details This function formats a file system with the specified parameters.
/// @param name   Name of the file system
/// @param disk_num Disk number to format
/// @param info   Information about the file system to format
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_format(const char* name, uint32_t disk_num, const scesFsInfo_t* info)
{
    SCES_MEDIA* media;
    void* media_space;
    diskInfo_t disk;

    scesFileDiskIO_t* diskio = search_diskio(disk_num);

    if (diskio == NULL)
    {
        return SCES_RET_FS_DISK_NOT_READY;
    }

    if (check_fs_feasibility(info, diskio) != SCES_RET_OK)
    {
        return SCES_RET_PARAM_ERR;
    }

    media = (SCES_MEDIA*)mem_block_alloc(&fs_media);

    if (media == NULL)
    {
        return SCES_RET_MEM_ALLOC_FAILURE;
    }

    media_space = mem_block_alloc(&fs_works);

    if (media_space == NULL)
    {
        mem_block_free((uint8_t*)media);
        return SCES_RET_MEM_ALLOC_FAILURE;
    }

    memset(media_space, 0, SCES_FS_WORK_STACK_SIZE);
    disk.disk_num = disk_num;

    if (fx_media_format(&media->media, diskio_main, &disk, media_space, SCES_FS_WORK_STACK_SIZE,
                        (CHAR*)name, info->num_of_fats, info->directory_entries, 0,
                        diskio->sector_count(), diskio->sector_size(), info->sectors_per_cluster, 0,
                        0) != FX_SUCCESS)
    {
        mem_block_free((uint8_t*)media);
        mem_block_free((uint8_t*)media_space);
        return SCES_RET_FS_FORMAT_FAILURE;
    }

    mem_block_free((uint8_t*)media);
    mem_block_free((uint8_t*)media_space);

    return SCES_RET_OK;
}

/// @brief Mount a file system
/// @details This function mounts a file system with the specified parameters.
/// @param name          Name of the file system
/// @param disk_num      Disk number to mount
/// @return Handle to the mounted file system, or NULL on failure
scesFsHandle_t sces_fs_mount(const char* name, uint32_t disk_num)
{
    SCES_MEDIA* media;
    void* media_space;
    diskInfo_t disk;

    scesFileDiskIO_t* diskio = search_diskio(disk_num);

    if (diskio == NULL)
    {
        return NULL;
    }

    media = (SCES_MEDIA*)mem_block_alloc(&fs_media);

    if (media == NULL)
    {
        return NULL;
    }

    media_space = mem_block_alloc(&fs_works);

    if (media_space == NULL)
    {
        mem_block_free((uint8_t*)media);
        return NULL;
    }

    if (tx_mutex_create(&media->mutex, "", TX_INHERIT) != TX_SUCCESS)
    {
        mem_block_free((uint8_t*)media);
        mem_block_free((uint8_t*)media_space);
        return NULL;
    }

    disk.disk_num = disk_num;
    memset(media_space, 0, SCES_FS_WORK_STACK_SIZE);

    if (fx_media_open(&media->media, (CHAR*)name, diskio_main, &disk, media_space,
                      SCES_FS_WORK_STACK_SIZE) != FX_SUCCESS)
    {
        tx_mutex_delete(&media->mutex);
        mem_block_free((uint8_t*)media);
        mem_block_free((uint8_t*)media_space);
        return NULL;
    }

    return (scesFsHandle_t)media;
}

/// @brief Unmount a file system
/// @details This function unmounts the specified file system.
/// @param fs Handle to the file system to unmount
void sces_fs_unmount(scesFsHandle_t fs)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;
    void* media_space = media->media.fx_media_memory_buffer;

    fx_media_close(&media->media);
    tx_mutex_delete(&media->mutex);

    mem_block_free((uint8_t*)media);
    mem_block_free((uint8_t*)media_space);
}

/// @brief Get the name of the file system
/// @details This function retrieves the name of the specified file system.
/// @param fs Handle to the file system
/// @return Pointer to the file system's name string
const char* sces_fs_name(scesFsHandle_t fs)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;
    return (const char*)media->media.fx_media_name;
}

/// @brief Synchronize the file system
/// @details This function synchronizes the specified file system, ensuring that all pending
///     changes are written to the underlying storage.
/// @param fs Handle to the file system
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_sync(scesFsHandle_t fs)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_media_flush(&media->media) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&media->mutex);

    return SCES_RET_OK;
}

/// @brief Create an empty file
/// @details Creates an empty file at the specified path.
///     If the file already exists, an error is returned.
/// @param fs Handle to the file system
/// @param path Path of the file to create
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_create_file(scesFsHandle_t fs, const char* path)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_file_create(&media->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_INSTANCE_CREATE_FAILURE;
    }

    mutex_unlock(&media->mutex);
    return SCES_RET_OK;
}

/// @brief Remove a file
/// @details Removes the file at the specified path.
/// @param fs Handle to the file system
/// @param path Path of the file to remove
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_remove_file(scesFsHandle_t fs, const char* path)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_file_delete(&media->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&media->mutex);

    return SCES_RET_OK;
}

/// @brief Create a directory
/// @details Creates a directory at the specified path.
///     If the directory already exists, an error is returned.
/// @param fs   Handle to the file system
/// @param path Path of the directory to create
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_create_dir(scesFsHandle_t fs, const char* path)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_directory_create(&media->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_INSTANCE_CREATE_FAILURE;
    }

    mutex_unlock(&media->mutex);
    return SCES_RET_OK;
}

/// @brief Remove a directory
/// @details Removes the directory at the specified path.
///     The directory must be empty to be removed successfully.
/// @param fs   Handle to the file system
/// @param path Path of the directory to remove
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_remove_dir(scesFsHandle_t fs, const char* path)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_directory_delete(&media->media, (CHAR*)path) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&media->mutex);
    return SCES_RET_OK;
}

/// @brief  Move or rename a file or directory
/// @details This function moves or renames a file or directory from old_path to new_path.
///     It do the same operation for both files and directories, and like the "rename" function
///     in standard C library. It can't move files or directories across different mounted file
///     systems.
/// @param fs        Handle to the file system
/// @param old_path  Current path of the file or directory
/// @param new_path  New path of the file or directory
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_move(scesFsHandle_t fs, const char* old_path, const char* new_path)
{
    SCES_MEDIA* media   = (SCES_MEDIA*)fs;
    UINT dir_test_value = fx_directory_name_test(&media->media, (CHAR*)old_path);

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (dir_test_value == FX_SUCCESS)
    {
        if (fx_directory_rename(&media->media, (CHAR*)old_path, (CHAR*)new_path) != FX_SUCCESS)
        {
            mutex_unlock(&media->mutex);
            return SCES_RET_LOW_LEVEL_FAILURE;
        }
    }
    else if (dir_test_value == FX_NOT_DIRECTORY)
    {
        if (fx_file_rename(&media->media, (CHAR*)old_path, (CHAR*)new_path) != FX_SUCCESS)
        {
            mutex_unlock(&media->mutex);
            return SCES_RET_LOW_LEVEL_FAILURE;
        }
    }
    else
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    mutex_unlock(&media->mutex);
    return SCES_RET_OK;
}

/// @brief Get the state of a file or directory
/// @details This function retrieves the state information of a file or directory at the specified
///          path.
/// @param fs    Handle to the file system
/// @param path  Path of the file or directory
/// @param state Pointer to a scesFileItemState_t structure to receive the state information
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_fs_item_state(scesFsHandle_t fs, const char* path, scesFileItemState_t* state)
{
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (state == NULL)
    {
        return SCES_RET_PARAM_ERR;
    }

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_directory_information_get(
            &media->media, (CHAR*)path, &state->attributes, &state->size, &state->time_stamp.year,
            &state->time_stamp.month, &state->time_stamp.day, &state->time_stamp.hour,
            &state->time_stamp.minute, &state->time_stamp.second) != FX_SUCCESS)
    {
        mutex_unlock(&media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    state->kind = (state->attributes & FX_DIRECTORY) ? SCES_FILE_DIR : SCES_FILE_FILE;

    mutex_unlock(&media->mutex);
    return SCES_RET_OK;
}

//===============================
//  File operations
//===============================

/// @brief Open a file
/// @details This function opens a file at the specified path with the given mode.
/// @param fs    Handle to the file system
/// @param path  Path of the file to open
/// @param mode  File open mode mask
/// @return Handle to the opened file, or NULL on failure
scesFileHandle_t sces_file_open(scesFsHandle_t fs, const char* path, scesFileOpenModeMask_t mode)
{
    FX_FILE* file;
    uint32_t fx_mode  = 0;
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mode & SCES_FILE_READ)
    {
        fx_mode |= FX_OPEN_FOR_READ;
    }

    if ((mode & SCES_FILE_WRITE) || (mode & SCES_FILE_APPEND))
    {
        fx_mode |= FX_OPEN_FOR_WRITE;
    }

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return NULL;
    }

    file = (FX_FILE*)mem_block_alloc(&fs_items);

    if (file == NULL)
    {
        mutex_unlock(&media->mutex);
        return NULL;
    }

    if (fx_file_open(&media->media, file, (CHAR*)path, fx_mode) != FX_SUCCESS)
    {
        mem_block_free((uint8_t*)file);
        mutex_unlock(&media->mutex);
        return NULL;
    }

    if (mode & SCES_FILE_APPEND)
    {
        if (fx_file_seek(file, file->fx_file_current_file_size) != FX_SUCCESS)
        {
            fx_file_close(file);
            mem_block_free((uint8_t*)file);
            mutex_unlock(&media->mutex);
            return NULL;
        }
    }

    mutex_unlock(&media->mutex);
    return (scesFileHandle_t)file;
}

/// @brief Close a file
/// @details This function closes the specified file and releases its resources.
/// @param file Handle to the file to be closed
void sces_file_close(scesFileHandle_t file)
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
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_file_read(scesFileHandle_t file, void* buffer, uint32_t size, uint32_t* read_size)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_read(fx_file, buffer, size, read_size) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    return SCES_RET_OK;
}

/// @brief Write data to a file
/// @details This function writes data to the specified file from the provided buffer.
/// @param file   Handle to the file to write to
/// @param buffer Pointer to the buffer containing the data to write
/// @param size   Number of bytes to write
/// @param written_size Pointer to a variable to receive the number of bytes actually written
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_file_write(scesFileHandle_t file, const void* buffer, uint32_t size,
                             uint32_t* written_size)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_write(fx_file, (VOID*)buffer, size) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    if (written_size != NULL)
    {
        *written_size = size;
    }

    return SCES_RET_OK;
}

/// @brief Seek to a position in a file
/// @details This function sets the file position indicator for the specified file.
/// @param file   Handle to the file
/// @param offset Offset in bytes to seek
/// @return SCES_RET_OK on success, error code otherwise
/// @note The offset must be within the range of a 32-bit unsigned integer due to FileX limitations.
scesRetVal_t sces_file_seek(scesFileHandle_t file, uint64_t offset)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (offset > UINT32_MAX)
    {
        return SCES_RET_PARAM_ERR;
    }

    if (fx_file_seek(fx_file, offset) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    return SCES_RET_OK;
}

/// @brief Seek to a position in a file from the current position
/// @details This function sets the file position indicator for the specified file,
///     relative to the current position.
/// @param file   Handle to the file
/// @param offset Offset in bytes to seek from the current position
/// @return SCES_RET_OK on success, error code otherwise
/// @note The resulting position must be within the range of a 32-bit unsigned integer
///       due to FileX limitations.
scesRetVal_t sces_file_seek_from_current(scesFileHandle_t file, uint64_t offset)
{
    FX_FILE* fx_file      = (FX_FILE*)file;
    uint64_t new_position = fx_file->fx_file_current_file_offset + offset;

    if ((fx_file->fx_file_current_file_offset >= UINT32_MAX) || (offset > UINT32_MAX) ||
        (new_position > UINT32_MAX))
    {
        return SCES_RET_PARAM_ERR;
    }

    if (fx_file_seek(fx_file, (uint32_t)new_position) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }
    return SCES_RET_OK;
}

/// @brief Get the current position in a file
/// @details This function retrieves the current file position indicator for the specified file.
/// @param file Handle to the file
/// @return Current position in bytes from the beginning of the file
uint32_t sces_file_tell(scesFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;
    return fx_file->fx_file_current_file_offset;
}

/// @brief Get the size of a file
/// @details This function retrieves the size of the specified file in bytes.
/// @param file Handle to the file
/// @return Size of the file in bytes
uint32_t sces_file_size(scesFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;
    return fx_file->fx_file_current_file_size;
}

/// @brief Truncate a file to the current position
/// @details This function truncates the specified file to the current file position.
/// @param file Handle to the file
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_file_truncate(scesFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_truncate(fx_file, fx_file->fx_file_current_file_offset) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    return SCES_RET_OK;
}

/// @brief Synchronize a file
/// @details This function synchronizes the specified file, ensuring that all pending changes
///     are written to the underlying storage.
/// @param file Handle to the file
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_file_sync(scesFileHandle_t file)
{
    FX_FILE* fx_file = (FX_FILE*)file;

    if (fx_file_flush(fx_file) != FX_SUCCESS)
    {
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    return SCES_RET_OK;
}

//===============================
//  Directory operations
//===============================

/// @brief Open a directory
/// @details This function opens a directory at the specified path.
/// @param fs   Handle to the file system
/// @param path Path of the directory to open
/// @return Handle to the opened directory, or NULL on failure
scesDirHandle_t sces_dir_open(scesFsHandle_t fs, const char* path)
{
    SCES_DIR* dir;
    SCES_MEDIA* media = (SCES_MEDIA*)fs;

    if (mutex_lock(&media->mutex) != SCES_RET_OK)
    {
        return NULL;
    }

    dir = (SCES_DIR*)mem_block_alloc(&fs_items);

    if (dir == NULL)
    {
        mutex_unlock(&media->mutex);
        return NULL;
    }

    dir->media       = media;
    dir->entry_index = 0;

    strncpy(dir->path, path, FX_MAX_LONG_NAME_LEN - 1);
    dir->path[FX_MAX_LONG_NAME_LEN - 1] = '\0';

    mutex_unlock(&media->mutex);
    return (scesDirHandle_t)dir;
}

/// @brief Close a directory
/// @details This function closes the specified directory and releases its resources.
/// @param dir Handle to the directory to be closed
void sces_dir_close(scesDirHandle_t dir)
{
    mem_block_free((uint8_t*)dir);
}

/// @brief Get the next item in a directory
/// @details This function retrieves the next item in the specified directory.
/// @param dir   Handle to the directory
/// @param state Pointer to a scesFileItemState_t structure to receive the item information
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_dir_next_item_state(scesDirHandle_t dir, scesFileItemState_t* state)
{
    CHAR* default_name;
    CHAR name[FX_MAX_LONG_NAME_LEN];
    SCES_DIR* fx_dir = (SCES_DIR*)dir;

    if (mutex_lock(&fx_dir->media->mutex) != SCES_RET_OK)
    {
        return SCES_RET_OS_MUTEX_ERR;
    }

    if (fx_dir->entry_index == 0)
    {
        if (fx_directory_default_set(&fx_dir->media->media, (CHAR*)fx_dir->path) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return SCES_RET_LOW_LEVEL_FAILURE;
        }

        if (fx_directory_first_full_entry_find(&fx_dir->media->media, name, &state->attributes,
                                               &state->size, &state->time_stamp.year,
                                               &state->time_stamp.month, &state->time_stamp.day,
                                               &state->time_stamp.hour, &state->time_stamp.minute,
                                               &state->time_stamp.second) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return SCES_RET_LOW_LEVEL_FAILURE;
        }

        state->kind = (state->attributes & FX_DIRECTORY) ? SCES_FILE_DIR : SCES_FILE_FILE;

        fx_dir->entry_index = fx_dir->media->media.fx_media_directory_next_full_entry_finds;
        mutex_unlock(&fx_dir->media->mutex);

        return SCES_RET_OK;
    }

    if (fx_directory_default_get(&fx_dir->media->media, &default_name) != FX_SUCCESS)
    {
        mutex_unlock(&fx_dir->media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    if (strncmp(fx_dir->path, default_name, FX_MAX_LONG_NAME_LEN) != 0)
    {
        if (fx_directory_default_set(&fx_dir->media->media, (CHAR*)fx_dir->path) != FX_SUCCESS)
        {
            mutex_unlock(&fx_dir->media->mutex);
            return SCES_RET_LOW_LEVEL_FAILURE;
        }

        fx_dir->media->media.fx_media_directory_next_full_entry_finds = fx_dir->entry_index;
    }

    if (fx_directory_next_full_entry_find(
            &fx_dir->media->media, name, &state->attributes, &state->size, &state->time_stamp.year,
            &state->time_stamp.month, &state->time_stamp.day, &state->time_stamp.hour,
            &state->time_stamp.minute, &state->time_stamp.second) != FX_SUCCESS)
    {
        mutex_unlock(&fx_dir->media->mutex);
        return SCES_RET_LOW_LEVEL_FAILURE;
    }

    state->kind = (state->attributes & FX_DIRECTORY) ? SCES_FILE_DIR : SCES_FILE_FILE;

    fx_dir->entry_index = fx_dir->media->media.fx_media_directory_next_full_entry_finds;
    mutex_unlock(&fx_dir->media->mutex);

    return SCES_RET_OK;
}

/// @brief Rewind a directory
/// @details This function rewinds the specified directory to the beginning.
/// @param dir Handle to the directory
/// @return SCES_RET_OK on success, error code otherwise
scesRetVal_t sces_dir_rewind(scesDirHandle_t dir)
{
    SCES_DIR* fx_dir = (SCES_DIR*)dir;

    fx_dir->entry_index = 0;

    return SCES_RET_OK;
}

#ifdef __cplusplus
}
#endif
