/*
 * wincurses.h
 * Justin J. Meza
 * 2007-2011
 *
 * You are free to do as you please with this source code so long as you
 * make any changes to it freely available.
 *
 * I, the original author, am not responsible for any malfunctions as a result
 * of this source code or any of its derivations (mine or otherwise).
 *
 * Please report any bugs/fixes/improvements.
 *
 * (See <http://github.com/justinmeza/wincurses> for more information.)
 */

#ifndef __WINCURSES__
#define __WINCURSES__

#include <windows.h>
#include <conio.h> /* For 'kbhit' */
#include <stdio.h>
#include <stdarg.h>

#define EOF -1
#define ERR 0
#define FALSE 0
#define OK 1
#define TRUE 1
/*
#define WEOF ???
#define _XOPEN_CURSES
*/

#define HAS_COLORS TRUE
#define CAN_CHANGE_COLOR FALSE
#define NUM_COLORS 8
#define COLOR_BITS 6     /* Must be a multiple of two */
#define MAX_NUM_PAIRS 64 /* Should be 2^COLOR_BITS and ideally NUM_COLORS^2. */

typedef int bool;


/* Background character (fills the screen when cleared) */
#define WC_BGND ' '

/* Global program flags */
typedef enum _flags {
	_WC_ECHO = 0,
	_WC_COLOR,
} _flags_t;

/* Global program flag bit masks */
#define WC_ECHO (1 << _WC_ECHO)
#define WC_COLOR (1 << _WC_COLOR)

/*
 * COLOR DATA LAYOUT
 *
 * High-order 'COLOR_BITS' bits are an index into color_pairs array which
 * stores foreground and background color data.
 * 
 * attrs -> XXXXXX00000000000000000000000000
 *          \____/ COLOR_BITS = 6
 *            |
 *            +------------------------------------+
 *                                                 |
 *                                                 v
 *                +----------+----------+-\     \-+----------+-\     \------------+
 * color_pairs -> |.f=X, .b=Y|.f=X, .b=Y| / ... / |.f=X, .b=Y| / ... / |.f=X, .b=Y|
 *                +----------+----------+-\     \-+----------+-\     \-+----------+
 *                 0          1                    XXXXXX               2^COLOR_BITS
 */

/* Color type */
typedef struct color {
	short r;
	short g;
	short b;
} color_t;

/* Color information type */
typedef struct colorinfo {
	short f;
	short b;
} colorinfo_t;

/*
 * Shifts out the attribute bits associated to color data. (Used for acquiring
 * an index into 'color_pairs'.
 */
#define A_COLOR_DATA(attrs) (attrs >> (32 - COLOR_BITS))

/* Convert from Windows foreground to background bitmasks */
#define FTOB(f) (f << 4)

/* Window-specific flags */
typedef enum _wflags {
	_WC_WKEYPAD = 0, /* KEY_* translations */
	_WC_NODELAY      /* 'No delay' mode */
} _wflags_t;

/* Window-specific flag bit masks */
#define WC_WKEYPAD (1 << _WC_WKEYPAD)
#define WC_NODELAY (1 << _WC_NODELAY)

/* Attributes */
typedef enum _attr {
	_A_ALTCHARSET = 0, /* Alternate character set */
	_A_BLINK,          /* Blinking */
	_A_BOLD,           /* Extra bright or bold */
	_A_DIM,            /* Half bright */
	_A_INVIS,          /* Invisible */
	_A_PROTECT,        /* Protected */
	_A_REVERSE,        /* Reverse video */
	_A_STANDOUT,       /* Best highlighting mode of the terminal */
	_A_UNDERLINE,      /* Underlining */
	/* Reserve COLOR_BITS higher-order bits for color information */
} _attr_t;

/* Attribute bit masks */
typedef enum attr {
	A_ALTCHARSET = (1 << _A_ALTCHARSET),
	A_BLINK      = (1 << _A_BLINK),
	A_BOLD       = (1 << _A_BOLD),
	A_DIM        = (1 << _A_DIM),
	A_INVIS      = (1 << _A_INVIS),
	A_PROTECT    = (1 << _A_PROTECT),
	A_REVERSE    = (1 << _A_REVERSE),
	A_STANDOUT   = (1 << _A_STANDOUT),
	A_UNDERLINE  = (1 << _A_UNDERLINE),
} attr_t;


/* Boolean type */
typedef int bool;

/* Color codes */
typedef enum colorcode {
	COLOR_BLACK = 0,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_MAGENTA,
	COLOR_YELLOW,
	COLOR_WHITE
} colorcode_t;

