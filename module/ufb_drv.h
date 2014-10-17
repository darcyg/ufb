#ifndef UFB_DRV_H
#define UFB_DRV_H

#include <linux/fb.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/wait.h>

struct ufb_dev {
	struct device *dev;
	int *vmem;
	size_t vmem_size;
	struct fb_info *fb_info;

	unsigned int vblank_count;
	wait_queue_head_t vblank_wait;
};

extern int ufb_fb_init(struct ufb_dev *dev);
extern void ufb_fb_deinit(struct ufb_dev *dev);

#endif //UFB_DRV_H

