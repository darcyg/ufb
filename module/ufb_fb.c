#include "ufb_drv.h"

#include <linux/mm.h>

static struct fb_var_screeninfo ufb_fb_var_default =
{
	.xres           = 640,
	.yres           = 480,
	.xres_virtual   = 640,
	.yres_virtual   = 480,
	.bits_per_pixel =  32,
	.red            = {  0, 8, 0 },
	.green          = {  8, 8, 0 },
	.blue           = { 16, 8, 0 },
	.transp         = { 24, 8, 0 },
	.activate       = FB_ACTIVATE_TEST,
	.height         = -1,
	.width          = -1,
	.pixclock       = 39722,
	.left_margin    = 48,
	.right_margin   = 16,
	.upper_margin   = 33,
	.lower_margin   = 10,
	.hsync_len      = 96,
	.vsync_len      = 2,
	.vmode          = FB_VMODE_NONINTERLACED,
};

static int ufb_fb_check_var(struct fb_var_screeninfo *var,
                            struct fb_info *info);
static int ufb_fb_set_par(struct fb_info *info);
static int ufb_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                            u_int transp, struct fb_info *info);
static int ufb_fb_pan_display(struct fb_var_screeninfo *var,
                              struct fb_info *info);
static int ufb_fb_mmap(struct fb_info *info,
                       struct vm_area_struct *vma);

static struct fb_ops ufb_ops = {
	.fb_read        = fb_sys_read,
	.fb_write       = fb_sys_write,
	.fb_check_var   = ufb_fb_check_var,
	.fb_set_par     = ufb_fb_set_par,
	.fb_setcolreg   = ufb_fb_setcolreg,
	.fb_pan_display = ufb_fb_pan_display,
	.fb_fillrect    = sys_fillrect,
	.fb_copyarea    = sys_copyarea,
	.fb_imageblit   = sys_imageblit,
	.fb_mmap        = ufb_fb_mmap,
};

/*
 *  Internal routines
 */
static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return (length);
}

/*
 *  Setting the video mode has been split into two parts.
 *  First part, xxxfb_check_var, must not write anything
 *  to hardware, it should only verify and adjust var.
 *  This means it doesn't alter par but it does use hardware
 *  data from it to check this var. 
 */
static int ufb_fb_check_var(struct fb_var_screeninfo *var,
                            struct fb_info *info)
{
	u_long line_length;

	printk(KERN_INFO "usrfb_check_var( var=%p, info=%p )\n", var, info);

	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/*
	 *  Some very basic checks
	 */
	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;
	if (var->bits_per_pixel <= 1)
		var->bits_per_pixel = 1;
	else if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 24)
		var->bits_per_pixel = 24;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
		return -EINVAL;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	line_length =
	    get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > info->fix.smem_start)
		return -ENOMEM;

	/*
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it 
	 * work. This way we let the user know what is acceptable.
	 */
	switch (var->bits_per_pixel) {
	case 1:
	case 8:
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 0;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 16:		/* RGBA 5551 */
		if (var->transp.length) {
			var->red.offset = 0;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 10;
			var->blue.length = 5;
			var->transp.offset = 15;
			var->transp.length = 1;
		} else {	/* RGB 565 */
			var->red.offset = 0;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 11;
			var->blue.length = 5;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 24:		/* RGB 888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:		/* RGBA 8888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	}
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

/* 
 * This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the 
 * change in par. For this driver it doesn't do much. 
 */
static int ufb_fb_set_par(struct fb_info *info)
{
	printk(KERN_INFO "usrfb_set_par( info=%p )\n", info);
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	return 0;
}

/*
 *  Set a single color register. The values supplied are already
 *  rounded down to the hardware's capabilities (according to the
 *  entries in the var structure). Return != 0 for invalid regno.
 */
static int ufb_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                            u_int transp, struct fb_info *info)
{
	if (regno >= 256)	/* no. of hw registers */
		return 1;

	/*
	 * Program hardware... do anything you want with transp
	 */
	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue =
		    (red * 77 + green * 151 + blue * 28) >> 8;
	}

	/* Directcolor:
	 *   var->{color}.offset contains start of bitfield
	 *   var->{color}.length contains length of bitfield
	 *   {hardwarespecific} contains width of RAMDAC
	 *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
	 *   RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Pseudocolor:
	 *    var->{color}.offset is 0 unless the palette index takes less than
	 *                        bits_per_pixel bits and is stored in the upper
	 *                        bits of the pixel value
	 *    var->{color}.length is set so that 1 << length is the number of available
	 *                        palette entries
	 *    cmap is not used
	 *    RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Truecolor:
	 *    does not use DAC. Usually 3 are present.
	 *    var->{color}.offset contains start of bitfield
	 *    var->{color}.length contains length of bitfield
	 *    cmap is programmed to (red << red.offset) | (green << green.offset) |
	 *                      (blue << blue.offset) | (transp << transp.offset)
	 *    RAMDAC does not exist
	 */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		red = CNVT_TOHW(red, info->var.red.length);
		green = CNVT_TOHW(green, info->var.green.length);
		blue = CNVT_TOHW(blue, info->var.blue.length);
		transp = CNVT_TOHW(transp, info->var.transp.length);
		break;
	case FB_VISUAL_DIRECTCOLOR:
		red = CNVT_TOHW(red, 8);	/* expect 8 bit DAC */
		green = CNVT_TOHW(green, 8);
		blue = CNVT_TOHW(blue, 8);
		/* hey, there is bug in transp handling... */
		transp = CNVT_TOHW(transp, 8);
		break;
	}
