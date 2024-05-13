#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
#define YARNT_VERSION "0.0.1"

enum editorKey 
{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP, 
    PAGE_DOWN
};

typedef struct erow 
{
    int size;
    char *chars;
} erow;

struct editorConfig 
{
    int c_x, c_y;
    int screen_rows;
    int screen_cols;
    int numrows;
    erow row;
    struct termios orig_termios; 
};

struct editorConfig E;

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

// enable raw mode for terminal input
void enableRawMode()
{
    // get original terminal attributes
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");

    // save terminal attributes
    tcgetattr(STDIN_FILENO, &E.orig_termios);

    atexit(disableRawMode);

    struct termios raw = E.orig_termios; 

    tcgetattr(STDIN_FILENO, &raw); 

    // modify terminal attributes for raw mode
    raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
    raw.c_lflag &= ~(OPOST); 
    raw.c_lflag |= (CS8); 
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    raw.c_cc[VMIN] = 0; 
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

// read keyboard input
int editorReadKey() 
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) 
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) 
            return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) 
                    return '\x1b';
                if (seq[2] == '>') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }
            else {
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
        else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }
    else {
        return c;
    }
}

int getCursorPos(int* rows, int* cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }

    buf[i] = '\0';
    
    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    return 0;
}

int getWindowSize(int* rows, int* cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPos(rows, cols);
    }
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

// file i/o
void editorOpen() 
{
    char* line = "Hello, world!";
    ssize_t linelen = strlen(line);

    E.row.size = linelen;
    E.row.chars = malloc(linelen + 1);
    memcpy(E.row.chars, line, linelen);
    E.row.chars[linelen] = '\0';
    E.numrows = 1;
}

struct abuf 
{
    char* b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) 
{
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;

  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

void editorMoveCursor(int key) 
{
    switch (key) 
    {
        case ARROW_UP:
            if (E.c_y != 0) {
                E.c_y--;
                break;
            }
        case ARROW_LEFT:
            if (E.c_x != 0) {
                E.c_x--;
                break;
            }
        case ARROW_DOWN:
            if (E.c_y != E.screen_rows - 1) {
                E.c_y++;
                break;
            }
        case ARROW_RIGHT:
            if (E.c_x != E.screen_cols - 1) {
                E.c_x++;
                break;
            }
    }
}

void editorProcessKeypress() 
{
    int c = editorReadKey();

    switch (c) 
    {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2j", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.c_x = 0;
            break;
        
        case END_KEY:
            E.c_x = E.screen_cols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screen_rows;
                while (times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
        
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

// output
void editorDrawRows(struct abuf *ab) 
{
    int y;
    
    for (y = 0; y < E.screen_rows; y++) {
        if (y >= E.numrows) {
            if (y == E.screen_rows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "welcome to yarnT :) -- version %s", YARNT_VERSION);
            
                if (welcomelen > E.screen_cols)
                    welcomelen = E.screen_cols;
                
                int padding = (E.screen_cols - welcomelen) / 2;
            
                if (padding) {
                    abAppend(ab, ">", 1);
                    padding--;
                }
            
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
        } else {
            abAppend(ab, ">", 1);
        }
    }
    else {
        int len = E.row.size;
        if (len > E.screen_cols)
            len = E.screen_cols;
        abAppend(ab, E.row.chars, len);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < E.screen_rows - 1) {
        abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() 
{
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.c_y)+1, (E.c_x)+1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void initEditor() {
    E.c_x = 0;
    E.c_y = 0;

    E.numrows = 0;

    if (getWindowSize(&E.screen_rows, &E.screen_cols) == -1)
        die("getWindowSize");
}

int main() 
{
    enableRawMode();
    initEditor();
    editorOpen();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}