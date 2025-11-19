#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>
#include<stdio.h>
#include<sys/ioctl.h>
#define CTRL_KEY(k) ((k) & 0x1f)
#define AUBF_INIT {NULL, 0}
// ------------data----------

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT ,
  ARROW_UP,
  ARROW_DOWN
};
struct editorConfig{
  int cx, cy;
  int screenRows;
  int screenCols;
  struct termios orig_termios;
};
struct editorConfig E;

//create a copy of the fd struct frpm the terminql
//-------terminal-------------
void die(char *s){
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}
void diable_raw_mode(){
  if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
    die("tcgetattr");
  }

}
void enable_raw_mode(){
  if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1){
    die("tcgetattr");
  }
  atexit(diable_raw_mode);
  // create a copy of the original terminos struct to mke changes locally 
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~( BRKINT | INPCK | ISTRIP |IXON | ICRNL);
  raw.c_cflag |= ~(CS8);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG |IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1){

    die("tcsetattr");
  }
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;

  if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    return -1;
  }
  else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}
//-------------input-----------------
struct aubf{ // stands for appending the buffer
  char *b;
  int len;
};// the struct acts as a buffer


void abAppend(struct aubf *ab, const char *s, int len){
  char *new = realloc(ab->b, ab->len+len);
  // this acts as an buffer so we are increasing theuffer size by len = oldlen+newlen
  if(new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct aubf *ab){
  free(ab->b);
}
int editorReadKey(){
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1))!=1){
    if(nread == -1 && errno != EAGAIN) die("read");
  }
  if(c == '\x1b'){
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }
    return '\x1b';
  }
  else{
  return c;
  }
}

void editorMoveCursor(int c){
  switch(c){
    case ARROW_UP: E.cy--;
    break;
    case ARROW_LEFT: E.cx--;

    break;
    case ARROW_DOWN: E.cy++;
    break;
    case ARROW_RIGHT: E.cx++;
    break;
  }
}

void editorProcessKeyPress(){
  int c= editorReadKey();
  switch (c) {
    case CTRL_KEY('q'):
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    editorMoveCursor(c);
    break;
  }

}

//------------output-----------------

void editorDrawRows(struct aubf *ab){
  int y;
  for (y = 0; y < E.screenRows; y++) {
    if (y == E.screenRows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
        "Kilo editor -- version v1");
      if (welcomelen > E.screenCols) welcomelen = E.screenCols;
      int padding = (E.screenCols - welcomelen) / 2;
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--) abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    } else {
      abAppend(ab, "~", 1);
    }
    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenRows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
  }

void editorRefreshScreen(){
  struct aubf ab = AUBF_INIT;
abAppend(&ab, "\x1b[?25l", 6); // clear the cursor
  abAppend(&ab, "\x1b[H", 3) ; //move the cursor to th top left
  editorDrawRows(&ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy+1, E.cx+1);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6); //get the cursor to br in the screen again
  write(STDIN_FILENO, ab.b, ab.len);
}

//------------init-----------------
//

void initEditor(){
  E.cx = 0;
  E.cy = 0;
  if(getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize0");
}

int main(){
  enable_raw_mode();
  initEditor();
  //read input from the terminal
      //control charecterare non printable
      //the above function is from ctype.h is a function to check if the charecter is a control charecter
  
     while (1) {
    editorRefreshScreen();
     editorProcessKeyPress();
  }
  return 0;
}
