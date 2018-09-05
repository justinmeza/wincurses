/*
 * wincurses.c
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
 *
 */

#include "wincurses.h"


/******************************************************************************
 *
 * initscr
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Initializes the curses environment. Among other things, it
 *                 allocates memory for 'stdscr', sets 'stdscr' up with its
 *                 default values, and creates the primary and back buffers.
 *
 * RETURN VALUE:   If successful, returns a pointer to 'stdscr' -- the default
 *                 window. Otherwise, an 'exit' call is made and control is
 *                 never returned to the caller.
 *
 * NOTES:          Make sure to call 'endwin' when done! This will free all of
 *                 the allocated memory and return the console to the same
 *                 state it was in before the call to 'initscr'.
 *
 *****************************************************************************/

WINDOW *initscr(void)
{
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	CONSOLE_CURSOR_INFO curinfo;

	/* Allocate memory for a new window for stdscr */
	stdscr = malloc(sizeof(WINDOW));

	/* Sanity check: successfully allocated a screen */
	if (!stdscr) exit(1);

	/* Set up our primary and back buffer indices */
	stdscr->bbuf = !(stdscr->prim = 0);

	/* Get and set up console information */
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo))
		exit(1);
	stdscr->rect = coninfo.srWindow;
	stdscr->size.Y = stdscr->rect.Bottom - stdscr->rect.Top + 1;
	stdscr->size.X = stdscr->rect.Right - stdscr->rect.Left + 1;
	LINES = stdscr->size.Y;
	COLS = stdscr->size.X;

	/* Set cursor position to origin (0, 0) */
	stdscr->cur.Y = stdscr->cur.X = 0;

	/* Create our console buffers */
	stdscr->hcon[stdscr->prim] = CreateConsoleScreenBuffer(
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	/* Sanity check: console buffer successfully created */
	if (stdscr->hcon[stdscr->prim] == INVALID_HANDLE_VALUE) exit(1);

	stdscr->hcon[stdscr->bbuf] = CreateConsoleScreenBuffer(
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	/* Sanity check: console buffer successfully created */
	if (stdscr->hcon[stdscr->bbuf] == INVALID_HANDLE_VALUE) exit(1);

	/* Set the size of our buffers to match the window size */
	if (!SetConsoleScreenBufferSize(stdscr->hcon[stdscr->prim], stdscr->size))
		exit(1);
	if (!SetConsoleScreenBufferSize(stdscr->hcon[stdscr->bbuf], stdscr->size))
		exit(1);

	/* Clear our buffers */
	cls(stdscr->hcon[stdscr->prim]);
	cls(stdscr->hcon[stdscr->bbuf]);

	/* Save a handle to our (soon to be) old console buffer */
	hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	/* Sanity check: able to get handle */
	if (hstdout == INVALID_HANDLE_VALUE) exit(1);

	/* Grab our standard cursor size */
	GetConsoleCursorInfo(hstdout, &curinfo);
	cursize = curinfo.dwSize;

	/* Get a handle to our input buffer */
	hstdin = GetStdHandle(STD_INPUT_HANDLE);

	/* Sanity check: able to get handle */
	if (hstdin == INVALID_HANDLE_VALUE) exit(1);

	/* Set any window attributes initially to grey text, black background */
	stdscr->attrs = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

	/* Start with a clean console mode */
	if (!SetConsoleMode(hstdin, 0)) exit(1);

	/* Set the primary buffer as the current console screen buffer */
	if (!SetConsoleActiveScreenBuffer(stdscr->hcon[stdscr->prim])) exit(1);

	/* Return the default window (stdscr) */
	return stdscr;
}


/******************************************************************************
 *
 * refresh
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Updates the console by swapping the primary and back
 *                 buffers.
 *
 * RETURN VALUE:   Returns OK on success or ERR on failure.
 *
 *****************************************************************************/

int refresh(void)
{
	CHAR_INFO *tbuf;
	COORD size, orig;

	/* Set the back buffer as the current console screen buffer */
	SetConsoleActiveScreenBuffer(stdscr->hcon[stdscr->bbuf]);

	/* Move our cursor to the correct position */
	SetConsoleCursorPosition(stdscr->hcon[stdscr->bbuf], stdscr->cur);

	/*
	 * Copy the contents of the back buffer to the primary buffer for
	 * persistency. (Remember: the buffers are swapped IN APPEARANCE at this
	 * point, but not IN NOTATION. Basically, we are currently looking at
	 * hcon[bbuf] from the console, so we want to copy from hcon[bbuf] to
	 * hcon[prim], which is our new back buffer (even though we haven't updated
	 * the values of 'bbuf' and 'prim' yet).
	 */

	/* Allocate enough memory for the temporary buffer */
	tbuf = malloc(sizeof(CHAR_INFO) * stdscr->size.Y * stdscr->size.X);

	/* We want to start at (0, 0) and have size (Y, X) */
	orig.Y = orig.X = 0;
	size.Y = stdscr->size.Y;
	size.X = stdscr->size.X;

	/* Read the output from stdscr */
	ReadConsoleOutput(stdscr->hcon[stdscr->bbuf], tbuf, size, orig,
			&stdscr->rect);

	/* Copy from the temporary buffer to the primary buffer */
	WriteConsoleOutput(stdscr->hcon[stdscr->prim], tbuf, size, orig,
			&stdscr->rect);

	/* Free our temporary buffer */
	free(tbuf);

	/* 'Swap' the primary and back buffers by swapping their values */
	stdscr->bbuf = !(stdscr->prim = !stdscr->prim);

	return OK;
}


/******************************************************************************
 *
 * endwin
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Ends a wincurses session. Namely, 'endwin' performs any
 *                 necessary cleanup and restores the console to its initial
 *                 condition.
 *
 * RETURN VALUE:   Returns OK on success or ERR on failure.
 *
 * NOTE:           'endwin' should push current buffer contents to console and
 *                 restore previous console session, but NOT clean up after
 *                 'stdscr'.
 *
 *****************************************************************************/

int endwin(void)
{
	/* Restore our saved stdout */
	SetConsoleActiveScreenBuffer(hstdout);

	/* TODO: Don't clean up after stdscr */
	/* Close the handle to our buffers */
	CloseHandle(stdscr->hcon[stdscr->prim]);
	CloseHandle(stdscr->hcon[stdscr->bbuf]);

	/* Free the stdscr pointer */
	free(stdscr);

	return OK;
}


/******************************************************************************
 *
 * addch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Adds the character 'ch' to 'stdscr' starting at the current
 *                 cursor location and increments the cursor location when done
 *                 to the location immediately following the written character.
 *
 *                 The cursor is advanced to the beginning of the next line if,
 *                 after writing the character, the cursor would run past the
 *                 end of a row in the console buffer.
 *
 * RETURN VALUE:   Returns OK if the character was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int addch(const chtype ch)
{
	return waddch(stdscr, ch);
}


/******************************************************************************
 *
 * mvaddch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on 'stdscr' to ('y', 'x'), adds a
 *                 character at that position, and increments the cursor
 *                 location when done to the location immediately following the
 *                 written character.
 *
 *                 (See 'addch' for more information.)
 *
 * RETURN VALUE:   Returns OK if the character was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int mvaddch(int y, int x, const chtype ch)
{
	return mvwaddch(stdscr, y, x, ch);
}


/******************************************************************************
 *
 * mvwaddch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on 'win' to ('y', 'x'), adds a
 *                 character at that position, and increments the cursor
 *                 location when done to the location immediately following the
 *                 written character.
 *
 *                 (See 'addch' for more information.)
 *
 * RETURN VALUE:   Returns OK if the character was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
	if (wmove(win, y, x) == ERR)
		return ERR;
	return waddch(win, ch);
}


/******************************************************************************
 *
 * waddch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Adds the character 'ch' to 'win' starting at the current
 *                 cursor location and increments the cursor location when done
 *                 to the location immediately following the written character.
 *
 *                 (See 'addch' for more information.)
 *
 * RETURN VALUE:   Returns OK if the character was successfully written.
 *                 Otherwise, ERR is returned.
 *
 * NOTES:          Handle 'special' characters (backspace, etc.)
 *
 *****************************************************************************/

int waddch(WINDOW *win, const chtype ch)
{
	CHAR_INFO c;
	COORD size = {1, 1};
	COORD origin = {0, 0};
	SMALL_RECT rect = {0, 0, 0, 0};

	/* Sanity check: NULL pointer */
	if (!win) return ERR;

	/* Set up our region to write to */
	rect.Top = win->cur.Y;
	rect.Bottom = win->cur.Y + size.Y;
	rect.Left = win->cur.X;
	rect.Right = win->cur.X + size.X;

	/* Set up our character for output */
	c.Char.AsciiChar = ch;
	c.Char.UnicodeChar = ch;
	c.Attributes = win->attrs;

	/* TODO: Handle special characters */
	/* Check for a special character */
	switch (ch) {
		case '\r':		/* Carriage return */
			/* Advance the character to the beginning of the line */
			win->cur.X = 0;
			break;
		case '\n':		/* Newline character */
			/* Advance the character to the beginning of the line */
			win->cur.X = 0;
			/* Advance the cursor to the beginning of a new line */
			win->cur.Y++;
			break;
		default:
			/* Write the character information to the back buffer with formatting */
			if (!WriteConsoleOutput(win->hcon[win->bbuf], &c, size, origin, &rect))
				return ERR;
			/* Advance the cursor */
			win->cur.X = (win->cur.X + 1) % win->size.X;
			/* Check for line wrap */
			if (!win->cur.X) win->cur.Y++;
	}

	return OK;
}


/******************************************************************************
 *
 * printw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Prints the specified formatted string to 'stdscr'.
 *
 * RETURN VALUE:   Returns OK if the formatted string was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int printw(char *fmt, ...)
{
	va_list args;
	int r;

	/* Argument initialization */
	va_start(args, fmt);

	/* Print our formatted string */
	r = va_wprintw(stdscr, fmt, &args);

	/* Argument cleanup */
	va_end(args);

	return r;
}


