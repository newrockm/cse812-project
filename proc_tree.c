/**
 * Group 5 project part 1
 * /proc/tree
 */ 

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

/* macros based on the next_task macro */
#define child_task(p)	list_entry((p)->children.next, struct task_struct, children)

#define PROC_TREE_OUTPUT_SIZE 4096

struct proc_tree_list_struct {
	struct task_struct *task;
	struct proc_tree_list_struct *first_child;
	struct proc_tree_list_struct *sibling;
	struct proc_tree_list_struct *parent;
};

struct proc_tree_list_seq_struct {
	struct task_struct *task;
	struct proc_tree_list_struct *entry;
	struct proc_tree_list_seq_struct *next;
};

static struct proc_tree_list_struct *proc_tree_list = NULL;
static struct proc_tree_list_seq_struct *proc_tree_list_seq = NULL;

static struct proc_dir_entry *proc_tree_file;
/*char *proc_tree_output; */
char proc_tree_output[PROC_TREE_OUTPUT_SIZE];
int proc_tree_output_size_used = 1;

struct proc_tree_list_struct *proc_tree_find_parent(struct task_struct *parent) {
	struct proc_tree_list_seq_struct *seq = proc_tree_list_seq;
	struct proc_tree_list_struct *found = NULL;

	while (seq != NULL) {
		if (seq->task == parent) {
			found = seq->entry;
			break;
		}
		seq = seq->next;
	}

	return found;
}

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
		proc_tree_list = new_entry;
		seq->next = NULL;
		proc_tree_list_seq = seq;
	}
	else {
		// no parent
		if (parent_entry == NULL) {
			sibling_entry = proc_tree_list;
			while (sibling_entry->sibling != NULL) {
				sibling_entry = sibling_entry->sibling;
			}
			sibling_entry->sibling = new_entry;
		}
		// parent has no children
		else if (parent_entry->first_child == NULL) {
			parent_entry->first_child = new_entry;
		}
		// parent exists, has children
		else {
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

void proc_tree_walk_list(void) {
	struct proc_tree_list_struct *entry = proc_tree_list;
	struct task_struct *task;
	int depth = 0;

	while (entry != NULL) {
		task = entry->task;
		printk("(%d) %d: %s\n", depth, task->pid, task->comm);

		if (entry->first_child != NULL) {
			depth++;
			entry = entry->first_child;
		}
		else if (entry->sibling != NULL) {
			entry = entry->sibling;
		}
		else if (entry->parent != NULL) {
			// we've already done the immediate parent.
			// get the parent's sibling.
			entry = entry->parent->sibling;
		}
		else {
			// top level last process may have neither sibling
			// nor parent.
			entry = NULL;
		}
	}
}

void proc_tree_clear_output(void) {
	/*
	kfree(proc_tree_output);
	proc_tree_output_size = 4096;
	proc_tree_output = kmalloc(proc_tree_output_size, GFP_KERNEL);
	proc_tree_output_size_used = 1;
	*/
	proc_tree_output[0] = '\0';
	proc_tree_output_size_used = 1;
}

void proc_tree_add_output(const char *string) {
	if (strlen(string) + proc_tree_output_size_used 
			>= PROC_TREE_OUTPUT_SIZE) {
		/* not handling this yet */
		return;
	}
	strcat(proc_tree_output, string);
}

void proc_tree_format_output(struct task_struct *task, int depth) {
	int n;
	char buffer[1024];
	snprintf(buffer, 1023, "%d: %s\n", task->pid, task->comm);

	/* create tree structure based on depth */
	for (n = 0; n < depth; n++) {
		proc_tree_add_output("|-");
	}

	proc_tree_add_output(buffer);
}

void proc_tree_get_tasks(struct task_struct *start_task, int depth)
{
	struct task_struct *current_task, *child;
	struct list_head *t_list;

	// safety switch
	if (depth > 3) {
		printk(KERN_ERR "Too much recursion in proc_tree_get_tasks\n");
		return;
	}

	if (start_task == NULL || strlen(start_task->comm) == 0) {
		printk("Invalid task found.\n");
	}

	//printk("start: %d: %d: %s\n", depth, start_task->pid, start_task->comm);
	//proc_tree_format_output(start_task, depth);

	/* find task children */
	list_for_each(t_list, &start_task->children) {
		child = list_entry(t_list, struct task_struct, children);
		if (strlen(child->comm) == 0) {
			printk("Invalid child process entry found.\n");
			continue;
		}
		printk("child: %d: %d: %s\n", depth, child->pid, child->comm);

		/*
		grandchild = child_task(child);
		if (strlen(grandchild->comm) > 0) {
			proc_tree_get_tasks(grandchild, 
		*/
	}

	/*
	child = child_task(start_task);
	t_list = child->sibling.next;
	if (!list_is_last(t_list, &child->sibling)) {
	//if (!list_is_singular(&start_task->children)) {
		proc_tree_get_tasks(child, depth + 1);
	}
	*/

	//list_for_each_entry(current_task, &start_task->sibling, sibling) {
	list_for_each(t_list, &start_task->sibling) {
		current_task = list_entry(t_list, struct task_struct, sibling);
		if (strlen(current_task->comm) == 0) {
			printk("Invalid sibling process entry found.\n");
			continue;
		}
		if (list_is_last(t_list, &start_task->sibling)) {
			printk("Last sibling.\n");
			continue;
		}
		printk("sibling: %d: %d: %s\n", depth, current_task->pid, current_task->comm);
		child = child_task(current_task);
		proc_tree_get_tasks(child, depth + 1);
		/*if (!list_empty(&current_task->children)) {
			child = child_task(current_task);
			proc_tree_get_tasks(child, depth + 1);
		}
		else {
			printk("%d: %d: %s\n", depth, current_task->pid, current_task->comm);
		}
		*/
	}
	/*
	current_task = task_sibling(start_task);
	while (current_task != start_task) {
		next_task = task_sibling(current_task);
		if (next_task != start_task) {
			printk("%d: %d: %s\n", depth, current_task->pid, current_task->comm);
		}
		//proc_tree_format_output(current_task, depth);
		current_task = next_task;
	}
	*/
}


int proc_tree_read(char *buffer, char **buffer_location, off_t offset, 
		int buffer_length, int *eof, void *data)
{
	struct task_struct *task;
	int ret;

	if (offset > 0) {
		/* we have finished reading, return 0 */
		ret = 0;
	}
	else {
		//proc_tree_clear_output();
		// task 0 is the swapper, let's start at task 1 (init)
		//proc_tree_get_tasks(next_task(&init_task), 0);
		//proc_tree_get_tasks(&init_task, 0);
		//strncpy(buffer, proc_tree_output, buffer_length - 1);
		//ret = strlen(buffer);

		for_each_process(task) {
			proc_tree_add_entry(task);
		}
		proc_tree_walk_list();
		proc_tree_clear_list();

		ret = 0;
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

