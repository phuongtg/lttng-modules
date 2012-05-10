/*
 * lttng-user-event.c
 *
 * Copyright (C) 2006 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * 2012-05-08 Ported to lttng 2 by Francis Giraldeau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; only
 * version 2 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* is there a prefered include order? */
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/irqflags.h>

#include "../instrumentation/events/lttng-module/lttng.h"

DEFINE_TRACE(lttng_uevent);
DEFINE_TRACE(lttng_uevent_cfu);
DEFINE_TRACE(lttng_uevent_memcpy);

#define LTTNG_UEVENT_FILE	"lttng_user_event"
#define LTTNG_UEVENT_FILE_CFU	"lttng_user_event_cfu"
#define LTTNG_UEVENT_FILE_MEMCPY	"lttng_user_event_memcpy"

/**
 * write_event - write a userspace string into the trace system
 * @file: file pointer
 * @user_buf: user string
 * @count: length to copy, including the final NULL
 * @ppos: unused
 *
 * Copy a string into a trace event "user_event". Pins pages in memory to avoid
 * intermediate copy.
 *
 * On success, returns the number of bytes copied from the source.
 *
 * Inspired from tracing_mark_write implementation from Steven Rostedt and
 * Ingo Molnar.
 */
static
ssize_t write_event(struct file *file, const char __user *user_buf,
		    size_t count, loff_t *fpos)
{
	char tmp[LTTNG_UEVENT_SIZE];
	unsigned long addr = (unsigned long)user_buf;
	struct page *pages[2];
	int nr_pages = 1;
	void *page1, *page2;
	ssize_t written;
	int offset, ret, len;

	if (count >= LTTNG_UEVENT_SIZE)
		count = LTTNG_UEVENT_SIZE - 1;

	BUILD_BUG_ON(LTTNG_UEVENT_SIZE >= PAGE_SIZE);

	if ((addr & PAGE_MASK) != ((addr + count) & PAGE_MASK))
		nr_pages = 2;

	offset = addr & (PAGE_SIZE - 1);
	addr &= PAGE_MASK;

	ret = get_user_pages_fast(addr, nr_pages, 0, pages);
	if (ret < nr_pages) {
		while (--ret >= 0)
			put_page(pages[ret]);
		written = -EFAULT;
		goto out;
	}

	page1 = kmap_atomic(pages[0]);
	if (nr_pages == 2)
		page2 = kmap_atomic(pages[1]);

	if (nr_pages == 2) {
		len = PAGE_SIZE - offset;
		memcpy(tmp, page1 + offset, len);
		memcpy(tmp + len, page2, count - len);
	} else
		memcpy(tmp, page1 + offset, count);

	/* make sure the string is null terminated */
	tmp[count] = '\0';
	trace_lttng_uevent(tmp);
	written = count;

	if (nr_pages == 2)
		kunmap_atomic(page2);
	kunmap_atomic(page1);
	while (nr_pages > 0)
		put_page(pages[--nr_pages]);
 out:
	return written;
}

/* CFU stands for Copy From User */
static
ssize_t write_event_copy_from_user(struct file *file, const char __user *user_buf,
		    size_t count, loff_t *fpos)
{
	/* How do we know how much data is written? Otherwise,
	 * this function can't return the appropriate value.
	 *
	 * If the user string is not null-terminated,
	 * then could it leak info beyong the string? : yes
	 *
	 * Assuming the userspace string is well formated
	 * seems not appropriate.
	 */
	trace_lttng_uevent_cfu(user_buf);
	return count;
}

static
ssize_t write_event_memcpy(struct file *file, const char __user *user_buf,
		    size_t count, loff_t *fpos)
{
	if (count >= LTTNG_UEVENT_SIZE)
		count = LTTNG_UEVENT_SIZE - 1;

	/*
	 * still unable to enforce null terminated string
	 * userspace won't be happy if we change their buffers in-place
	 */
	trace_lttng_uevent_memcpy(user_buf, count);
	return count;
}

static const struct file_operations write_file_ops = {
	.owner = THIS_MODULE,
	.write = write_event
};

static const struct file_operations write_file_cfu_ops = {
	.owner = THIS_MODULE,
	.write = write_event_copy_from_user
};

static const struct file_operations write_file_memcpy_ops = {
	.owner = THIS_MODULE,
	.write = write_event_memcpy
};

static int init_proc_entry(char *name, const struct file_operations *fops)
{
	struct proc_dir_entry *write_file;

	write_file = create_proc_entry(name, 0644, NULL);
	if (!write_file) {
		return -ENOENT;
	}
	write_file->proc_fops = fops;
	write_file->mode = S_IFREG | S_IWUGO;
	write_file->uid = 0;
	write_file->gid = 0;
	return 0;
}

static int __init lttng_user_event_init(void)
{
	int err = 0;

	if ((err = init_proc_entry(LTTNG_UEVENT_FILE, &write_file_ops)) < 0)
		goto err_1;
	if ((err = init_proc_entry(LTTNG_UEVENT_FILE_CFU, &write_file_cfu_ops)) < 0)
		goto err_2;
	if ((err = init_proc_entry(LTTNG_UEVENT_FILE_MEMCPY, &write_file_memcpy_ops)) < 0)
		goto err_3;

	return err;

 err_3:
	remove_proc_entry(LTTNG_UEVENT_FILE_MEMCPY, NULL);
 err_2:
	remove_proc_entry(LTTNG_UEVENT_FILE_CFU, NULL);
 err_1:
	remove_proc_entry(LTTNG_UEVENT_FILE, NULL);
	return err;
}

static void __exit lttng_user_event_exit(void)
{
	remove_proc_entry(LTTNG_UEVENT_FILE, NULL);
	printk(KERN_INFO "/proc/%s removed\n", LTTNG_UEVENT_FILE);
	remove_proc_entry(LTTNG_UEVENT_FILE_CFU, NULL);
	printk(KERN_INFO "/proc/%s removed\n", LTTNG_UEVENT_FILE_CFU);
	remove_proc_entry(LTTNG_UEVENT_FILE_MEMCPY, NULL);
	printk(KERN_INFO "/proc/%s removed\n", LTTNG_UEVENT_FILE_MEMCPY);
}

module_init(lttng_user_event_init);
module_exit(lttng_user_event_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Francis Giraldeau <francis.giraldeau@polymtl.ca>");
MODULE_DESCRIPTION("Append custom events to kernel trace from userspace");