/******************************************************************************
 *
 * mvprintw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves to the desired coordinate (y, x) in 'stdscr' and
 *                 and prints the specified formatted string.
 *
 * RETURN VALUE:   Returns OK if the formatted stirng was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int mvprintw(int y, int x, char *fmt, ...)
{
	/* Save our old cursor position in case it needs to be restored later */
	COORD oldcur = stdscr->cur;
	va_list args;
	int r = 0;

	/* Argument initialization */
	va_start(args, fmt);

	/* Move to desired location in the desired window */
	wmove(stdscr, y, x);

	/* Print our formatted string */
	r = va_wprintw(stdscr, fmt, &args);

	/* If we were unable to print the string, */
	if (r == ERR)
		/* Restore the original cursor location */
		stdscr->cur = oldcur;

	/* Argument cleanup */
	va_end(args);

	return r;
}


/******************************************************************************
 *
 * nodelay
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Sets the 'no delay' mode of the window pointed to by 'win'
 *                 depending on the value of 'bf'.
 *
 *                 If 'bf' is 'TRUE', 'no delay' mode is turned on and all
 *                 input functions immediately return if there is no input
 *                 present.
 *
 *                 Otherwise, 'no delay' mode is turned off and all input
 *                 functions wait for input before returning.
 *
 * RETURN VALUE:   Returns OK if 'no delay' mode was set successfully.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int nodelay(WINDOW *win, bool bf)
{
	/* Sanity check: make sure window exists */
	if (!win) return ERR;

	/* Set our flag appropriately */
	if (bf == TRUE)
		win->flags |= WC_NODELAY;
	else
		win->flags &= ~WC_NODELAY;

	return OK;
}


