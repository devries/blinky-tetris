#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include "tclled.h"
#include "hashtable.h"

static const char *device ="/dev/spidev2.0";
// static const char *device="spidev";
static const int nx = 12;
static const int ny = 25;
static const int leds = 1250;
static const int expand_factor=2;
static const unsigned long start_drop_interval=600000L; // microseconds between drops
static const unsigned long delta_drop_interval=30000L; // increment in drop rate
static const unsigned long min_drop_interval=30000L; // fastest drop interval
static const int ROTR = 1<<0;
static const int ROTL = 1<<1;
static const int LEFT = 1<<2;
static const int RIGHT = 1<<3;
static const int DOWN = 1<<4;

/* Will set the following GPIO for controllers:
 * pin 11 = gpio45 (pulldown) = ROTR
 * pin 13 = gpio23 (pulldown) = ROTL
 * pin 15 = gpio47 (pulldown) = LEFT
 * pin 17 = gpio27 (pulldown) = RIGHT
 * pin 19 = gpio22 (pulldown) = DOWN
 */

static int fp_rotr;
static int fp_rotl;
static int fp_left;
static int fp_right;
static int fp_down;

struct tetris_grid {
  int nx;
  int ny;
  char *data;
};

void make_grid(struct tetris_grid *grid, int nx, int ny);
void free_grid(struct tetris_grid *grid);
char get_point(struct tetris_grid *grid, int x, int y);
void set_point(struct tetris_grid *grid, int x, int y, char c);

struct tetromino {
  char color;
  int x[4];
  int y[4];
};

void copy_random_tetromino(struct tetromino **pieces, struct tetromino *returned, int npieces);
struct tetromino **initialize_tetrominos(int *npieces);
void free_tetrominos(struct tetromino **pieces, int npieces);
void rotate_tetromino_right(struct tetromino *piece);
void rotate_tetromino_left(struct tetromino *piece);

void combine_grid(struct tetris_grid *ingrid, struct tetromino *piece, int xoff, int yoff, struct tetris_grid *outgrid);
void copy_grid(struct tetris_grid *source, struct tetris_grid *destination);
int clear_full_rows(struct tetris_grid *grid);
void clear_grid(struct tetris_grid *grid);
unsigned long milliseconds_since(struct timeval *tv);

hashtable *initialize_colors();

void gpio_init();
int get_inputs();

int check_bounds_overlap(struct tetris_grid *grid, struct tetromino *piece, int xoff, int yoff);
void load_grid(struct tetris_grid *grid, tcl_buffer *buf, int expand, hashtable *colorvalues);

