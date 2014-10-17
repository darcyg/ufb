#ifndef UFB_DRV_H
#define UFB_DRV_H

#include <linux/fb.h>
#include <linux/device.h>
#include <linux/types.h>

struct ufb_dev {
	struct device *dev;
	int *vmem;
	size_t vmem_size;
   struct fb_info *fb_info;
};

extern int ufb_fb_init(struct ufb_dev *dev);
extern void ufb_fb_deinit(struct ufb_dev *dev);

#endif //UFB_DRV_H

