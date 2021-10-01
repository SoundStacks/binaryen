#define SYS_exit		  1
#define SYS_read		  3
#define SYS_write		  4
#define SYS_open		  5
#define SYS_link		  9
#define SYS_unlink		 10
#define SYS_chdir		 12
#define SYS_mknod		 14
#define SYS_chmod		 15
#define SYS_getpid		 20
#define SYS_pause		 29
#define SYS_access		 33
#define SYS_nice		 34
#define SYS_sync		 36
#define SYS_rename		 38
#define SYS_mkdir		 39
#define SYS_rmdir		 40
#define SYS_dup		 41
#define SYS_pipe		 42
#define SYS_acct		 51
#define SYS_ioctl		 54
#define SYS_setpgid		 57
#define SYS_umask		 60
#define SYS_dup2		 63
#define SYS_getppid		 64
#define SYS_setsid		 66
#define SYS_setrlimit		 75
#define SYS_getrusage		 77
#define SYS_symlink		 83
#define SYS_readlink		 85
#define SYS_munmap		 91
#define SYS_fchmod		 94
#define SYS_getpriority	 96
#define SYS_setpriority	 97
#define SYS_setitimer		104
#define SYS_getitimer		105
#define SYS_wait4		114
#define SYS_setdomainname	121
#define SYS_uname		122
#define SYS_mprotect		125
#define SYS_getpgid		132
#define SYS_fchdir		133
#define SYS__newselect		142
#define SYS_msync		144
#define SYS_getsid		147
#define SYS_fdatasync		148
#define SYS_mlock		150
#define SYS_munlock		151
#define SYS_mlockall		152
#define SYS_munlockall		153
#define SYS_mremap		163
#define SYS_poll		168
#define SYS_rt_sigqueueinfo	178
#define SYS_pread64		180
#define SYS_pwrite64		181
#define SYS_getcwd		183
#define SYS_ugetrlimit		191
#define SYS_mmap2		192
#define SYS_truncate64		193
#define SYS_ftruncate64	194
#define SYS_stat64		195
#define SYS_lstat64		196
#define SYS_fstat64		197
#define SYS_lchown32		198
#define SYS_getuid32		199
#define SYS_getgid32		200
#define SYS_geteuid32		201
#define SYS_getegid32		202
#define SYS_getgroups32	205
#define SYS_fchown32		207
#define SYS_getresuid32	209
#define SYS_getresgid32	211
#define SYS_chown32		212
#define SYS_mincore		218
#define SYS_madvise		219
#define SYS_madvise1		219
#define SYS_getdents64		220
#define SYS_fcntl64		221
#define SYS_exit_group		252
#define SYS_statfs64		268
#define SYS_fstatfs64		269
#define SYS_fadvise64_64	272
#define SYS_openat		295
#define SYS_mkdirat		296
#define SYS_mknodat		297
#define SYS_fchownat		298
#define SYS_fstatat64		300
#define SYS_unlinkat		301
#define SYS_renameat		302
#define SYS_linkat		303
#define SYS_symlinkat		304
#define SYS_readlinkat		305
#define SYS_fchmodat		306
#define SYS_faccessat		307
#define SYS_pselect6		308
#define SYS_utimensat		320
#define SYS_fallocate		324
#define SYS_dup3		330
#define SYS_pipe2		331
#define SYS_preadv		333
#define SYS_pwritev		334
#define SYS_recvmmsg		337
#define SYS_prlimit64		340
#define SYS_sendmmsg		345
#define SYS_socket		359
#define SYS_socketpair		360
#define SYS_bind		361
#define SYS_connect		362
#define SYS_listen		363
#define SYS_accept4		364
#define SYS_getsockopt		365
#define SYS_setsockopt		366
#define SYS_getsockname		367
#define SYS_getpeername		368
#define SYS_sendto		369
#define SYS_sendmsg		370
#define SYS_recvfrom		371
#define SYS_recvmsg		372
#define SYS_shutdown		373
