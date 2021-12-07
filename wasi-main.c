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

typedef u32 wasm_ptr;

IMPORT_IMPL_WASI_ALL(u32, Z_fd_prestat_getZ_iii, (u32 fd, wasm_ptr buf), {
    uvwasi_prestat_t prestat;
    uvwasi_errno_t ret = uvwasi_fd_prestat_get(&uvwasi, fd, &prestat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_WRITE32(buf+0, prestat.pr_type);
        MEM_WRITE32(buf+4, prestat.u.dir.pr_name_len);
    }
    return ret;
});


IMPORT_IMPL_WASI_ALL(u32, Z_fd_prestat_dir_nameZ_iiii, (u32 fd, wasm_ptr path, u32 path_len), {
    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(&uvwasi, fd, (char*)MEMACCESS(path), path_len);
    
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_environ_sizes_getZ_iii, (wasm_ptr env_count, wasm_ptr env_buf_size), {
    MEM_WRITE32(env_count,      0);
    MEM_WRITE32(env_buf_size,   0);
    return UVWASI_ESUCCESS;
});

IMPORT_IMPL_WASI_ALL(u32, Z_environ_getZ_iii, (wasm_ptr env, wasm_ptr env_buf), {
    return UVWASI_ESUCCESS;
});

IMPORT_IMPL_WASI_ALL(u32, Z_args_sizes_getZ_iii, (wasm_ptr argc, wasm_ptr argv_buf_size), {
    MEM_WRITE32(argc,            0);
    MEM_WRITE32(argv_buf_size,   0);
    return UVWASI_ESUCCESS;
});

IMPORT_IMPL_WASI_ALL(u32, Z_args_getZ_iii, (wasm_ptr argv, wasm_ptr argv_buf), {
    return UVWASI_ESUCCESS;
});

IMPORT_IMPL_WASI_ALL(void, Z_proc_exitZ_vi, (u32 code), {
    exit(code);
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_fdstat_getZ_iii, (u32 fd, wasm_ptr buf), {
    uvwasi_fdstat_t stat;
    uvwasi_errno_t ret = uvwasi_fd_fdstat_get(&uvwasi, fd, &stat);
    if (ret == UVWASI_ESUCCESS) {
        MEM_SET(buf, 0, 24);
        MEM_WRITE8 (buf+0, stat.fs_filetype);
        MEM_WRITE16(buf+2, stat.fs_flags);
        MEM_WRITE64(buf+8, stat.fs_rights_base);
        MEM_WRITE64(buf+16, stat.fs_rights_inheriting);
    }
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_fd_closeZ_ii, (u32 fd), {
    uvwasi_errno_t ret = uvwasi_fd_close(&uvwasi, fd);
    return ret;
});

IMPORT_IMPL_WASI_ALL(u32, Z_clock_time_getZ_iiji, (u32 clk_id, u64 precision, wasm_ptr result), {
    uvwasi_timestamp_t t;
    uvwasi_errno_t ret = uvwasi_clock_time_get(&uvwasi, clk_id, precision, &t);
    MEM_WRITE64(result, t);
    return ret;
});

typedef struct wasi_iovec_t
{
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

IMPORT_IMPL_WASI_ALL(u32, Z_fd_writeZ_iiiii, (u32 fd, wasm_ptr iovs_offset, u32 iovs_len, wasm_ptr nwritten), {
    
    wasi_iovec_t * wasi_iovs = (wasi_iovec_t *)MEMACCESS(iovs_offset);

#if defined(M3_COMPILER_MSVC)
    if (iovs_len > 32) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[32];
#else
    if (iovs_len > 128) return UVWASI_EINVAL;
    uvwasi_ciovec_t  iovs[iovs_len];
#endif
    for (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf = MEMACCESS(*(u32*)(&wasi_iovs[i].buf));
        iovs[i].buf_len = *(u32*)(&wasi_iovs[i].buf_len);
    }
    
    uvwasi_size_t num_written;
    uvwasi_errno_t ret = uvwasi_fd_write(&uvwasi, fd, iovs, iovs_len, &num_written);
    MEM_WRITE32(nwritten, num_written);
    return ret;
});

IMPORT_IMPL_WASI_UNSTABLE(u32, Z_fd_seekZ_iijii, (u32 fd, u64 offset, u32 wasi_whence, wasm_ptr result), {
    
    uvwasi_whence_t whence = -1;
    switch (wasi_whence) {
    case 0: whence = UVWASI_WHENCE_CUR; break;
    case 1: whence = UVWASI_WHENCE_END; break;
    case 2: whence = UVWASI_WHENCE_SET; break;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t ret = uvwasi_fd_seek(&uvwasi, fd, offset, whence, &pos);
    MEM_WRITE64(result, pos);
    return ret;
});

IMPORT_IMPL_WASI_PREVIEW1(u32, Z_fd_seekZ_iijii, (u32 fd, u64 offset, u32 wasi_whence, wasm_ptr result), {
    
    uvwasi_whence_t whence = -1;
    switch (wasi_whence) {
    case 0: whence = UVWASI_WHENCE_SET; break;
    case 1: whence = UVWASI_WHENCE_CUR; break;
    case 2: whence = UVWASI_WHENCE_END; break;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t ret = uvwasi_fd_seek(&uvwasi, fd, offset, whence, &pos);
    MEM_WRITE64(result, pos);
    return ret;
});


int main(int argc, char** argv)
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

    init_options.argc = 0;
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

    return 0;
}
