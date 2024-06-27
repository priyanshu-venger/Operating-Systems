#include<sys/sem.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/msg.h>
#include<signal.h>
#include<stdbool.h>
#include<string.h>
#define V(semid) semop(semid,&sem_sig,1)
#define P(semid) semop(semid,&sem_wait,1)
struct message
{
    long mtype;
    char mtext[20];
};
int main(int argc,char* argv[]){
    char* ref=argv[1],*loc;
    int mq1=atoi(argv[2]),mq3=atoi(argv[3]),k=atoi(argv[4]),n,frame;
    int semid=semget(ftok("/home",'B'),k,IPC_CREAT);
    struct message msg;
    struct sembuf sem_sig={k,1,0},sem_wait={k,-1,0};
    msg.mtype=k+1;
    strcpy(msg.mtext,"ready");
    if((msgsnd(mq1,(void*)&msg,strlen(msg.mtext),0))==-1){
        perror("send failed\nTerminating\n");
    }
    P(semid);
    loc=0;
    ref=strtok_r(ref,",",&loc);
    while(ref){
        strcpy(msg.mtext,ref);
        msg.mtype=k+2;
        if(msgsnd(mq3,(void*)&msg,strlen(msg.mtext),0)==-1){
            perror("send failed1\n");
            exit(1);
        }
        if((n=msgrcv(mq3,(void*)&msg,13,1,0))==-1){
            perror("rcv failed2\n");
            exit(1);
        }
        msg.mtext[n]='\0';
        frame=atoi(msg.mtext);
        if(frame>-1){
            ref=strtok_r(0,",",&loc);
        }
        else if(frame==-1){
            P(semid);
        }
        else return 0;
    }
    sprintf(msg.mtext,"%d",-9);
    msg.mtype=k+2;
    if(msgsnd(mq3,(void*)&msg,strlen(msg.mtext),0)==-1){
        perror("send failed3\n");
        exit(1);
    }
    return 0;
}