/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/proc_fs.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_tree_file;

/* macros based on the next_task macro */
#define child_task(p)	list_entry((p)->children.next, struct task_struct, children)


/* Output functions */
char *proc_tree_output = NULL;
//char proc_tree_output[PROC_TREE_OUTPUT_SIZE];
int proc_tree_output_size = 0;
int proc_tree_output_size_used = 1;


void proc_tree_clear_output(void) {
	if (proc_tree_output != NULL) {
		kfree(proc_tree_output);
	}
	proc_tree_output_size = 1024;
	proc_tree_output = kmalloc(proc_tree_output_size, GFP_KERNEL);
	proc_tree_output_size_used = 1;
}

/**
 * Append a string to the module's output buffer.  Increase the size if it
 * isn't big enough.
 * @string
 */
void proc_tree_add_output(const char *string) {
	char *new_output;
	int len = strlen(string);

	if (len + proc_tree_output_size_used >= proc_tree_output_size) {
		// buffer wasn't big enough, double it it.
		proc_tree_output_size *= 2;
		new_output = krealloc(proc_tree_output, proc_tree_output_size, GFP_KERNEL);
		if (new_output == NULL) {
			printk("Failed to allocate memory for proc_tree output.\n");
			return;
		}
		proc_tree_output = new_output;
	}

	strcat(proc_tree_output, string);
	proc_tree_output_size_used += len;
}

/**
 * Generate a string from the task info.  Prepend something looking like
 * a tree based on how far down in the heirarchy it is.
 * @task: process to describe.
 * @depth: how far down in the heirarchy it is.
 */
void proc_tree_format_output(struct task_struct *task, int depth) {
	int n;
	char buffer[1024];
	snprintf(buffer, 1023, "[%d] %s\n", task->pid, task->comm);

	// create tree structure based on depth
	for (n = 0; n < depth; n++) {
		proc_tree_add_output("|- ");
	}

	proc_tree_add_output(buffer);
}


/* structures and functions to hold a heirarchical task list */

/** 
 * Heirarchical process list.  New entries are added to the end of the
 * sibling list at the appropriate depth (under the appropriate parent).
 */
struct proc_tree_list_struct {
	struct task_struct *task;
	struct proc_tree_list_struct *first_child;
	struct proc_tree_list_struct *sibling;
	struct proc_tree_list_struct *parent;
};

/** 
 * A linear list pointing at the heirarchical list, for easy access.  New
 * entries are added to the beginning of this list.  Nothing fancy here.
 */
struct proc_tree_list_seq_struct {
	struct task_struct *task;
	struct proc_tree_list_struct *entry;
	struct proc_tree_list_seq_struct *next;
};


static struct proc_tree_list_struct *proc_tree_list = NULL;
static struct proc_tree_list_seq_struct *proc_tree_list_seq = NULL;

/**
 * Find the entry for the specified parent task from the heirarchical list.
 * @parent: the parent task
 */
struct proc_tree_list_struct *proc_tree_find_parent(struct task_struct *parent) {
	struct proc_tree_list_seq_struct *seq = proc_tree_list_seq;
	struct proc_tree_list_struct *found = NULL;

	// walk the sequential list to find the parent list entry.
	while (seq != NULL) {
		if (seq->task == parent) {
			found = seq->entry;
			break;
		}
		seq = seq->next;
	}

	return found;
}

/**
 * Add a new entry into both the heirarchical and sequential lists.
 * @task: the new task to create entries for.
 */
