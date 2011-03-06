
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


