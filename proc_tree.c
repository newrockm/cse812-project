/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");

struct proc_dir_entry *proc_tree_root;


/* directory read */
int proc_tree_readdir(struct file * filp, void * dirent, filldir_t filldir)
{
	int i;
	struct dentry *dentry = filp->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	ino_t ino;
	int ret;

	struct timespec uptime;
	char uptime_fname[16];
	int len;

	i = filp->f_pos;
	switch (i){
	case 0:
		ino = inode->i_ino;
		if (filldir(dirent, ".", 1, i, ino, DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
		/* fall through */
	case 1:
		ino = parent_ino(dentry);
		if (filldir(dirent, "..", 2, i, ino, DT_DIR) < 0)
			goto out;
		i++;
		filp->f_pos++;
		/* fall through */
	case 2:
		uptime = current_kernel_time();
		snprintf(uptime_fname, 15, "%d", (int)uptime.tv_sec);
		len = strlen(uptime_fname);

		ino = iunique(inode->i_sb, 2);
		if (filldir(dirent, uptime_fname, len, i, ino, DT_REG) < 0)
			goto out;
		i++;
		filp->f_pos++;
		break;

	}

	ret = 1;
out:
	return ret;
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

