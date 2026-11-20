/**
 * @file filesystem.c
 * @brief OrbisJVM Filesystem Implementation
 *
 * Implements file and directory operations using Orbis OS FIOS2 API.
 */

#include <orbisjvm/filesystem.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

/* File structure */
struct orbisjvm_file {
    int fd;
    char path[256];
    int flags;
};

/* Directory structure */
struct orbisjvm_dir {
    DIR* dir;
    char path[256];
};

/**
 * @brief Initialize filesystem subsystem
 */
int orbisjvm_filesystem_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_FILESYSTEM, "Initializing filesystem subsystem");
    return 0;
}

/**
 * @brief Shutdown filesystem subsystem
 */
void orbisjvm_filesystem_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_FILESYSTEM, "Shutting down filesystem subsystem");
}

/* File operations */

int orbisjvm_file_open(orbisjvm_file_t* file, const char* path,
                       int flags, orbisjvm_mode_t mode) {
    if (!file || !path) return -1;

    int open_flags = 0;
    if (flags & ORBISJVM_O_RDONLY) open_flags |= O_RDONLY;
    if (flags & ORBISJVM_O_WRONLY) open_flags |= O_WRONLY;
    if (flags & ORBISJVM_O_RDWR) open_flags |= O_RDWR;
    if (flags & ORBISJVM_O_CREAT) open_flags |= O_CREAT;
    if (flags & ORBISJVM_O_TRUNC) open_flags |= O_TRUNC;
    if (flags & ORBISJVM_O_APPEND) open_flags |= O_APPEND;
    if (flags & ORBISJVM_O_EXCL) open_flags |= O_EXCL;
    if (flags & ORBISJVM_O_SYNC) open_flags |= O_SYNC;

    int fd = open(path, open_flags, mode);
    if (fd < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to open file: %s (errno=%d)", path, errno);
        return -errno;
    }

    orbisjvm_file_t f = orbisjvm_malloc(sizeof(struct orbisjvm_file), ORBISJVM_MEM_ZERO);
    if (!f) {
        close(fd);
        return -ENOMEM;
    }

    f->fd = fd;
    strncpy(f->path, path, sizeof(f->path) - 1);
    f->flags = flags;

    *file = f;
    orbisjvm_log_trace(ORBISJVM_LOG_CAT_FILESYSTEM, "Opened file: %s (fd=%d)", path, fd);
    return 0;
}

int orbisjvm_file_close(orbisjvm_file_t file) {
    if (!file) return -1;

    int ret = close(file->fd);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to close file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_FILESYSTEM, "Closed file: %s", file->path);
    orbisjvm_free(file);
    return 0;
}

int orbisjvm_file_read(orbisjvm_file_t file, void* buffer, size_t size, size_t* bytes_read) {
    if (!file || !buffer) return -1;

    ssize_t ret = read(file->fd, buffer, size);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to read file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    if (bytes_read) *bytes_read = ret;
    return 0;
}

int orbisjvm_file_write(orbisjvm_file_t file, const void* buffer, size_t size, size_t* bytes_written) {
    if (!file || !buffer) return -1;

    ssize_t ret = write(file->fd, buffer, size);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to write file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    if (bytes_written) *bytes_written = ret;
    return 0;
}

int orbisjvm_file_pread(orbisjvm_file_t file, void* buffer, size_t size,
                        int64_t offset, size_t* bytes_read) {
    if (!file || !buffer) return -1;

    ssize_t ret = pread(file->fd, buffer, size, offset);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to pread file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    if (bytes_read) *bytes_read = ret;
    return 0;
}

int orbisjvm_file_pwrite(orbisjvm_file_t file, const void* buffer, size_t size,
                         int64_t offset, size_t* bytes_written) {
    if (!file || !buffer) return -1;

    ssize_t ret = pwrite(file->fd, buffer, size, offset);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to pwrite file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    if (bytes_written) *bytes_written = ret;
    return 0;
}