int main(int argc, char *argv[]) {
  struct tetris_grid game_grid;
  struct tetris_grid current_grid;
  struct tetromino **pieces;
  struct tetromino current_piece;
  int npieces;
  int ret;
  tcl_buffer buf;
  int fd;
  struct timeval start_time;
  hashtable *colorvalues;
  tcl_color led_color;
  tcl_color *color_p;
  int i, j;
  int xpos, ypos; // Tetromino piece positions
  int new_tetromino_required=1;
  int input_state;
  unsigned long drop_interval;
  int nrows_clear;

  make_grid(&game_grid,nx,ny);
  if(game_grid.data==NULL) {
    fprintf(stderr,"Memory error: game_grid\n");
    exit(1);
  }

  make_grid(&current_grid,nx,ny);
  if(current_grid.data==NULL) {
    fprintf(stderr,"Memory error: current_grid\n");
    exit(1);
  }

  pieces = initialize_tetrominos(&npieces);
  if(pieces==NULL) {
    fprintf(stderr,"Memory error: pieces\n");
    exit(1);
  }

  colorvalues = initialize_colors();

  fd = open(device,O_WRONLY);
  if(fd<0) {
    fprintf(stderr,"Can't open device.\n");
    exit(1);
  }

  ret = spi_init(fd);
  if(ret==-1) {
    fprintf(stderr, "error=%d, %s\n",errno, strerror(errno));
    exit(1);
  }

  tcl_init(&buf,leds);
  // Blank out all the pixels so that borders are black.
  color_p = buf.pixels;
  for(i=0;i<leds;i++) {
    write_color(color_p,0x00,0x00,0x00);
    color_p++;
  }

  // Prepare the io
  gpio_init();

  ret = gettimeofday(&start_time,NULL);
  if(ret==-1) {
    fprintf(stderr, "gettimeofday error: %s\n",strerror(errno));
    exit(1);
  }

  drop_interval=start_drop_interval;

  while(1) {
    ret = gettimeofday(&start_time,NULL);
    if(ret==-1) {
      fprintf(stderr, "gettimeofday error: %s\n",strerror(errno));
      exit(1);
    }
    // Start with a new piece
    if(new_tetromino_required) {
      copy_random_tetromino(pieces,&current_piece,npieces);
      xpos = nx/2;
      ypos = ny-1;
      new_tetromino_required=0;
    }

    while(milliseconds_since(&start_time)<drop_interval) {
      input_state=get_inputs();
      if(input_state&ROTR) {
        rotate_tetromino_right(&current_piece);
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos-=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos+=1;
          ypos-=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          ypos+=2;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          ypos-=1;
          xpos+=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos-=1;
          rotate_tetromino_left(&current_piece);
        }
      }
      else if(input_state&ROTL) {
        rotate_tetromino_left(&current_piece);
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos+=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos-=1;
          ypos-=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          ypos+=2;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          ypos-=1;
          xpos-=1;
        }
        if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos+=1;
          rotate_tetromino_right(&current_piece);
        }
      }
      else if(input_state&RIGHT) {
        xpos+=1;
         if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos-=1;
        }
      }
      else if(input_state&LEFT) {
        xpos-=1;
         if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
          xpos+=1;
        }
      }
      else if(input_state&DOWN) {
        break;
      }
      combine_grid(&current_grid,&current_piece,xpos,ypos,&game_grid);
      load_grid(&game_grid,&buf,expand_factor,colorvalues);
      send_buffer(fd,&buf);
      usleep(100);
    }

    ypos-=1;
    if(check_bounds_overlap(&current_grid,&current_piece,xpos,ypos)) {
      ypos+=1;
      new_tetromino_required=1;
      if(ypos==game_grid.ny-1) {
        usleep(5000000);
        clear_grid(&current_grid);
        drop_interval=start_drop_interval;
      }
      else {
        combine_grid(&current_grid,&current_piece,xpos,ypos,&game_grid);
        nrows_clear=clear_full_rows(&game_grid);
        if(nrows_clear>0 && drop_interval>min_drop_interval) {
          drop_interval-=delta_drop_interval;
        }
        copy_grid(&game_grid,&current_grid);
      }
    }
  }

  free_grid(&game_grid);
  free_grid(&current_grid);
  free_tetrominos(pieces,npieces);
  tcl_free(&buf);
  close(fd);
}

void make_grid(struct tetris_grid *grid, int nx, int ny) {
  int i;
  int points;

  grid->nx=nx;
  grid->ny=ny;

  points = nx*ny;

  grid->data = (char*)malloc(points*sizeof(char));

  for(i=0;i<points;i++) {
    grid->data[i]='x';
  }
}

char get_point(struct tetris_grid *grid, int x, int y) {
  char retchar = 'x';

  if(x>=0 && x<grid->nx && y>=0 && y<grid->ny) {
    retchar = grid->data[x+grid->nx*y];
  }

  return retchar;
}

void set_point(struct tetris_grid *grid, int x, int y, char c) {
  if(x>=0 && x<grid->nx && y>=0 && y<grid->ny) {
    grid->data[x+grid->nx*y]=c;
  }
}

void combine_grid(struct tetris_grid *ingrid, struct tetromino *piece, int xoff, int yoff, struct tetris_grid *outgrid) {
  int i;
  int j;

  for(i=0;i<ingrid->nx;i++) {
    for(j=0;j<ingrid->ny;j++) {
      set_point(outgrid,i,j,get_point(ingrid,i,j));
    }
  }

  for(i=0;i<4;i++) {
    set_point(outgrid,piece->x[i]+xoff,piece->y[i]+yoff,piece->color);
  }
}

