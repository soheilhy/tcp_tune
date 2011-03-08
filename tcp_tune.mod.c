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
	{ 0x30bc26e3, "module_layout" },
	{ 0x99ede469, "proc_dointvec_minmax" },
	{ 0xcd9c5d72, "kmalloc_caches" },
	{ 0xa0ae432b, "proc_dointvec" },
	{ 0x4aabc7c4, "__tracepoint_kmalloc" },
	{ 0x750a60b1, "proc_net_fops_create" },
	{ 0x2ffd1ef2, "kmem_cache_alloc_notrace" },
	{ 0x11089ac7, "_ctype" },
	{ 0xea147363, "printk" },
	{ 0xa48f2dd5, "tcp_register_congestion_control" },
	{ 0x49c44995, "proc_net_remove" },
	{ 0xb4390f9a, "mcount" },
	{ 0xa228a0cd, "init_net" },
	{ 0xa9f3f261, "net_ipv4_ctl_path" },
	{ 0xbea3055a, "tcp_unregister_congestion_control" },
	{ 0x61431752, "unregister_sysctl_table" },
	{ 0xd0a91bab, "skip_spaces" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xd84de0df, "register_sysctl_paths" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "0447E051211A3F9C21C8F1A");
