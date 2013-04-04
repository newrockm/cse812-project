/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");


int proc_tree_init(void)
{
	proc_mkdir("tree", NULL);
	return 0;
}
module_init(proc_tree_init);

void proc_tree_cleanup(void)
{
	remove_proc_entry("tree", NULL);
}
module_exit(proc_tree_cleanup);

