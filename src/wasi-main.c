#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "wasi-app.h"
#include "wasm-rt.h"
#include "uvwasi.h"

static uvwasi_t uvwasi;

#define IMPORT_IMPL(ret, name, params, body)            \
  static ret _##name params body                        \
  ret (*WASM_RT_ADD_PREFIX(name)) params = _##name;

#define MEMACCESS(addr) ((void*)&WASM_RT_ADD_PREFIX(Z_memory)->data[(addr)])

#define MEM_SET(addr, value, len) memset(MEMACCESS(addr), value, len)
#define MEM_WRITE8(addr, value)  (*(u8*) MEMACCESS(addr)) = value
#define MEM_WRITE16(addr, value) (*(u16*)MEMACCESS(addr)) = value
#define MEM_WRITE32(addr, value) (*(u32*)MEMACCESS(addr)) = value
#define MEM_WRITE64(addr, value) (*(u64*)MEMACCESS(addr)) = value
  
#define MEM_READ32(addr) (*(u32*)MEMACCESS(addr))
  
#define IMPORT_IMPL_WASI_UNSTABLE(ret, name, params, body)  IMPORT_IMPL(ret, Z_wasi_unstable##name, params, body)
#define IMPORT_IMPL_WASI_PREVIEW1(ret, name, params, body)  IMPORT_IMPL(ret, Z_wasi_snapshot_preview1##name, params, body)
#define IMPORT_IMPL_WASI_ALL(ret, name, params, body) \
  IMPORT_IMPL_WASI_UNSTABLE(ret, name, params, body)  \
  IMPORT_IMPL_WASI_PREVIEW1(ret, name, params, body)

#define READ32(x)   (*(u32*)(x))

// TODO: Add linear memory boundary checks

typedef u32 wasm_ptr;

