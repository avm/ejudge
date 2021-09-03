/* -*- mode: c; c-basic-offset: 4 -*- */

/* Copyright (C) 2021 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/resource.h>
#include <grp.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "config.h"

#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP    0x02000000
#endif

#ifndef CLONE_PIDFD
#define CLONE_PIDFD   0x00001000
#endif

#ifndef P_PIDFD
#define P_PIDFD 3
#endif

#if defined EJUDGE_PRIMARY_USER
#define PRIMARY_USER EJUDGE_PRIMARY_USER
#else
#define PRIMARY_USER "ejudge"
#endif

#if defined EJUDGE_PRIMARY_GROUP
#define PRIMARY_GROUP EJUDGE_PRIMARY_GROUP
#else
#define PRIMARY_GROUP PRIMARY_USER
#endif

#if defined EJUDGE_EXEC_USER
#define EXEC_USER EJUDGE_EXEC_USER
#define EXEC_GROUP EJUDGE_EXEC_USER
#else
#define EXEC_USER "ejexec"
#define EXEC_GROUP "ejexec"
#endif

#ifndef EJUDGE_PREFIX_DIR
#define EJUDGE_PREFIX_DIR "/opt/ejudge"
#endif

static char safe_dir_path[PATH_MAX];

static const char *program_name = "prog";
static int response_fd = 2;
static char *log_s = NULL;
static size_t log_z = 0;
static FILE *log_f = NULL;

static int enable_cgroup = 1;
static int enable_ipc_ns = 1;
static int enable_net_ns = 1;
static int enable_mount_ns = 1;
static int enable_pid_ns = 1;
static int enable_proc = 1;
static int enable_sys = 0;
static int enable_sandbox_dir = 1;
static int enable_home = 0;
static int enable_chown = 1;
static int enable_pgroup = 1;
static int enable_prc_count = 0;
static int enable_ipc_count = 0;

static char *working_dir = NULL;

static int bash_mode = 0;

static int exec_uid = -1;
static int exec_gid = -1;
static int primary_uid = -1;
static int primary_gid = -1;

// standard stream redirections
static int enable_redirect_null = 0;
static int enable_output_merge = 0;
static int stdout_mode = O_WRONLY | O_CREAT | O_TRUNC;
static int stderr_mode = O_WRONLY | O_CREAT | O_TRUNC;
static char *stdin_name = NULL;
static char *stdout_name = NULL;
static char *stderr_name = NULL;
static int stdin_fd = -1;
static int stdout_fd = -1;
static int stderr_fd = -1;
static char *start_program_name = NULL;
static int stdin_external_fd = -1;
static int stdout_external_fd = -1;

// resource limits
static int limit_umask = -1;
static int limit_open_files = -1;
static long long limit_stack_size = -1;
static long long limit_vm_size = 67108864;  // 64M
static long long limit_file_size = -1;
static int limit_processes = 5;
static int limit_cpu_time_ms = 1000;
static int limit_real_time_ms = 5000;

static char *start_program;
static char **start_args;
extern char **environ;

static void __attribute__((noreturn))
fatal()
{
    if (log_f) {
        fclose(log_f); log_f = NULL;
    }
    (void) log_z;
    if (log_s && *log_s) {
        int len = strlen(log_s);
        dprintf(response_fd, "1L%d,%s", len, log_s);
    } else {
        dprintf(response_fd, "1");
    }
    _exit(1);
}

static void __attribute__((format(printf, 1, 2)))
flog(const char *format, ...)
{
    char buf[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    fprintf(log_f, "%s: %s\n", program_name, buf);
}

static void __attribute__((format(printf, 1, 2), noreturn))
ffatal(const char *format, ...)
{
    char buf[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    fprintf(log_f, "%s: %s\n", program_name, buf);

    if (log_f) {
        fclose(log_f); log_f = NULL;
    }
    (void) log_z;
    if (log_s && *log_s) {
        int len = strlen(log_s);
        dprintf(response_fd, "1L%d,%s", len, log_s);
    } else {
        dprintf(response_fd, "1");
    }
    _exit(1);
}

static int
getl(char *buf, size_t size, FILE *f)
{
    if (!fgets(buf, size, f)) return -1;
    size_t len = strlen(buf);
    if (len + 1 == size) {
        ffatal("input line is too long, increase buffer size!");
    }
    while (len > 0 && isspace((unsigned char) buf[len - 1])) --len;
    buf[len] = 0;
    return len;
}

static void
get_user_ids(void)
{
    // don't use getpwnam because it depends on PAM, etc

    FILE *f = fopen("/etc/passwd", "r");
    if (!f) ffatal("cannot open /etc/passwd: %s", strerror(errno));
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        int len = strlen(buf);
        if (len + 1 >= sizeof(buf)) ffatal("input line in /etc/passwd is too long");

        const char *user_name = buf;
        char *s = strchr(buf, ':');
        if (!s) ffatal("invalid /etc/passwd (1)");
        *s = 0;
        s = strchr(s + 1, ':');
        if (!s) ffatal("invalid /etc/passwd (2)");
        const char *user_id_str = s + 1;
        s = strchr(s + 1, ':');
        if (!s) ffatal("invalid /etc/passwd (3)");
        *s = 0;
        int *dest_uid = NULL;
        if (!strcmp(user_name, EXEC_USER)) dest_uid = &exec_uid;
        else if (!strcmp(user_name, PRIMARY_USER)) dest_uid = &primary_uid;
        if (dest_uid) {
            char *eptr = NULL;
            errno = 0;
            long v = strtol(user_id_str, &eptr, 10);
            if (errno || *eptr || eptr == user_id_str || v <= 0 || (int) v != v)
                ffatal("invalid uid in /etc/passwd for %s", user_name);
            *dest_uid = (int) v;
            //printf("user %s uid %d\n", user_name, *dest_uid);
        }
    }
    fclose(f); f = NULL;

    if (exec_uid < 0) ffatal("no user %s", EXEC_USER);
    if (exec_uid == 0) ffatal("user %s cannot be root", EXEC_USER);
    if (primary_uid < 0) ffatal("no user %s", PRIMARY_USER);
    if (primary_uid == 0) ffatal("user %s cannot be root", PRIMARY_USER);

    if (!(f = fopen("/etc/group", "r"))) ffatal("cannot open /etc/group: %s", strerror(errno));
    while (fgets(buf, sizeof(buf), f)) {
        int len = strlen(buf);
        if (len + 1 >= sizeof(buf)) ffatal("input line in /etc/group is too long");

        const char *group_name = buf;
        char *s = strchr(buf, ':');
        if (!s) ffatal("invalid /etc/group (1)");
        *s = 0;
        s = strchr(s + 1, ':');
        if (!s) ffatal("invalid /etc/group (2)");
        const char *group_id_str = s + 1;
        s = strchr(s + 1, ':');
        if (!s) ffatal("invalid /etc/group (3)");
        *s = 0;
        int *dest_gid = NULL;
        if (!strcmp(group_name, EXEC_GROUP)) dest_gid = &exec_gid;
        else if (!strcmp(group_name, PRIMARY_GROUP)) dest_gid = &primary_gid;
        if (dest_gid) {
            char *eptr = NULL;
            errno = 0;
            long v = strtol(group_id_str, &eptr, 10);
            if (errno || *eptr || eptr == group_id_str || v <= 0 || (int) v != v)
                ffatal("invalid uid in /etc/group for %s", group_name);
            *dest_gid = (int) v;
            //printf("group %s gid %d\n", group_name, *dest_gid);
        }
    }
    fclose(f); f = NULL;

    if (exec_gid < 0) ffatal("no group %s", EXEC_GROUP);
    if (exec_gid == 0) ffatal("group %s cannot be root", EXEC_GROUP);
    if (primary_gid < 0) ffatal("no group %s", PRIMARY_GROUP);
    if (primary_gid == 0) ffatal("group %s cannot be root", PRIMARY_GROUP);
}

static void
safe_chown(const char *full, int to_user_id, int to_group_id, int from_user_id)
{
    int fd = open(full, O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0);
    if (fd < 0) return;
    struct stat stb;
    if (fstat(fd, &stb) < 0) {
        close(fd);
        return;
    }
    if (S_ISDIR(stb.st_mode)) {
        if (stb.st_uid == from_user_id) {
            fchown(fd, to_user_id, to_group_id);
            //fchmod(fd, (stb.st_mode & 0777) | 0700);
        }
    } else {
        if (stb.st_uid == from_user_id) {
            fchown(fd, to_user_id, to_group_id);
        }
    }
    close(fd);
}

static void
safe_chown_rec(const char *path, int user_id, int group_id, int from_user_id)
{
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *dd;
    int names_a = 32, names_u = 0;
    char **names_s = malloc(names_a * sizeof(names_s[0]));
    while ((dd = readdir(d))) {
        if (!strcmp(dd->d_name, ".") || !strcmp(dd->d_name, "..")) continue;
        if (names_u == names_a) {
            names_s = realloc(names_s, (names_a *= 2) * sizeof(names_s[0]));
        }
        names_s[names_u++] = strdup(dd->d_name);
    }
    closedir(d); d = NULL;
    for (int i = 0; i < names_u; ++i) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", path, names_s[i]);
        struct stat stb;
        if (lstat(full, &stb) < 0) continue;
        if (S_ISDIR(stb.st_mode)) {
            safe_chown_rec(full, user_id, group_id, from_user_id);
        }
        safe_chown(full, user_id, group_id, from_user_id);
    }
    for (int i = 0; i < names_u; ++i)
        free(names_s[i]);
    free(names_s);
}

static void
change_ownership(int user_id, int group_id, int from_user_id)
{
    const char *dir = working_dir;
    if (!dir) dir = ".";
    safe_chown(dir, user_id, group_id, from_user_id);
    safe_chown_rec(dir, user_id, group_id, from_user_id);
}

// proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0
struct MountInfo
{
    char *src_path;
    char *dst_path;
    char *type;
    char *options;
    char *n1;
    char *n2;
    int   dst_len;
};

static int
sort_func_1(const void *p1, const void *p2)
{
    return ((struct MountInfo *) p2)->dst_len - ((struct MountInfo *) p1)->dst_len;
}

static void
reconfigure_fs(void)
{
    int r;
    char *mnt_s = NULL;
    size_t mnt_z = 0;
    FILE *mnt_f = open_memstream(&mnt_s, &mnt_z);
    int fd = open("/proc/self/mounts", O_RDONLY);
    if (fd < 0) ffatal("failed to open /proc/self/mounts: %s", strerror(errno));
    char buf[4096];
    ssize_t z;
    while ((z = read(fd, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, z, mnt_f);
    }
    if (z < 0) ffatal("read error from /proc/self/mounts");
    close(fd); fd = -1;
    fclose(mnt_f); mnt_f = NULL;

    if (!mnt_z) ffatal("empty file /proc/self/mounts");
    if (mnt_s[mnt_z - 1] != '\n') ffatal("invalid /proc/self/mounts (1)");

    // count lines
    int lcount = 0;
    for (int i = 0; mnt_s[i]; ++i) {
        lcount += (mnt_s[i] == '\n');
    }

    struct MountInfo *mi = calloc(lcount, sizeof(mi[0]));
    int ind = -1;
    char *p = mnt_s;
    while (*p) {
        ++ind;
        struct MountInfo *pmi = &mi[ind];
        char *q = strchr(p, ' ');
        if (!q) ffatal("invalid /proc/self/mounts (2)");
        *q = 0;
        pmi->src_path = p;
        p = q + 1;

        if (!(q = strchr(p, ' '))) ffatal("invalid /proc/self/mounts (3)");
        *q = 0;
        pmi->dst_path = p;
        pmi->dst_len = strlen(p);
        p = q + 1;

        if (!(q = strchr(p, ' '))) ffatal("invalid /proc/self/mounts (4)");
        *q = 0;
        pmi->type = p;
        p = q + 1;

        if (!(q = strchr(p, ' '))) ffatal("invalid /proc/self/mounts (5)");
        *q = 0;
        pmi->options = p;
        p = q + 1;

        if (!(q = strchr(p, ' '))) ffatal("invalid /proc/self/mounts (6)");
        *q = 0;
        pmi->n1 = p;
        p = q + 1;

        if (!(q = strchr(p, '\n'))) ffatal("invalid /proc/self/mounts (7)");
        *q = 0;
        pmi->n2 = p;
        p = q + 1;
    }

    qsort(mi, lcount, sizeof(mi[0]), sort_func_1);

    // make everything private
    for (int i = 0; i < lcount; ++i) {
        if ((r = mount(NULL, mi[i].dst_path, NULL, MS_PRIVATE, NULL)) < 0) {
            ffatal("failed to make '%s' private: %s", mi[i].dst_path, strerror(errno));
        }
    }

    // unmount what we don't need
    for (int i = 0; i < lcount; ++i) {
        struct MountInfo *pmi = &mi[i];
        
        if (!strcmp(pmi->type, "fusectl")
            || !strcmp(pmi->type, "rpc_pipefs")
            || !strcmp(pmi->type, "securityfs")
            || !strcmp(pmi->type, "tracefs")
            || !strcmp(pmi->type, "configfs")
            || !strcmp(pmi->type, "fuse.portal")
            || !strcmp(pmi->type, "debugfs")
            || !strcmp(pmi->type, "pstore")
            || !strcmp(pmi->type, "bpf")
            || !strcmp(pmi->type, "hugetlbfs")) {
            if ((r = umount(pmi->dst_path)) < 0) {
                ffatal("failed to unmount '%s': %s", mi[i].dst_path, strerror(errno));
            }
        }
    }

    if (enable_sandbox_dir) {
        if (working_dir && *working_dir) {
            if ((r = mount(working_dir, "/sandbox", NULL, MS_BIND, NULL)) < 0) {
                ffatal("failed to mount /sandbox: %s", strerror(errno));
            }
        } else {
            char wd[PATH_MAX];
            if (!getcwd(wd, sizeof(wd))) {
                ffatal("failed to get current dir: %s", strerror(errno));
            }
            if ((r = mount(wd, "/sandbox", NULL, MS_BIND, NULL)) < 0) {
                ffatal("failed to mount /sandbox: %s", strerror(errno));
            }
        }
    }

    char empty_bind_path[PATH_MAX];
    snprintf(empty_bind_path, sizeof(empty_bind_path), "%s/empty", safe_dir_path);

    if (enable_proc) {
        if (enable_pid_ns) {
            // remout /proc to show restricted pids
            if ((r = mount("proc", "/proc", "proc", 0, NULL)) < 0) {
                ffatal("failed to mount /proc: %s", strerror(errno));
            }
        }
    } else {
        // remout /proc to empty directory
        if ((r = mount(empty_bind_path, "/proc", NULL, MS_BIND, NULL)) < 0) {
            ffatal("failed to mount %s as /proc: %s", empty_bind_path, strerror(errno));
        }
    }

    if (!enable_sys) {
        // remout /sys to empty directory
        if ((r = mount(empty_bind_path, "/sys", NULL, MS_BIND, NULL)) < 0) {
            ffatal("failed to mount /sys: %s", strerror(errno));
        }
    }

    if ((r = mount(empty_bind_path, "/boot", NULL, MS_BIND, NULL)) < 0) {
        ffatal("failed to mount /boot: %s", strerror(errno));
    }

    char bind_path[PATH_MAX];
    snprintf(bind_path, sizeof(bind_path), "%s/root", safe_dir_path);
    if ((r = mount(bind_path, "/root", NULL, MS_BIND, NULL)) < 0) {
        ffatal("failed to mount %s as /root: %s", bind_path, strerror(errno));
    }
    snprintf(bind_path, sizeof(bind_path), "%s/etc", safe_dir_path);
    if ((r = mount(bind_path, "/etc", NULL, MS_BIND, NULL)) < 0) {
        ffatal("failed to mount %s as /etc: %s", bind_path, strerror(errno));
    }
    snprintf(bind_path, sizeof(bind_path), "%s/var", safe_dir_path);
    if ((r = mount(bind_path, "/var", NULL, MS_BIND, NULL)) < 0) {
        ffatal("failed to mount %s as /var: %s", bind_path, strerror(errno));
    }

    // mount pristine /tmp, /dev/shm, /run
    if ((r = mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID | MS_NODEV, "size=1024m,nr_inodes=1024")) < 0) {
        ffatal("failed to mount /tmp: %s", strerror(errno));
    }
    if ((r = mount("mqueue", "/dev/mqueue", "mqueue", MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RELATIME, NULL)) < 0) {
        ffatal("failed to mount /dev/mqueue: %s", strerror(errno));
    }
    if ((r = mount("/tmp", "/run", NULL, MS_BIND, NULL)) < 0){
        ffatal("failed to mount /run: %s", strerror(errno));
    }
    if ((r = mount("/tmp", "/dev/shm", NULL, MS_BIND, NULL)) < 0){
        ffatal("failed to mount /dev/shm: %s", strerror(errno));
    }

    if (!enable_home) {
        if ((r = mount(empty_bind_path, "/home", NULL, MS_BIND, NULL)) < 0) {
            ffatal("failed to mount /home: %s", strerror(errno));
        }
    }

    free(mi);
    free(mnt_s);
}

static int
open_redirections(void)
{
    int retval = -1;

    // change euid
    if (setegid(exec_gid) < 0) {
        flog("setegid failed to group %d: %s", exec_gid, strerror(errno));
        goto failed;
    }
    if (seteuid(exec_uid) < 0) {
        flog("seteuid failed to user %d: %s", exec_uid, strerror(errno));
        goto failed_2;
    }

    if (stdin_name && *stdin_name) {
        if ((stdin_fd = open(stdin_name, O_RDONLY | O_CLOEXEC, 0)) < 0) {
            flog("failed to open %s for stdin: %s", stdin_name, strerror(errno));
            goto failed_3;
        }
    } else if (stdin_external_fd >= 0) {
        stdin_fd = stdin_external_fd;
    } else if (enable_redirect_null) {
        if ((stdin_fd = open("/dev/null", O_RDONLY | O_CLOEXEC, 0)) < 0) {
            flog("failed to open /dev/null for stdin: %s", strerror(errno));
            goto failed_3;
        }
    }

    if (stdout_name && *stdout_name) {
        if ((stdout_fd = open(stdout_name, stdout_mode | O_CLOEXEC, 0600)) < 0) {
            flog("failed to open %s for stdout: %s", stdout_name, strerror(errno));
            goto failed_3;
        }
    } else if (stdout_external_fd >= 0) {
        stdout_fd = stdout_external_fd;
    } else if (enable_redirect_null) {
        if ((stdout_fd = open("/dev/null", stdout_mode | O_CLOEXEC, 0600)) < 0) {
            flog("failed to open /dev/null for stdout: %s", strerror(errno));
            goto failed_3;
        }
    }

    if (stderr_name && *stderr_name) {
        if ((stderr_fd = open(stderr_name, stderr_mode | O_CLOEXEC, 0600)) < 0) {
            flog("failed to open %s for stderr: %s", stderr_name, strerror(errno));
            goto failed_3;
        }
    } else if (enable_output_merge) {
        if ((stderr_fd = dup((stdout_fd >= 0)?stdout_fd:1)) < 0) {
            flog("failed to duplicate for stderr");
            goto failed_3;
        }
        fcntl(stderr_fd, F_SETFD, fcntl(stderr_fd, F_GETFD) | O_CLOEXEC);
    } else if (enable_redirect_null) {
        if ((stderr_fd = open("/dev/null", stderr_mode | O_CLOEXEC, 0600)) < 0) {
            flog("failed to open /dev/null for stderr: %s", strerror(errno));
            goto failed_3;
        }
    }

    retval = 0;

failed_3:
    if (seteuid(0) < 0) {
        ffatal("cannot restore user to 0: %s", strerror(errno));
    }

failed_2:
    if (setegid(0) < 0) {
        ffatal("cannot restore group to 0: %s", strerror(errno));
    }

failed:
    return retval;
}

// kill all ejexec processes
static int
kill_all(void)
{
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "killing all processes: fork() failed: %s\n", strerror(errno));
        return -1;
    }
    if (!pid) {
        if (setuid(exec_uid) < 0) {
            fprintf(stderr, "killing all processes: setuid() failed: %s\n", strerror(errno));
            _exit(127);
        }
        // will kill this process as well
        kill(-1, SIGKILL);
        _exit(0);
    }

    // wait for any child remaining
    while (wait(NULL) > 0) {}

    return 0;
}

struct process_info
{
  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  unsigned flags;
  unsigned long minflt;
  unsigned long cminflt;
  unsigned long majflt;
  unsigned long cmajflt;
  unsigned long utime;
  unsigned long stime;
  unsigned long cutime;
  unsigned long cstime;
  long priority;
  long nice;
  long num_threads;
  long itrealvalue;
  long long starttime;
  unsigned long vsize;
  long rss;
  unsigned long rsslim;
  unsigned long startcode;
  unsigned long endcode;
  unsigned long startstack;
  unsigned long kstkesp;
  unsigned long kstkeip;
  unsigned long signal;
  unsigned long blocked;
  unsigned long sigignore;
  unsigned long sigcatch;
  unsigned long wchan;
  unsigned long nswap;
  unsigned long cnswap;
  int exit_signal;
  int processor;
};

static int
parse_proc_pid_stat(int pid, struct process_info *info)
{
  char path[PATH_MAX];
  FILE *f = NULL;
  char buf[8192];
  int blen;

  memset(info, 0, sizeof(*info));
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);
  f = fopen(path, "r");
  if (!f) goto fail;
  if (!fgets(buf, sizeof(buf), f)) goto fail;
  blen = strlen(buf);
  if (blen + 1 == sizeof(buf)) goto fail;
  fclose(f); f = NULL;

  char *p = strrchr(buf, ')');
  if (!p) goto fail;
  ++p;

  int r = sscanf(p, " %c%d%d%d%d%d%u%lu%lu%lu%lu%lu%lu%lu%lu%ld%ld%ld%ld%llu%lu%ld%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%d%d",
                 &info->state,
                 &info->ppid,
                 &info->pgrp,
                 &info->session,
                 &info->tty_nr,
                 &info->tpgid,
                 &info->flags,
                 &info->minflt,
                 &info->cminflt,
                 &info->majflt,
                 &info->cmajflt,
                 &info->utime,
                 &info->stime,
                 &info->cutime,
                 &info->cstime,
                 &info->priority,
                 &info->nice,
                 &info->num_threads,
                 &info->itrealvalue,
                 &info->starttime,
                 &info->vsize,
                 &info->rss,
                 &info->rsslim,
                 &info->startcode,
                 &info->endcode,
                 &info->startstack,
                 &info->kstkesp,
                 &info->kstkeip,
                 &info->signal,
                 &info->blocked,
                 &info->sigignore,
                 &info->sigcatch,
                 &info->wchan,
                 &info->nswap,
                 &info->cnswap,
                 &info->exit_signal,
                 &info->processor);
  if (r != 37) goto fail;

  return 0;

fail:
  if (f) fclose(f);
  return -1;
}

static int
count_processes(void)
{
    DIR *d = opendir("/proc");
    if (!d) {
        kill_all();
        ffatal("failed to open /proc: %s", strerror(errno));
    }
    struct dirent *dd;
    int count = 0;
    while ((dd = readdir(d))) {
        errno = 0;
        char *eptr = NULL;
        long v = strtol(dd->d_name, &eptr, 10);
        if (!errno && !*eptr && eptr != dd->d_name && v > 1) {
            ++count;
        }
    }
    closedir(d);
    return count;
}

#define MQUEUE_MOUNT_DIR "/dev/mqueue"

static int
scan_posix_mqueue(int search_uid)
{
    DIR *d = NULL;
    struct dirent *dd;
    int count = 0;

    if (!(d = opendir(MQUEUE_MOUNT_DIR))) return 0;

    while ((dd = readdir(d))) {
        char buf[PATH_MAX];
        if (snprintf(buf, sizeof(buf), "%s/%s", MQUEUE_MOUNT_DIR, dd->d_name) >= sizeof(buf)) continue;
        struct stat stb;
        if (lstat(buf, &stb) < 0) continue;
        if (!S_ISREG(stb.st_mode)) continue;
        if (stb.st_uid != search_uid) continue;
        ++count;
        flog("POSIX message queue: name = /%s, perms = %03o", dd->d_name, (stb.st_mode & 0777));
    }
    closedir(d); d = NULL;

    return count;
}

static int
scan_msg(int search_uid)
{
    int count = 0;
    char buf[1024];
    

    FILE *f = fopen("/proc/sysvipc/msg", "r");
    if (!f) return 0;

    if (getl(buf, sizeof(buf), f) < 0) {
        ffatal("unexpected EOF in '/proc/sysvipc/msg'");
    }
    while (getl(buf, sizeof(buf), f) >= 0) {
        int key = 0, msgid = 0, perms = 0, cbytes = 0, qnum = 0, lspid = 0, lrpid = 0, uid = 0, gid = 0;
        if (sscanf(buf, "%d%d%o%d%d%d%d%d%d", &key, &msgid, &perms, &cbytes, &qnum, &lspid, &lrpid, &uid, &gid) != 9) {
            ffatal("format error in '/proc/sysvipc/msg'");
        }
        if (uid == search_uid) {
            flog("message queue: key = 0x%08x, msgid = %d, perms = %03o", key, msgid, perms);
            if (msgctl(msgid, IPC_RMID, NULL) < 0) {
                flog("msgctl failed: %s", strerror(errno));
            }
            ++count;
        }
    }

    fclose(f);
    return count;
}

static int
scan_sem(int search_uid)
{
    int count = 0;
    char buf[1024];

    FILE *f = fopen("/proc/sysvipc/sem", "r");
    if (!f) return 0;

    if (getl(buf, sizeof(buf), f) < 0) {
        ffatal("unexpected EOF in '/proc/sysvipc/sem'");
    }
    while (getl(buf, sizeof(buf), f) >= 0) {
        int key = 0, semid = 0, perms = 0, nsems = 0, uid = 0, gid = 0, cuid = 0;
        if (sscanf(buf, "%d%d%o%d%d%d%d", &key, &semid, &perms, &nsems, &uid, &gid, &cuid) != 7) {
            ffatal("format error in '/proc/sysvipc/sem'");
        }
        if (uid == search_uid || cuid == search_uid) {
            flog("semaphore array: key = 0x%08x, semid = %d, perms = %03o", key, semid, perms);
            if (semctl(semid, 0, IPC_RMID, NULL) < 0) {
                flog("semctl failed: %s", strerror(errno));
            }
            ++count;
        }
    }

    fclose(f);
    return count;
}

static int
scan_shm(int search_uid)
{
    int count = 0;
    char buf[1024];

    FILE *f = fopen("/proc/sysvipc/shm", "r");
    if (!f) return 0;

    if (getl(buf, sizeof(buf), f) < 0) {
        ffatal("unexpected EOF in '/proc/sysvipc/shm'");
    }
    while (getl(buf, sizeof(buf), f) >= 0) {
        int key = 0, shmid = 0, perms = 0, size = 0, cpid = 0, lpid = 0, nattch = 0, uid = 0, gid = 0;
        if (sscanf(buf, "%d%d%o%d%d%d%d%d%d", &key, &shmid, &perms, &size, &cpid, &lpid, &nattch, &uid, &gid) != 9) {
            ffatal("format error in '/proc/sysvipc/shm'");
        }
        if (uid == search_uid) {
            flog("shared memory: key = 0x%08x, shmid = %d, perms = %03o", key, shmid, perms);
            if (shmctl(shmid, IPC_RMID, NULL) < 0) {
                flog("shmctl failed: %s", strerror(errno));
            }
            ++count;
        }
    }

    fclose(f);
    return count;
}

static char *
extract_string(const char **ppos, int init_offset, const char *opt_name)
{
    const char *pos = *ppos + init_offset;
    if (*pos >= '0' && *pos <= '9') {
        char *eptr = NULL;
        errno = 0;
        long v = strtol(pos, &eptr, 10);
        if (errno || v < 0 || (int) v != v) {
            ffatal("invalid specification for option %s", opt_name);
        }
        int len = v;
        pos = eptr;
        char *str = calloc(len + 1, 1);
        strncpy(str, pos, len);
        int len2 = strlen(pos);
        if (len2 < len) len = len2;
        pos += len;
        *ppos = pos;
        return str;
    } else if (*pos) {
        char sep = *pos++;
        const char *start = pos;
        int len = 0;
        while (*pos && *pos != sep) { ++len; ++pos; }
        if (!*pos) ffatal("invalid specification for option %s", opt_name);
        char *str = calloc(len + 1, 1);
        memcpy(str, start, len);
        *ppos = ++pos;
        return str;
    } else {
        ffatal("invalid specification for option %s", opt_name);
    }
}

static long long
extract_size(const char **ppos, int init_offset, const char *opt_name)
{
    const char *pos = *ppos + init_offset;
    errno = 0;
    char *eptr = NULL;
    long long v = strtoll(pos, &eptr, 10);
    if (errno || eptr == pos || v < 0) {
        ffatal("invalid size for option %s", opt_name);
    }
    pos = eptr;
    if (*pos == 'k' || *pos == 'K') {
        if (__builtin_mul_overflow(v, 1024LL, &v)) fatal("size overflow for option %s", opt_name);
    } else if (*pos == 'm' || *pos == 'M') {
        if (__builtin_mul_overflow(v, 1048576LL, &v)) fatal("size overflow for option %s", opt_name);
    } else if (*pos == 'g' || *pos == 'G') {
        if (__builtin_mul_overflow(v, 1073741824LL, &v)) fatal("size overflow for option %s", opt_name);
    }
    if ((size_t) v != v) ffatal("size overflow for option %s", opt_name);
    *ppos = eptr;
    return v;
}

/*
 * option specification:
 *   f<FD>  - set log file descriptor
 *   mg     - disable control group
 *   mi     - disable IPC namespace
 *   mn     - disable net namespace
 *   mm     - disable mount namespace
 *   mp     - disable PID namespace
 *   mP     - enable /proc filesystem
 *   mS     - enable /sys filesystem
 *   ms     - disable bindind of working dir to /sandbox
 *   mh     - enable /home filesystem
 *   mo     - disable chown to ejexec
 *   mG     - disable process group
 *   mc     - enable orphaned process count
 *   mI     - enable IPC count
 *   ma     - unlimited cpu time
 *   mb     - unlimited real time
 *   w<DIR> - working directory (cwd by default)
 *   rn     - redirect to/from /dev/null for standard streams
 *   rm     - merge stdout and stderr output
 *   ri<FI> - redirect stdin < FI
 *   ro<FI> - redirect stdout > FI
 *   rO<FI> - redirect stdout >> FI
 *   re<FI> - redirect stderr > FI
 *   rE<FI> - redirect stderr >> FI
 *   rp<S>  - set start program name (argv[0])
 *   ra<FD> - redirect stdin from FD
 *   rb<FD> - redirect stdout to FD
 *   lt<T>  - set CPU time limit (ms)
 *   lr<T>  - set REAL time limit (ms)
 *   lm<M>  - set umask (M - octal value)
 *   lo<N>  - set limit to file descriptors
 *   ls<Z>  - set stack limit
 *   lv<Z>  - set VM limit
 *   lf<Z>  - set file size limit
 *   lu<N>  - set user processes limit
 */

