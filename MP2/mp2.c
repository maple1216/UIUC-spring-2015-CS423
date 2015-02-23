#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "mp2_given.h"
#include "mp1_given.h"

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/jiffies.h>

#include <linux/sched.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("16");
MODULE_DESCRIPTION("CS-423 MP2");
#define DEBUG 1
#define FILENAME "status"
#define DIRECTORY "mp2"
#define TIMEINTERVAL 5000

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


typedef struct 
{
    struct list_head list;
    unsigned int pid;
    unsigned int period;
    unsigned int computation;
    struct timer_list wakeup_timer;

    //task_struct contains state, pid list_head structure
    struct task_struct* linux_task; 

    unsigned int task_state;
    uint64_t next_period;
        

} task_struct_list;


static struct proc_dir_entry *proc_dir, *proc_entry;
LIST_HEAD(mp2_task_struct_list);

static struct timer_list mp2_timer;
static struct workqueue_struct *mp2_workqueue;
static spinlock_t mp2_lock;
static struct work_struct *mp2_work;

static ssize_t mp2_read(struct file *file, char __user *buffer, size_t count, loff_t *data)
{
    unsigned long copied = 0;
    char *buf;
    task_struct_list *tmp;
    unsigned long flags;

    buf = (char *)kmalloc(count, GFP_KERNEL);

    // enter critical section
    spin_lock_irqsave(&mp2_lock, flags);
    list_for_each_entry(tmp, &mp2_task_struct_list, list) {
        copied += sprintf(buf + copied, "%u, %u, %u\n", tmp->pid, tmp->period, tmp->computation);
    }
    spin_unlock_irqrestore(&mp2_lock, flags);

    buf[copied] = '\0';

    copy_to_user(buffer, buf, copied);

    kfree(buf);

    return copied;
}

static ssize_t mp2_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
    task_struct_list *tmp;
    unsigned long flags;
    unsigned long d_pid;
    char *buf, *opt;
    task_struct_list *pos, *n;


    // initialize tmp->list
    tmp = (task_struct_list *)kmalloc(sizeof(task_struct_list), GFP_KERNEL);
    INIT_LIST_HEAD(&(tmp->list));

    // set tmp->pid
    buf = (char *)kmalloc(count + 1, GFP_KERNEL);
    copy_from_user(buf, buffer, count);
    buf[count] = '\0';

    printk(KERN_INFO "===================\n");
    // parse operation character
    opt = strsep(&buf, ",");

    switch (my_mapping(opt)) {
        case REGISTRATION:
            printk("REGISTRATION\n");
            sscanf(buf, "%u,%u,%u", &tmp->pid, &tmp->period, &tmp->computation);

            //create admission controll function, reject process which make utilization > 0.693

            spin_lock_irqsave(&mp2_lock, flags);
            list_add(&(tmp->list), &mp2_task_struct_list);
            spin_unlock_irqrestore(&mp2_lock, flags);

            //if has higher priority -> interrupt
            break;
        case YIELD:
            printk("YIELD\n");
            //do yield
            break;
        case DEREGISTRATION:
            printk("DEREGISTRATION\n");
            sscanf(buf, "%u", &d_pid);
            printk(KERN_INFO "try to allocate pid:%u\n", d_pid);
            list_for_each_entry_safe(pos, n, &mp2_task_struct_list, list) {
                if(pos->pid == d_pid){
                    list_del(&pos->list);
                    kfree(pos);
                    printk(KERN_INFO "pid:%u memory freed!\n", pos->pid);
                }
            }

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

void mp2_timer_callback(unsigned long data)
{
    //timer will wake up at the start of each period
    queue_work(mp2_workqueue, mp2_work);
}

static void mp2_work_function(struct work_struct *work)
{
    //use scheduler api to do context switch
    //usage: set_current_state() -> schedule()
    //use two-half, context switch(schedule() should be called in bottom half)
    //mod_timer for another period
}

// mp2_init - Called when module is loaded
static int __init mp2_init(void)
{
    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE LOADING\n");
    #endif

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


    
    // initialize and start timer
    setup_timer(&mp2_timer, mp2_timer_callback, 0);
    //set mod_timer here 
    
    // initialize workqueue
    if ((mp2_workqueue = create_workqueue("mp2_workqueue")) == NULL) {
        printk(KERN_INFO "create_workqueue ERROR\n");
        return -ENOMEM;
    }

    // initialize work
    mp2_work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    INIT_WORK(mp2_work, mp2_work_function);
    
    // initialize lock
    spin_lock_init(&mp2_lock);
    
    printk(KERN_ALERT "MP2 MODULE LOADED\n");
    return 0;
}

// mp2_exit - Called when module is unloaded
static void __exit mp2_exit(void)
{
    task_struct_list *pos, *n;

    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
    #endif

    // remove /proc/mp2/status
    remove_proc_entry(FILENAME, proc_dir);
    // remove /proc/mp2
    remove_proc_entry(DIRECTORY, NULL);

    
    // free timer
    del_timer_sync(&mp2_timer);

    // free mp2_proc_list
    list_for_each_entry_safe(pos, n, &mp2_task_struct_list, list) {
        list_del(&pos->list);
        kfree(pos);
    }

    // free workqueue
    flush_workqueue(mp2_workqueue);
    destroy_workqueue(mp2_workqueue);

    // free work
    kfree(mp2_work);
    
    printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);
