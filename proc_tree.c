/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

struct proc_dir_entry *proc_tree_root;


/* directory read */
int proc_tree_readdir(struct file * filp, void * dirent, filldir_t filldir)
{
	printk(KERN_INFO "Called proc_tree_readdir.\n");
	return 0;
}

static const struct file_operations proc_tree_file_operations = {
	.read		= generic_read_dir,
	.readdir	= proc_tree_readdir,
};


/* startup and shutdown */
int proc_tree_init(void)
{
	proc_tree_root = proc_mkdir("tree", NULL);
	
	proc_tree_root->proc_fops = &proc_tree_file_operations;
	return 0;
}
module_init(proc_tree_init);

void proc_tree_cleanup(void)
{
	remove_proc_entry("tree", NULL);
}
module_exit(proc_tree_cleanup);

