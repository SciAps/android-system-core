#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>


#include <cutils/memory.h>

#define from565_r(x) ((((x) >> 11) & 0x1f) * 255 / 31)
#define from565_g(x) ((((x) >> 5) & 0x3f) * 255 / 63)
#define from565_b(x) (((x) & 0x1f) * 255 / 31)

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
uint8_t *fbp;


inline uint32_t pixel_color_32(uint8_t r, uint8_t g, uint8_t b)
{
	return (r<<vinfo.red.offset) | (g<<vinfo.green.offset) | (b<<vinfo.blue.offset);
}

inline uint16_t pixel_color_565(uint8_t r, uint8_t g, uint8_t b)
{
  //return (r<<vinfo->red.offset) | (g<<vinfo->green.offset) | (b<<vinfo->blue.offset);
  return ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3));
}


int load_565rle_image(uint16_t* data, int length_bytes) {
  int max, i, j;
  uint16_t x, y, n, color_16;
  uint32_t* location;
  uint32_t color_32;

  x = 0;
  y = 0;

  printf("length_bytes: %d\n", length_bytes);

  for(i=0;i<length_bytes/2;i++) {
    n = data[2*i];
    color_16 = data[2*i+1];

    //printf("n: %d: color_16: %d\n", n, color_16);

    color_32 = pixel_color_32(from565_r(color_16), from565_g(color_16), from565_b(color_16));

    for(j=0;j<n;++j) {

      if(x >= vinfo.xres || y >= vinfo.yres) {
        printf("prevent buffer overrun: x: %d y: %d\n", x, y);
        return -1;
      } else {
        location = (uint32_t*) (fbp + (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length);
        *location = color_32;
      }

      x++;
      if(x >= vinfo.xres) {
        x = 0;
        y++;
      }
    }

  }

  return 0;
}


int load_565rle_file(const char* fn)
{
  int fd, rc;
  struct stat s;
  void* data;

  printf("here\n");

  rc = -1;

  fd = open(fn, O_RDONLY);
  if (fd < 0) {
      goto fail;
  }

  if (fstat(fd, &s) < 0) {
      goto fail_close_file;
  }

  data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
      goto fail_close_file;
  }

  rc = load_565rle_image(data, s.st_size);

  munmap(data, s.st_size);

  fail_close_file:
    close(fd);

  fail:

  return rc;
}


int main()
{
  int rc;
	int fb_fd = open("/dev/graphics/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

  rc = load_565rle_file("/initlogo.rle");
  printf("rc: %d\n", rc);

/*
	unsigned int x,y;
	for (x=0;x<vinfo.xres;x++) {
		for (y=0;y<vinfo.yres;y++) {
			long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel_color_32(0x00,0xFF,0x00);
		}
  }
*/

  munmap(fbp, screensize);
  close(fb_fd);

	return 0;
}
