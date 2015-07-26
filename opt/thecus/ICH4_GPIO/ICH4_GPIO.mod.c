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
	{ 0xa6942b40, "module_layout" },
	{ 0xfd6293c2, "boot_tvec_bases" },
	{ 0x427220ad, "single_release" },
	{ 0xea975a33, "seq_read" },
	{ 0xbdebc234, "seq_lseek" },
	{ 0x3980aac1, "unregister_reboot_notifier" },
	{ 0x9bce482f, "__release_region" },
	{ 0x1cc6719a, "register_reboot_notifier" },
	{ 0xadf42bd5, "__request_region" },
	{ 0x59d8223a, "ioport_resource" },
	{ 0x53200e73, "pci_bus_write_config_word" },
	{ 0xd6321896, "pci_bus_write_config_byte" },
	{ 0x931f9ad2, "pci_get_device" },
	{ 0xb6c6e9f1, "pci_match_id" },
	{ 0x467f2dce, "remove_proc_entry" },
	{ 0xf6288e02, "__init_waitqueue_head" },
	{ 0x88a193, "__alloc_workqueue_key" },
	{ 0x8b4ac869, "proc_create_data" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x50eedeb8, "printk" },
	{ 0x42224298, "sscanf" },
	{ 0x6c2e3320, "strncmp" },
	{ 0xd0d8621b, "strlen" },
	{ 0x33d169c9, "_copy_from_user" },
	{ 0x93fca811, "__get_free_pages" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x4abdb61c, "seq_printf" },
	{ 0x47c149ab, "queue_delayed_work" },
	{ 0xb9e52429, "__wake_up" },
	{ 0x91715312, "sprintf" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x3d0ab305, "single_open" },
	{ 0xfac7354c, "pci_bus_read_config_byte" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v00008086d000024C0sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00008086d000024CCsv*sd*bc*sc*i*");
