/*
 * tcptune - Tune TCP global parameters 
 *
 * Copyright (C) 2010, Soheil Hassas Yeganeh <soheil@cs.toronto.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/sysctl.h>
#include <net/tcp.h>

//#define TCP_TUNE_DEBUG

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
#define TUNE_COMPAT 18
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
#define TUNE_COMPAT 32
#else
#define TUNE_COMPAT 35
#endif

#if TUNE_COMPAT < 19
#define CTL_UNNUMBERED -2
#define CTL_NAME(NAME) .ctl_name = (NAME)
#define CTL_UNNAME .ctl_name = CTL_UNNUMBERED,

static inline void set_dst_metric_rtt(struct dst_entry *dst, int metric,
                              unsigned long rtt)
{
        dst->metrics[metric-1] = jiffies_to_msecs(rtt);
}

#else
#define CTL_UNNAME 
#define CTL_NAME(NAME)
#endif


MODULE_AUTHOR("Soheil Hassas Yeganeh <soheil@cs.toronto.edu>");
MODULE_DESCRIPTION("TCP parameter tuner");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1-ALPHA");

int sysctl_tcp_initial_cwnd __read_mostly = 3;
int sysctl_tcp_cwnd_clamp __read_mostly = 64;
int sysctl_tcp_max_rto __read_mostly = 200000;
int sysctl_tcp_random_rto __read_mostly = 0;
int sysctl_tcp_min_rto __read_mostly = 20;

int min_cwnd = 3;
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


static inline void set_dst_metric_cwnd(struct dst_entry *dst, int metric, __u32 cwnd) {
    if (!dst_metric_locked(dst, metric)) {
        dst->metrics[metric-1] = cwnd;
    }
}

static void tcp_tune_dst(struct tcp_sock *tp, struct dst_entry *dst) {
    if (dst != NULL) {
        __u32 cwnd = dst_metric(dst, RTAX_INITCWND);
        set_dst_metric_cwnd(dst, RTAX_INITCWND, 
                max_t ( __u32, sysctl_tcp_initial_cwnd, cwnd ) );
        cwnd = dst_metric(dst, RTAX_CWND);
        set_dst_metric_cwnd(dst, RTAX_CWND, 
                max_t( __u32, sysctl_tcp_initial_cwnd, cwnd) );
        set_dst_metric_cwnd(dst, RTAX_SSTHRESH, 
                max_t( __u32, sysctl_tcp_initial_cwnd, cwnd) << 1 ) ; 

        tp->snd_cwnd_clamp = max_t(__u32, sysctl_tcp_cwnd_clamp, tp->snd_cwnd_clamp);


        if (dst_metric(dst, RTAX_RTO_MIN) != sysctl_tcp_min_rto) {

            sysctl_tcp_min_rto = 
                jiffies_to_msecs(msecs_to_jiffies(sysctl_tcp_min_rto));
            set_dst_metric_rtt(dst, RTAX_RTO_MIN, 
                    msecs_to_jiffies(sysctl_tcp_min_rto));

            dst->metrics[RTAX_LOCK-1] |= (1<<RTAX_RTO_MIN);

            tp->rttvar = dst_metric(dst, RTAX_RTO_MIN);
        }
    } 
}


static __u32 jtcp_init_cwnd(struct tcp_sock *tp, struct dst_entry *dst) {
    tcp_tune_dst(tp, dst);
    jprobe_return();
    return 0;
}

static struct jprobe tcp_init_cwnd_jprobe = {
    .kp = {
        .symbol_name	= "tcp_init_cwnd",
    },
    .entry	= (kprobe_opcode_t*) jtcp_init_cwnd,
};

static void  jtcp_init_congestion_control (struct sock* sk) {
    struct tcp_sock *tp = tcp_sk(sk);
    struct dst_entry *dst = __sk_dst_get(sk);
    __u32 cwnd;

    if (dst != NULL) {
        tcp_tune_dst(tp, dst);
        cwnd = (dst ? dst_metric(dst, RTAX_INITCWND) : 0);
        tp->snd_cwnd = min_t(__u32, cwnd, tp->snd_cwnd_clamp);
    }
    jprobe_return();
}

static struct jprobe tcp_init_congestion_control_jprobe = {
    .kp = {
        .symbol_name	= "tcp_init_congestion_control",
    },
    .entry	= (kprobe_opcode_t*) jtcp_init_congestion_control,
};

static int jtcp_retransmit_skb(struct sock *sk, struct sk_buff *skb) {
    struct tcp_sock *tp = tcp_sk(sk);
    struct dst_entry *dst = __sk_dst_get(sk);
    __u32 cwnd;

    if (dst != NULL) {
        tcp_tune_dst(tp, dst);
        cwnd = (dst ? dst_metric(dst, RTAX_INITCWND) : 0);
        tp->snd_cwnd = min_t(__u32, cwnd, tp->snd_cwnd_clamp);
    }

    jprobe_return();
    return 0;
}

static struct jprobe tcp_retransmit_skb_jprobe = {
    .kp = {
        .symbol_name	= "tcp_retransmit_skb",
    },
    .entry	= (kprobe_opcode_t*) jtcp_retransmit_skb,
};


static struct ctl_table_header * hdr;

static __init int tcptune_init(void) {
    int ret = register_jprobe(&tcp_init_cwnd_jprobe);
    ret = register_jprobe(&tcp_init_congestion_control_jprobe);
    ret = register_jprobe(&tcp_retransmit_skb_jprobe);

    if (ret) {
        goto err;
    }

#if TUNE_COMPAT > 18
    hdr = register_sysctl_paths(net_ipv4_ctl_path, tcp_tune_table);
#else
    hdr = register_sysctl_table(net_root_table, 0);
#endif
    if (hdr == NULL) {
        goto err;
    }

    pr_info("TCP Tune registered\n"); 
    return 0;

err:
    pr_info("TCP Tune cannot be registered\n");
    return -ENOMEM;
}
module_init(tcptune_init);

static __exit void tcptune_exit(void) {
    unregister_jprobe(&tcp_init_cwnd_jprobe);
    unregister_jprobe(&tcp_init_congestion_control_jprobe);
    unregister_jprobe(&tcp_retransmit_skb_jprobe);
    unregister_sysctl_table(hdr);
}
module_exit(tcptune_exit);
