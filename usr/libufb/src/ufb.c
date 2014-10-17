#include "ufb.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ufb_context {
	int width;
	int height;
	void *vmem;
	size_t vmem_size;
	int fd;
};

#define UFB_IOCTL_ALLOC_VMEM (_IOW('U',0,uint32_t*))
#define UFB_IOCTL_CREATE_FB  (_IO('U',1))

#define DEVICE_FILENAME ("/dev/ufb")

static ufb_err_t _ufb_alloc_vmem(ufb_context_t *context)
{
	int ret;

	ret = ioctl(context->fd, UFB_IOCTL_ALLOC_VMEM, &context->vmem_size);

	if( -1 == ret ) {
		return UFB_ERR_ALLOC_VMEM;
	}

	return UFB_OK;
}

static ufb_err_t _ufb_map_vmem(ufb_context_t *context)
{
	context->vmem = mmap(NULL, context->vmem_size, PROT_READ|PROT_WRITE, 
	                     MAP_SHARED, context->fd, 0);

	if( MAP_FAILED == context->vmem ) {
		context->vmem = NULL;
		perror("UFB");
		return UFB_ERR_MMAP;
	}

	return UFB_OK;
}

static ufb_err_t _ufb_create_fb(ufb_context_t *context)
{
	int ret;

	ret = ioctl(context->fd, UFB_IOCTL_CREATE_FB);

	if( -1 == ret ) {
		return UFB_ERR_CREATE_FB;
	}

	return UFB_OK;
}

ufb_err_t ufb_init(ufb_context_t **context_ptr, int width, int height, 
                   size_t vmem_size )
{
	ufb_err_t err = UFB_OK;
	ufb_context_t *context;

	if( !context_ptr ) {
		return UFB_ERR_INVALID_PARAM;
	}

	*context_ptr = NULL;

	context = malloc(sizeof(*context));
	if(!context) {
		err = UFB_ERR_NO_MEM;
		goto error_base;
	}

	context->width = width;
	context->height = height;
	context->vmem = NULL;
	context->vmem_size = vmem_size;

	context->fd = open("/dev/ufb", O_RDWR|O_SYNC);

	if( -1 == context->fd ) {
		err = UFB_ERR_OPENING_DEVICE;
		goto error_free;
	}

	if( UFB_OK != (err = _ufb_alloc_vmem(context)) ) {
		goto error_close;
	}

	if( UFB_OK != (err = _ufb_map_vmem(context)) ) {
		goto error_close;
	}

	if( UFB_OK != (err = _ufb_create_fb(context)) ) {
		goto error_close;
	}

	*context_ptr = context;

	return err;

error_close:
	close(context->fd);
error_free:
	free(context);
error_base:
	return err;
}

void* ufb_get_vmem(ufb_context_t *context)
{
	return context->vmem;
}

void ufb_free(ufb_context_t *context)
{
	if( context ) {
		close(context->fd);
		free(context);
	}
}