void copy_grid(struct tetris_grid *source, struct tetris_grid *destination) {
  int i, j;

  for(i=0;i<source->nx;i++) {
    for(j=0;j<source->ny;j++) {
      set_point(destination,i,j,get_point(source,i,j));
    }
  }
}

int clear_full_rows(struct tetris_grid *grid) {
  int i;
  int j;
  int srow=0;
  int isfull;
  int ret=0;

  for(j=0;j<grid->ny;j++) {
    isfull=1;
    while(isfull) {
      for(i=0;i<grid->nx;i++) {
        if(get_point(grid,i,srow)=='x') {
          isfull=0;
        }
      }
      if(isfull) {
        srow++;
        ret++;
      }
    }
    
    for(i=0;i<grid->nx;i++) {
      if(srow<grid->ny) {
        set_point(grid,i,j,get_point(grid,i,srow));
      }
      else {
        set_point(grid,i,j,'x');
      }
    }
    srow++;
  }

  return ret;
}

void free_grid(struct tetris_grid *grid) {
  free(grid->data);
}

void copy_random_tetromino(struct tetromino **pieces, struct tetromino *returned, int npieces) {
  int piece_num=rand()%npieces;
  int i;

  returned->color=pieces[piece_num]->color;
  for(i=0;i<4;i++) {
    returned->x[i]=pieces[piece_num]->x[i];
    returned->y[i]=pieces[piece_num]->y[i];
  }
}

struct tetromino **initialize_tetrominos(int *npieces) {
  *npieces=7;
  struct tetromino **pieces;

  pieces = (struct tetromino **)malloc((*npieces)*sizeof(struct tetromino *));

  if(pieces==NULL) return pieces;

  /* Flat piece */
  pieces[0] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[0]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[0]->color='c';
  pieces[0]->x[0]=-1;
  pieces[0]->y[0]=0;
  pieces[0]->x[1]=0;
  pieces[0]->y[1]=0;
  pieces[0]->x[2]=1;
  pieces[0]->y[2]=0;
  pieces[0]->x[3]=2;
  pieces[0]->y[3]=0;

  /* Backward L piece */
  pieces[1] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[1]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[1]->color='b';
  pieces[1]->x[0]=-1;
  pieces[1]->y[0]=1;
  pieces[1]->x[1]=-1;
  pieces[1]->y[1]=0;
  pieces[1]->x[2]=0;
  pieces[1]->y[2]=0;
  pieces[1]->x[3]=1;
  pieces[1]->y[3]=0;

  /* L piece */
  pieces[2] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[2]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[2]->color='o';
  pieces[2]->x[0]=1;
  pieces[2]->y[0]=1;
  pieces[2]->x[1]=-1;
  pieces[2]->y[1]=0;
  pieces[2]->x[2]=0;
  pieces[2]->y[2]=0;
  pieces[2]->x[3]=1;
  pieces[2]->y[3]=0;

  /* square piece */
  pieces[3] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[3]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[3]->color='y';
  pieces[3]->x[0]=1;
  pieces[3]->y[0]=1;
  pieces[3]->x[1]=1;
  pieces[3]->y[1]=0;
  pieces[3]->x[2]=0;
  pieces[3]->y[2]=0;
  pieces[3]->x[3]=0;
  pieces[3]->y[3]=1;

  /* S piece */
  pieces[4] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[4]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[4]->color='g';
  pieces[4]->x[0]=1;
  pieces[4]->y[0]=1;
  pieces[4]->x[1]=0;
  pieces[4]->y[1]=1;
  pieces[4]->x[2]=0;
  pieces[4]->y[2]=0;
  pieces[4]->x[3]=-1;
  pieces[4]->y[3]=0;

  /* T piece */
  pieces[5] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[5]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[5]->color='p';
  pieces[5]->x[0]=1;
  pieces[5]->y[0]=0;
  pieces[5]->x[1]=0;
  pieces[5]->y[1]=1;
  pieces[5]->x[2]=0;
  pieces[5]->y[2]=0;
  pieces[5]->x[3]=-1;
  pieces[5]->y[3]=0;

