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

static int s_shmid = -1;
static int s_EndFlag = 0;

void EndSignal(int sig)
{
  s_EndFlag=1;
}

void DestroyObjs()
{
  int ret;
  ret = sem_unlink(s_strsemname);
  if(ret < 0){
    fprintf(stderr, "sem_unlink error");
  }

  if(s_shmid >= 0){
    ret = shmctl(s_shmid, IPC_RMID, 0);
    if(ret < 0){
      fprintf(stderr, "shmctl error");
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
  s_shmid = shmget(memkey, sizeof(TIMEINFO) * TIMEINFONUMMAX, IPC_CREAT);
  if(s_shmid < 0){
    fprintf(stderr, "shmget error");
    return 1;
  }
  TIMEINFO* shmptr;
  shmptr = (TIMEINFO*)shmat(s_shmid, 0, SHM_MODE);
  if(shmptr == (void*)-1){
    fprintf(stderr, "shmat error");
    DestroyObjs();
    return 1;
  }


  sem_t *semptr;
  semptr = sem_open(s_strsemname, O_CREAT, SHM_MODE, 1);
  if(semptr == SEM_FAILED){
    fprintf(stderr, "sem_open error");
    DestroyObjs();
    return 1;
  }
  
  struct timespec sleeptime;
  memset(&sleeptime, 0, sizeof(struct timespec));
  sleeptime.tv_nsec = 1000 * 1000 * 500;//1500ms

  int setno = 0;
  while(s_EndFlag == 0){
    TIMEINFO curtimeinfo;
    memset(&curtimeinfo, 0, sizeof(TIMEINFO));

    curtimeinfo.setflag = 1;
    curtimeinfo.no = setno;
    time_t curtime;
    time(&curtime);
    struct tm *tmptr;
    tmptr = localtime(&curtime);
    if(tmptr){
      curtimeinfo.time = *tmptr;
    } 

    //get access right of shm
    int semret;
    semret = sem_wait(semptr);
    if(semret != 0){
      fprintf(stderr, "sem_wait error");
      DestroyObjs();
      return 1;
    }
    //write to shm
    *(shmptr + setno) = curtimeinfo;
    //release access right of shm
    semret = sem_post(semptr);
    if(semret != 0){
      fprintf(stderr, "sem_post error");
      DestroyObjs();
      return 1;
    }

    char strtime[256];
    strftime(strtime, 256, "%Y/%b/%d  %r", &curtimeinfo.time);
    fprintf(stdout, "setno %d, %s\n",
	    setno,
	    strtime );

    setno++;
    if(setno >= TIMEINFONUMMAX){
      setno = 0;
    }

    nanosleep(&sleeptime, NULL);

  }
  
  DestroyObjs();

  return 0;
}
