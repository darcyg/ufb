#ifndef UFB_H
#define UFB_H

#include <stddef.h>

struct ufb_context;
typedef struct ufb_context ufb_context_t;

typedef enum {
	UFB_OK,
	UFB_ERR_INVALID_PARAM,
	UFB_ERR_NO_MEM,
	UFB_ERR_OPENING_DEVICE,
	UFB_ERR_ALLOC_VMEM,
	UFB_ERR_MMAP,
	UFB_ERR_CREATE_FB,
} ufb_err_t;

extern ufb_err_t ufb_init(ufb_context_t **context, int width, int height, 
                          size_t vmem_size);

extern void* ufb_get_vmem(ufb_context_t *context);

extern ufb_err_t ufb_signal_vblank(ufb_context_t *context);

extern void ufb_free(ufb_context_t *context);

extern const char* ufb_strerror(ufb_err_t);

#endif //UFB_H