/******************************************************************************
 *
 * mvwprintw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves to the desired coordinate (y, x) in the window pointed
 *                 to by 'win' and prints the specified formatted string.
 *
 * RETURN VALUE:   Returns OK if the formatted stirng was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int mvwprintw(WINDOW *win, int y, int x, char *fmt, ...)
{
	/* Save our old cursor position in case it needs to be restored later */
	COORD oldcur = stdscr->cur;
	va_list args;
	int r = 0;

	/* Sanity check: NULL pointer */
	if (!win) return ERR;

	/* Argument initialization */
	va_start(args, fmt);

	/* Move to desired location in the desired window */
	wmove(win, y, x);

	/* Print our formatted string */
	r = va_wprintw(win, fmt, &args);

	/* If we were unable to print the string, */
	if (r == ERR)
		/* Restore the original cursor location */
		stdscr->cur = oldcur;

	/* Argument cleanup */
	va_end(args);

	return r;
}


/******************************************************************************
 *
 * wprintw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Prints the specified formatted string in the window pointed
 *                 to by 'win'.
 *
 * RETURN VALUE:   Returns OK if the formatted stirng was successfully written.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int wprintw(WINDOW *win, char *fmt, ...)
{
	va_list args;
	int r;

	/* Sanity check: NULL pointer */
	if (!win) return ERR;

	/* Argument initialization */
	va_start(args, fmt);

	/* Print our formatted string */
	r = va_wprintw(win, fmt, &args);

	/* Argument cleanup */
	va_end(args);

	return r;
}


/******************************************************************************
 *
 * attron
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables the attributes specified by the 'attrs' bit map in
 *                 'stdscr'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int attron(int attrs)
{
	return wattron(stdscr, attrs);
}


/******************************************************************************
 *
 * attroff
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Disables the attributes specified by the 'attrs' bit map in
 *                 'stdscr'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int attroff(int attrs)
{
	return wattroff(stdscr, attrs);
}


/******************************************************************************
 *
 * attrset
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables ONLY the attributes specified by the 'attrs' bit map
 *                 in 'stdscr'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int attrset(int attrs)
{
	return wattrset(stdscr, attrs);
}


/******************************************************************************
 *
 * wattron
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables the attributes specified by the 'attrs' bit map in
 *                 the window pointed to by 'win'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int wattron(WINDOW *win, int attrs)
{
	/* Set up our bit mask */
	WORD bmask = getattrs(attrs);

	/* Set our bit mask */
	win->attrs |= bmask;

	return OK;
}


/******************************************************************************
 *
 * wattroff
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Disables the attributes specified by the 'attrs' bit map in
 *                 the window pointed to by 'win'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int wattroff(WINDOW *win, int attrs)
{
	/* Set up our bit mask */
	WORD bmask = getattrs(attrs);

	/* Clear our bit mask */
	win->attrs &= ~bmask;

	return OK;
}


/******************************************************************************
 *
 * wattrset
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables ONLY the attributes specified by the 'attrs' bit map
 *                 in the window pointed to by 'win'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int wattrset(WINDOW *win, int attrs)
{
	/* Set up our bit mask */
	WORD bmask = getattrs(attrs);

	/* Assign our bit mask */
	win->attrs = bmask;

	return OK;
}


/******************************************************************************
 *
 * getch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Reads a single character from 'stdscr'.
 *
 *                 (See 'wgetch' for more information.)
 *
 * RETURN VALUE:   Returns the character read or one of several KEY_*
 *                 constants (if the 'keypad' flag is set).
 *
 *                 (See 'wincurses.h' and 'keypad' for more information.)
 *
 *****************************************************************************/

int getch(void)
{
	return wgetch(stdscr);
}


