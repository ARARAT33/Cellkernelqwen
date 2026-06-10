/*
 * CellKernel - VFS (Virtual File System) Core
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define MAX_FILESYSTEMS     16
#define MAX_MOUNT_POINTS    64
#define MAX_OPEN_FILES      4096
#define VFS_MAGIC           0x56465321  /* "VFS!" */

/* File types */
#define FILE_TYPE_UNKNOWN   0
#define FILE_TYPE_REGULAR   1
#define FILE_TYPE_DIRECTORY 2
#define FILE_TYPE_CHAR      3
#define FILE_TYPE_BLOCK     4
#define FILE_TYPE_FIFO      5
#define FILE_TYPE_SOCKET    6
#define FILE_TYPE_SYMLINK   7

/* File operations */
typedef struct {
    int (*open)(void *inode, void *file);
    int (*close)(void *file);
    ssize_t (*read)(void *file, void *buf, size_t count, off_t offset);
    ssize_t (*write)(void *file, const void *buf, size_t count, off_t offset);
    int (*seek)(void *file, off_t offset, int whence);
    int (*stat)(void *inode, void *stat_buf);
    int (*readdir)(void *file, void *dir_entry);
    int (*mkdir)(void *parent, const char *name);
    int (*unlink)(void *parent, const char *name);
} file_operations_t;

/* Inode structure */
typedef struct vfs_inode {
    uint64_t ino;                       /* Inode number */
    uint32_t mode;                      /* File mode/permissions */
    uint32_t uid;                       /* Owner UID */
    uint32_t gid;                       /* Owner GID */
    uint64_t size;                      /* File size */
    uint64_t atime;                     /* Access time */
    uint64_t mtime;                     /* Modification time */
    uint64_t ctime;                     /* Change time */
    uint32_t nlink;                     /* Link count */
    uint32_t type;                      /* File type */
    void *private_data;                 /* Filesystem-specific data */
    file_operations_t *fops;            /* File operations */
    atomic_t refcount;                  /* Reference count */
} vfs_inode_t;

/* Mount point */
typedef struct {
    char path[256];                     /* Mount point path */
    struct vfs_filesystem *fs;          /* Filesystem type */
    vfs_inode_t *root_inode;            /* Root inode */
    void *private_data;                 /* Filesystem-specific data */
    uint32_t flags;                     /* Mount flags */
} mount_point_t;

/* Filesystem type */
typedef struct vfs_filesystem {
    char name[32];                      /* Filesystem name */
    uint32_t magic;                     /* Filesystem magic */
    
    int (*mount)(mount_point_t *mp, const char *source, uint32_t flags);
    int (*umount)(mount_point_t *mp);
    int (*statfs)(mount_point_t *mp, void *stat_buf);
    
    file_operations_t *fops;            /* Default file operations */
    
    struct vfs_filesystem *next;        /* Next filesystem in list */
} vfs_filesystem_t;

/* Open file descriptor */
typedef struct vfs_file {
    uint32_t fd;                        /* File descriptor number */
    vfs_inode_t *inode;                 /* Inode */
    mount_point_t *mount;               /* Mount point */
    uint32_t flags;                     /* Open flags */
    off_t position;                     /* Current position */
    void *private_data;                 /* File-specific data */
    atomic_t refcount;                  /* Reference count */
} vfs_file_t;

/* Global VFS state */
static vfs_filesystem_t *g_filesystems = NULL;
static mount_point_t g_mount_points[MAX_MOUNT_POINTS];
static vfs_file_t g_open_files[MAX_OPEN_FILES];
static atomic_t g_next_fd = ATOMIC_INIT(0);

/* External functions */
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

/* Initialize VFS subsystem */
void vfs_init(void) {
    serial_println("[*] Initializing VFS...");
    
    memset(g_mount_points, 0, sizeof(g_mount_points));
    memset(g_open_files, 0, sizeof(g_open_files));
    
    /* Create root mount point */
    strncpy(g_mount_points[0].path, "/", sizeof(g_mount_points[0].path) - 1);
    
    serial_println("[+] VFS initialized");
}

/* Register filesystem type */
int vfs_register_filesystem(vfs_filesystem_t *fs) {
    if (!fs || !fs->name[0]) {
        return CK_ERROR_INVALID_ARG;
    }
    
    fs->next = g_filesystems;
    g_filesystems = fs;
    
    serial_print("[-] Registered filesystem: ");
    serial_println(fs->name);
    
    return CK_SUCCESS;
}

