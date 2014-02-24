#ifndef TUNE_VERSION
#define TUNE_VERSION

#include <linux/version.h>
#include <linux/kernel.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
#define TUNE_COMPAT 18
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
#define TUNE_COMPAT 32
#else
#define TUNE_COMPAT 35
#endif

#endif

