#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int fbfd = -1;
char *fbp = NULL;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

void gradient(const int starting_x, const int starting_y)
{
	int x = 0;
	int y = 0;
    long int location = 0;

    // Figure out where in memory to put the pixel
    for ( y = starting_y; y < starting_y + 256; y++ ) {
        for ( x = starting_x; x < starting_x + 256; x++ ) {

            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                       (y+vinfo.yoffset) * finfo.line_length;

            if ( vinfo.bits_per_pixel == 32 ) {
                fbp[location + 0] = 100;        // Some blue
                fbp[location + 1] = y - starting_y;     // A little green
                fbp[location + 2] = x - starting_x;    // A lot of red
                //fbp[location + 3] = 0;      // No transparency

                //fbp[location]     = 0;
                //fbp[location + 1] = 0;
                //fbp[location + 2] = 0;
                fbp[location + 3] = 0;
            } else  { //assume 16bpp
                int b = 10;
                int g = (x-100)/6;     // A little green
                int r = 31-(y-100)/16;    // A lot of red
                unsigned short int t = r<<11 | g << 5 | b;
                *((unsigned short int*)(fbp + location)) = t;
            }

        }
    }
}

void clear(void)
{
	unsigned int i;

	for( i = 0; i < vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8); i++ ) {
		fbp[i] = 0;
	}
}

void waitforvblank(void)
{
	if(ioctl(fbfd, FBIO_WAITFORVSYNC)) {
		perror("waitforvlbank()");
		exit(5);
	}
}

void meander(void)
{
	unsigned int x = 100;
	unsigned int y = 100;

	int x_inc = 1;
	int y_inc = 1;

	while( 1 ) {
		//clear();

		gradient(x, y);

		x += x_inc;
		y += y_inc;

		if( (x == 0) || (x == (vinfo.xres - 256 - 1)) ) {
			x_inc *= -1;
		}

		if( (y == 0) || (y == (vinfo.yres - 256 - 1)) ) {
			y_inc *= -1;
		}

		waitforvblank();
	}
}

int main(int argc, char **argv)
{
    long int screensize = 0;

    if (argc != 3) {
        printf("Usage:  %s DEVICE_FILE clear|gradient\n", argv[0]);
        exit(1);
    }

    // Open the file for reading and writing
    fbfd = open(argv[1], O_RDWR);
    if ( -1 == fbfd) {
        perror("Error: cannot open framebuffer device");
        exit(2);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        exit(3);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        exit(4);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fbfd, 0);
    if (fbp == MAP_FAILED) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

	if( strcmp(argv[2], "clear") == 0 ) {
		clear();
	} else if( strcmp(argv[2], "gradient") == 0 ) {
		gradient(100, 100);
	}
	else if( strcmp(argv[2], "meander") == 0 ) {
		meander();
	}
	else {
		printf( "Unknown command:  \"%s\"\n", argv[2] );
		return 1;
	}

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

