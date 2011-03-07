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
	{ 0xf5d6b4d, "kmalloc_caches" },
	{ 0xa0ae432b, "proc_dointvec" },
	{ 0x4aabc7c4, "__tracepoint_kmalloc" },
	{ 0x480bd6bb, "kmem_cache_alloc_notrace" },
	{ 0xea147363, "printk" },
	{ 0xef5e8c32, "tcp_register_congestion_control" },
	{ 0xb4390f9a, "mcount" },
	{ 0xa9f3f261, "net_ipv4_ctl_path" },
	{ 0x86d5a940, "tcp_unregister_congestion_control" },
	{ 0x35c31276, "unregister_sysctl_table" },
	{ 0x31a5a89b, "register_sysctl_paths" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "DE518964E2C716C738C72C5");
