#include "sysctl_handler.h"

static struct ctl_table tcp_tune_table[] = {
    {
        CTL_UNNAME
        .procname	= "tcp_initial_cwnd",
        .data		= &sysctl_tcp_initial_cwnd,
        .maxlen		= sizeof(int),
        .mode		= 0644,
        .proc_handler	= proc_dointvec_minmax,
        .extra2     = &sysctl_tcp_cwnd_clamp,
        .extra1     = &min_cwnd
    },
    {
        CTL_UNNAME
        .procname   = "tcp_cwnd_clamp",
        .data       = &sysctl_tcp_cwnd_clamp,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler	= proc_dointvec,
    },
    {
        CTL_UNNAME
        .procname   = "tcp_min_rto",
        .data       = &sysctl_tcp_min_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler	= proc_dointvec,
    },
    {
        CTL_UNNAME
        .procname   = "tcp_max_rto",
        .data       = &sysctl_tcp_max_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler	= proc_dointvec,
    },
    {
        CTL_UNNAME
        .procname   = "tcp_random_rto",
        .data       = &sysctl_tcp_random_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
        .proc_handler	= proc_dointvec,
    },
    {
        CTL_NAME(0)
    }
};

#if TUNE_COMPAT < 19
static ctl_table ipv4_net_table[] = {
    {
        .ctl_name   = CTL_UNNUMBERED,
        .procname   = "ipv4",
        .mode       = 0555,
        .child      = tcp_tune_table
    },
    { .ctl_name = 0 }
};

static ctl_table net_root_table[] = {
    {
        .ctl_name   = CTL_UNNUMBERED,
        .procname   = "net",
        .mode       = 0555,
        .child      = ipv4_net_table
    },
    { .ctl_name = 0 }
};
#endif

static struct ctl_table_header * hdr;


int register_sysctl_entries(void) 
{

#if TUNE_COMPAT > 18
    hdr = register_sysctl_paths(net_ipv4_ctl_path, tcp_tune_table);
#else
    hdr = register_sysctl_table(net_root_table, 0);
#endif

    if (hdr == NULL) {
        return -1;
    }

    return 0;
}

void unregister_sysctl_enteries(void)
{
    unregister_sysctl_table(hdr);
}