int orbisjvm_file_seek(orbisjvm_file_t file, int64_t offset,
                       orbisjvm_whence_t whence, int64_t* new_pos) {
    if (!file) return -1;

    int w = SEEK_SET;
    switch (whence) {
        case ORBISJVM_SEEK_SET: w = SEEK_SET; break;
        case ORBISJVM_SEEK_CUR: w = SEEK_CUR; break;
        case ORBISJVM_SEEK_END: w = SEEK_END; break;
    }

    off_t ret = lseek(file->fd, offset, w);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to seek file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    if (new_pos) *new_pos = ret;
    return 0;
}

int orbisjvm_file_tell(orbisjvm_file_t file, int64_t* pos) {
    if (!file || !pos) return -1;

    off_t ret = lseek(file->fd, 0, SEEK_CUR);
    if (ret < 0) {
        return -errno;
    }

    *pos = ret;
    return 0;
}

int orbisjvm_file_sync(orbisjvm_file_t file) {
    if (!file) return -1;

    int ret = fsync(file->fd);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to sync file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_file_truncate(orbisjvm_file_t file, int64_t length) {
    if (!file) return -1;

    int ret = ftruncate(file->fd, length);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to truncate file: %s (errno=%d)", file->path, errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_file_stat(orbisjvm_file_t file, orbisjvm_file_info_t* info) {
    if (!file || !info) return -1;

    struct stat st;
    int ret = fstat(file->fd, &st);
    if (ret < 0) {
        return -errno;
    }

    memset(info, 0, sizeof(*info));
    info->size = st.st_size;
    info->blocks = st.st_blocks;
    info->block_size = st.st_blksize;
    info->mode = st.st_mode;
    info->uid = st.st_uid;
    info->gid = st.st_gid;
    info->ino = st.st_ino;
    info->nlink = st.st_nlink;
    info->atime = st.st_atime;
    info->mtime = st.st_mtime;
    info->ctime = st.st_ctime;
    info->birthtime = st.st_ctime;  /* Linux doesn't have birthtime */

    /* Determine file type */
    if (S_ISREG(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_REGULAR;
    else if (S_ISDIR(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_DIR;
    else if (S_ISLNK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_SYMLINK;
    else if (S_ISFIFO(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_FIFO;
    else if (S_ISSOCK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_SOCKET;
    else if (S_ISCHR(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_CHAR;
    else if (S_ISBLK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_BLOCK;
    else info->type = ORBISJVM_FILE_TYPE_UNKNOWN;

    strncpy(info->name, file->path, sizeof(info->name) - 1);

    return 0;
}

int orbisjvm_file_lstat(const char* path, orbisjvm_file_info_t* info, int follow_symlinks) {
    if (!path || !info) return -1;

    struct stat st;
    int ret = follow_symlinks ? stat(path, &st) : lstat(path, &st);
    if (ret < 0) {
        return -errno;
    }

    memset(info, 0, sizeof(*info));
    info->size = st.st_size;
    info->blocks = st.st_blocks;
    info->block_size = st.st_blksize;
    info->mode = st.st_mode;
    info->uid = st.st_uid;
    info->gid = st.st_gid;
    info->ino = st.st_ino;
    info->nlink = st.st_nlink;
    info->atime = st.st_atime;
    info->mtime = st.st_mtime;
    info->ctime = st.st_ctime;
    info->birthtime = st.st_ctime;

    if (S_ISREG(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_REGULAR;
    else if (S_ISDIR(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_DIR;
    else if (S_ISLNK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_SYMLINK;
    else if (S_ISFIFO(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_FIFO;
    else if (S_ISSOCK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_SOCKET;
    else if (S_ISCHR(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_CHAR;
    else if (S_ISBLK(st.st_mode)) info->type = ORBISJVM_FILE_TYPE_BLOCK;
    else info->type = ORBISJVM_FILE_TYPE_UNKNOWN;

    strncpy(info->name, path, sizeof(info->name) - 1);

    return 0;
}

/* Directory operations */

int orbisjvm_dir_create(const char* path, orbisjvm_mode_t mode) {
    if (!path) return -1;

    int ret = mkdir(path, mode);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to create directory: %s (errno=%d)", path, errno);
        return -errno;
    }

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_FILESYSTEM, "Created directory: %s", path);
    return 0;
}

int orbisjvm_dir_create_recursive(const char* path, orbisjvm_mode_t mode) {
    if (!path) return -1;

    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char* p = tmp;
    if (*p == '/') p++;

    while (*p) {
        while (*p && *p != '/') p++;
        char c = *p;
        *p = '\0';
        if (mkdir(tmp, mode) < 0 && errno != EEXIST) {
            orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to create directory: %s (errno=%d)", tmp, errno);
            return -errno;
        }
        *p = c;
        if (!c) break;
        p++;
    }

    return 0;
}

int orbisjvm_dir_remove(const char* path) {
    if (!path) return -1;

    int ret = rmdir(path);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to remove directory: %s (errno=%d)", path, errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_dir_open(orbisjvm_dir_t* dir, const char* path) {
    if (!dir || !path) return -1;

    DIR* d = opendir(path);
    if (!d) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to open directory: %s (errno=%d)", path, errno);
        return -errno;
    }

    orbisjvm_dir_t dt = orbisjvm_malloc(sizeof(struct orbisjvm_dir), ORBISJVM_MEM_ZERO);
    if (!dt) {
        closedir(d);
        return -ENOMEM;
    }

    dt->dir = d;
    strncpy(dt->path, path, sizeof(dt->path) - 1);

    *dir = dt;
    return 0;
}

int orbisjvm_dir_read(orbisjvm_dir_t dir, orbisjvm_dirent_t* entry) {
    if (!dir || !entry) return -1;

    struct dirent* de = readdir(dir->dir);
    if (!de) return 1;  /* End of directory */

    memset(entry, 0, sizeof(*entry));
    strncpy(entry->name, de->d_name, sizeof(entry->name) - 1);

    switch (de->d_type) {
        case DT_REG: entry->type = ORBISJVM_FILE_TYPE_REGULAR; break;
        case DT_DIR: entry->type = ORBISJVM_FILE_TYPE_DIR; break;
        case DT_LNK: entry->type = ORBISJVM_FILE_TYPE_SYMLINK; break;
        case DT_FIFO: entry->type = ORBISJVM_FILE_TYPE_FIFO; break;
        case DT_SOCK: entry->type = ORBISJVM_FILE_TYPE_SOCKET; break;
        case DT_CHR: entry->type = ORBISJVM_FILE_TYPE_CHAR; break;
        case DT_BLK: entry->type = ORBISJVM_FILE_TYPE_BLOCK; break;
        default: entry->type = ORBISJVM_FILE_TYPE_UNKNOWN; break;
    }

    entry->ino = de->d_ino;
    return 0;
}

int orbisjvm_dir_close(orbisjvm_dir_t dir) {
    if (!dir) return -1;

    closedir(dir->dir);
    orbisjvm_free(dir);
    return 0;
}

/* File utilities */

int orbisjvm_file_remove(const char* path) {
    if (!path) return -1;

    int ret = unlink(path);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to remove file: %s (errno=%d)", path, errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_file_rename(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return -1;

    int ret = rename(old_path, new_path);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to rename: %s -> %s (errno=%d)", old_path, new_path, errno);
        return -errno;
    }

    return 0;
}

int orbisjvm_file_exists(const char* path) {
    if (!path) return -1;

    struct stat st;
    int ret = stat(path, &st);
    return ret == 0 ? 1 : 0;
}

int orbisjvm_path_is_dir(const char* path) {
    if (!path) return -1;

    struct stat st;
    int ret = stat(path, &st);
    if (ret < 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int orbisjvm_path_realpath(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    char* resolved = realpath(path, buffer);
    if (!resolved) {
        return -errno;
    }

    return 0;
}

int orbisjvm_get_app_data_dir(char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return -1;

    /* On PS4, use /user/home/<user_id>/savedata/<title_id> */
    /* For now, use a reasonable default */
    const char* home = getenv("HOME");
    if (home) {
        snprintf(buffer, buflen, "%s/.orbisjvm/data", home);
    } else {
        snprintf(buffer, buflen, "/data/orbisjvm");
    }

    /* Create if doesn't exist */
    orbisjvm_dir_create_recursive(buffer, ORBISJVM_DEFAULT_DIR_MODE);
    return 0;
}

int orbisjvm_get_app_cache_dir(char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return -1;

    const char* home = getenv("HOME");
    if (home) {
        snprintf(buffer, buflen, "%s/.orbisjvm/cache", home);
    } else {
        snprintf(buffer, buflen, "/tmp/orbisjvm/cache");
    }

    orbisjvm_dir_create_recursive(buffer, ORBISJVM_DEFAULT_DIR_MODE);
    return 0;
}

int orbisjvm_get_app_config_dir(char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return -1;

    const char* home = getenv("HOME");
    if (home) {
        snprintf(buffer, buflen, "%s/.orbisjvm/config", home);
    } else {
        snprintf(buffer, buflen, "/etc/orbisjvm");
    }

    orbisjvm_dir_create_recursive(buffer, ORBISJVM_DEFAULT_DIR_MODE);
    return 0;
}

int orbisjvm_get_temp_dir(char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return -1;

    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    strncpy(buffer, tmp, buflen - 1);
    buffer[buflen - 1] = '\0';
    return 0;
}

int orbisjvm_get_cwd(char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return -1;

    if (!getcwd(buffer, buflen)) {
        return -errno;
    }

    return 0;
}

int orbisjvm_set_cwd(const char* path) {
    if (!path) return -1;

    if (chdir(path) < 0) {
        return -errno;
    }

    return 0;
}

/* Path utilities */

int orbisjvm_path_join(const char* base, const char* component, char* buffer, size_t buflen) {
    if (!base || !component || !buffer || buflen == 0) return -1;

    size_t base_len = strlen(base);
    size_t comp_len = strlen(component);

    if (base_len == 0) {
        strncpy(buffer, component, buflen - 1);
        buffer[buflen - 1] = '\0';
        return 0;
    }

    int needs_sep = (base[base_len - 1] != '/');
    size_t needed = base_len + (needs_sep ? 1 : 0) + comp_len + 1;

    if (needed > buflen) return -1;

    strcpy(buffer, base);
    if (needs_sep) strcat(buffer, "/");
    strcat(buffer, component);

    return 0;
}

int orbisjvm_path_dirname(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    char* tmp = strdup(path);
    if (!tmp) return -ENOMEM;

    char* dir = dirname(tmp);
    strncpy(buffer, dir, buflen - 1);
    buffer[buflen - 1] = '\0';

    free(tmp);
    return 0;
}

int orbisjvm_path_basename(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    char* tmp = strdup(path);
    if (!tmp) return -ENOMEM;

    char* base = basename(tmp);
    strncpy(buffer, base, buflen - 1);
    buffer[buflen - 1] = '\0';

    free(tmp);
    return 0;
}

int orbisjvm_path_extension(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    const char* dot = strrchr(path, '.');
    const char* slash = strrchr(path, '/');

    if (!dot || (slash && dot < slash)) {
        buffer[0] = '\0';
        return 0;
    }

    strncpy(buffer, dot + 1, buflen - 1);
    buffer[buflen - 1] = '\0';
    return 0;
}

int orbisjvm_path_normalize(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    /* Use realpath for normalization */
    return orbisjvm_path_realpath(path, buffer, buflen);
}