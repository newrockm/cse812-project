/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_tree_file;

int proc_tree_read(char *buffer, char **buffer_location, off_t offset, 
		int buffer_length, int *eof, void *data)
{
	int ret;

	if (offset > 0) {
		/* we have finished reading, return 0 */
		ret = 0;
	}
	else {
		ret = sprintf(buffer, "Welcome to the proc_tree module!\n");
	}

	return ret;
}


/* startup and shutdown */
int proc_tree_init(void)
{
	proc_tree_file = create_proc_entry("tree", 0644, NULL);

	proc_tree_file->read_proc = proc_tree_read;

	return 0;
}
module_init(proc_tree_init);

void proc_tree_cleanup(void)
{
	remove_proc_entry("tree", NULL);
}
module_exit(proc_tree_cleanup);

