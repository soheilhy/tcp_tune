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

#include <net/tcp.h>

//#define TCP_TUNE_DEBUG

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
#define TUNE_COMPAT 18
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
#define TUNE_COMPAT 32
#else
#define TUNE_COMPAT 35
#endif

MODULE_AUTHOR("Soheil Hassas Yeganeh <soheil@cs.toronto.edu>");
MODULE_DESCRIPTION("TCP parameter tuner");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1-ALPHA");
/*
static int port __read_mostly = 0;
MODULE_PARM_DESC(port, "Port to match (0=all)");
module_param(port, int, 0);

static unsigned int bufsize __read_mostly = 4096;
MODULE_PARM_DESC(bufsize, "Log buffer size in packets (4096)");
module_param(bufsize, uint, 0);

static int bucket_length __read_mostly = 1;
MODULE_PARM_DESC(bucket_length, "Length of each bucket in the histogram (1) except the last bucket length is not bounded.");
module_param(bucket_length, int, 0);

static int number_of_buckets  __read_mostly = 1;
MODULE_PARM_DESC(number_of_buckets, "Number of buckets in the histogram (1)");
module_param(number_of_buckets, int, 0);

static int live __read_mostly = 0;
MODULE_PARM_DESC(live, "(0) stats of completed flows are printed, (1) stats of live flows are printed.");
module_param(live, int, 0);

static inline struct timespec get_time(void) {
    struct timespec ts;
    ktime_get_ts(&ts);
    return ts;
}

static int funct() {
ret:
    jprobe_return();
    return 0;
}

static struct jprobe tcp_recv_jprobe = {
    .kp = {
        .symbol_name	= "tcp_v4_do_rcv",
    },
    .entry	= (kprobe_opcode_t*) jtcp_v4_do_rcv,
};

static inline struct timespec tcpprobe_timespec_sub
                                (struct timespec lhs, struct timespec rhs) {
    struct timespec tv;
    tv.tv_sec = lhs.tv_sec - rhs.tv_sec;
    tv.tv_nsec = lhs.tv_nsec - rhs.tv_sec;
    return tv;
}

static inline int tcpprobe_timespec_larger( struct timespec lhs, 
                                            struct timespec rhs) {
    int ret = lhs.tv_sec > rhs.tv_sec || 
        (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec > rhs.tv_nsec);
    return ret;
}


#define EXPIRE_TIMEOUT (jiffies + HZ )
#define EXPIRE_SKB (2*60)
static void prune_timer(unsigned long data) {
    int i;
    struct timespec now = get_time(); 
    spin_lock_bh(&tcp_flow_spy.lock);
    for (i = 0; i < HASHTABLE_SIZE; i++) {
        struct hashtable_entry* entry = &tcp_flow_hashtable.entries[i];
        struct tcp_flow_log* log = 0;

    	if (unlikely(!entry)) {
	        continue;
	    }

        log = entry->head;
        while (log) {
            struct timespec interval
                = tcpprobe_timespec_sub(now, log->last_packet_tstamp);
            struct tcp_flow_log* nextLog = log->next;
            if (interval.tv_sec > EXPIRE_SKB) {
                remove_from_hashentry(entry, log); 
                log->next = tcp_flow_spy.finished;
                tcp_flow_spy.finished = log;
            }
            log = nextLog;
        }
    }

    tcp_flow_spy.timer.expires = EXPIRE_TIMEOUT;
    spin_unlock_bh(&tcp_flow_spy.lock);
    add_timer(&tcp_flow_spy.timer);
}

*/

int sysctl_tcp_initial_cwnd __read_mostly = 3;
int sysctl_tcp_cwnd_clamp __read_mostly = 64;
int sysctl_tcp_max_rto __read_mostly = 200000;
int sysctl_tcp_random_rto __read_mostly = 0;
int sysctl_tcp_min_rto __read_mostly = 20;

