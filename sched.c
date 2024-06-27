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
struct message
{
    long mtype;
    char mtext[20];
};
int main(int argc,char* argv[]){
    int mq1=atoi(argv[1]),mq2=atoi(argv[2]),k=atoi(argv[3]),n;
    long pid=0;
    int semid=semget(ftok("/home",'A'),1,IPC_CREAT),semid1=semget(ftok("/home",'B'),k,IPC_CREAT);
    struct message msg;
    struct sembuf sem_sig={0,1,0};
    while(k){
        if((n=msgrcv(mq1,(void*)&msg,13,0,0))==-1){
            perror("rcv failed11\nTerminating\n");
            V(semid);
        }
        pid=msg.mtype-1;
        // printf("Scheduling process %ld\n",msg.mtype-1);
        sem_sig.sem_num=msg.mtype-1;
        V(semid1);
        if((n=msgrcv(mq2,(void*)&msg,20,0,0))==-1){
            perror("rcv failed12\nTerminating\n");
            V(semid);
        }
        else{
            msg.mtext[n]='\0';
            if(!strcmp(msg.mtext,"PAGE FAULT HANDLED")){
                strcpy(msg.mtext,"ready");
                msg.mtype=pid+1;
                // printf("Page fault for process %ld handled & added to end of queue\n",pid);
                if((msgsnd(mq1,(void*)&msg,strlen(msg.mtext),0))==-1){
                    perror("send failed13\nTerminating\n");
                    V(semid);
                }
            }
            else{
                // printf("Process %ld terminated\n",pid);
                k--;
            }
        }
    }
    sem_sig.sem_num=0;
    V(semid);
    return 0;
}