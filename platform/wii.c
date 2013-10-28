/*
 * Posix-esque support routines for PhysicsFS.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#define __PHYSICSFS_INTERNAL__
#include "physfs_platforms.h"

#ifdef PHYSFS_PLATFORM_WII

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include "realpath.h"

#include "physfs_internal.h"

const char *__PHYSFS_platformDirSeparator = "/";

int __PHYSFS_platformInit(void)
{
    return (1);  /* always succeed. */
} /* __PHYSFS_platformInit */

int __PHYSFS_platformDeinit(void)
{
    return (1);  /* always succeed. */
} /* __PHYSFS_platformDeinit */

/* open() helper function */
static void *doOpen(const char *filename, int mode)
{
    const int appending = (mode & O_APPEND);
    int fd;
    int *retval;
    errno = 0;

    /* O_APPEND doesn't actually behave as we'd like. */
    mode &= ~O_APPEND;

    fd = open(filename, mode, S_IRUSR | S_IWUSR);
    BAIL_IF_MACRO(fd < 0, strerror(errno), NULL);

    if (appending)
    {
        if (lseek(fd, 0, SEEK_END) < 0)
        {
            close(fd);
            BAIL_MACRO(strerror(errno), NULL);
        } /* if */
    } /* if */

    retval = (int *) allocator.Malloc(sizeof (int));
    if (retval == NULL)
    {
        close(fd);
        BAIL_MACRO(ERR_OUT_OF_MEMORY, NULL);
    } /* if */

    *retval = fd;
    return ((void *) retval);
} /* doOpen */

void *__PHYSFS_platformOpenRead(const char *filename)
{
    return (doOpen(filename, O_RDONLY));
} /* __PHYSFS_platformOpenRead */

void *__PHYSFS_platformOpenWrite(const char *filename)
{
    return (doOpen(filename, O_WRONLY | O_CREAT | O_TRUNC));
} /* __PHYSFS_platformOpenWrite */

void *__PHYSFS_platformOpenAppend(const char *filename)
{
    return (doOpen(filename, O_WRONLY | O_CREAT | O_APPEND));
} /* __PHYSFS_platformOpenAppend */

PHYSFS_sint64 __PHYSFS_platformRead(void *opaque, void *buffer,
                                    PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    int fd = *((int *) opaque);
    int max = size * count;
    int rc = read(fd, buffer, max);

    BAIL_IF_MACRO(rc == -1, strerror(errno), rc);
    assert(rc <= max);

    if ((rc < max) && (size > 1))
        lseek(fd, -(rc % size), SEEK_CUR); /* rollback to object boundary. */

    return (rc / size);
} /* __PHYSFS_platformRead */

PHYSFS_sint64 __PHYSFS_platformWrite(void *opaque, const void *buffer,
                                     PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    int fd = *((int *) opaque);
    int max = size * count;
    int rc = write(fd, (void *) buffer, max);

    BAIL_IF_MACRO(rc == -1, strerror(errno), rc);
    assert(rc <= max);

    if ((rc < max) && (size > 1))
        lseek(fd, -(rc % size), SEEK_CUR); /* rollback to object boundary. */

    return (rc / size);
} /* __PHYSFS_platformWrite */

int __PHYSFS_platformSeek(void *opaque, PHYSFS_uint64 pos)
{
    int fd = *((int *) opaque);

    BAIL_IF_MACRO(lseek(fd, (int) pos, SEEK_SET) == -1, strerror(errno), 0);

    return (1);
} /* __PHYSFS_platformSeek */

PHYSFS_sint64 __PHYSFS_platformTell(void *opaque)
{
    int fd = *((int *) opaque);
    PHYSFS_sint64 retval;

    retval = (PHYSFS_sint64) lseek(fd, 0, SEEK_CUR);
    BAIL_IF_MACRO(retval == -1, strerror(errno), -1);

    return (retval);
} /* __PHYSFS_platformTell */

PHYSFS_sint64 __PHYSFS_platformFileLength(void *opaque)
{
    int fd = *((int *) opaque);
    struct stat statbuf;

    BAIL_IF_MACRO(fstat(fd, &statbuf) == -1, strerror(errno), -1);

    return ((PHYSFS_sint64) statbuf.st_size);
} /* __PHYSFS_platformFileLength */

