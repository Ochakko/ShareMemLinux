#ifndef SHAREMEMH
#define SHAREMEMH

typedef struct tag_timeinfo
{
  int setflag;
  int no;
  struct tm time;
}TIMEINFO;

#define TIMEINFONUMMAX 2048
#define SHM_MODE 0600

#define SHAREMEM_KEY_MEM 4339
static char s_strsemname[256] = "/sem_sharemem";

#endif
