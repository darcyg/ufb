#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* this is a test program that opens the mmap_drv.
   It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness.
   You need a device special file to access the driver.
   The device special file is called 'node' and searched
   in the current directory.
   To create it
   - load the driver
     'insmod mmap_mod.o'
   - find the major number assigned to the driver
     'grep mmapdrv /proc/devices'
   - and create the special file (assuming major number 254)
     'mknod node c 254 0'
*/

#define UFB_IOCTL_ALLOC_VMEM (_IOW('U',0,uint32_t*))

int main(int argc, char **argv)
{
	int fd;
	unsigned int *vadr;
	char *device_filename;
	uint32_t vmem_size = 2 * 1024 * 1024;

	if( argc != 2 ) {
		fprintf(stderr, "Usage:  %s DEVICE_FILE\n", argv[0]);
		return 1;
	}

	device_filename = argv[1];

	if( (fd = open(device_filename, O_RDWR|O_SYNC)) < 0 ) {
		perror("open");
		return 1;
	}

	if( -1 == ioctl(fd, UFB_IOCTL_ALLOC_VMEM, &vmem_size) ) {
		perror("alloc_vmem");
		return 1;
	}

	vadr = mmap(0, vmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if( vadr == MAP_FAILED ) {
		perror("mmap");
		return 1;
	}

	close(fd);

	printf( "Success\n" );

	return 0 ;
}