void proc_tree_add_entry(struct task_struct *task) {
	struct proc_tree_list_struct *parent_entry, *sibling_entry, *new_entry;
	struct proc_tree_list_seq_struct *seq;
	struct task_struct *parent;

	parent = task->real_parent;

	if (parent == NULL) {
		parent_entry = NULL;
	}
	else {
		parent_entry = proc_tree_find_parent(parent);
	}

	new_entry = kmalloc(sizeof(struct proc_tree_list_struct), GFP_KERNEL);
	new_entry->task = task;
	new_entry->parent = parent_entry;
	new_entry->first_child = NULL;
	new_entry->sibling = NULL;

	seq = kmalloc(sizeof(struct proc_tree_list_seq_struct), GFP_KERNEL);
	seq->task = task;
	seq->entry = new_entry;

	if (proc_tree_list == NULL) {
		// first entry
		proc_tree_list = new_entry;
		seq->next = NULL;
		proc_tree_list_seq = seq;
	}
	else {
		if (parent_entry == NULL) {
			// no parent
			sibling_entry = proc_tree_list;
			while (sibling_entry->sibling != NULL) {
				sibling_entry = sibling_entry->sibling;
			}
			sibling_entry->sibling = new_entry;
		}
		else if (parent_entry->first_child == NULL) {
			// parent has no children
			parent_entry->first_child = new_entry;
		}
		else {
			// parent exists, has children
			sibling_entry = parent_entry->first_child;
			while (sibling_entry->sibling != NULL) {
				sibling_entry = sibling_entry->sibling;
			}
			sibling_entry->sibling = new_entry;
		}
		// add to proc list seq
		seq->next = proc_tree_list_seq;
		proc_tree_list_seq = seq;
	}
}

/**
 * Clear the stored list of processes, both the heirarchical list and the
 * sequential list.
 */
void proc_tree_clear_list(void) {
	struct proc_tree_list_struct *entry;
	struct proc_tree_list_seq_struct *seq, *next;

	seq = proc_tree_list_seq;
	while (seq != NULL) {
		entry = seq->entry;
		kfree(entry);
		next = seq->next;
		kfree(seq);
		seq = next;
	}

	proc_tree_list = NULL;
	proc_tree_list_seq = NULL;
}

/**
 * Go through the heirarchical process list, starting at the beginning.
 * For each process, get info about that process, then the process's children,
 * then the process's siblings, then back to the parent if it exists.
 */
void proc_tree_walk_list(void) {
	struct proc_tree_list_struct *entry = proc_tree_list;
	struct task_struct *task;
	int depth = 0;

	while (entry != NULL) {
		task = entry->task;
		//printk("(%d) %d: %s\n", depth, task->pid, task->comm);
		proc_tree_format_output(task, depth);
		if (entry->first_child != NULL) {
			// process children.
			depth++;
			entry = entry->first_child;
		}
		else if (entry->sibling != NULL) {
			// process next sibling.
			entry = entry->sibling;
		}
		else if (entry->parent != NULL) {
			// we've already done the immediate parent.
			// get the parent's sibling.
			depth--;
			entry = entry->parent->sibling;
		}
		else {
			// top level last process may have neither sibling
			// nor parent.
			entry = NULL;
		}
	}
}



int proc_tree_read(char *buffer, char **buffer_location, off_t offset, 
		int buffer_length, int *eof, void *data)
{
	struct task_struct *task;
	int ret;

	if (offset > 0) {
		const char *output = proc_tree_output + offset;

		strncpy(buffer, output, buffer_length - 1);
		ret = strlen(buffer);

		if (ret == 0) {
			/* we have finished reading, clear output */
			proc_tree_clear_output();
		}
	}
	else {
		// go through all processes, store in the heirarchical struct.
		for_each_process(task) {
			proc_tree_add_entry(task);
		}
		proc_tree_walk_list();
		proc_tree_clear_list();

		strncpy(buffer, proc_tree_output, buffer_length - 1);
		ret = strlen(buffer);
	}

	return ret;
}


/* startup and shutdown */
int proc_tree_init(void)
{
	proc_tree_file = create_proc_entry("tree", 0644, NULL);
	proc_tree_file->read_proc = proc_tree_read;

	proc_tree_clear_output();
	return 0;
}
module_init(proc_tree_init);

void proc_tree_cleanup(void)
{
	remove_proc_entry("tree", NULL);
}
module_exit(proc_tree_cleanup);