  /* Z piece */
  pieces[6] = (struct tetromino *)malloc(sizeof(struct tetromino));
  if(pieces[6]==NULL) {
    pieces=NULL;
    return pieces;
  }
  pieces[6]->color='r';
  pieces[6]->x[0]=-1;
  pieces[6]->y[0]=0;
  pieces[6]->x[1]=0;
  pieces[6]->y[1]=0;
  pieces[6]->x[2]=0;
  pieces[6]->y[2]=-1;
  pieces[6]->x[3]=1;
  pieces[6]->y[3]=-1;

  return pieces;
}

void free_tetrominos(struct tetromino **pieces, int npieces) {
  int i;

  for(i=0;i<npieces;i++) {
    free(pieces[i]);
  }

  free(pieces);
}

void rotate_tetromino_right(struct tetromino *piece) {
  int i;
  int temp;

  for(i=0;i<4;i++) {
    temp=piece->x[i];
    piece->x[i]=piece->y[i];
    piece->y[i]=-temp;
  }
}

void rotate_tetromino_left(struct tetromino *piece) {
  int i;
  int temp;

  for(i=0;i<4;i++) {
    temp=piece->x[i];
    piece->x[i]=-piece->y[i];
    piece->y[i]=temp;
  }
}

unsigned long milliseconds_since(struct timeval *tv) {
  int ret;
  struct timeval now;
  struct timeval diff;
  unsigned long retval;

  ret = gettimeofday(&now,NULL);
  if(ret==-1) {
    return 0L;
  }

  diff.tv_sec=now.tv_sec-tv->tv_sec;
  diff.tv_usec=now.tv_usec-tv->tv_usec;

  retval = diff.tv_sec*1000000L;
  retval += diff.tv_usec;

  return retval;
}

hashtable *initialize_colors() {
  hashtable *colortable;
  tcl_color led_color;

  colortable = hashtable_create(16,NULL);
  if(!colortable) {
    return NULL;
  }

  /* black = x */
  write_color(&led_color,0x00,0x00,0x00);
  hashtable_insert(colortable,&(char){'x'},sizeof(char),&led_color,sizeof(led_color));

  /* cyan = c */
  write_color(&led_color,0x00,0x8b,0x8b);
  hashtable_insert(colortable,&(char){'c'},sizeof(char),&led_color,sizeof(led_color));

  /* blue = b */
  write_color(&led_color,0x00,0x00,0xff);
  hashtable_insert(colortable,&(char){'b'},sizeof(char),&led_color,sizeof(led_color));

  /* orange = o */
  write_color(&led_color,0xff,0x60,0x00);
  hashtable_insert(colortable,&(char){'o'},sizeof(char),&led_color,sizeof(led_color));

  /* yellow = y */
  write_color(&led_color,0xff,0xb0,0x00);
  hashtable_insert(colortable,&(char){'y'},sizeof(char),&led_color,sizeof(led_color));

  /* green = g */
  write_color(&led_color,0x00,0x80,0x00);
  hashtable_insert(colortable,&(char){'g'},sizeof(char),&led_color,sizeof(led_color));

  /* purple = p */
  write_color(&led_color,0x55,0x28,0xd0);
  hashtable_insert(colortable,&(char){'p'},sizeof(char),&led_color,sizeof(led_color));

  /* red = r */
  write_color(&led_color,0xff,0x00,0x00);
  hashtable_insert(colortable,&(char){'r'},sizeof(char),&led_color,sizeof(led_color));

  return colortable;
}

int check_bounds_overlap(struct tetris_grid *grid, struct tetromino *piece, int xoff, int yoff) {
  int i;
  int retval=0;

  for(i=0;i<4;i++) {
    // Check bounds
    if(xoff+piece->x[i] >= grid->nx || xoff+piece->x[i]<0 || yoff+piece->y[i]<0) {
      retval=1;
    }
    // Check overlap
    else if(get_point(grid,xoff+piece->x[i],yoff+piece->y[i])!='x') {
      retval=1;
    }
  }

  return retval;
}

