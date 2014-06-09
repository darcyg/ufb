#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>

#define DRIVER_AUTHOR "Tristan Miller"
#define DRIVER_DESC   "Test module for user mode framebuffer driver wrapper"

#define DEVICE_NAME "ufb"

// internal data
// length of the two memory areas
#define VMEM_SIZE (10 * 1024 * 1024)
#define NPAGES    (VMEM_SIZE / PAGE_SIZE)

static int ufb_device_open(struct inode *inode, struct file *file);
static int ufb_device_release(struct inode *inode, struct file *file);
static int ufb_device_mmap(struct file *filp, struct vm_area_struct *vma);

struct ufb_dev {
	int *vmem;
};

static struct file_operations ufb_device_operations = {
	.owner = THIS_MODULE,

	.open = ufb_device_open,
	.release = ufb_device_release,

	.mmap = ufb_device_mmap,
};

static struct miscdevice ufb_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops = &ufb_device_operations,
};

static int ufb_device_open(struct inode *inode, struct file *file)
{
	struct ufb_dev *dev;
	int err = 0;
	int i;

	printk(KERN_INFO "ufb:  ufb_device_open( inode=%p, file=%p )\n", inode, file );

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if( !dev ) {
		err = -ENOMEM;
		goto out;
	}

	/* allocate a memory area with vmalloc. */
	if ((dev->vmem = (int *)vmalloc(VMEM_SIZE)) == NULL) {
		err = -ENOMEM;
		goto out_kfree;
	}

	/* mark the pages reserved */
	for (i = 0; i < VMEM_SIZE; i+= PAGE_SIZE) {
		SetPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->vmem) + i)));
	}

	/* store a pattern in the memory - the test application will check for it */
	for (i = 0; i < (VMEM_SIZE / sizeof(int)); i += 2) {
		dev->vmem[i + 0] = (0xaffe << 16) + i;
		dev->vmem[i + 1] = (0xbeef << 16) + i;
	}

	file->private_data = dev;

	return err;

out_kfree:
	kfree(dev);
out:
	return err;
}

static int ufb_device_release(struct inode *inode, struct file *file)
{
	struct ufb_dev *dev;
	int i;

	printk(KERN_INFO "ufb:  ufb_device_release( inode=%p, file=%p )\n", inode, file );

	dev = file->private_data;

	/* unreserve the pages */
	for (i = 0; i < VMEM_SIZE; i+= PAGE_SIZE) {
		ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->vmem) + i)));
	}

	vfree(dev->vmem);

	kfree(dev);

	return 0;
}

static int ufb_device_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ufb_dev *dev;
	int ret;
	long length = vma->vm_end - vma->vm_start;
	unsigned long start = vma->vm_start;
	char *vmalloc_area_ptr;
	unsigned long pfn;

	printk(KERN_INFO "ufb:  ufb_device_mmap( file=%p, vma=%p )\n", file, vma );

	dev = file->private_data;
	vmalloc_area_ptr = (char*)dev->vmem;

	/* check length - do not allow larger mappings than the number of
	   pages allocated */
	if (length > VMEM_SIZE) {
		return -EIO;
	}

	/* loop over all pages, map it page individually */
	while (length > 0) {
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		if ((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE,
		                           PAGE_SHARED)) < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}
	return 0;
}

static int __init ufb_init(void)
{
	int err = 0;

	printk(KERN_INFO "ufb:  ufb_init()\n" );

	if(0 != (err = misc_register(&ufb_miscdevice))) {
		return err;
	}

	return 0;
}

module_init(ufb_init);

static void __exit ufb_exit(void)
{
	printk(KERN_INFO "ufb:  ufb_exit()\n" );

	misc_deregister(&ufb_miscdevice);
}

module_exit(ufb_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

MODULE_SUPPORTED_DEVICE(DEVICE_NAME);