#undef CNVT_TOHW
	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v;

		if (regno >= 16)
			return 1;

		v = (red << info->var.red.offset) |
		    (green << info->var.green.offset) |
		    (blue << info->var.blue.offset) |
		    (transp << info->var.transp.offset);
		switch (info->var.bits_per_pixel) {
		case 8:
			break;
		case 16:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		case 24:
		case 32:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		}
		return 0;
	}
	return 0;
}

/*
 *  Pan or Wrap the Display
 *
 *  This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 */
static int ufb_fb_pan_display(struct fb_var_screeninfo *var,
                              struct fb_info *info)
{
	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset >= info->var.yres_virtual ||
		    var->xoffset)
			return -EINVAL;
	} else {
		if (var->xoffset + info->var.xres > info->var.xres_virtual ||
		    var->yoffset + info->var.yres > info->var.yres_virtual)
			return -EINVAL;
	}
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
	return 0;
}

/*
 *  Most drivers don't need their own mmap function 
 */
static int ufb_fb_mmap(struct fb_info *info,
                       struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long page, pos;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	if (size > info->fix.smem_len)
		return -EINVAL;
	if (offset > info->fix.smem_len - size)
		return -EINVAL;

	pos = (unsigned long)info->fix.smem_start + offset;

	while (size > 0) {
		page = vmalloc_to_pfn((void *)pos);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED)) {
			return -EAGAIN;
		}
		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

	return 0;

}

int ufb_fb_init(struct ufb_dev *dev)
{
	int retval = 0;

	if (dev->fb_info) {
		retval = -EINVAL;
		goto err1;
	}

	dev->fb_info = framebuffer_alloc(0, dev->dev);

	if (!dev->fb_info) {
		retval = -ENOMEM;
		goto err1;
	}

	dev->fb_info->screen_base = (char __iomem *)dev->vmem;
	dev->fb_info->fbops = &ufb_ops;

	retval = fb_find_mode(&dev->fb_info->var, dev->fb_info, NULL, NULL, 0, 
	                      NULL, 8);

	if (!retval || (retval == 4)) {
		dev->fb_info->var = ufb_fb_var_default;
	}

	dev->fb_info->fix.smem_start = (unsigned long)dev->vmem;
	dev->fb_info->fix.smem_len   = dev->vmem_size;
	strcpy(dev->fb_info->fix.id, "User FB");
	dev->fb_info->fix.type       = FB_TYPE_PACKED_PIXELS;
	dev->fb_info->fix.visual     = FB_VISUAL_PSEUDOCOLOR;
	dev->fb_info->fix.xpanstep   = 1,
	dev->fb_info->fix.ypanstep   = 1,
	dev->fb_info->fix.ywrapstep  = 1,
	dev->fb_info->fix.line_length = 640 * sizeof(u32);
	dev->fb_info->fix.accel      = FB_ACCEL_NONE,

	dev->fb_info->pseudo_palette = kzalloc(sizeof(u32) * 256, GFP_KERNEL);
	if (!dev->fb_info->pseudo_palette) {
		retval = -ENOMEM;
		goto err2;
	}

	dev->fb_info->par = dev;

	dev->fb_info->flags = FBINFO_FLAG_DEFAULT;

	retval = fb_alloc_cmap(&dev->fb_info->cmap, 256, 0);
	if (retval < 0 ) {
		goto err3;
	}

	retval = register_framebuffer(dev->fb_info);
	if (retval < 0) {
		goto err4;
	}

	return 0;

err4:
	fb_dealloc_cmap(&dev->fb_info->cmap);
err3:
	kfree(dev->fb_info->pseudo_palette);
err2:
	framebuffer_release(dev->fb_info);
	dev->fb_info = NULL;
err1:
	return retval;
}


void ufb_fb_deinit(struct ufb_dev *dev)
{
	if (dev->fb_info) {
		unregister_framebuffer(dev->fb_info);
		fb_dealloc_cmap(&dev->fb_info->cmap);
		kfree(dev->fb_info->pseudo_palette);
		framebuffer_release(dev->fb_info);
		dev->fb_info = NULL;
	}
}