/* Unregister filesystem type */
int vfs_unregister_filesystem(const char *name) {
    vfs_filesystem_t **prev = &g_filesystems;
    vfs_filesystem_t *curr = g_filesystems;
    
    while (curr) {
        if (strncmp(curr->name, name, sizeof(curr->name)) == 0) {
            *prev = curr->next;
            return CK_SUCCESS;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    
    return CK_ERROR_NOT_FOUND;
}

/* Find filesystem by name */
static vfs_filesystem_t *find_filesystem(const char *name) {
    vfs_filesystem_t *fs = g_filesystems;
    
    while (fs) {
        if (strncmp(fs->name, name, sizeof(fs->name)) == 0) {
            return fs;
        }
        fs = fs->next;
    }
    
    return NULL;
}

/* Mount filesystem */
int vfs_mount(const char *source, const char *target, const char *fstype,
              uint32_t flags, void *data) {
    vfs_filesystem_t *fs = find_filesystem(fstype);
    if (!fs) {
        serial_print("[!] Unknown filesystem: ");
        serial_println(fstype);
        return CK_ERROR_NOT_FOUND;
    }
    
    /* Find free mount point slot */
    int i;
    for (i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (g_mount_points[i].path[0] == '\0') {
            break;
        }
    }
    
    if (i >= MAX_MOUNT_POINTS) {
        return CK_ERROR_NO_MEMORY;
    }
    
    mount_point_t *mp = &g_mount_points[i];
    strncpy(mp->path, target, sizeof(mp->path) - 1);
    mp->fs = fs;
    mp->flags = flags;
    
    /* Call filesystem mount function */
    if (fs->mount) {
        int ret = fs->mount(mp, source, flags);
        if (ret != CK_SUCCESS) {
            mp->path[0] = '\0';
            return ret;
        }
    }
    
    serial_print("[+] Mounted ");
    serial_print(fstype);
    serial_print(" on ");
    serial_println(target);
    
    return CK_SUCCESS;
}

/* Unmount filesystem */
int vfs_umount(const char *target) {
    int i;
    for (i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (g_mount_points[i].path[0] && 
            strncmp(g_mount_points[i].path, target, sizeof(g_mount_points[i].path)) == 0) {
            
            mount_point_t *mp = &g_mount_points[i];
            
            /* Call filesystem umount function */
            if (mp->fs && mp->fs->umount) {
                int ret = mp->fs->umount(mp);
                if (ret != CK_SUCCESS) {
                    return ret;
                }
            }
            
            mp->path[0] = '\0';
            serial_print("[+] Unmounted ");
            serial_println(target);
            
            return CK_SUCCESS;
        }
    }
    
    return CK_ERROR_NOT_FOUND;
}

/* Allocate file descriptor */
static int alloc_fd(void) {
    int fd = atomic_inc(&g_next_fd);
    
    if (fd >= MAX_OPEN_FILES) {
        atomic_dec(&g_next_fd);
        return CK_ERROR_NO_MEMORY;
    }
    
    return fd;
}

/* Free file descriptor */
static void free_fd(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        g_open_files[fd].inode = NULL;
        g_open_files[fd].flags = 0;
    }
}

/* Open file */
int vfs_open(const char *pathname, int flags, int mode) {
    /* TODO: Path resolution and actual open implementation */
    
    int fd = alloc_fd();
    if (fd < 0) {
        return fd;
    }
    
    vfs_file_t *file = &g_open_files[fd];
    file->fd = fd;
    file->flags = flags;
    file->position = 0;
    atomic_set(&file->refcount, 1);
    
    return fd;
}

/* Close file */
int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return CK_ERROR_INVALID_ARG;
    }
    
    vfs_file_t *file = &g_open_files[fd];
    
    if (atomic_read(&file->refcount) > 0) {
        atomic_dec(&file->refcount);
        
        if (file->inode && file->inode->fops && file->inode->fops->close) {
            file->inode->fops->close(file);
        }
    }
    
    free_fd(fd);
    
    return CK_SUCCESS;
}

/* Read from file */
ssize_t vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return CK_ERROR_INVALID_ARG;
    }
    
    vfs_file_t *file = &g_open_files[fd];
    
    if (!file->inode || !file->inode->fops || !file->inode->fops->read) {
        return CK_ERROR_NOT_SUPPORTED;
    }
    
    return file->inode->fops->read(file, buf, count, file->position);
}

/* Write to file */
ssize_t vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return CK_ERROR_INVALID_ARG;
    }
    
    vfs_file_t *file = &g_open_files[fd];
    
    if (!file->inode || !file->inode->fops || !file->inode->fops->write) {
        return CK_ERROR_NOT_SUPPORTED;
    }
    
    return file->inode->fops->write(file, buf, count, file->position);
}
