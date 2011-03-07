#ifndef TUNE_SYSCTL_HANDLER
#define TUNE_SYSCTL_HANDLER

#include <linux/proc_fs.h>
#include <linux/tcp.h>
#include <linux/sysctl.h>
#include <net/tcp.h>


#include "version.h"


#if TUNE_COMPAT < 19
#define CTL_UNNUMBERED -2
#define CTL_NAME(NAME) .ctl_name = (NAME)
#define CTL_UNNAME .ctl_name = CTL_UNNUMBERED,
#define CONFIG_DEFAULT_TCP_CONG "bic"
#else
#define CTL_UNNAME 
#define CTL_NAME(NAME)
#endif

static int sysctl_tcp_initial_cwnd __read_mostly = 3;
static int sysctl_tcp_cwnd_clamp __read_mostly = 64;
static int sysctl_tcp_max_rto __read_mostly = 200000;
static int sysctl_tcp_random_rto __read_mostly = 0;
static int sysctl_tcp_min_rto __read_mostly = 20;

static int min_cwnd = 2;

int register_sysctl_entries(void);
void unregister_sysctl_enteries(void);

#endif
