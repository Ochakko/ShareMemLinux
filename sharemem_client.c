#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <signal.h>

#include "sharemem.h"

static sem_t* s_semptr = 0;
static int s_shmid = -1;
static TIMEINFO* s_shmptr = 0;
static TIMEINFO* s_localshmptr = 0;
static int s_EndFlag = 0;

void EndSignal(int sig)
{
  s_EndFlag=1;
}

void DestroyObjs()
{
  int ret;
  if(s_semptr){
    ret = sem_close(s_semptr);
    if(ret < 0){
      fprintf(stderr, "sem_close error");
    }
  }
  if(s_shmid >= 0){
    if(s_shmptr){
      ret = shmdt(s_shmptr);
      if(ret < 0){
	fprintf(stderr, "shmdt error");
      }
    }
  }
  if(s_localshmptr){
    free(s_localshmptr);
    s_localshmptr = 0;
  }

}

void PrintTimeInfo()
{
  fprintf(stdout, "\n\n\n");

  int tino;
  for(tino = 0; tino < TIMEINFONUMMAX; tino++){
   TIMEINFO curtimeinfo;
    memset(&curtimeinfo, 0, sizeof(TIMEINFO));
    curtimeinfo = *(s_localshmptr + tino);

    if(curtimeinfo.setflag == 1){
      char strtime[256];
      strftime(strtime, 256, "%Y/%b/%d  %r", &curtimeinfo.time);
      fprintf(stdout, "tino %d, %s\n",
	      tino,
	      strtime );
    }
  }
}

int main(int argc, char** argv)
{
  s_EndFlag = 0;
  signal(SIGINT,EndSignal);
  signal(SIGTERM,EndSignal);
  signal(SIGQUIT,EndSignal);
  
  signal(SIGPIPE,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);

  key_t memkey;
  memkey = (key_t)SHAREMEM_KEY_MEM;
  s_shmid = shmget(memkey, 0, 0);
  if(s_shmid < 0){
    fprintf(stderr, "shmget error");
    return 1;
  }

  fprintf(stdout, "shmget success %d\n", s_shmid);

  s_shmptr = (TIMEINFO*)shmat(s_shmid, 0, 0);
  if(s_shmptr == (void*)-1){
    fprintf(stderr, "shmat error");
    DestroyObjs();
    return 1;
  }

  fprintf(stdout, "shmat %p\n", s_shmptr);


  s_semptr = sem_open(s_strsemname, 0);
  if(s_semptr == SEM_FAILED){
    fprintf(stderr, "sem_open error");
    DestroyObjs();
    return 1;
  }

  fprintf(stdout, "sem_open semptr %p\n", s_semptr);


  
  s_localshmptr = (TIMEINFO*)malloc(sizeof(TIMEINFO) * TIMEINFONUMMAX);
  if(!s_localshmptr){
    fprintf(stderr, "localshmptr malloc error");
    return 1;
  }
  memset(s_localshmptr, 0, sizeof(TIMEINFO) * TIMEINFONUMMAX);


  struct timespec sleeptime;
  memset(&sleeptime, 0, sizeof(struct timespec));
  sleeptime.tv_nsec = 1000 * 1000 * 800;//3500ms

  while(s_EndFlag == 0){
    //get access right of shm
    int semret;
    semret = sem_wait(s_semptr);
    if(semret != 0){
      fprintf(stderr, "sem_wait error");
      DestroyObjs();
      return 1;
    }
    //copy and release(sem_post) as soon as possible
    memcpy(s_localshmptr, s_shmptr, sizeof(TIMEINFO) * TIMEINFONUMMAX);
    semret = sem_post(s_semptr);
    if(semret != 0){
      fprintf(stderr, "sem_post error");
      DestroyObjs();
      return 1;
    }

    PrintTimeInfo();//operation to mem leisurely

    nanosleep(&sleeptime, NULL);

  }
  
  DestroyObjs();

  return 0;
}