void gpio_init() {
  FILE *fp;

  fp = fopen("/sys/class/gpio/export","w");
  fprintf(fp,"45");
  fflush(fp);
  rewind(fp);
  fprintf(fp,"23");
  fflush(fp);
  rewind(fp);
  fprintf(fp,"47");
  fflush(fp);
  rewind(fp);
  fprintf(fp,"27");
  fflush(fp);
  rewind(fp);
  fprintf(fp,"22");
  fflush(fp);
  fclose(fp);

  fp_rotr = open("/sys/class/gpio/gpio45/value",O_RDONLY);
  fp_rotl = open("/sys/class/gpio/gpio23/value",O_RDONLY);
  fp_left = open("/sys/class/gpio/gpio47/value",O_RDONLY);
  fp_right = open("/sys/class/gpio/gpio27/value",O_RDONLY);
  fp_down = open("/sys/class/gpio/gpio22/value",O_RDONLY);
}

int get_inputs() {
  static int rotr_enabled=1;
  static int rotl_enabled=1;
  static int left_enabled=1;
  static int right_enabled=1;
  static int down_enabled=1;
  int ret=0;
  char gpio_val;

  if(read(fp_rotr,&gpio_val,1)>0) {
    if(gpio_val=='1' && rotr_enabled) {
      ret |= ROTR;
      rotr_enabled=0;
    }
    else if(gpio_val=='0') {
      rotr_enabled=1;
    }
    lseek(fp_rotr,0,SEEK_SET);
  }

  if(read(fp_rotl,&gpio_val,1)>0) {
    if(gpio_val=='1' && rotl_enabled) {
      ret |= ROTL;
      rotl_enabled=0;
    }
    else if(gpio_val=='0') {
      rotl_enabled=1;
    }
    lseek(fp_rotl,0,SEEK_SET);
  }

  if(read(fp_left,&gpio_val,1)>0) {
    if(gpio_val=='1' && left_enabled) {
      ret |= LEFT;
      left_enabled=0;
    }
    else if(gpio_val=='0') {
      left_enabled=1;
    }
    lseek(fp_left,0,SEEK_SET);
  }

  if(read(fp_right,&gpio_val,1)>0) {
    if(gpio_val=='1' && right_enabled) {
      ret |= RIGHT;
      right_enabled=0;
    }
    else if(gpio_val=='0') {
      right_enabled=1;
    }
    lseek(fp_right,0,SEEK_SET);
  }

  if(read(fp_down,&gpio_val,1)>0) {
    if(gpio_val=='1' && down_enabled) {
      ret |= DOWN;
      down_enabled=0;
    }
    else if(gpio_val=='0') {
      down_enabled=1;
    }
    lseek(fp_down,0,SEEK_SET);
  }

  return ret;
}

void load_grid(struct tetris_grid *grid, tcl_buffer *buf, int expand, hashtable *colorvalues) {
  int i, j;
  int x, y;
  tcl_color *p;
  tcl_color *source;
  int jstart, jstop, jint;
  char pix_color;

  // We will go in order of pixels in screen
  p = buf->pixels;

  for(i=24;i>=0;i--) {
    if(i%2==0) {
      jstart=0;
      jstop=50;
      jint=1;
    }
    else {
      jstart=49;
      jstop=-1;
      jint=-1;
    }
    for(j=jstart;j!=jstop;j+=jint) {
      x=i/expand;
      y=j/expand;
      if(x<grid->nx && y<grid->ny) {
        pix_color = get_point(grid,x,y);
        source = hashtable_get(colorvalues,&pix_color,sizeof(char));
      }
      else {
        source = hashtable_get(colorvalues,&(char){'x'},sizeof(char));
      }
      memcpy(p,source,sizeof(tcl_color));

      p++;
    }
  }
}

void clear_grid(struct tetris_grid *grid) {
  int i;
  int j;

  for(i=0;i<grid->nx;i++) {
    for(j=0;j<grid->ny;j++) {
      set_point(grid,i,j,'x');
    }
  }
}