/******************************************************************************
 *
 * mvgetch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on 'stdscr' to ('y', 'x') and
 *                 reads a single character from 'stdscr'.
 *
 *                 (See 'mvwgetch' for more information.)
 *
 * RETURN VALUE:   Returns the character read or one of several KEY_*
 *                 constants (if the 'keypad' flag is set).
 *
 *                 (See 'wincurses.h' and 'keypad' for more information.)
 *
 *****************************************************************************/

int mvgetch(int y, int x)
{
	return mvwgetch(stdscr, y, x);
}


/******************************************************************************
 *
 * mvwgetch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on the window pointed to by 'win'
 *                 to ('y', 'x') and reads a single character from the window
 *                 pointed to by 'win'.
 *
 *                 (See 'wmove' and 'wgetch' for more information.)
 *
 * RETURN VALUE:   Returns the character read or one of several KEY_* constants
 *                 (if the 'keypad' flag is set).
 *
 *                 (See 'wincurses.h' and 'keypad' for more information.)
 *
 *****************************************************************************/

int mvwgetch(WINDOW *win, int y, int x)
{
	/* Move to the desired location */
	wmove(stdscr, y, x);

	/* Get input from the window */
	return wgetch(win);
}


/******************************************************************************
 *
 * wgetch
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Reads a single character from the window pointed to by
 *                 'win'.
 *
 * RETURN VALUE:   Returns the character read or one of several KEY_*
 *                 constants (if the 'keypad' flag is set).
 *
 *                 (See 'wincurses.h' and 'keypad' for more information.)
 *
 *****************************************************************************/

int wgetch(WINDOW *win)
{
	INPUT_RECORD c;
	int len, r;

	/* Poll the console for input */
	do {
		/* If we're in 'nodelay' mode and we have no input, */
		if (win->flags & WC_NODELAY && !kbhit()) return ERR;

		ReadConsoleInput(hstdin, &c, 1, &len);
	/* Wait until we have a key down event */
	} while (c.EventType != KEY_EVENT || !c.Event.KeyEvent.bKeyDown);

	/* Echo input if applicable */
	if (flags & WC_ECHO) addch(c.Event.KeyEvent.uChar.AsciiChar);

	/* By default, pass on the raw character value */
	r = c.Event.KeyEvent.uChar.AsciiChar;

	/* If we've chosen to translate special keys, */
	if (win->flags & WC_WKEYPAD) {
		/* Return the appropriate key code */
		switch(c.Event.KeyEvent.wVirtualKeyCode) {
			case VK_ESCAPE:             /* Escape key ==> Exit key */
				r = KEY_EXIT;
				break;
			case VK_CANCEL:             /* Cancel key */
				r = KEY_CANCEL;
				break;
			case VK_BACK:               /* Backspace key */
				r = KEY_BACKSPACE;
				break;
			case VK_CLEAR:              /* Clear key */
				r = KEY_CLEAR;
				break;
			case VK_RETURN:             /* Enter key */
				r = KEY_ENTER;
				break;
			case VK_CONTROL:            /* Control key */
				r = KEY_COMMAND;
				break;
			case VK_PRIOR:              /* Page up key */
				r = KEY_PPAGE;
				break;
			case VK_NEXT:               /* Page down key */
				r = KEY_NPAGE;
				break;
			case VK_END:                /* End key */
				r = KEY_END;
				break;
			case VK_HOME:               /* Home key */
				r = KEY_HOME;
				break;
			case VK_LEFT:               /* Left key */
				r = KEY_LEFT;
				break;
			case VK_UP:                 /* Up key */
				r = KEY_UP;
				break;
			case VK_RIGHT:              /* Right key */
				r = KEY_RIGHT;
				break;
			case VK_DOWN:               /* Down key */
				r = KEY_DOWN;
				break;
			case VK_SELECT:             /* Select key */
				r = KEY_SELECT;
				break;
			case VK_PRINT:              /* Print key */
				r = KEY_PRINT;
				break;
			case VK_DELETE:             /* Delete key */
				r = KEY_DC;
				break;
			case VK_HELP:               /* Help key */
				r = KEY_HELP;
				break;
			case VK_NUMPAD1:            /* Numeric keypad 1 key */
				r = KEY_C1;
				break;
			case VK_NUMPAD2:            /* Numeric keypad 2 key */
				r = KEY_DOWN;
				break;
			case VK_NUMPAD3:            /* Numeric keypad 3 key */
				r = KEY_C3;
				break;
			case VK_NUMPAD4:            /* Numeric keypad 4 key */
				r = KEY_LEFT;
				break;
			case VK_NUMPAD5:            /* Numeric keypad 5 key */
				r = KEY_B2;
				break;
			case VK_NUMPAD6:            /* Numeric keypad 6 key */
				r = KEY_RIGHT;
				break;
			case VK_NUMPAD7:            /* Numeric keypad 7 key */
				r = KEY_A1;
				break;
			case VK_NUMPAD8:            /* Numeric keypad 8 key */
				r = KEY_UP;
				break;
			case VK_NUMPAD9:            /* Numeric keypad 9 key */
				r = KEY_A3;
				break;
			case VK_F1:                 /* F1 key */
				r = KEY_F(1);
				break;
			case VK_F2:                 /* F2 key */
				r = KEY_F(2);
				break;
			case VK_F3:                 /* F3 key */
				r = KEY_F(3);
				break;
			case VK_F4:                 /* F4 key */
				r = KEY_F(4);
				break;
			case VK_F5:                 /* F5 key */
				r = KEY_F(5);
				break;
			case VK_F6:                 /* F6 key */
				r = KEY_F(6);
				break;
			case VK_F7:                 /* F7 key */
				r = KEY_F(7);
				break;
			case VK_F8:                 /* F8 key */
				r = KEY_F(8);
				break;
			case VK_F9:                 /* F9 key */
				r = KEY_F(9);
				break;
			case VK_F10:                /* F10 key */
				r = KEY_F(10);
				break;
			case VK_F11:                /* F11 key */
				r = KEY_F(11);
				break;
			case VK_F12:                /* F12 key */
				r = KEY_F(12);
				break;
			case VK_F13:                /* F13 key */
				r = KEY_F(13);
				break;
			case VK_F14:                /* F14 key */
				r = KEY_F(14);
				break;
			case VK_F15:                /* F15 key */
				r = KEY_F(15);
				break;
			case VK_F16:                /* F16 key */
				r = KEY_F(16);
				break;
			case VK_F17:                /* F17 key */
				r = KEY_F(17);
				break;
			case VK_F18:                /* F18 key */
				r = KEY_F(18);
				break;
			case VK_F19:                /* F19 key */
				r = KEY_F(19);
				break;
			case VK_F20:                /* F20 key */
				r = KEY_F(20);
				break;
			case VK_F21:                /* F21 key */
				r = KEY_F(21);
				break;
			case VK_F22:                /* F22 key */
				r = KEY_F(22);
				break;
			case VK_F23:                /* F23 key */
				r = KEY_F(23);
				break;
			case VK_F24:                /* F24 key */
				r = KEY_F(24);
				break;
		}
	}

	return r;
}


