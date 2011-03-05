#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x44cf4255, "module_layout" },
	{ 0x99ede469, "proc_dointvec_minmax" },
	{ 0xa0ae432b, "proc_dointvec" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0xea147363, "printk" },
	{ 0xef5e8c32, "tcp_register_congestion_control" },
	{ 0x94d32a88, "__tracepoint_module_get" },
	{ 0x7ec9bfbc, "strncpy" },
	{ 0xb4390f9a, "mcount" },
	{ 0xa9f3f261, "net_ipv4_ctl_path" },
	{ 0xb7becdb3, "proc_dostring" },
	{ 0xebe38de8, "module_put" },
	{ 0x86d5a940, "tcp_unregister_congestion_control" },
	{ 0x35c31276, "unregister_sysctl_table" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x31a5a89b, "register_sysctl_paths" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "A4BAE2B372ABD0B4621D517");
