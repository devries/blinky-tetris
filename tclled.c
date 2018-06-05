#include "tclled.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>

void write_frame(tcl_color *p, uint8_t flag, uint8_t red, uint8_t green, uint8_t blue);
uint8_t make_flag(uint8_t red, uint8_t greem, uint8_t blue);
ssize_t write_all(int filedes, const void *buf, size_t size);

void tcl_init(tcl_buffer *buf, int leds) {
  buf->leds = leds;
  buf->size = (leds+3)*sizeof(tcl_color);
  buf->buffer = (tcl_color*)malloc(buf->size);
  if(buf->buffer==NULL) {
    fprintf(stderr, "ERROR: Unable to allocate sufficient memory.");
    exit(1);
  }

  buf->pixels = buf->buffer+1;

  write_frame(buf->buffer,0x00,0x00,0x00,0x00);
  write_frame(buf->pixels+leds,0x00,0x00,0x00,0x00);
  write_frame(buf->pixels+leds+1,0x00,0x00,0x00,0x00);
}

int spi_init(int filedes) {
  int ret;
  const uint8_t mode = SPI_MODE_0;
  const uint8_t bits = 8;
  const uint32_t speed = 15000000;

  ret = ioctl(filedes,SPI_IOC_WR_MODE, &mode);
  if(ret==-1) {
    return -1;
  }

  ret = ioctl(filedes,SPI_IOC_WR_BITS_PER_WORD, &bits);
  if(ret==-1) {
    return -1;
  }

  ret = ioctl(filedes,SPI_IOC_WR_MAX_SPEED_HZ,&speed);
  if(ret==-1) {
    return -1;
  }

  return 0;
}

void write_color(tcl_color *p, uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t flag;

  flag = make_flag(red,green,blue);
  write_frame(p,flag,red,green,blue);
}

int send_buffer(int filedes, tcl_buffer *buf) {
  int ret;

  ret = (int)write_all(filedes,buf->buffer,buf->size);
  return ret;
}

void tcl_free(tcl_buffer *buf) {
  free(buf->buffer);
  buf->buffer=NULL;
  buf->pixels=NULL;
}

void write_frame(tcl_color *p, uint8_t flag, uint8_t red, uint8_t green, uint8_t blue) {
  p->flag=flag;
  p->blue=blue;
  p->green=green;
  p->red=red;
}

uint8_t make_flag(uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t flag;

  flag =  (red&0xc0)>>6;
  flag |= (green&0xc0)>>4;
  flag |= (blue&0xc0)>>2;

  return ~flag;
}

ssize_t write_all(int filedes, const void *buf, size_t size) {
  ssize_t buf_len = (ssize_t)size;
  size_t attempt = size;
  ssize_t result;

  while(size>0) {
    result = write(filedes,buf,attempt);
    if(result<0) {
      if(errno==EINTR) continue;
      else if(errno==EMSGSIZE) {
        attempt = attempt/2;
        result = 0;
      }
      else {
        return result;
      }
    }
    buf+=result;
    size-=result;
    if(attempt>size) attempt=size;
  }

  return buf_len;
}


