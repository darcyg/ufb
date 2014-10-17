#include "ufb_drv.h"

#include <asm/uaccess.h>
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

static int ufb_device_open(struct inode *inode, struct file *file);
static int ufb_device_release(struct inode *inode, struct file *file);
static long ufb_device_unlocked_ioctl(struct file *file, unsigned int cmd, 
                                      unsigned long arg);
static int ufb_device_mmap(struct file *filp, struct vm_area_struct *vma);

static struct file_operations ufb_device_operations = {
	.owner = THIS_MODULE,

	.open = ufb_device_open,
	.release = ufb_device_release,

	.unlocked_ioctl = ufb_device_unlocked_ioctl,
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

	printk(KERN_INFO "ufb:  ufb_device_open( inode=%p, file=%p )\n", inode, file );

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if( !dev ) {
		err = -ENOMEM;
		goto out;
	}

	dev->dev = ufb_miscdevice.this_device;

	file->private_data = dev;

out:
	return err;
}

static int ufb_device_release(struct inode *inode, struct file *file)
{
	struct ufb_dev *dev;
	int i;

	printk(KERN_INFO "ufb:  ufb_device_release( inode=%p, file=%p )\n", inode, file );

	dev = file->private_data;

	ufb_fb_deinit(dev);

	/* unreserve the pages */
	for (i = 0; i < dev->vmem_size; i+= PAGE_SIZE) {
		ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->vmem) + i)));
	}

	vfree(dev->vmem);

	kfree(dev);

	return 0;
}

static int _ufb_alloc_vmem(struct ufb_dev *dev, size_t vmem_size)
{
	int err = 0;
	int i;

	dev->vmem_size = vmem_size;

	if ((dev->vmem = (int *)vmalloc(dev->vmem_size)) == NULL) {
		err = -ENOMEM;
		goto out;
	}

	memset(dev->vmem, 0, dev->vmem_size);

	for (i = 0; i < dev->vmem_size; i+= PAGE_SIZE) {
		SetPageReserved(vmalloc_to_page((void *)(((unsigned long)dev->vmem) + i)));
	}

out:
	return err;
}

static long ufb_device_unlocked_ioctl(struct file *file, unsigned int cmd, 
                                 unsigned long arg)
{
	struct ufb_dev *dev;
	long err;
	int nr;

	printk(KERN_INFO "ufb:  ufb_device_unlocked_ioctl( file=%p, cmd=%x, arg=0x%lx )\n",
	       file, cmd, arg);

	nr = _IOC_NR(cmd);
	dev = file->private_data;

	err = -EINVAL;
	switch(nr) {
		case 0: {
			u32 new_vmem_size;

			copy_from_user(&new_vmem_size, (void*)arg, sizeof(new_vmem_size));

			printk(KERN_INFO "ufb:  alloc_vmem:  0x%x\n", new_vmem_size );

			dev->vmem_size = new_vmem_size;

			err = _ufb_alloc_vmem(dev, new_vmem_size);
		}
		break;

		case 1: {
			printk(KERN_INFO "ufb:  create_fb\n");

			err = ufb_fb_init(dev);
		}
		break;

		default: {
			printk(KERN_INFO "ufb:  unknown nr:  %d\n", nr);
			err = -EINVAL;
		}
		break;
	}

	return err;
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

	if( dev->vmem == NULL ) {
		return -EINVAL;
	}

	vmalloc_area_ptr = (char*)dev->vmem;

	if (length > dev->vmem_size) {
		return -EIO;
	}

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

