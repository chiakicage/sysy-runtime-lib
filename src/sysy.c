#include "sysy.h"

#ifdef NO_LIBC
#include "nolibc/io.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// ============================================================
// Internal implementations of IO operations.
// ============================================================

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_SPACE(c)                                           \
  ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || \
   (c) == '\t' || (c) == '\v')

static char last_char;
static int last_char_valid = 0;

static void PutChar(int fd, char c) { write(STDOUT_FILENO, &c, 1); }

static void PutString(int fd, const char *str) {
  for (int i = 0; str[i]; ++i) PutChar(fd, str[i]);
}

static void PutInt(int fd, int num) {
  // check if is a negative integer
  if (num < 0) {
    putch('-');
    num = -num;
  }
  // convert integer to string
  char digits[21];
  int i = 20;
  if (!num) {
    i = 19;
    digits[19] = '0';
  }
  else {
    while (num) {
      i -= 1;
      digits[i] = (num % 10) + '0';
      num /= 10;
    }
  }
  // write string to stdout
  write(fd, digits + i, 20 - i);
}

// ============================================================
// Implementations of IO functions.
// ============================================================

int getint() {
  // skip spaces
  char c = getch();
  while (IS_SPACE(c)) c = getch();
  // check if is a negative integer
  int is_neg = 0;
  if (c == '-') {
    is_neg = 1;
    c = getch();
  }
  // read digits
  int num = 0;
  for (; IS_DIGIT(c); c = getch()) {
    num = num * 10 + c - '0';
  }
  // unget char
  last_char = c;
  last_char_valid = 1;
  return is_neg ? -num : num;
}

int getch() {
  if (last_char_valid) {
    // char buffer is valid, consume the char in it
    last_char_valid = 0;
    return last_char;
  }
  else {
    // char buffer is not valid, read char from stdin
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
  }
}

int getarray(int a[]) {
  int n = getint();
  for (int i = 0; i < n; i++) a[i] = getint();
  return n;
}

void putint(int num) { PutInt(STDOUT_FILENO, num); }

void putch(int ch) { PutChar(STDOUT_FILENO, ch); }

void putarray(int n, int a[]) {
  putint(n);
  putch(':');
  for (int i = 0; i < n; i++) {
    putch(' ');
    putint(a[i]);
  }
  putch('\n');
}

// ============================================================
// Implementations of timing functions.
// ============================================================

#define TIMER_COUNT_MAX 1024
static struct timeval timer_start, timer_end;
static int timer_h[TIMER_COUNT_MAX], timer_m[TIMER_COUNT_MAX],
    timer_s[TIMER_COUNT_MAX], timer_us[TIMER_COUNT_MAX];
static int timer_idx;

void __attribute((constructor)) before_main() {
  for (int i = 0; i < TIMER_COUNT_MAX; i++) {
    timer_h[i] = timer_m[i] = timer_s[i] = timer_us[i] = 0;
  }
  timer_idx = 1;
}

void __attribute((destructor)) after_main() {
  for (int i = 1; i < timer_idx; i++) {
    PutString(STDERR_FILENO, "Timer: ");
    PutInt(STDERR_FILENO, timer_h[i]);
    PutString(STDERR_FILENO, "H-");
    PutInt(STDERR_FILENO, timer_m[i]);
    PutString(STDERR_FILENO, "M-");
    PutInt(STDERR_FILENO, timer_s[i]);
    PutString(STDERR_FILENO, "S-");
    PutInt(STDERR_FILENO, timer_us[i]);
    PutString(STDERR_FILENO, "us\n");
    timer_us[0] += timer_us[i];
    timer_s[0] += timer_s[i];
    timer_us[0] %= 1000000;
    timer_m[0] += timer_m[i];
    timer_s[0] %= 60;
    timer_h[0] += timer_h[i];
    timer_m[0] %= 60;
  }
  PutString(STDERR_FILENO, "TOTAL: ");
  PutInt(STDERR_FILENO, timer_h[0]);
  PutString(STDERR_FILENO, "H-");
  PutInt(STDERR_FILENO, timer_m[0]);
  PutString(STDERR_FILENO, "M-");
  PutInt(STDERR_FILENO, timer_s[0]);
  PutString(STDERR_FILENO, "S-");
  PutInt(STDERR_FILENO, timer_us[0]);
  PutString(STDERR_FILENO, "us\n");
}

void starttime() { gettimeofday(&timer_start, NULL); }

void stoptime() {
  gettimeofday(&timer_end, NULL);
  timer_us[timer_idx] += 1000000 * (timer_end.tv_sec - timer_start.tv_sec) +
                         timer_end.tv_usec - timer_start.tv_usec;
  timer_s[timer_idx] += timer_us[timer_idx] / 1000000;
  timer_us[timer_idx] %= 1000000;
  timer_m[timer_idx] += timer_s[timer_idx] / 60;
  timer_s[timer_idx] %= 60;
  timer_h[timer_idx] += timer_m[timer_idx] / 60;
  timer_m[timer_idx] %= 60;
  timer_idx++;
}