/******************************************************************************
 *
 * echo
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables 'echo' mode in which characters are written to the
 *                 screen as they are typed.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int echo(void)
{
	/* Set the echo input flag */
	flags |= WC_ECHO;

	return OK;
}


/******************************************************************************
 *
 * noecho
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Disables 'echo' mode.
 *
 *                 (See 'echo' for more information.)
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 * NOTES:          This function always succeeds.
 *
 *****************************************************************************/

int noecho(void)
{
	/* Clear the echo input flag */
	flags &= ~WC_ECHO;

	return OK;
}


/******************************************************************************
 *
 * keypad
 *
 ******************************************************************************
 *
 * DESCRIPTION:    If 'bf' is 'TRUE', 'keypad' mode will be enabled in the
 *                 window pointed to by 'win'.
 *
 *                 If 'bf' is 'FALSE', 'keypad' mode will be disabled in the
 *                 window pointed to by 'win'.
 *
 *                 When in keypad mode, special character codes sent to the
 *                 window pointed to by 'win' are translated to one of the
 *                 KEY_* constants defined in 'wincurses.h'.
 *
 * RETURN VALUE:   Returns OK on success. Otherwise, ERR is returned.
 *
 *****************************************************************************/

int keypad(WINDOW *win, bool bf)
{
	/* Sanity check: NULL pointer */
	if (!win) return ERR;

	/* If the flag value is 'FALSE', */
	if (bf == FALSE)
		/* Unset the window's keypad flag */
		win->flags &= ~WC_WKEYPAD;
	/* Else, */
	else
		/* Set the window's keypad flag */
		win->flags |= WC_WKEYPAD;

	return OK;
}


/******************************************************************************
 *
 * cbreak
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables 'cbreak' mode in which input is processed as it is
 *                 received but special character codes or combinations are
 *                 handled by the operating system.
 *
 * RETURN VALUE:   Returns OK if mode was successfully changed. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int cbreak(void)
{
	/* Clear the enable line input flag */
	if (clearmode(hstdin, ENABLE_LINE_INPUT) == ERR)
		return ERR;

	/* Set the enable processed input flag */
	if (setmode(hstdin, ENABLE_PROCESSED_INPUT) == ERR)
		return ERR;

	return OK;
}


/******************************************************************************
 *
 * nocbreak
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Disables 'cbreak' mode.
 *
 *                 (See 'cbreak' for more information.)
 *
 * RETURN VALUE:   Returns OK if mode was successfully changed. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int nocbreak(void)
{
	/* Set the enable line input and enable processed input flags */
	return setmode(hstdin, ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
}


/******************************************************************************
 *
 * raw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Enables 'raw' mode in which input is processed as it is
 *                 received and ALL characters or character codes are passed on
 *                 to the program.
 *
 * RETURN VALUE:   Returns OK if mode was successfully changed. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int raw(void)
{
	/* Clear the enable line input flag */
	if (clearmode(hstdin, ENABLE_LINE_INPUT) == ERR)
		return ERR;

	/* Clear the enable processed input flag */
	if (clearmode(hstdin, ENABLE_PROCESSED_INPUT) == ERR)
		return ERR;

	return OK;
}


/******************************************************************************
 *
 * noraw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Disables 'raw' mode.
 *
 *                 (See 'raw' for more information.)
 *
 * RETURN VALUE:   Returns OK if mode was successfully changed. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int noraw(void)
{
	/* Set the enable line input and enable processed input flags */
	return setmode(hstdin, ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
}


