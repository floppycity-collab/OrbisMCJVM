/**
 * @file filesystem.h
 * @brief OrbisJVM Filesystem API
 *
 * Provides file and directory operations compatible with Orbis OS using FIOS2.
 */

#ifndef ORBISJVM_FILESYSTEM_H
#define ORBISJVM_FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief File handle
 */
typedef struct orbisjvm_file* orbisjvm_file_t;

/**
 * @brief Directory handle
 */
typedef struct orbisjvm_dir* orbisjvm_dir_t;

/**
 * @brief File open flags
 */
typedef enum {
    ORBISJVM_O_RDONLY     = (1 << 0),  ///< Open for reading only
    ORBISJVM_O_WRONLY     = (1 << 1),  ///< Open for writing only
    ORBISJVM_O_RDWR       = (1 << 2),  ///< Open for reading and writing
    ORBISJVM_O_CREAT      = (1 << 3),  ///< Create file if it doesn't exist
    ORBISJVM_O_TRUNC      = (1 << 4),  ///< Truncate file to zero length
    ORBISJVM_O_APPEND     = (1 << 5),  ///< Append to end of file
    ORBISJVM_O_EXCL       = (1 << 6),  ///< Fail if file exists (with O_CREAT)
    ORBISJVM_O_SYNC       = (1 << 7),  ///< Synchronous writes
    ORBISJVM_O_DIRECTORY  = (1 << 8),  ///< Open directory (for dir operations)
    ORBISJVM_O_NOFOLLOW   = (1 << 9),  ///< Don't follow symlinks
    ORBISJVM_O_CLOEXEC    = (1 << 10), ///< Close on exec
} orbisjvm_open_flags_t;

/**
 * @brief File mode/permissions
 */
typedef enum {
    ORBISJVM_S_IRUSR = (1 << 0),  ///< Owner read
    ORBISJVM_S_IWUSR = (1 << 1),  ///< Owner write
    ORBISJVM_S_IXUSR = (1 << 2),  ///< Owner execute
    ORBISJVM_S_IRGRP = (1 << 3),  ///< Group read
    ORBISJVM_S_IWGRP = (1 << 4),  ///< Group write
    ORBISJVM_S_IXGRP = (1 << 5),  ///< Group execute
    ORBISJVM_S_IROTH = (1 << 6),  ///< Others read
    ORBISJVM_S_IWOTH = (1 << 7),  ///< Others write
    ORBISJVM_S_IXOTH = (1 << 8),  ///< Others execute

    ORBISJVM_S_IRWXU = (ORBISJVM_S_IRUSR | ORBISJVM_S_IWUSR | ORBISJVM_S_IXUSR),
    ORBISJVM_S_IRWXG = (ORBISJVM_S_IRGRP | ORBISJVM_S_IWGRP | ORBISJVM_S_IXGRP),
    ORBISJVM_S_IRWXO = (ORBISJVM_S_IROTH | ORBISJVM_S_IWOTH | ORBISJVM_S_IXOTH),

    ORBISJVM_DEFAULT_FILE_MODE = (ORBISJVM_S_IRUSR | ORBISJVM_S_IWUSR | ORBISJVM_S_IRGRP | ORBISJVM_S_IROTH),
    ORBISJVM_DEFAULT_DIR_MODE  = (ORBISJVM_S_IRWXU | ORBISJVM_S_IRWXG | ORBISJVM_S_IRWXO),
} orbisjvm_mode_t;

/**
 * @brief Seek whence values
 */
typedef enum {
    ORBISJVM_SEEK_SET = 0,  ///< Beginning of file
    ORBISJVM_SEEK_CUR = 1,  ///< Current position
    ORBISJVM_SEEK_END = 2,  ///< End of file
} orbisjvm_whence_t;

/**
 * @brief File type
 */
typedef enum {
    ORBISJVM_FILE_TYPE_UNKNOWN = 0,
    ORBISJVM_FILE_TYPE_REGULAR = 1,
    ORBISJVM_FILE_TYPE_DIR     = 2,
    ORBISJVM_FILE_TYPE_SYMLINK = 3,
    ORBISJVM_FILE_TYPE_FIFO    = 4,
    ORBISJVM_FILE_TYPE_SOCKET  = 5,
    ORBISJVM_FILE_TYPE_CHAR    = 6,
    ORBISJVM_FILE_TYPE_BLOCK   = 7,
} orbisjvm_file_type_t;

/**
 * @brief File information (stat equivalent)
 */
typedef struct {
    orbisjvm_file_type_t type;
    uint64_t size;              ///< File size in bytes
    uint64_t blocks;            ///< Number of blocks allocated
    uint32_t block_size;        ///< Block size
    uint32_t mode;              ///< File permissions
    uint32_t uid;               ///< Owner user ID
    uint32_t gid;               ///< Owner group ID
    uint64_t ino;               ///< Inode number
    uint64_t nlink;             ///< Number of hard links
    time_t atime;               ///< Last access time
    time_t mtime;               ///< Last modification time
    time_t ctime;               ///< Last status change time
    time_t birthtime;           ///< Creation time
    char name[256];             ///< File name
} orbisjvm_file_info_t;

/**
 * @brief Directory entry
 */
typedef struct {
    char name[256];
    orbisjvm_file_type_t type;
    uint64_t ino;
} orbisjvm_dirent_t;