int min_cwnd = 3;
static struct ctl_table ipv4_table[] = {
	{
		.procname	= "tcp_initial_cwnd",
		.data		= &sysctl_tcp_initial_cwnd,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
        .extra2     = &sysctl_tcp_cwnd_clamp,
        .extra1     = &min_cwnd
	},
    {
        .procname   = "tcp_cwnd_clamp",
        .data       = &sysctl_tcp_cwnd_clamp,
        .maxlen     = sizeof(int),
        .mode       = 0644,
    	.proc_handler	= proc_dointvec,
    },
    {
        .procname   = "tcp_min_rto",
        .data       = &sysctl_tcp_min_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
    	.proc_handler	= proc_dointvec,
    },
    {
        .procname   = "tcp_max_rto",
        .data       = &sysctl_tcp_max_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
    	.proc_handler	= proc_dointvec,
    },
    {
        .procname   = "tcp_random_rto",
        .data       = &sysctl_tcp_random_rto,
        .maxlen     = sizeof(int),
        .mode       = 0644,
    	.proc_handler	= proc_dointvec,
    },
    { }
};

static inline void set_dst_metric_cwnd(struct dst_entry *dst, int metric, __u32 cwnd) {
    if (!dst_metric_locked(dst, metric)) {
        dst->metrics[metric-1] = cwnd;
    }
}

static __u32 jtcp_init_cwnd(struct tcp_sock *tp, struct dst_entry *dst) {
    tp->snd_cwnd_clamp = sysctl_tcp_cwnd_clamp;
    
    if (dst != NULL) {
        __u32 cwnd = dst_metric(dst, RTAX_INITCWND);
        set_dst_metric_cwnd(dst, RTAX_INITCWND, 
                max_t ( __u32, sysctl_tcp_initial_cwnd, cwnd ) );
        cwnd = dst_metric(dst, RTAX_CWND);
        set_dst_metric_cwnd(dst, RTAX_CWND, 
                max_t( __u32, sysctl_tcp_initial_cwnd, cwnd) );
        set_dst_metric_cwnd(dst, RTAX_SSTHRESH, 
                max_t( __u32, sysctl_tcp_initial_cwnd, cwnd) << 1 ) ; 

        if (dst_metric(dst, RTAX_RTO_MIN) != sysctl_tcp_min_rto) {
            
            sysctl_tcp_min_rto = 
                jiffies_to_msecs(msecs_to_jiffies(sysctl_tcp_min_rto));
            set_dst_metric_rtt(dst, RTAX_RTO_MIN, 
                    msecs_to_jiffies(sysctl_tcp_min_rto));
            
            dst->metrics[RTAX_LOCK-1] |= (1<<RTAX_RTO_MIN);

            tp->rttvar = dst_metric(dst, RTAX_RTO_MIN);
        }
    } 

    jprobe_return();
    return 0;
}

static struct jprobe tcp_init_cwnd_jprobe = {
    .kp = {
        .symbol_name	= "tcp_init_cwnd",
    },
    .entry	= (kprobe_opcode_t*) jtcp_init_cwnd,
};

static struct ctl_table_header * hdr;

static __init int tcptune_init(void) {

    int ret = register_jprobe(&tcp_init_cwnd_jprobe);
    if (ret) {
        goto err;
    }

//    add_timer(&tcp_flow_spy.timer);*/  

    hdr = register_sysctl_paths(net_ipv4_ctl_path, ipv4_table);
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
    unregister_sysctl_table(hdr);
/*    int i = 0;
    if (timer_pending(&tcp_flow_spy.timer)) {
        del_timer(&tcp_flow_spy.timer);
    }

    proc_net_remove(
#if SPY_COMPAT >= 32
            &init_net,
#endif
            procname);
    unregister_jprobe(&tcp_transmit_jprobe);

    for (i = 0; i < bufsize; i++) {
        kfree(tcp_flow_spy.storage[i].snd_cwnd_histogram);
    }
    kfree(tcp_flow_spy.available);*/
}
module_exit(tcptune_exit);
