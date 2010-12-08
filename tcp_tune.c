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
#define CONFIG_DEFAULT_TCP_CONG "bic"
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
int min_cwnd = 2;

struct tcp_tune {
    u32                         icsk_ca_priv[16];
};

char default_congestion_control[TCP_CA_NAME_MAX];
static struct tcp_congestion_ops* selected_congestion_control;

static struct list_head* tcp_cong_list;

static struct tcp_congestion_ops *tcp_ca_find(const char *name)
{
	struct tcp_congestion_ops *e;

    rcu_read_lock();
	list_for_each_entry_rcu(e, tcp_cong_list, list) {
		if (strcmp(e->name, name) == 0)
			return e;
	}
    rcu_read_unlock();

	return NULL;
}

void tcp_get_default_congestion_control(char *name)
{
	struct tcp_congestion_ops *ca;
	/* We will always have reno... */
	BUG_ON(list_empty(tcp_cong_list));

	rcu_read_lock();
	ca = list_entry(tcp_cong_list->next, struct tcp_congestion_ops, list);
	strncpy(name, ca->name, TCP_CA_NAME_MAX);
	rcu_read_unlock();
}

#if TUNE_COMPAT < 19
static int proc_tcp_tune_congestion_control(ctl_table *ctl, int write,
                            struct file* filep,
                            void __user *buffer, size_t *lenp, loff_t *ppos) {
#else
static int proc_tcp_tune_congestion_control(ctl_table *ctl, int write,
                            struct file* filep,
                            void __user *buffer, size_t *lenp, loff_t *ppos) {
#endif
	ctl_table tbl = {
		.data = default_congestion_control,
		.maxlen = TCP_CA_NAME_MAX,
	};
	int ret;

//	tcp_get_default_congestion_control(default_congestion_control);
#if TUNE_COMPAT < 19
	ret = proc_dostring(&tbl, write, filep, buffer, lenp, ppos);
#else
	ret = proc_dostring(&tbl, write, buffer, lenp, ppos);
#endif
	if (write && ret == 0) {
        struct tcp_congestion_ops* ca;
        int ret = -ENOENT;
        // There is no way to lock the list! :(
        ca = tcp_ca_find(tbl.data);

        if (ca) {
            selected_congestion_control = ca;
            ret = 0;
        }
    }
    return ret;
}


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
        CTL_UNNAME
        .procname   = "tcp_tune_cc",
        .maxlen     = TCP_CA_NAME_MAX,
        .mode       = 0644,
        .proc_handler = proc_tcp_tune_congestion_control,
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

#if TUNE_COMPAT < 19
static inline void set_dst_metric_rtt(struct dst_entry *dst, int metric,
                              unsigned long rtt) {
        dst->metrics[metric-1] = jiffies_to_msecs(rtt);
}
#endif 

static void tcp_tune_dst(struct tcp_sock *tp, struct dst_entry *dst) {
    if (dst != NULL) {
        __u32 cwnd = dst_metric(dst, RTAX_INITCWND);
        set_dst_metric_cwnd(dst, RTAX_INITCWND, sysctl_tcp_initial_cwnd);
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

static inline void tcp_tune_tuneit(struct sock *sk) { 
    struct tcp_sock *tp = tcp_sk(sk);
    struct dst_entry *dst = __sk_dst_get(sk);

    if (dst != NULL) {
        tcp_tune_dst(tp, dst);
    }

    tp->snd_cwnd = max_t(__u32, sysctl_tcp_initial_cwnd, tp->snd_cwnd);
    tp->snd_cwnd = min_t(__u32, tp->snd_cwnd, tp->snd_cwnd_clamp);
    tp->snd_ssthresh = max_t(__u32, sysctl_tcp_initial_cwnd, tp->snd_ssthresh);
}

#define TUNE_TCP_CA(sk)  (selected_congestion_control)

static void tcp_tune_init(struct sock *sk) {
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
        TUNE_TCP_CA(sk)->init(sk);
    }
    tcp_tune_tuneit(sk);
}

static u32 tcp_tune_recalc_ssthresh(struct sock *sk) {
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
        u32 ssthresh = TUNE_TCP_CA(sk)->ssthresh(sk);
        tcp_tune_tuneit(sk);
        return max_t(u32, ssthresh, sysctl_tcp_initial_cwnd);
    }
    return sysctl_tcp_initial_cwnd;
}

#if TUNE_COMPAT < 19
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 rtt, u32 in_flight, int flag)
#else
static void tcp_tune_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
#endif
{
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
#if TUNE_COMPAT < 19
        TUNE_TCP_CA(sk)->cong_avoid(sk, ack, rtt, in_flight, flag);
#else
        TUNE_TCP_CA(sk)->cong_avoid(sk, ack, in_flight);
#endif 

    }
    tcp_tune_tuneit(sk); 
}