/******************************************************************************
 *
 * move
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on 'stdscr' to ('y', 'x').
 *
 * RETURN VALUE:   Returns OK if the destination coordinates were within window
 *                 bounds. Otherwise, ERR is returned.
 *
 *****************************************************************************/

int move(int y, int x)
{
	return wmove(stdscr, y, x);
}


/******************************************************************************
 *
 * wmove
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Moves the cursor location on 'win' to ('y', 'x').
 *
 * RETURN VALUE:   Returns OK if the destination coordinates were within window
 *                 bounds. Otherwise, ERR is returned.
 *
 *****************************************************************************/

int wmove(WINDOW *win, int y, int x)
{
	/* Sanity check: within bounds? */
	if (y >= win->size.Y || x >= win->size.X || y < 0 || x < 0)
		return ERR;

	/* Set the cursor position */
	win->cur.Y = y;
	win->cur.X = x;

	return OK;
}


/******************************************************************************
 *
 * can_change_color
 *
 ******************************************************************************
 *
 * RETURN VALUE:   Returns TRUE if the terminal supports user-defined colors,
 *                 Otherwise, FALSE is returned.
 *
 *****************************************************************************/

bool can_change_color(void)
{
	return CAN_CHANGE_COLOR;
}


/******************************************************************************
 *
 * color_content
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Given a color number, assigns pointers to the red, green,
 *                 and blue color components for the color indexed in the
 *                 'colors' array by 'color'. The pointers are assigned to
 *                 'red', 'green', and 'blue', respectively.
 *
 * RETURN VALUE:   Returns the color pair value if colors are available.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int color_content(short color, short *red, short *green, short *blue)
{
	/* Make sure we are using colors and have values within range */
	if (flags & WC_COLOR && has_colors() && can_change_color() &&
			color < COLORS && color > 0) {
		/* Assign pointers to our color values */
		red   = &colors[color].r;
		green = &colors[color].g;
		blue  = &colors[color].b;
		return OK;
	} else return ERR;
}


/******************************************************************************
 *
 * COLOR_PAIR
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Given a color pair number, returns the value as it would be
 *                 extracted from a chtype.
 *
 * RETURN VALUE:   Returns the color pair value of the given color pair number.
 *
 *****************************************************************************/

int COLOR_PAIR(int n)
{
	/* Return the high-order color bits */
	return (n << (32 - COLOR_BITS));
}


/******************************************************************************
 *
 * has_colors
 *
 ******************************************************************************
 *
 * RETURN VALUE:   Returns TRUE if the terminal supports colors. Otherwise,
 *                 FALSE is returned.
 *
 *****************************************************************************/

bool has_colors(void)
{
	COLORS = 0;
	COLOR_PAIRS = 0;

	if (HAS_COLORS == TRUE) {
		/* Set up our global variables */
		COLORS = NUM_COLORS;
		COLOR_PAIRS = MAX_NUM_PAIRS;
	}

	return HAS_COLORS;
}


/******************************************************************************
 *
 * init_color
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Initializes a user-defined color by replacing an old one.
 *
 *                 'color' refers to the existing color to be replaced by the
 *                 red, green, and blue intensities given by 'red', 'green',
 *                 and 'blue', respectively, all within the range of 0 to 1000,
 *                 inclusive.
 *
 * RETURN VALUE:   Returns OK if the color was set up correctly. Otherwise, ERR
 *                 is returned.
 *
 * NOTES:          Windows does not support this functionality. However, for
 *                 completeness, we have set up a framework if it were to be
 *                 emulated somehow.
 *
 *                 Basically, though we are using Windows console flags to
 *                 control color usage, we actually are able to address RGB
 *                 values.
 *
 *                 Namely, colors are applied based upon color pair data stored
 *                 in each window's 'attrs' variable. (In fact, the color pair
 *                 can be extracted using the 'A_COLOR_DATA' macro, passing it
 *                 the 'attrs' variable in question.) This, in turn, merely
 *                 points to an index in the global 'color_pairs' array, which
 *                 contains a listing of 'color_t' foreground and background
 *                 colors.
 *
 *                 This feature has not beed tested and code is provided mainly
 *                 for demonstration purposes.
 *
 *****************************************************************************/

int init_color(short color, short red, short green, short blue)
{
	/* Make sure we are using colors and have values within range */
	if (flags & WC_COLOR && has_colors() && can_change_color() &&
			color < COLORS && color > 0) {
		/* Make sure our color values are within range */
		if (red > 1000 || green > 1000 || blue > 1000 ||
				red < 0 || green < 0 || blue < 0)
			return ERR;

		/* TODO: Add support for custom colors */
		/* Assign our color value */
		colors[color].r = red;
		colors[color].g = green;
		colors[color].b = blue;

		return OK;
	} else return ERR;
}


/******************************************************************************
 *
 * init_pair
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Initializes a pair of foreground and background colors for
 *                 later use.
 *
 *                 Both 'f' and 'b' refer to indices within the global 'colors'
 *                 array.
 *
 * RETURN VALUE:   Returns OK if the color pair was set up correctly.
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int init_pair(short pair, short f, short b)
{
	/*
	 * Make sure we are using colors and have values within range. (Note that
	 * we do not change pair number zero as it is the default console
	 * foreground and background.)
	 */
	if (flags & WC_COLOR && has_colors() && pair < COLOR_PAIRS && pair > 0) {
		/* Assign our color values */
		color_pairs[pair].f = f;
		color_pairs[pair].b = b;
		return OK;
	} else return ERR;
}