/* Color value */
color_t colors[NUM_COLORS];

/* Key codes, mapped to Windows key codes when applicable */
typedef enum keycode {
	KEY_CODE_YES = 256,         /* Variable contains a key code */
	KEY_BREAK,                  /* Break key */
	KEY_DOWN,                   /* Down arrow key */
	KEY_UP,                     /* Up arrow key */
	KEY_LEFT,                   /* Left arrow key */
	KEY_RIGHT,                  /* Right arrow key */
	KEY_HOME,                   /* Home key */
	KEY_BACKSPACE,              /* Backspace key */
	KEY_F0,                     /* Function key 0 */
/*	KEY_F(n)*/                  /* (See definition, below) */
	KEY_DL = KEY_F0 + 64,       /* Delete line */
	KEY_IL,                     /* Insert line */
	KEY_DC,                     /* Delete character */
	KEY_IC,                     /* Insert character */
	KEY_EIC,                    /* Exit insert char mode */
	KEY_CLEAR,                  /* Clear screen */
	KEY_EOS,                    /* Clear to end of screen */
	KEY_EOL,                    /* Clear to end of line */
	KEY_SF,                     /* Scroll one line forward */
	KEY_SR,                     /* Scroll one line backward (reverse) */
	KEY_NPAGE,                  /* Next page */
	KEY_PPAGE,                  /* Previous page */
	KEY_STAB,                   /* Set tab */
	KEY_CTAB,                   /* Clear tab */
	KEY_CATAB,                  /* Clear all tabs */
	KEY_ENTER,                  /* Enter or send */
	KEY_SRESET,                 /* Soft (partial) reset */
	KEY_RESET,                  /* Reset or hard reset */
	KEY_PRINT,                  /* Print or copy */
	KEY_LL,                     /* Home down or bottom */
	KEY_A1,                     /* Upper left of keypad */
	KEY_A3,                     /* Upper right of keypad */
	KEY_B2,                     /* Center of keypad */
	KEY_C1,                     /* Lower left of keypad */
	KEY_C3,                     /* Lower right of keypad */
/*
 * Virtual keypad arrangement:
 *
 * +-------+-------+-------+
 * |  A1   |  UP   |  A3   |
 * +-------+-------+-------+
 * | LEFT  |  B2   | RIGHT |
 * +-------+-------+-------+
 * |  C1   | DOWN  |  C3   |
 * +-------+-------+-------+
 *
 */
	KEY_BTAB,                   /* Back tab key */
	KEY_BEG,                    /* Beginning key */
	KEY_CANCEL,                 /* Cancel key */
	KEY_CLOSE,                  /* Close key */
	KEY_COMMAND,                /* Cmd (command) key */
	KEY_COPY,                   /* Copy key */
	KEY_CREATE,                 /* Create key */
	KEY_END,                    /* End key */
	KEY_EXIT,                   /* Exit key */
	KEY_FIND,                   /* Find key */
	KEY_HELP,                   /* Help key */
	KEY_MARK,                   /* Mark key */
	KEY_MESSAGE,                /* Message key */
	KEY_MOVE,                   /* Move key */
	KEY_NEXT,                   /* Next object key */
	KEY_OPEN,                   /* Open key */
	KEY_OPTIONS,                /* Options key */
	KEY_PREVIOUS,               /* Previous object key */
	KEY_REDO,                   /* Redo key */
	KEY_REFERENCE,              /* Reference key */
	KEY_REFRESH,                /* Refresh key */
	KEY_REPLACE,                /* Replace key */
	KEY_RESTART,                /* Restart key */
	KEY_RESUME,                 /* Resume key */
	KEY_SAVE,                   /* Save key */
	KEY_SBEG,                   /* Shifted beginning key */
	KEY_SCANCEL,                /* Shifted cancel key */
	KEY_SCOMMAND,               /* Shifted command key */
	KEY_SCOPY,                  /* Shifted copy key */
	KEY_SCREATE,                /* Shifted create key */
	KEY_SDC,                    /* Shifted delete character key */
	KEY_SDL,                    /* Shifted delete line key */
	KEY_SELECT,                 /* Select key */
	KEY_SEND,                   /* Shifted end key */
	KEY_SEOL,                   /* Shifted clear line key */
	KEY_SEXIT,                  /* Shifted exit key */
	KEY_SFIND,                  /* Shifted find key */
	KEY_SHELP,                  /* Shifted help key */
	KEY_SHOME,                  /* Shifted home key */
	KEY_SIC,                    /* Shifted input key */
	KEY_SLEFT,                  /* Shifted left key */
	KEY_SMESSAGE,               /* Shifted message key */
	KEY_SMOVE,                  /* Shifted move key */
	KEY_SNEXT,                  /* Shifted next key */
	KEY_SOPTIONS,               /* Shifted options key */
	KEY_SPREVIOUS,              /* Shifted previous key */
	KEY_SPRINT,                 /* Shifted print key */
	KEY_SREDO,                  /* Shifted redo key */
	KEY_SREPLACE,               /* Shifted replace key */
	KEY_SRIGHT,                 /* Shifted right arrow */
	KEY_SRSUME,                 /* Shifted resume key */
	KEY_SSAVE,                  /* Shifted save key */
	KEY_SSUSPEND,               /* Shifted suspend key */
	KEY_SUNDO,                  /* Shifted undo key */
	KEY_SUSPEND,                /* Suspend key */
	KEY_UNDO                    /* Undo key */
} keycode_t;

