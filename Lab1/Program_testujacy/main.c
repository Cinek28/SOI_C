#include </usr/include/lib.h>
#include </usr/include/stdio.h>

int getprocnr(int procNr);

int main(int argc,char* argv[])
{
  int result;
  int i;
  if(argc==1)
    printf("Brak parametru. Podaj PID.\n");
  else{
    int value = atoi(argv[1]);
    for(i = 0;i<10;i++)
      {
       result = getprocnr(value+i);
       if(result != -1)
         printf("Proces o PID: %d to %d.\n",value+i,result);
       else 
         printf("PID: %d. Blad: %d.\n",value+i,errno);
      }
  }
return 0;
}

int getprocnr(int procNr)
{
  message msg;
  msg.m1_i1 = procNr;
  return (_syscall(MM,78,&msg));
}