/******************************************************************************
 *
 * pair_content
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Modifies 'f' and 'b' to point to the respective foreground
 *                 and background values for the given pair.
 *
 * RETURN VALUE:   Returns OK if the color pair was accessible. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int pair_content(short pair, short *f, short *b)
{
	/* Make sure we are using colors and have values within range */
	if (flags & WC_COLOR && has_colors() && pair < COLOR_PAIRS && pair >= 0) {
		/* Assign our color pointer values */
		f = &color_pairs[pair].f;
		b = &color_pairs[pair].b;
		return OK;
	} else return ERR;
}


/******************************************************************************
 *
 * PAIR_NUMBER
 *
 ******************************************************************************
 *
 * DESCRIPTION:    (See 'A_COLOR_DATA' in 'wincurses.h' for more information.)
 *
 * RETURN VALUE:   Returns the color pair number given a color pair value.
 *
 *****************************************************************************/

int PAIR_NUMBER(int value)
{
	return A_COLOR_DATA(value);
}


/******************************************************************************
 *
 * start_color
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Initializes color mode for the console. Namely, it sets up
 *                 the bit masks for the various console colors, sets the
 *                 console colors to the default set (white foreground text on
 *                 a black background) and turns colors on globally.
 *
 * RETURN VALUE:   Returns OK if colors were set up correctly on the console
 *                 Otherwise, ERR is returned.
 *
 *****************************************************************************/

int start_color(void)
{
	/* Make sure we are using colors */
	if (has_colors()) {
		/* Initialize the eight basic console colors */
		colors[COLOR_BLACK].r   = 0;
		colors[COLOR_BLACK].g   = 0;
		colors[COLOR_BLACK].b   = 0;

		colors[COLOR_BLUE].r    = 0;
		colors[COLOR_BLUE].g    = 0;
		colors[COLOR_BLUE].b    = 1000;

		colors[COLOR_GREEN].r   = 0;
		colors[COLOR_GREEN].g   = 1000;
		colors[COLOR_GREEN].b   = 0;

		colors[COLOR_CYAN].r    = 0;
		colors[COLOR_CYAN].g    = 1000;
		colors[COLOR_CYAN].b    = 1000;

		colors[COLOR_RED].r     = 1000;
		colors[COLOR_RED].g     = 0;
		colors[COLOR_RED].b     = 0;

		colors[COLOR_MAGENTA].r = 1000;
		colors[COLOR_MAGENTA].g = 0;
		colors[COLOR_MAGENTA].b = 1000;

		colors[COLOR_YELLOW].r  = 1000;
		colors[COLOR_YELLOW].g  = 1000;
		colors[COLOR_YELLOW].b  = 0;

		colors[COLOR_WHITE].r   = 1000;
		colors[COLOR_WHITE].g   = 1000;
		colors[COLOR_WHITE].b   = 1000;

		/* Set up the console defaults */
		init_pair(0, COLOR_WHITE, COLOR_BLACK);
		attrset(COLOR_PAIR(0));

		/* Turn colors on globally */
		flags |= WC_COLOR;

		return OK;
	} else return ERR;
}


/******************************************************************************
 *
 * curs_set
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Sets the visibility mode of the cursor depending on the
 *                 value of 'visibility'.
 *
 *                 0 means make the cursor invisible.
 *                 1 means make the cursor normal visibility.
 *                 2 means make the cursor have increased visibility.
 *
 * RETURN VALUE:   Returns OK if cursor visibility was set correctly. Otherwise
 *                 ERR is returned.
 *
 *****************************************************************************/

int curs_set(int visibility)
{
	int r;
	CONSOLE_CURSOR_INFO curinfo, curold;

	/* Get our old cursor information */
	r = GetConsoleCursorInfo(stdscr->hcon[stdscr->prim], &curold);
	if (!r) return ERR;

	/* Copy our old cursor information to our new cursor */
	curinfo = curold;

	/* Assign the proper visibility */
	switch (visibility) {
		case 0:		/* Invisible */
			curinfo.bVisible = FALSE;
			break;
		case 1:		/* Normal visibility */
			curinfo.bVisible = TRUE;
			curinfo.dwSize = cursize;
			break;
		case 2:		/* High visibility */
			curinfo.bVisible = TRUE;
			curinfo.dwSize = 100;
			break;
		default:
			return ERR;
	}

	/* Apply the new visibility settings to all buffers */
	r = SetConsoleCursorInfo(stdscr->hcon[stdscr->prim], &curinfo);
	r |= SetConsoleCursorInfo(stdscr->hcon[stdscr->bbuf], &curinfo);
	if (!r) return ERR;

	/* Return our old state */
	if (curold.bVisible == FALSE) return 0;
	if (curold.dwSize == cursize) return 1;
	return 2;
}


/******************************************************************************
 * Windows-specific helper function declarations follow.
 *****************************************************************************/


/******************************************************************************
 *
 * cls
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Clears the console screen buffer pointed to by 'hcon' by
 *                 filling it with the character 'WC_BGND'.
 *
 *****************************************************************************/