int __PHYSFS_platformEOF(void *opaque)
{
    PHYSFS_sint64 pos = __PHYSFS_platformTell(opaque);
    PHYSFS_sint64 len = __PHYSFS_platformFileLength(opaque);

    return ((pos < 0) || (len < 0) || (pos >= len));
} /* __PHYSFS_platformEOF */

int __PHYSFS_platformFlush(void *opaque)
{
    int fd = *((int *) opaque);

    BAIL_IF_MACRO(fsync(fd) == -1, strerror(errno), 0);

    return (1);
} /* __PHYSFS_platformFlush */

int __PHYSFS_platformClose(void *opaque)
{
    int fd = *((int *) opaque);

    BAIL_IF_MACRO(close(fd) == -1, strerror(errno), 0);
    allocator.Free(opaque);

    return (1);
} /* __PHYSFS_platformClose */

#ifdef PHYSFS_NO_CDROM_SUPPORT

/* Stub version for platforms without CD-ROM support. */
void __PHYSFS_platformDetectAvailableCDs(PHYSFS_StringCallback cb, void *data)
{
/* empty on purpose */
} /* __PHYSFS_platformDetectAvailableCDs */

#else

#error TODO: Wii DVD ROM Driver

#endif /* PHYSFS_NO_CDROM_SUPPORT */

char *__PHYSFS_platformCalcBaseDir(const char *argv0)
{
    // TODO: rewrite

    // set the default path if libfat couldnt set it
    // this allows loading over tcp/usbgecko
    char *cwd = (char *) allocator.Malloc(MAXPATHLEN);
    BAIL_IF_MACRO(cwd == NULL, ERR_OUT_OF_MEMORY, NULL);

    if (getcwd(cwd, MAXPATHLEN)) {
        size_t len = strlen(cwd);

        if (len > 2 && (cwd[len - 1] == ':' || cwd[len - 2] == ':')) {
            chdir("/");
            cwd[0] = '/';
            cwd[1] = '\0';
        }
    }

    return (cwd);
} /* __PHYSFS_platformCalcBaseDir */

char *__PHYSFS_platformGetUserName(void)
{
    return (NULL);  /* "default", we don't care (yet) */
} /* __PHYSFS_platformGetUserName */

char *__PHYSFS_platformGetUserDir(void)
{
    // TODO: rewrite

    // set the default path
    char *ud = (char *) allocator.Malloc(2);
    BAIL_IF_MACRO(ud == NULL, ERR_OUT_OF_MEMORY, NULL);

    ud[0] = '/';
    ud[1] = '\0';

    return (ud);
} /* __PHYSFS_platformGetUserDir */

#if (defined PHYSFS_NO_THREAD_SUPPORT)

void *__PHYSFS_platformGetThreadID(void) { return ((void *) 0x0001); }

#else

#error TODO: Wii Threads

#endif /* PHYSFS_NO_THREAD_SUPPORT */

int __PHYSFS_platformExists(const char *fname)
{
    struct stat statbuf;
    BAIL_IF_MACRO(stat(fname, &statbuf) == -1, strerror(errno), 0);

    return (1);
} /* __PHYSFS_platformExists */

PHYSFS_sint64 __PHYSFS_platformGetLastModTime(const char *fname)
{
    struct stat statbuf;

    BAIL_IF_MACRO(stat(fname, &statbuf) < 0, strerror(errno), -1);

    return (statbuf.st_mtime);
} /* __PHYSFS_platformGetLastModTime */

int __PHYSFS_platformIsSymLink(const char *fname)
{
    return (0);  /* not available */
} /* __PHYSFS_platformIsSymlink */

int __PHYSFS_platformIsDirectory(const char *fname)
{
    struct stat statbuf;
    BAIL_IF_MACRO(stat(fname, &statbuf) == -1, strerror(errno), 0);

    return ((S_ISDIR(statbuf.st_mode)) ? 1 : 0);
} /* __PHYSFS_platformIsDirectory */