/* For function key definitions */
#define KEY_F(n) (KEY_F0 + n)

/* Character type */
typedef char chtype;

/* Global program flags type */
typedef unsigned int flags_t;

/* Window-specific flags type */
typedef unsigned int wflags_t;

/* Window type */
typedef struct window_t
{
	/* Window region and size */
	SMALL_RECT rect;
	COORD size;

	/* Coordinate of cursor */
	COORD cur;

	/*
	 * Two handles to console screen buffers. (Only one is active at a time,
	 * while the other is used as a back buffer.)
	 */
	HANDLE hcon[2];

	/*
	 * The indices of the console screen buffer being used as the primary and
	 * back buffer.
	 */
	int prim, bbuf;

	/* Window-specific flags */
	wflags_t flags;

	/* Window-specific attributes */
	WORD attrs;
} WINDOW;


/*
 * Stdscr is the 'standard screen' (similar to stdin, stdout, and stderr). It is
 * essentially the window representing the console.
 */
WINDOW *stdscr;

/* Handle for restoring standard output later on */
HANDLE hstdout;

/* Handle to input buffer */
HANDLE hstdin;

/* Standard console cursor size */
DWORD cursize;

/* Program flags */
flags_t flags;

/* The number of lines and columns the terminal supports */
int LINES;
int COLS;

/* The number of colors a terminal supports */
int COLORS;

/* The number of color pairs available for use */
int COLOR_PAIRS;

/* Color pair data */
colorinfo_t color_pairs[MAX_NUM_PAIRS];


/* Wincurses-specific declarations */

WINDOW *initscr(void);
int refresh(void);
int endwin(void);

int addch(const chtype ch);
int mvaddch(int y, int x, const chtype ch);
int mvwaddch(WINDOW *win, int y, int x, const chtype ch);
int waddch(WINDOW *win, const chtype ch);

int printw(char *fmt, ...);
int mvprintw(int y, int x, char *fmt, ...);
int mvwprintw(WINDOW *win, int y, int x, char *fmt, ...);
int wprintw(WINDOW *win, char *fmt, ...);

int attron(int attrs);
int attroff(int attrs);
int attrset( int attrs);

int wattron(WINDOW *win, int attrs);
int wattroff(WINDOW *win, int attrs);
int wattrset(WINDOW *win, int attrs);

int getch(void);
int mvgetch(int y, int x);
int mvwgetch(WINDOW *win, int y, int x);
int wgetch(WINDOW *win);

int echo(void);
int noecho(void);

int keypad(WINDOW *win, bool bf);
int cbreak(void);
int nocbreak(void);
int raw(void);
int noraw(void);

int move(int y, int x);
int wmove(WINDOW *win, int y, int x);

int nodelay(WINDOW *win, bool bf);

bool can_change_color(void);
int color_content(short color, short *red, short *green, short *blue);
int COLOR_PAIR(int n);
bool has_colors(void);
int init_color(short color, short red, short green, short blue);
int init_pair(short pair, short f, short b);
int pair_content(short pair, short *f, short *b);
int PAIR_NUMBER(int value);
int start_color(void);

int curs_set(int visibility);


/* Windows-specific helper function declarations */

void cls(HANDLE hcon);
int setmode(HANDLE hcon, DWORD bmask);
int clearmode(HANDLE hcon, DWORD bmask);
int va_wprintw(WINDOW *win, char *fmt, va_list *args);
WORD getattrs(unsigned int attrs);


#endif /* __WINCURSES__ */