void cls(HANDLE hcon)
{
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	COORD orig;
	DWORD len, size;

	/* Get console information */
	GetConsoleScreenBufferInfo(hcon, &coninfo);

	/* Calculate the number of spaces to fill ('size') */
	size = coninfo.dwSize.Y * coninfo.dwSize.X;

	/* We want to start at (0, 0) */
	orig.Y = orig.X = 0;

	/*
	 * Now fill 'size' characters with WG_BGND (defined in wincurses.h)
	 * starting at 'orig'.
	 */
	FillConsoleOutputCharacter(hcon, WC_BGND, size, orig, &len);
}


/******************************************************************************
 *
 * setmode
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Sets the particular console mode specified by the bitmask
 *                 on the console with handle specified by 'hcon'.
 *
 * RETURN VALUE:   Returns OK if the mode was successfully set. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int setmode(HANDLE hcon, DWORD bmask)
{
	DWORD mode;

	/* Get the current console mode */
	if (!GetConsoleMode(hcon, &mode));
		/* Return ERR on error */
		return ERR;

	/* Apply the bit mask */
	mode |= bmask;

	/* Set the new console mode and return its status */
	return SetConsoleMode(hcon, mode) ? OK : ERR;
}


/******************************************************************************
 *
 * clearmode
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Clears the particular console mode specified by the bitmask
 *                 on the console with handle specified by 'hcon'.
 *
 * RETURN VALUE:   Returns OK if the mode was successfully set. Otherwise, ERR
 *                 is returned.
 *
 *****************************************************************************/

int clearmode(HANDLE hcon, DWORD bmask)
{
	DWORD mode;

	/* Get the current console mode */
	if (!GetConsoleMode(hcon, &mode))
		/* Return ERR on error */
		return ERR;

	/* Apply the bit mask */
	mode &= ~bmask;

	/* Set the new console mode and return its status */
	return SetConsoleMode(hcon, mode) ? OK : ERR;
}


/******************************************************************************
 *
 * va_wprintw
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Helper function to aid the *printw functions in their tasks
 *                 by printing from a va_list 'list' using a specified format,
 *                 'fmt' in the window pointed to by 'win'.
 *
 * RETURN VALUE:   Returns OK if the mode was successfully set. Otherwise, ERR
 *                 is returned, such as in the case when there is not enough
 *                 screen space to write the formatted text.
 *
 *****************************************************************************/

int va_wprintw(WINDOW *win, char *fmt, va_list *args)
{
	char *s;
	int n = 0;

	/* Allocate enough memory to hold the formatted string */
	s = malloc(sizeof(char) * stdscr->size.Y * stdscr->size.X);

	/* Save our formatted string into a temporary location in a safe way */
	if (vsprintf_s(s, stdscr->size.Y * stdscr->size.X, fmt, *args) < 0)
		return ERR;

	/* Print our formatted string */
	while (s[n] && waddch(win, s[n]) == OK)
		n++;

	return OK;
}


/******************************************************************************
 *
 * getattrs
 *
 ******************************************************************************
 *
 * DESCRIPTION:    Translates 'attrs' into a Windows-specific console display
 *                 attributes bit mask.
 *
 * RETURN VALUE:   Returns a bitmask representing Windows-specific console
 *                 display attributes.
 *
 * NOTES:          The current implementation uses Windows console bit masks to
 *                 represent colors. Presumably, any ARGB color combination can
 *                 be represented and implemented under the current design.
 *
 *****************************************************************************/

WORD getattrs(unsigned int attrs)
{
	WORD bmask = 0;

	/* Set up our bit mask */
	if (attrs & A_BOLD)      bmask |= FOREGROUND_INTENSITY;
	if (attrs & A_REVERSE)   bmask |= COMMON_LVB_REVERSE_VIDEO;
	if (attrs & A_STANDOUT)  bmask |= BACKGROUND_INTENSITY;
	if (attrs & A_UNDERLINE) bmask |= COMMON_LVB_UNDERSCORE;
	/* TODO: Add support for custom colors */
	/* Apply colors using console flags if we cannot change them */
	if (flags & WC_COLOR && can_change_color() == FALSE) {
		int colormask =
				((colors[color_pairs[PAIR_NUMBER(attrs)].f].r == 0
					? 0 : FOREGROUND_RED) |
				(colors[color_pairs[PAIR_NUMBER(attrs)].f].g == 0
					? 0 : FOREGROUND_GREEN) |
				(colors[color_pairs[PAIR_NUMBER(attrs)].f].b == 0
					? 0 : FOREGROUND_BLUE));
		bmask |= colormask;
		/*
		 * We use the 'FTOB' macro to derive the Windows console background
		 * color flag from a Windows console foreground flag. (It's just a bit
		 * shift.)
		 */
		colormask =
				(((colors[color_pairs[PAIR_NUMBER(attrs)].b].r == 0)
					? 0 : FOREGROUND_RED) |
				((colors[color_pairs[PAIR_NUMBER(attrs)].b].g == 0)
					? 0 : FOREGROUND_GREEN) |
				((colors[color_pairs[PAIR_NUMBER(attrs)].b].b == 0)
					? 0 : FOREGROUND_BLUE));
		bmask |= FTOB(colormask);
	}

	return bmask;
}
