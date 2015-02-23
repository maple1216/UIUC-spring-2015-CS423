#define LINUX

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

#include "mp1_given.h"
/*#include "mp2_given.h"*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("16");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG            1
#define FILENAME         "status"
#define DIRECTORY        "mp2"
#define REGISTRATION     1
#define YIELD            2
#define DEREGISTRATION   3

struct str2num {
    char *str;
    int num;
};

const struct str2num mapping[] = {
    { "R", 1 },
    { "Y", 2 },
    { "D", 3 },
    { NULL, 0 }
};

static struct proc_dir_entry *proc_dir, *proc_entry;

static ssize_t mp2_read(struct file *file, char __user *buffer, size_t count, loff_t *data)
{
    unsigned long copied = 0;
    char *buf;

    buf = (char *)kmalloc(count, GFP_KERNEL);
    copied += sprintf(buf, "Hello World!\n");
    buf[copied] = '\0';

    copy_to_user(buffer, buf, copied);

    kfree(buf);
    return copied;
}

int my_mapping(char *str)
{
    int i;
    for (i = 0; i < mapping[i].str != NULL; i ++) {
        if (strcmp(str, mapping[i].str) == 0) {
            return mapping[i].num;
        }
    }
    return 0;
}

static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    char *buf, *opt;

    buf = (char *)kmalloc(count + 1, GFP_KERNEL);
    copy_from_user(buf, buffer, count);
    buf[count] = '\0';

    printk(KERN_INFO "===================\n");
    // parse operation character
    opt = strsep(&buf, ",");

    switch (my_mapping(opt)) {
        case REGISTRATION:
            printk("REGISTRATION\n");
            break;
        case YIELD:
            printk("YIELD\n");
            break;
        case DEREGISTRATION:
            printk("DEREGISTRATION\n");
            break;
        default:
            printk(KERN_INFO "error in my_mapping(opt)\n");
    }

    printk(KERN_INFO "===================\n");

    kfree(buf);

    return count;
}

static const struct file_operations mp2_fops = {
    .owner   = THIS_MODULE,
    .read    = mp2_read,
    .write   = mp2_write
};

// mp2_init - Called when module is loaded
static int __init mp2_init(void)
{
    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE LOADING\n");
    #endif
    // Insert your code here ...

    // create /proc/mp2
    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_INFO "proc_mkdir ERROR\n");
        return -ENOMEM;
    }
    // create /proc/mp2/status
    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &mp2_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_INFO "proc_create ERROR\n");
        return -ENOMEM;
    }

    printk(KERN_ALERT "MP2 MODULE LOADED\n");
    return 0;
}

// mp1_exit - Called when module is unloaded
static void __exit mp2_exit(void)
{
    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
    #endif
    // Insert your code here ...

    // remove /proc/mp1/status
    remove_proc_entry(FILENAME, proc_dir);
    // remove /proc/mp1
    remove_proc_entry(DIRECTORY, NULL);

    printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);