/**
 * @brief Open a file
 *
 * @param[out] file Output file handle
 * @param path File path
 * @param flags Open flags (orbisjvm_open_flags_t)
 * @param mode File mode (used with O_CREAT)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_open(orbisjvm_file_t* file, const char* path,
                       int flags, orbisjvm_mode_t mode);

/**
 * @brief Close a file
 *
 * @param file File handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_close(orbisjvm_file_t file);

/**
 * @brief Read from a file
 *
 * @param file File handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @param[out] bytes_read Actual bytes read (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_read(orbisjvm_file_t file, void* buffer, size_t size, size_t* bytes_read);

/**
 * @brief Write to a file
 *
 * @param file File handle
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @param[out] bytes_written Actual bytes written (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_write(orbisjvm_file_t file, const void* buffer, size_t size, size_t* bytes_written);

/**
 * @brief Read from a file at offset (pread)
 *
 * @param file File handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @param offset Offset to read from
 * @param[out] bytes_read Actual bytes read (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_pread(orbisjvm_file_t file, void* buffer, size_t size,
                        int64_t offset, size_t* bytes_read);

/**
 * @brief Write to a file at offset (pwrite)
 *
 * @param file File handle
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @param offset Offset to write to
 * @param[out] bytes_written Actual bytes written (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_pwrite(orbisjvm_file_t file, const void* buffer, size_t size,
                         int64_t offset, size_t* bytes_written);

/**
 * @brief Seek to position in file
 *
 * @param file File handle
 * @param offset Offset to seek to
 * @param whence Whence (SEEK_SET, SEEK_CUR, SEEK_END)
 * @param[out] new_pos New file position (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_seek(orbisjvm_file_t file, int64_t offset,
                       orbisjvm_whence_t whence, int64_t* new_pos);

/**
 * @brief Get current file position
 *
 * @param file File handle
 * @param[out] pos Current position
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_tell(orbisjvm_file_t file, int64_t* pos);

/**
 * @brief Flush file buffers to disk
 *
 * @param file File handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_sync(orbisjvm_file_t file);

/**
 * @brief Truncate file to specified length
 *
 * @param file File handle
 * @param length New file length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_truncate(orbisjvm_file_t file, int64_t length);

/**
 * @brief Get file information
 *
 * @param file File handle
 * @param[out] info File information structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_stat(orbisjvm_file_t file, orbisjvm_file_info_t* info);

/**
 * @brief Get file information by path
 *
 * @param path File path
 * @param[out] info File information structure
 * @param follow_symlinks 1 to follow symlinks, 0 to not follow
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_lstat(const char* path, orbisjvm_file_info_t* info, int follow_symlinks);

/**
 * @brief Create a directory
 *
 * @param path Directory path
 * @param mode Directory permissions
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dir_create(const char* path, orbisjvm_mode_t mode);

/**
 * @brief Create directory recursively (mkdir -p equivalent)
 *
 * @param path Directory path
 * @param mode Directory permissions
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dir_create_recursive(const char* path, orbisjvm_mode_t mode);

/**
 * @brief Remove a directory
 *
 * @param path Directory path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dir_remove(const char* path);

/**
 * @brief Open a directory for reading
 *
 * @param[out] dir Output directory handle
 * @param path Directory path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dir_open(orbisjvm_dir_t* dir, const char* path);

/**
 * @brief Read next directory entry
 *
 * @param dir Directory handle
 * @param[out] entry Output directory entry
 * @return 0 on success, negative error code on failure, 1 on end of directory
 */
int orbisjvm_dir_read(orbisjvm_dir_t dir, orbisjvm_dirent_t* entry);

/**
 * @brief Close a directory
 *
 * @param dir Directory handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dir_close(orbisjvm_dir_t dir);

/**
 * @brief Remove a file
 *
 * @param path File path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_remove(const char* path);

/**
 * @brief Rename a file or directory
 *
 * @param old_path Old path
 * @param new_path New path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_file_rename(const char* old_path, const char* new_path);

/**
 * @brief Check if file exists
 *
 * @param path File path
 * @return 1 if exists, 0 if not, negative error code on failure
 */
int orbisjvm_file_exists(const char* path);

/**
 * @brief Check if path is a directory
 *
 * @param path Directory path
 * @return 1 if directory, 0 if not, negative error code on failure
 */
int orbisjvm_path_is_dir(const char* path);

/**
 * @brief Get canonical absolute path
 *
 * @param path Input path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_realpath(const char* path, char* buffer, size_t buflen);

/**
 * @brief Get application data directory
 *
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_app_data_dir(char* buffer, size_t buflen);

/**
 * @brief Get application cache directory
 *
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_app_cache_dir(char* buffer, size_t buflen);

/**
 * @brief Get application config directory
 *
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_app_config_dir(char* buffer, size_t buflen);

/**
 * @brief Get temporary directory
 *
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_temp_dir(char* buffer, size_t buflen);

/**
 * @brief Get current working directory
 *
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_cwd(char* buffer, size_t buflen);

/**
 * @brief Set current working directory
 *
 * @param path New working directory
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_cwd(const char* path);

/**
 * @brief Path separator character
 */
#define ORBISJVM_PATH_SEP '/'

/**
 * @brief Join two path components
 *
 * @param base Base path
 * @param component Path component to append
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_join(const char* base, const char* component, char* buffer, size_t buflen);

/**
 * @brief Get directory name from path
 *
 * @param path Full path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_dirname(const char* path, char* buffer, size_t buflen);

/**
 * @brief Get base name from path
 *
 * @param path Full path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_basename(const char* path, char* buffer, size_t buflen);

/**
 * @brief Get file extension
 *
 * @param path File path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_extension(const char* path, char* buffer, size_t buflen);

/**
 * @brief Normalize path (resolve . and ..)
 *
 * @param path Input path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_normalize(const char* path, char* buffer, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_FILESYSTEM_H