/* options to start default bash: */

int
main(int argc, char *argv[])
{
    int argi = 1;

    {
        char *p = strrchr(argv[0], '/');
        if (p) program_name = p + 1;
        else program_name = argv[0];
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_DFL);
    log_f = open_memstream(&log_s, &log_z);

    get_user_ids();

    snprintf(safe_dir_path, sizeof(safe_dir_path), "%s/share/ejudge/container", EJUDGE_PREFIX_DIR);

#ifndef ENABLE_ANY_USER
    {
        int self_uid = getuid();
        if (self_uid != primary_uid && self_uid != 0) {
            ffatal("not allowed");
        }
    }
#endif

    if (argc < 1) {
        flog("wrong number of arguments");
        fatal();
    }

    if (argi < argc && argv[argi][0] == '-') {
        // parse options
        const char *opt = argv[argi++] + 1;
        while (*opt) {
            if (*opt == ',') {
                ++opt;
            } else if (*opt == 'f') {
                // log file descriptor
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 1, &eptr, 10);
                int flags;
                if (errno || eptr == opt + 1 || v < 0 || (int) v != v
                    || (flags = fcntl((int) v, F_GETFD)) < 0
                    || fcntl((int) v, F_SETFD, flags | O_CLOEXEC) < 0) {
                    flog("invalig log file descriptor");
                    fatal();
                }
                response_fd = v;
                opt = eptr;
            } else if (*opt == 'm' && opt[1] == 'g') {
                enable_cgroup = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'i') {
                enable_ipc_ns = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'n') {
                enable_net_ns = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'm') {
                enable_mount_ns = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'p') {
                enable_pid_ns = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'P') {
                enable_proc = 1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'S') {
                enable_sys = 1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 's') {
                enable_sandbox_dir = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'h') {
                enable_home = 1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'o') {
                enable_chown = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'G') {
                enable_pgroup = 0;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'c') {
                enable_prc_count = 1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'I') {
                enable_ipc_count = 1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'a') {
                limit_cpu_time_ms = -1;
                opt += 2;
            } else if (*opt == 'm' && opt[1] == 'b') {
                limit_real_time_ms = -1;
                opt += 2;
            } else if (*opt == 'w') {
                working_dir = extract_string(&opt, 1, "w");
            } else if (*opt == 'r' && opt[1] == 'n') {
                enable_redirect_null = 1;
                opt += 2;
            } else if (*opt == 'r' && opt[1] == 'm') {
                enable_output_merge = 1;
                opt += 2;
            } else if (*opt == 'r' && opt[1] == 'i') {
                stdin_name = extract_string(&opt, 2, "ri");
            } else if (*opt == 'r' && opt[1] == 'o') {
                stdout_name = extract_string(&opt, 2, "ro");
                stdout_mode = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (*opt == 'r' && opt[1] == 'O') {
                stdout_name = extract_string(&opt, 2, "rO");
                stdout_mode = O_WRONLY | O_CREAT | O_APPEND;
            } else if (*opt == 'r' && opt[1] == 'e') {
                stderr_name = extract_string(&opt, 2, "re");
                stderr_mode = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (*opt == 'r' && opt[1] == 'E') {
                stderr_name = extract_string(&opt, 2, "rE");
                stderr_mode = O_WRONLY | O_CREAT | O_APPEND;
            } else if (*opt == 'r' && opt[1] == 'p') {
                start_program_name = extract_string(&opt, 2, "rp");
            } else if (*opt == 'r' && opt[1] == 'a') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                struct stat stb;
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v || fstat(v, &stb) < 0) {
                    ffatal("invalid file descriptor");
                }
                stdin_external_fd = v;
                opt = eptr;
            } else if (*opt == 'r' && opt[1] == 'b') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                struct stat stb;
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v || fstat(v, &stb) < 0) {
                    ffatal("invalid file descriptor");
                }
                stdout_external_fd = v;
                opt = eptr;
            } else if (*opt == 'l' && opt[1] == 'm') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 8);
                if (errno || eptr == opt + 2 || v < 0) {
                    ffatal("invalid umask");
                }
                limit_umask = v & 0777;
                opt = eptr;
            } else if (*opt == 'l' && opt[1] == 'o') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v) {
                    ffatal("invalid open files limit");
                }
                if (!v) v = -1;
                limit_open_files = v;
                opt = eptr;
            } else if (*opt == 'l' && opt[1] == 's') {
                limit_stack_size = extract_size(&opt, 2, "ls");
            } else if (*opt == 'l' && opt[1] == 'v') {
                limit_vm_size = extract_size(&opt, 2, "lv");
            } else if (*opt == 'l' && opt[1] == 'f') {
                limit_file_size = extract_size(&opt, 2, "lf");
            } else if (*opt == 'l' && opt[1] == 'u') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v) {
                    ffatal("invalid processes limit");
                }
                if (!v) v = -1;
                limit_processes = v;
                opt = eptr;
            } else if (*opt == 'l' && opt[1] == 't') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v) {
                    ffatal("invalid cpu time limit");
                }
                if (!v) v = -1;
                limit_cpu_time_ms = v;
                opt = eptr;
            } else if (*opt == 'l' && opt[1] == 'r') {
                char *eptr = NULL;
                errno = 0;
                long v = strtol(opt + 2, &eptr, 10);
                if (errno || eptr == opt + 2 || v < 0 || (int) v != v) {
                    ffatal("invalid real time limit");
                }
                if (!v) v = -1;
                limit_real_time_ms = v;
                opt = eptr;
            } else {
                flog("invalid option: %s", opt);
                fatal();
            }
        }
    }

    start_args = argv + argi;
    start_program = argv[argi];
    if (argi == argc) {
#ifdef ENABLE_BASH
        bash_mode = 1;
#else
        ffatal("no program to run");
#endif
    }

    if (enable_chown) {
        change_ownership(exec_uid, exec_gid, primary_uid);
    }

    if (open_redirections() < 0) {
        if (enable_chown) {
            change_ownership(primary_uid, primary_gid, exec_uid);
        }
        fatal();
    }

    unsigned clone_flags = CLONE_PIDFD | CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | SIGCHLD;
    if (enable_cgroup) clone_flags |= CLONE_NEWCGROUP;
    if (enable_ipc_ns) clone_flags |= CLONE_NEWIPC;
    if (enable_net_ns) clone_flags |= CLONE_NEWNET;
    if (enable_mount_ns) clone_flags |= CLONE_NEWNS;
    if (enable_pid_ns) clone_flags |= CLONE_NEWPID;

    int pidfd = -1;
    pid_t tidptr = 0;
    int pid = syscall(__NR_clone, clone_flags, NULL, &pidfd, &tidptr);
    if (pid < 0) {
        change_ownership(primary_uid, primary_gid, exec_uid);
        ffatal("clone failed: %s", strerror(errno));
    }

    if (!pid) {
        if (enable_mount_ns) {
            reconfigure_fs();
        }

        sigset_t bs;
        sigemptyset(&bs); sigaddset(&bs, SIGCHLD);
        sigprocmask(SIG_BLOCK, &bs, NULL);

        // we need another child, because this one has PID 1
        int pid2 = fork();
        if (pid2 < 0) {
            ffatal("pid failed: %s", strerror(errno));
        }

        if (!pid2) {
            if (enable_pgroup) {
                //setpgid(0, 0);
                if (setsid() < 0) fprintf(stderr, "setsid() failed: %s\n", strerror(errno));
            }
            if (enable_sandbox_dir) {
                if (chdir("/sandbox") < 0) {
                    fprintf(stderr, "failed to change dir to /sandbox: %s\n", strerror(errno));
                    _exit(127);
                }
            } else if (working_dir && *working_dir) {
                if (chdir(working_dir) < 0) {
                    fprintf(stderr, "failed to change dir to %s: %s\n", working_dir, strerror(errno));
                    _exit(127);
                }
            }
            /*
            if (enable_proc) {
                if (enable_pid_ns) {
                    // remout /proc to show restricted pids
                    int r;
                    if ((r = mount("proc", "/proc", "proc", 0, NULL)) < 0) {
                        fprintf(stderr, "failed to mount /proc: %s\n", strerror(errno));
                        _exit(127);
                    }
                }
            } else {
                // remout /proc to empty directory
                int r;
                if ((r = mount("/var/empty", "/proc", NULL, MS_BIND, NULL)) < 0) {
                    fprintf(stderr, "failed to mount /proc: %s", strerror(errno));
                    _exit(127);
                }
            }
            */

            if (limit_umask >= 0) {
                umask(limit_umask & 0777);
            }

            /* not yet supported: RLIMIT_MEMLOCK, RLIMIT_MSGQUEUE, RLIMIT_NICE, RLIMIT_RTPRIO, RLIMIT_SIGPENDING */

            if (limit_vm_size > 0) {
                struct rlimit lim = { .rlim_cur = limit_vm_size, .rlim_max = limit_vm_size };
                if (setrlimit(RLIMIT_AS, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_AS %lld failed: %s", limit_vm_size, strerror(errno));
                    _exit(127);
                }
            }

            if (limit_stack_size >= 0) {
                struct rlimit lim = { .rlim_cur = limit_stack_size, .rlim_max = limit_stack_size };
                if (setrlimit(RLIMIT_STACK, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_STACK %lld failed: %s", limit_stack_size, strerror(errno));
                    _exit(127);
                }
            }

            if (limit_file_size >= 0) {
                struct rlimit lim = { .rlim_cur = limit_file_size, .rlim_max = limit_file_size };
                if (setrlimit(RLIMIT_FSIZE, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_FILE %lld failed: %s", limit_file_size, strerror(errno));
                    _exit(127);
                }
            }

            if (limit_processes >= 0) {
                struct rlimit lim = { .rlim_cur = limit_processes, .rlim_max = limit_processes };
                if (setrlimit(RLIMIT_NPROC, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_NPROC %d failed: %s", limit_processes, strerror(errno));
                    _exit(127);
                }
            }

            // disable core dumps
            {
                struct rlimit lim = { .rlim_cur = 0, .rlim_max = 0 };
                if (setrlimit(RLIMIT_CORE, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_CORE 0 failed: %s", strerror(errno));
                    _exit(127);
                }
            }

            if (stdin_fd >= 0) {
                dup2(stdin_fd, 0); close(stdin_fd);
            }
            if (stdout_fd >= 0) {
                dup2(stdout_fd, 1); close(stdout_fd);
            }
            if (stderr_fd >= 0) {
                dup2(stderr_fd, 2); close(stderr_fd);
            }

            if (limit_open_files >= 0) {
                struct rlimit lim = { .rlim_cur = limit_open_files, .rlim_max = limit_open_files };
                if (setrlimit(RLIMIT_NOFILE, &lim) < 0) {
                    fprintf(stderr, "rlimit for RLIMIT_NOFILE %d failed: %s", limit_open_files, strerror(errno));
                    _exit(127);
                }
            }

            sigset_t ss;
            sigemptyset(&ss);
            sigprocmask(SIG_SETMASK, &ss, NULL);

            // switch to ejexec user
            if (setgid(exec_gid) < 0) {
                fprintf(stderr, "setgid failed: %s\n", strerror(errno));
                _exit(127);
            }
            gid_t supp_groups[1] = { exec_gid };
            if (setgroups(1, supp_groups) < 0) {
                fprintf(stderr, "setgroups failed: %s\n", strerror(errno));
                _exit(127);
            }
            if (setuid(exec_uid) < 0) {
                fprintf(stderr, "setuid setuid failed: %s\n", strerror(errno));
                _exit(127);
            }

            if (bash_mode) {
                printf("child: %d, %d, %d\n", getpid(), getppid(), tidptr);
                printf("init success, starting /bin/bash\n");
                // FIXME: change prompt
                execlp("/bin/bash", "/bin/bash", "-i", NULL);
                fprintf(stderr, "failed to exec /bin/bash: %s\n", strerror(errno));
            } else {
                if (start_program_name) {
                    start_args[0] = start_program_name;
                }
                execve(start_program, start_args, environ);
                fprintf(stderr, "failed to exec '%s': %s\n", start_program, strerror(errno));
            }

            _exit(127);
        }

        // now the child process is already running,
        // so we can't just fail, we have to kill created processes

        // parent
        /*
        if (enable_pid_ns) {
            // remout /proc to show restricted pids
            int r;
            if ((r = mount("proc", "/proc", "proc", 0, NULL)) < 0) {
                kill_all();
                ffatal("failed to mount /proc: %s", strerror(errno));
            }
        }
        */

        if (enable_pgroup) {
            //setpgid(pid2, pid2);
        }
        if (stdin_fd >= 0) close(stdin_fd);
        if (stdout_fd >= 0) close(stdout_fd);
        if (stderr_fd >= 0) close(stderr_fd);
        stdin_fd = -1; stdout_fd = -1; stderr_fd = -1;

        int sfd = signalfd(-1, &bs, 0);
        if (sfd < 0) {
            kill_all();
            ffatal("failed to create signalfd: %s", strerror(errno));
        }

        int tfd = timerfd_create(CLOCK_REALTIME, 0);
        if (tfd < 0) {
            kill_all();
            ffatal("failed to create timerfd: %s", strerror(errno));
        }

        // 100ms interval
        struct itimerspec its = { .it_interval = { .tv_nsec = 100000000 }, .it_value = { .tv_nsec = 100000000 } };
        if (timerfd_settime(tfd, 0, &its, NULL) < 0) {
            kill_all();
            ffatal("failed timerfd_settime: %s", strerror(errno));
        }

        int efd = epoll_create1(0);
        if (efd < 0) {
            kill_all();
            ffatal("failed to create eventfd: %s", strerror(errno));
        }

        struct epoll_event see = { .events = EPOLLIN, .data = { .fd = sfd } };
        if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &see) < 0) {
            kill_all();
            ffatal("failed epoll_ctl: %s", strerror(errno));
        }

        struct epoll_event tee = { .events = EPOLLIN, .data = { .fd = tfd } };
        if (epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &tee) < 0) {
            kill_all();
            ffatal("failed epoll_ctl: %s", strerror(errno));
        }

        long clock_ticks = sysconf(_SC_CLK_TCK);

        int prc_finished = 0;
        int prc_status = 0;
        struct rusage prc_usage = {};
        struct process_info prc_info = {};
        int prc_real_time_exceeded = 0;
        long long prc_start_time_us = 0;
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            prc_start_time_us = tv.tv_sec * 1000000LL + tv.tv_usec;
        }
        long long prc_stop_time_us = 0;
        long long prc_vm_size = -1;
        int prc_time_exceeded = 0;

        int flag = 1;
        while (flag) {
            struct epoll_event events[2];
            int res = epoll_wait(efd, events, 2, -1);
            if (res < 0) {
                kill_all();
                ffatal("failed epoll_wait: %s", strerror(errno));
            }
            if (!res) {
                kill_all();
                ffatal("unexpected 0 from epoll_wait");
            }
            for (int i = 0; i < res; ++i) {
                struct epoll_event *curev = &events[i];
                if (curev->data.fd == sfd) {
                    // received signal
                    struct signalfd_siginfo sss;
                    int z;
                    if ((z = read(sfd, &sss, sizeof(sss))) != sizeof(sss)) {
                        kill_all();
                        ffatal("read from signalfd return %d", z);
                    }
                    if (sss.ssi_signo != SIGCHLD) {
                        kill_all();
                        ffatal("unexpected signal %d", sss.ssi_signo);
                    }
                    int status = 0;
                    struct rusage ru;
                    while (1) {
                        int res = wait4(-1, &status, WNOHANG, &ru);
                        if (res < 0) {
                            if (errno == ECHILD) {
                                if (!prc_finished) {
                                    kill_all();
                                    ffatal("child lost");
                                }
                                break;
                            } else {
                                kill_all();
                                ffatal("wait4 failed %s", strerror(errno));
                            }
                        } else if (res == pid2) {
                            prc_finished = 1;
                            prc_status = status;
                            prc_usage = ru;
                            {
                                struct timeval tv;
                                gettimeofday(&tv, NULL);
                                prc_stop_time_us = tv.tv_sec * 1000000LL + tv.tv_usec;
                            }
                            flag = 0;
                        }
                    }
                } else if (curev->data.fd == tfd) {
                    uint64_t val;
                    if (read(tfd, &val, sizeof(val)) != sizeof(val)) {
                        kill_all();
                        ffatal("invalid timer read");
                    }

                    // 0.1s elapsed
                    if (!prc_finished) {
                        if (parse_proc_pid_stat(pid2, &prc_info) < 0) {
                            kill_all();
                            ffatal("parsing of /proc/pid/stat failed");
                        }

                        if (prc_info.vsize > 0) {
                            if (prc_vm_size < 0 || prc_info.vsize > prc_vm_size) {
                                prc_vm_size = prc_info.vsize;
                            }
                        }

                        long long cur_cpu_time = (long long) prc_info.utime + (long long) prc_info.stime;
                        cur_cpu_time = (cur_cpu_time * 1000) / clock_ticks;
                        if (limit_cpu_time_ms > 0 && cur_cpu_time >= limit_cpu_time_ms) {
                            flag = 0;
                            prc_time_exceeded = 1;
                            break;
                        }
                        long long cur_time_us = 0;
                        {
                            struct timeval tv;
                            gettimeofday(&tv, NULL);
                            cur_time_us = tv.tv_sec * 1000000LL + tv.tv_usec;
                        }

                        if (limit_real_time_ms > 0 && (cur_time_us - prc_start_time_us) >= limit_real_time_ms * 1000LL) {
                            // REAL-TIME limit exceeded
                            flag = 0;
                            prc_real_time_exceeded = 1;
                            break;
                        }
                    }
                }
            }
        }

        if (prc_time_exceeded) {
            kill_all();

            if (log_f) {
                fclose(log_f); log_f = NULL;
            }
            dprintf(response_fd, "tT%lldu%lldk%lld", limit_cpu_time_ms * 1000LL, limit_cpu_time_ms * 1000LL, 0LL);
            if (log_s && *log_s) {
                int len = strlen(log_s);
                dprintf(response_fd, "L%d,%s", len, log_s);
            }
            _exit(0);
        }

        if (prc_real_time_exceeded) {
            kill_all();

            if (log_f) {
                fclose(log_f); log_f = NULL;
            }
            dprintf(response_fd, "rR%lld", limit_real_time_ms * 1000LL);
            if (log_s && *log_s) {
                int len = strlen(log_s);
                dprintf(response_fd, "L%d,%s", len, log_s);
            }
            _exit(0);
        }

        if (!WIFEXITED(prc_status) && !WIFSIGNALED(prc_status)) {
            kill_all();
            ffatal("wait4 process is neither exited nor signaled");
        }

        int orphaned_processes = 0;
        if (enable_prc_count) {
            orphaned_processes = count_processes();
        }

        kill_all();

        int ipc_objects = 0;
        if (enable_ipc_count) {
            ipc_objects += scan_posix_mqueue(exec_uid);
            ipc_objects += scan_msg(exec_uid);
            ipc_objects += scan_sem(exec_uid);
            ipc_objects += scan_shm(exec_uid);
        }

        if (WIFEXITED(prc_status)) {
            dprintf(response_fd, "e%d", WEXITSTATUS(prc_status));
        } else if (WIFSIGNALED(prc_status)) {
            dprintf(response_fd, "s%d", WTERMSIG(prc_status));
        } else {
            abort();
        }

        long long cpu_utime_us = prc_usage.ru_utime.tv_sec * 1000000LL + prc_usage.ru_utime.tv_usec;
        long long cpu_stime_us = prc_usage.ru_stime.tv_sec * 1000000LL + prc_usage.ru_stime.tv_usec;
        long long cpu_time_us = cpu_utime_us + cpu_stime_us;
        dprintf(response_fd, "T%lldR%lldu%lldk%lld", cpu_time_us, prc_stop_time_us - prc_start_time_us, cpu_utime_us, cpu_stime_us);
        if (prc_vm_size > 0) dprintf(response_fd, "v%lld", prc_vm_size);
        if (prc_usage.ru_maxrss > 0) dprintf(response_fd, "e%lld", (long long) prc_usage.ru_maxrss * 1024LL);
        dprintf(response_fd, "a%lldb%lld", (long long) prc_usage.ru_nvcsw, (long long) prc_usage.ru_nivcsw);
        if (ipc_objects > 0) dprintf(response_fd, "i%d", ipc_objects);
        if (orphaned_processes > 0) dprintf(response_fd, "o%d", orphaned_processes);
            if (log_f) {
                fclose(log_f); log_f = NULL;
            }
            if (log_s && *log_s) {
                int len = strlen(log_s);
                dprintf(response_fd, "L%d,%s", len, log_s);
            }

        _exit(0);
    }

    if (stdin_fd >= 0) close(stdin_fd);
    if (stdout_fd >= 0) close(stdout_fd);
    if (stderr_fd >= 0) close(stderr_fd);
    stdin_fd = -1; stdout_fd = -1; stderr_fd = -1;

    siginfo_t infop = {};
    //waitid(P_PIDFD, pidfd, &infop, WEXITED);
    waitid(P_PID, pid, &infop, WEXITED);
    if (bash_mode) {
        printf("bash finished\n");
        printf("parent: %d, %d, %d\n", pid, pidfd, tidptr);
    }

    change_ownership(primary_uid, primary_gid, exec_uid);

    if (infop.si_code == CLD_EXITED) {
        if (infop.si_status == 0 || infop.si_status == 1) _exit(infop.si_status);
        ffatal("unexpected exit code from container leader: %d", infop.si_status);
    } else if (infop.si_code == CLD_KILLED || infop.si_code == CLD_DUMPED) {
        ffatal("container leader terminated by signal %d", infop.si_status);
    } else {
        ffatal("unexpected si_code from waitid");
    }
}
