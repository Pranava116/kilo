
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L
#include<unistd.h>
#include<termios.h>
#include<sys/types.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>
#include<stdio.h>
#include<sys/ioctl.h>
#define CTRL_KEY(k) ((k) & 0x1f)
#define AUBF_INIT {NULL, 0}
// ------------data----------
typedef struct erow{
	int size;
	char *chars;

}erow;
enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT ,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DEL_KEY
};
struct editorConfig{
  int cx, cy;
  int screenRows;
  int screenCols;
  int numrows;
  erow *row;
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
void editorAppendRow(char *s, size_t len){
	  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  int at = E.numrows;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.numrows++;
}
void editorOpen(char *filename){
	FILE *fp = fopen(filename, "r");
	if(!fp) die("fileopen");
	char *line = NULL;
	ssize_t linecap = 0;
	ssize_t linelen;
		while( (linelen = getline(&line, &linecap, fp))!= -1){
		while(linelen>0 && (line[linelen-1] == '\n' || line[linelen -1] == '\r'))
		       linelen--; 	
	
		       editorAppendRow(line, linelen);
	}
	free(line);
	free(fp);	
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
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
		case '1': return HOME_KEY;
		case '3': return DEL_KEY;
		case '4': return END_KEY;
		case '8': return END_KEY;
		case '7': return HOME_KEY;
            	case '5': return PAGE_UP;
            	case '6': return PAGE_DOWN;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
	
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    }
      else if(seq[0] == '0'){
      	switch(seq[1]){
		case 'H': return HOME_KEY;
		case 'F': return END_KEY;
	}
}
    return '\x1b';
  } else {
    return c;
  }
}

void editorMoveCursor(int key){
  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (E.cx != E.screenCols - 1) {
        E.cx++;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      if (E.cy != E.screenRows - 1) {
        E.cy++;
      }
      break;
  }
}
void editorProcessKeyPress(){
int c = editorReadKey();
printf("KEy : %d\r\n" ,c);
  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    case HOME_KEY:
      E.cx = 0;
      break;
    case END_KEY:
      E.cx = E.screenCols -1;
     break;
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      {
	      int times = E.screenRows;
	      while(times--){
		      editorMoveCursor(c == PAGE_UP?ARROW_UP:ARROW_DOWN);
	      }
	      break;
      }
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
	  if(y >= E.numrows){
    if (y == E.screenRows / 3) {
	    if(E.numrows == 0&& E.screenRows/3){
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
    }
	  }
	  else{
		  int len = E.row[y].size;
		  if(len>E.screenCols) len = E.screenCols;
		  abAppend(ab, E.row[y].chars, len);
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
  E.numrows = 0;
  E.row = NULL;
  if(getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize0");
}

int main(int argc, char *argv[]){
  enable_raw_mode();
  initEditor();

  if(argc >= 2){
  	editorOpen(argv[1]);
  }
  //read input from the terminal
      //control charecterare non printable
      //the above function is from ctype.h is a function to check if the charecter is a control charecter
  
     while (1) {
    editorRefreshScreen();
     editorProcessKeyPress();
  }
  return 0;
}