IMPORT_IMPL_WASI_ALL(u32, Z_fd_prestat_getZ_iii, (u32 fd, wasm_ptr buf),
{
    uvwasi_prestat_t prestat;
    uvwasi_errno_t ret = uvwasi_fd_prestat_get(&uvwasi, fd, &prestat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_WRITE32(buf+0, prestat.pr_type);
        MEM_WRITE32(buf+4, prestat.u.dir.pr_name_len);
    }
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_prestat_dir_nameZ_iiii, (u32 fd, wasm_ptr path, u32 path_len),
{
    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(&uvwasi, fd, (char*)MEMACCESS(path), path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_environ_sizes_getZ_iii, (wasm_ptr env_count, wasm_ptr env_buf_size),
{
    uvwasi_size_t uvcount;
    uvwasi_size_t uvbufsize;
    uvwasi_errno_t ret = uvwasi_environ_sizes_get(&uvwasi, &uvcount, &uvbufsize);
    if (ret == UVWASI_ESUCCESS) {
        MEM_WRITE32(env_count,      uvcount);
        MEM_WRITE32(env_buf_size,   uvbufsize);
    }
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_environ_getZ_iii, (wasm_ptr env, wasm_ptr buf),
{
    uvwasi_size_t uvcount;
    uvwasi_size_t uvbufsize;
    uvwasi_errno_t ret;
    ret = uvwasi_environ_sizes_get(&uvwasi, &uvcount, &uvbufsize);
    if (ret != UVWASI_ESUCCESS) {
        return ret;
    }

    // TODO: check mem

    char** uvenv = calloc(uvcount, sizeof(char*));
    if (uvenv == NULL) {
        return UVWASI_ENOMEM;
    }

    ret = uvwasi_environ_get(&uvwasi, uvenv, (char*)MEMACCESS(buf));
    if (ret != UVWASI_ESUCCESS) {
        free(uvenv);
        return ret;
    }

    for (u32 i = 0; i < uvcount; ++i)
    {
        uint32_t offset = buf + (uvenv[i] - uvenv[0]);
        MEM_WRITE32(env+(i*sizeof(wasm_ptr)), offset);
    }

    free(uvenv);

    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_args_sizes_getZ_iii, (wasm_ptr argc, wasm_ptr argv_buf_size),
{
    uvwasi_size_t uvcount;
    uvwasi_size_t uvbufsize;
    uvwasi_errno_t ret = uvwasi_args_sizes_get(&uvwasi, &uvcount, &uvbufsize);
    if (ret == UVWASI_ESUCCESS) {
        MEM_WRITE32(argc,            uvcount);
        MEM_WRITE32(argv_buf_size,   uvbufsize);
    }
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_args_getZ_iii, (wasm_ptr argv, wasm_ptr buf),
{
    uvwasi_size_t uvcount;
    uvwasi_size_t uvbufsize;
    uvwasi_errno_t ret;
    ret = uvwasi_args_sizes_get(&uvwasi, &uvcount, &uvbufsize);
    if (ret != UVWASI_ESUCCESS) {
        return ret;
    }

    // TODO: check mem

    char** uvarg = calloc(uvcount, sizeof(char*));
    if (uvarg == NULL) {
        return UVWASI_ENOMEM;
    }

    ret = uvwasi_args_get(&uvwasi, uvarg, (char*)MEMACCESS(buf));
    if (ret != UVWASI_ESUCCESS) {
        free(uvarg);
        return ret;
    }

    for (u32 i = 0; i < uvcount; ++i)
    {
        uint32_t offset = buf + (uvarg[i] - uvarg[0]);
        MEM_WRITE32(argv+(i*sizeof(wasm_ptr)), offset);
    }

    free(uvarg);

    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_fdstat_getZ_iii, (u32 fd, wasm_ptr stat),
{
    uvwasi_fdstat_t uvstat;
    uvwasi_errno_t ret = uvwasi_fd_fdstat_get(&uvwasi, fd, &uvstat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(stat, 0, 24);
        MEM_WRITE8 (stat+0,  uvstat.fs_filetype);
        MEM_WRITE16(stat+2,  uvstat.fs_flags);
        MEM_WRITE64(stat+8,  uvstat.fs_rights_base);
        MEM_WRITE64(stat+16, uvstat.fs_rights_inheriting);
    }
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_fdstat_set_flagsZ_iii, (u32 fd, u32 flags),
{
    uvwasi_errno_t ret = uvwasi_fd_fdstat_set_flags(&uvwasi, fd, flags);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_fdstat_set_rightsZ_iijj, (u32 fd, u64 fs_rights_base, u64 fs_rights_inheriting),
{
    uvwasi_errno_t ret = uvwasi_fd_fdstat_set_rights(&uvwasi, fd, fs_rights_base, fs_rights_inheriting);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_filestat_set_timesZ_iijj, (u32 fd, u32 flags, wasm_ptr path, u32 path_len, u64 atim, u64 mtim, u32 fst_flags),
{
    uvwasi_errno_t ret = uvwasi_path_filestat_set_times(&uvwasi, fd, flags, (char*)MEMACCESS(path), path_len, atim, mtim, fst_flags);
    return ret;
});



IMPORT_IMPL_WASI_UNSTABLE(u32, Z_path_filestat_getZ_iiiiii, (u32 fd, u32 flags, wasm_ptr path, u32 path_len, wasm_ptr stat),
{
    uvwasi_filestat_t uvstat;
    uvwasi_errno_t ret = uvwasi_path_filestat_get(&uvwasi, fd, flags, (char*)MEMACCESS(path), path_len, &uvstat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(stat, 0, 56);
        MEM_WRITE64(stat+0,  uvstat.st_dev);
        MEM_WRITE64(stat+8,  uvstat.st_ino);
        MEM_WRITE8 (stat+16, uvstat.st_filetype);
        MEM_WRITE32(stat+20, uvstat.st_nlink);
        MEM_WRITE64(stat+24, uvstat.st_size);
        MEM_WRITE64(stat+32, uvstat.st_atim);
        MEM_WRITE64(stat+40, uvstat.st_mtim);
        MEM_WRITE64(stat+48, uvstat.st_ctim);
    }
    return ret;
});

IMPORT_IMPL_WASI_PREVIEW1(u32, Z_path_filestat_getZ_iiiiii, (u32 fd, u32 flags, wasm_ptr path, u32 path_len, wasm_ptr stat),
{
    uvwasi_filestat_t uvstat;
    uvwasi_errno_t ret = uvwasi_path_filestat_get(&uvwasi, fd, flags, (char*)MEMACCESS(path), path_len, &uvstat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(stat, 0, 64);
        MEM_WRITE64(stat+0,  uvstat.st_dev);
        MEM_WRITE64(stat+8,  uvstat.st_ino);
        MEM_WRITE8 (stat+16, uvstat.st_filetype);
        MEM_WRITE64(stat+24, uvstat.st_nlink);
        MEM_WRITE64(stat+32, uvstat.st_size);
        MEM_WRITE64(stat+40, uvstat.st_atim);
        MEM_WRITE64(stat+48, uvstat.st_mtim);
        MEM_WRITE64(stat+56, uvstat.st_ctim);
    }
    return ret;
});

IMPORT_IMPL_WASI_UNSTABLE(u32, Z_fd_filestat_getZ_iii, (u32 fd, wasm_ptr stat),
{
    uvwasi_filestat_t uvstat;
    uvwasi_errno_t ret = uvwasi_fd_filestat_get(&uvwasi, fd, &uvstat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(stat, 0, 56);
        MEM_WRITE64(stat+0,  uvstat.st_dev);
        MEM_WRITE64(stat+8,  uvstat.st_ino);
        MEM_WRITE8 (stat+16, uvstat.st_filetype);
        MEM_WRITE32(stat+20, uvstat.st_nlink);
        MEM_WRITE64(stat+24, uvstat.st_size);
        MEM_WRITE64(stat+32, uvstat.st_atim);
        MEM_WRITE64(stat+40, uvstat.st_mtim);
        MEM_WRITE64(stat+48, uvstat.st_ctim);
    }
    return ret;
});

IMPORT_IMPL_WASI_PREVIEW1(u32, Z_fd_filestat_getZ_iii, (u32 fd, wasm_ptr stat),
{
    uvwasi_filestat_t uvstat;
    uvwasi_errno_t ret = uvwasi_fd_filestat_get(&uvwasi, fd, &uvstat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(stat, 0, 64);
        MEM_WRITE64(stat+0,  uvstat.st_dev);
        MEM_WRITE64(stat+8,  uvstat.st_ino);
        MEM_WRITE8 (stat+16, uvstat.st_filetype);
        MEM_WRITE64(stat+24, uvstat.st_nlink);
        MEM_WRITE64(stat+32, uvstat.st_size);
        MEM_WRITE64(stat+40, uvstat.st_atim);
        MEM_WRITE64(stat+48, uvstat.st_mtim);
        MEM_WRITE64(stat+56, uvstat.st_ctim);
    }
    return ret;
});


IMPORT_IMPL_WASI_UNSTABLE(u32, Z_fd_seekZ_iijii, (u32 fd, u64 offset, u32 wasi_whence, wasm_ptr pos),
{
    uvwasi_whence_t whence = -1;
    switch (wasi_whence) {
    case 0: whence = UVWASI_WHENCE_CUR; break;
    case 1: whence = UVWASI_WHENCE_END; break;
    case 2: whence = UVWASI_WHENCE_SET; break;
    }

    uvwasi_filesize_t uvpos;
    uvwasi_errno_t ret = uvwasi_fd_seek(&uvwasi, fd, offset, whence, &uvpos);
    MEM_WRITE64(pos, uvpos);
    return ret;
});

IMPORT_IMPL_WASI_PREVIEW1(u32, Z_fd_seekZ_iijii, (u32 fd, u64 offset, u32 wasi_whence, wasm_ptr pos),
{
    uvwasi_whence_t whence = -1;
    switch (wasi_whence) {
    case 0: whence = UVWASI_WHENCE_SET; break;
    case 1: whence = UVWASI_WHENCE_CUR; break;
    case 2: whence = UVWASI_WHENCE_END; break;
    }

    uvwasi_filesize_t uvpos;
    uvwasi_errno_t ret = uvwasi_fd_seek(&uvwasi, fd, offset, whence, &uvpos);
    MEM_WRITE64(pos, uvpos);
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_tellZ_iii, (u32 fd, wasm_ptr pos),
{
    uvwasi_filesize_t uvpos;
    uvwasi_errno_t ret = uvwasi_fd_tell(&uvwasi, fd, &uvpos);
    MEM_WRITE64(pos, uvpos);
    return ret;
});




IMPORT_IMPL_WASI_ALL(u32, Z_fd_filestat_set_sizeZ_iij, (u32 fd, u64 filesize),
{
    uvwasi_errno_t ret = uvwasi_fd_filestat_set_size(&uvwasi, fd, filesize);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_filestat_set_timesZ_iijj, (u32 fd, u64 atim, u64 mtim, u32 fst_flags),
{
    uvwasi_errno_t ret = uvwasi_fd_filestat_set_times(&uvwasi, fd, atim, mtim, fst_flags);
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_syncZ_ii, (u32 fd),
{
    uvwasi_errno_t ret = uvwasi_fd_sync(&uvwasi, fd);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_datasyncZ_ii, (u32 fd),
{
    uvwasi_errno_t ret = uvwasi_fd_datasync(&uvwasi, fd);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_renumberZ_ii, (u32 fd_from, u32 fd_to),
{
    uvwasi_errno_t ret = uvwasi_fd_renumber(&uvwasi, fd_from, fd_to);
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_allocateZ_iijj, (u32 fd, u64 offset, u64 len),
{
    uvwasi_errno_t ret = uvwasi_fd_allocate(&uvwasi, fd, offset, len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_adviseZ_iijji, (u32 fd, u64 offset, u64 len, u32 advice),
{
    uvwasi_errno_t ret = uvwasi_fd_advise(&uvwasi, fd, offset, len, advice);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_openZ_iiiiiijjii, (u32 dirfd, u32 dirflags,
                                                    wasm_ptr path, u32 path_len,
                                                    u32 oflags, u64 fs_rights_base, u64 fs_rights_inheriting,
                                                    u32 fs_flags, wasm_ptr fd),
{
    uvwasi_fd_t uvfd;
    uvwasi_errno_t ret = uvwasi_path_open(&uvwasi,
                                 dirfd,
                                 dirflags,
                                 (char*)MEMACCESS(path),
                                 path_len,
                                 oflags,
                                 fs_rights_base,
                                 fs_rights_inheriting,
                                 fs_flags,
                                 &uvfd);
    MEM_WRITE32(fd, uvfd);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_closeZ_ii, (u32 fd), {
    uvwasi_errno_t ret = uvwasi_fd_close(&uvwasi, fd);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_symlinkZ_iiiiii, (wasm_ptr old_path, u32 old_path_len, u32 fd,
                                                   wasm_ptr new_path, u32 new_path_len),
{
    uvwasi_errno_t ret = uvwasi_path_symlink(&uvwasi, (char*)MEMACCESS(old_path), old_path_len,
                                                  fd, (char*)MEMACCESS(new_path), new_path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_renameZ_iiiiiii, (u32 old_fd, wasm_ptr old_path, u32 old_path_len,
                                                   u32 new_fd, wasm_ptr new_path, u32 new_path_len),
{
    uvwasi_errno_t ret = uvwasi_path_rename(&uvwasi, old_fd, (char*)MEMACCESS(old_path), old_path_len,
                                                     new_fd, (char*)MEMACCESS(new_path), new_path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_linkZ_iiiiiiii, (u32 old_fd, u32 old_flags, wasm_ptr old_path, u32 old_path_len,
                                                  u32 new_fd,                wasm_ptr new_path, u32 new_path_len),
{
    uvwasi_errno_t ret = uvwasi_path_link(&uvwasi, old_fd, old_flags, (char*)MEMACCESS(old_path), old_path_len,
                                                   new_fd,            (char*)MEMACCESS(new_path), new_path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_unlink_fileZ_iiii, (u32 fd, wasm_ptr path, u32 path_len),
{
    uvwasi_errno_t ret = uvwasi_path_unlink_file(&uvwasi, fd, (char*)MEMACCESS(path), path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_readlinkZ_iiiiiii, (u32 fd, wasm_ptr path, u32 path_len,
                                                     wasm_ptr buf, u32 buf_len, wasm_ptr bufused),
{
    uvwasi_size_t uvbufused;
    uvwasi_errno_t ret = uvwasi_path_readlink(&uvwasi, fd, (char*)MEMACCESS(path), path_len, MEMACCESS(buf), buf_len, &uvbufused);

    MEM_WRITE32(bufused, uvbufused);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_create_directoryZ_iiii, (u32 fd, wasm_ptr path, u32 path_len),
{
    uvwasi_errno_t ret = uvwasi_path_create_directory(&uvwasi, fd, (char*)MEMACCESS(path), path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_path_remove_directoryZ_iiii, (u32 fd, wasm_ptr path, u32 path_len),
{
    uvwasi_errno_t ret = uvwasi_path_remove_directory(&uvwasi, fd, (char*)MEMACCESS(path), path_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_readdirZ_iiiiji, (u32 fd, wasm_ptr buf, u32 buf_len, u64 cookie, wasm_ptr bufused),
{
    uvwasi_size_t uvbufused;
    uvwasi_errno_t ret = uvwasi_fd_readdir(&uvwasi, fd, MEMACCESS(buf), buf_len, cookie, &uvbufused);
    MEM_WRITE32(bufused, uvbufused);
    return ret;
});

typedef struct wasi_iovec_t
{
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

IMPORT_IMPL_WASI_ALL(u32, Z_fd_writeZ_iiiii, (u32 fd, wasm_ptr iovs_offset, u32 iovs_len, wasm_ptr nwritten),
{
    wasi_iovec_t * wasi_iovs = (wasi_iovec_t *)MEMACCESS(iovs_offset);

#if defined(_MSC_VER)
    if (iovs_len > 32) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[32];
#else
    if (iovs_len > 128) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[iovs_len];
#endif
    for (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf = MEMACCESS(READ32(&wasi_iovs[i].buf));
        iovs[i].buf_len = READ32(&wasi_iovs[i].buf_len);
    }
    
    uvwasi_size_t num_written;
    uvwasi_errno_t ret = uvwasi_fd_write(&uvwasi, fd, iovs, iovs_len, &num_written);
    MEM_WRITE32(nwritten, num_written);
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_pwriteZ_iiiii, (u32 fd, wasm_ptr iovs_offset, u32 iovs_len, u64 offset, wasm_ptr nwritten),
{
    wasi_iovec_t * wasi_iovs = (wasi_iovec_t *)MEMACCESS(iovs_offset);

#if defined(_MSC_VER)
    if (iovs_len > 32) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[32];
#else
    if (iovs_len > 128) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[iovs_len];
#endif
    for (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf = MEMACCESS(READ32(&wasi_iovs[i].buf));
        iovs[i].buf_len = READ32(&wasi_iovs[i].buf_len);
    }

    uvwasi_size_t num_written;
    uvwasi_errno_t ret = uvwasi_fd_pwrite(&uvwasi, fd, iovs, iovs_len, offset, &num_written);
    MEM_WRITE32(nwritten, num_written);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_readZ_iiiii, (u32 fd, wasm_ptr iovs_offset, u32 iovs_len, wasm_ptr nread),
{
    wasi_iovec_t * wasi_iovs = (wasi_iovec_t *)MEMACCESS(iovs_offset);

#if defined(_MSC_VER)
    if (iovs_len > 32) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[32];
#else
    if (iovs_len > 128) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[iovs_len];
#endif

    for (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf = MEMACCESS(READ32(&wasi_iovs[i].buf));
        iovs[i].buf_len = READ32(&wasi_iovs[i].buf_len);
    }

    uvwasi_size_t num_read;
    uvwasi_errno_t ret = uvwasi_fd_read(&uvwasi, fd, (const uvwasi_iovec_t *)iovs, iovs_len, &num_read);
    MEM_WRITE32(nread, num_read);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_preadZ_iiiiji, (u32 fd, wasm_ptr iovs_offset, u32 iovs_len, u64 offset, wasm_ptr nread),
{
    wasi_iovec_t * wasi_iovs = (wasi_iovec_t *)MEMACCESS(iovs_offset);

#if defined(_MSC_VER)
    if (iovs_len > 32) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[32];
#else
    if (iovs_len > 128) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[iovs_len];
#endif

    for (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf = MEMACCESS(READ32(&wasi_iovs[i].buf));
        iovs[i].buf_len = READ32(&wasi_iovs[i].buf_len);
    }

    uvwasi_size_t num_read;
    uvwasi_errno_t ret = uvwasi_fd_pread(&uvwasi, fd, (const uvwasi_iovec_t *)iovs, iovs_len, offset, &num_read);
    MEM_WRITE32(nread, num_read);
    return ret;
});

// TODO: unstable/snapshot_preview1 compatibility
IMPORT_IMPL_WASI_ALL(u32, Z_poll_oneoffZ_iiiii, (wasm_ptr in, wasm_ptr out, u32 nsubscriptions, wasm_ptr nevents),
{
    uvwasi_size_t uvnevents;
    uvwasi_errno_t ret = uvwasi_poll_oneoff(&uvwasi, MEMACCESS(in), MEMACCESS(out), nsubscriptions, &uvnevents);
    MEM_WRITE32(nevents, uvnevents);
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_clock_res_getZ_iii, (u32 clk_id, wasm_ptr result),
{
    uvwasi_timestamp_t t;
    uvwasi_errno_t ret = uvwasi_clock_res_get(&uvwasi, clk_id, &t);
    MEM_WRITE64(result, t);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_clock_time_getZ_iiji, (u32 clk_id, u64 precision, wasm_ptr result),
{
    uvwasi_timestamp_t t;
    uvwasi_errno_t ret = uvwasi_clock_time_get(&uvwasi, clk_id, precision, &t);
    MEM_WRITE64(result, t);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_random_getZ_iii, (wasm_ptr buf, u32 buf_len),
{
    uvwasi_errno_t ret = uvwasi_random_get(&uvwasi, MEMACCESS(buf), buf_len);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_sched_yieldZ_iv, (void),
{
    uvwasi_errno_t ret = uvwasi_sched_yield(&uvwasi);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_proc_raiseZ_iv, (u32 sig),
{
    uvwasi_errno_t ret = uvwasi_proc_raise(&uvwasi, sig);
    return ret;
});

IMPORT_IMPL_WASI_ALL(void, Z_proc_exitZ_vi, (u32 code),
{
    uvwasi_destroy(&uvwasi);
    exit(code);
});


int main(int argc, const char** argv)
{
    #define ENV_COUNT       7
    #define PREOPENS_COUNT  2

    char* env[ENV_COUNT];
    env[0] = "TERM=xterm-256color";
    env[1] = "COLORTERM=truecolor";
    env[2] = "LANG=en_US.UTF-8";
    env[3] = "PWD=/";
    env[4] = "HOME=/";
    env[5] = "PATH=/";
    env[6] = NULL;

    uvwasi_preopen_t preopens[PREOPENS_COUNT];
    preopens[0].mapped_path = "/";
    preopens[0].real_path = ".";
    preopens[1].mapped_path = "./";
    preopens[1].real_path = ".";

    uvwasi_options_t init_options;
    uvwasi_options_init(&init_options);

    init_options.argc = argc;
    init_options.argv = argv;
    init_options.envp = (const char **) env;
    init_options.preopenc = PREOPENS_COUNT;
    init_options.preopens = preopens;

    uvwasi_errno_t ret = uvwasi_init(&uvwasi, &init_options);

    if (ret != UVWASI_ESUCCESS) {
        printf("uvwasi_init failed");
        exit(1);
    }

    init();

    Z__startZ_vv();

    uvwasi_destroy(&uvwasi);

    return 0;
}