char *__PHYSFS_platformCvtToDependent(const char *prepend,
                                      const char *dirName,
                                      const char *append)
{
    int len = ((prepend) ? strlen(prepend) : 0) +
              ((append) ? strlen(append) : 0) +
              strlen(dirName) + 1;
    char *retval = (char *) allocator.Malloc(len);
    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    /* platform-independent notation is Unix-style already. :) */

    if (prepend)
        strcpy(retval, prepend);
    else
        retval[0] = '\0';

    strcat(retval, dirName);

    if (append)
        strcat(retval, append);

    return (retval);
} /* __PHYSFS_platformCvtToDependent */

void __PHYSFS_platformEnumerateFiles(const char *dirname,
                                     int omitSymLinks,
                                     PHYSFS_EnumFilesCallback callback,
                                     const char *origdir,
                                     void *callbackdata)
{
    DIR *dir;
    struct dirent *ent;
    errno = 0;

    dir = opendir(dirname);
    if (dir == NULL)
    {
        return;
    } /* if */

    while ((ent = readdir(dir)) != NULL)
    {
        /* ignore metaentries */
        if (strcmp(ent->d_name, ".") == 0)
            continue;

        if (strcmp(ent->d_name, "..") == 0)
            continue;

        callback(callbackdata, origdir, ent->d_name);
    } /* while */

    closedir(dir);
} /* __PHYSFS_platformEnumerateFiles */

char *__PHYSFS_platformCurrentDir(void)
{
    /*
     * This can't just do platformRealPath("."), since that would eventually
     *  just end up calling back into here.
     */
    int allocSize = 0;
    char *retval = NULL;
    char *ptr;

    do
    {
        allocSize += 100;
        ptr = (char *) allocator.Realloc(retval, allocSize);
        if (ptr == NULL)
        {
            if (retval != NULL)
                allocator.Free(retval);

            BAIL_MACRO(ERR_OUT_OF_MEMORY, NULL);
        } /* if */

        retval = ptr;
        ptr = getcwd(retval, allocSize);
    } while (ptr == NULL && errno == ERANGE);

    if (ptr == NULL && errno)
    {
        /*
         * getcwd() failed for some reason, for example current directory 
         * not existing.
         */
        if (retval != NULL)
            allocator.Free(retval);

        BAIL_MACRO(ERR_NO_SUCH_FILE, NULL);
    } /* if */

    return (retval);
} /* __PHYSFS_platformCurrentDir */

char *__PHYSFS_platformRealPath(const char *path)
{
    // TODO: proper rewrite
    char resolved_path[MAXPATHLEN];
    char *retval = NULL;
    errno = 0;

    BAIL_IF_MACRO(!realpath(path, resolved_path), strerror(errno), NULL);

    retval = (char *) allocator.Malloc(strlen(resolved_path) + 1);

    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    strcpy(retval, resolved_path);

    return (retval);
} /* __PHYSFS_platformRealPath */

int __PHYSFS_platformMkDir(const char *path)
{
    int rc;
    errno = 0;

    rc = mkdir(path, S_IRWXU);
    BAIL_IF_MACRO(rc == -1, strerror(errno), 0);

    return (1);
} /* __PHYSFS_platformMkDir */

int __PHYSFS_platformDelete(const char *path)
{
    BAIL_IF_MACRO(remove(path) == -1, strerror(errno), 0);

    return (1);
} /* __PHYSFS_platformDelete */

#if (defined PHYSFS_NO_THREAD_SUPPORT)

void *__PHYSFS_platformCreateMutex(void) { return ((void *) 0x0001); }
void __PHYSFS_platformDestroyMutex(void *mutex) {}
int __PHYSFS_platformGrabMutex(void *mutex) { return (1); }
void __PHYSFS_platformReleaseMutex(void *mutex) {}

#else

#error TODO: Wii Threads

#endif /* PHYSFS_NO_THREAD_SUPPORT */

int __PHYSFS_platformSetDefaultAllocator(PHYSFS_Allocator *a)
{
    // TODO: maybe use mem2_alloc or something
    return (0);  /* just use malloc() and friends for now. */
} /* __PHYSFS_platformSetDefaultAllocator */

#endif  /* PHYSFS_PLATFORM_WII */

/* end of wii.c ... */
