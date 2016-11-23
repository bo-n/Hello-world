#include "adder.h"
#include "xadder_hw.h"
#include <linux/init.h>
#include <linux/kernel.h>/* We're doing kernel work*/
#include <linux/module.h>/* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>/*for get_user and put_user*/
#include <asm/io.h>
