#ifndef UFB_DRV_H
#define UFB_DRV_H

#include <linux/fb.h>
#include <linux/types.h>

struct ufb_dev {
	int *vmem;
	size_t vmem_size;
};

#endif //UFB_DRV_H