static void tcp_tune_state(struct sock *sk, u8 new_state) {
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
        TUNE_TCP_CA(sk)->set_state(sk, new_state);
    }
    tcp_tune_tuneit(sk);
}

static u32 tcp_tune_undo_cwnd(struct sock *sk) {
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
        u32 cwnd = TUNE_TCP_CA(sk)->undo_cwnd(sk);
        return max_t(u32, sysctl_tcp_initial_cwnd, cwnd);
    }
    return sysctl_tcp_initial_cwnd;
}
#if TUNE_COMPAT < 19
static void tcp_tune_acked(struct sock *sk, u32 cnt)
#else
static void tcp_tune_acked(struct sock *sk, u32 cnt, s32 rtt_us)
#endif
{
    BUG_ON(TUNE_TCP_CA(sk) == NULL);
    if (likely(TUNE_TCP_CA(sk))) {
#if TUNE_COMPAT < 19
        TUNE_TCP_CA(sk)->pkts_acked(sk, cnt);
#else
        TUNE_TCP_CA(sk)->pkts_acked(sk, cnt, rtt_us);
#endif
    }
    tcp_tune_tuneit(sk);
}

static struct tcp_congestion_ops tcptune = {
	.init		= tcp_tune_init,
	.ssthresh	= tcp_tune_recalc_ssthresh,
	.cong_avoid	= tcp_tune_cong_avoid,
	.set_state	= tcp_tune_state,
	.undo_cwnd	= tcp_tune_undo_cwnd,
	.pkts_acked     = tcp_tune_acked,
	.owner		= THIS_MODULE,
	.name		= "tune",
};

static struct ctl_table_header * hdr;


static __init int tcptune_module_init(void) {
    int error = 0;
    BUILD_BUG_ON(sizeof(struct tcp_tune) > ICSK_CA_PRIV_SIZE);


#if TUNE_COMPAT > 18
    hdr = register_sysctl_paths(net_ipv4_ctl_path, tcp_tune_table);
#else
    hdr = register_sysctl_table(net_root_table, 0);
#endif

    if (hdr == NULL) {
        error = -ENOMEM;
        goto err;
    }
#if TUNE_COMPAT > 18
    tcptune.flags |= TCP_CONG_NON_RESTRICTED;
#endif
    error = tcp_register_congestion_control(&tcptune);
    if (error) {
        goto err; 
    }
#if TUNE_COMPAT > 18
    tcp_cong_list = (struct list_head*) &tcptune.list.next->next;
#else
    tcp_cong_list = (struct list_head*) &tcptune.list.prev;
#endif

    selected_congestion_control = tcp_ca_find(CONFIG_DEFAULT_TCP_CONG);

    pr_info("TCP Tune registeration finalized \n"); 
    return 0;

err:
    pr_info("TCP Tune cannot be registered\n");
    return error;
}
module_init(tcptune_module_init);

static __exit void tcptune_module_exit(void) {
    unregister_sysctl_table(hdr);
    tcp_unregister_congestion_control(&tcptune);
    pr_info("TCP Tune unregistered\n");
}
module_exit(tcptune_module_exit);

