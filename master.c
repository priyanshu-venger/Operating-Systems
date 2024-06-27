
#include<sys/sem.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/msg.h>
#include<signal.h>
#include<stdbool.h>
#include<string.h>
#include<limits.h>
#include<wait.h>
#define P(semid) semop(semid,&sem_wait,1)
#define V(semid) semop(semid,&sem_sig,1)
int semid;
struct table{
    int frame;
    bool valid;
    int lref;
};
struct list{
    int frame;
    int next;
};
void MMU(char *cmnd[8]){
    execvp("xterm",cmnd);
}
void SCHED(char *cmnd[8]){
    execv("./sched",cmnd);
}
void PROCESS(char *cmnd[8]){
    execv("./process",cmnd);
}
void handler(int sig){
    printf("Terminated\n");
    if(sig==SIGINT){
        struct sembuf sem_sig={0,1,0};
        V(semid);
    }
}
int main(){
    int n,k,m,f,nref;
    struct sembuf sem_wait;
    printf("Enter k,m,f:");
    scanf("%d%d%d",&k,&m,&f);
    int SM1,SM2,MQ1,MQ2,MQ3,semid1,*sm1,pid[k+2],t=0,id,id1,*valid_size,shmid,q;
    char *ref[k],temp[12];
    char *cmnd[8];
    signal(SIGTERM,handler);
    signal(SIGINT,handler);
    struct list *sm2,*prev;
    SM2=shmget(IPC_PRIVATE,sizeof(struct list),IPC_CREAT|0777);
    MQ1=msgget(IPC_PRIVATE,IPC_CREAT|0777);
    MQ2=msgget(IPC_PRIVATE,IPC_CREAT|0777);
    MQ3=msgget(IPC_PRIVATE,IPC_CREAT|0777);
    semid=semget(ftok("/home",'A'),1,0777|IPC_CREAT);
    semid1=semget(ftok("/home",'B'),k,0777|IPC_CREAT);
    valid_size=shmat((shmid=shmget(ftok("/home",'p'),k*sizeof(int),0777|IPC_CREAT)),0,0);
    for(int i=0;i<k;i++) semctl(semid1,i,SETVAL,0);
    SM1=shmget(IPC_PRIVATE,k*sizeof(int),IPC_CREAT|0777);
    sm1=(int*)shmat(SM1,0,0);
    semctl(semid,0,SETVAL,0);
    sem_wait.sem_flg=sem_wait.sem_num=0;
    sem_wait.sem_op=-1;
    sm2=(struct list*)shmat(SM2,0,0);
    sm2->frame=-1;
    sm2->next=-1;
    for(int i=0;i<f;i++){
        prev=sm2;
        prev->next=shmget(IPC_PRIVATE,sizeof(struct list),IPC_CREAT|0777);
        sm2=(struct list*)shmat(prev->next,0,0);
        shmdt(prev);
        sm2->frame=i;
        sm2->next=-1;
    }
    for(int i=0;i<k;i++){
        n=rand()%m+1;
        nref=rand()%(8*n+1)+2*n;
        ref[i]=(char*)malloc(nref*12+1);
        for(int j=0;j<nref;j++){
            q=rand()%100;
            if(q<95){
                sprintf(temp,"%d,",(int)(rand()%n));
                strcat(ref[i],temp);
            }
            else if((q<99)&&(m-n) ){
                sprintf(temp,"%d,",(int)(rand()%(m-n)+n));
                strcat(ref[i],temp);
            }
            else{
                sprintf(temp,"%d,",(int)(rand()%(INT_MAX-m)+m));
                strcat(ref[i],temp);
            }
        }
        sm1[i]=shmget(IPC_PRIVATE,sizeof(struct table)*m,IPC_CREAT|0777);
        valid_size[i]=n;
    }
    shmdt(sm2);
    for(int i=0;i<8;i++) cmnd[i]=(char*)malloc(15);
    sprintf(cmnd[0],"./sched");
    sprintf(cmnd[1],"%d",MQ1);
    sprintf(cmnd[2],"%d",MQ2);
    sprintf(cmnd[3],"%d",k);
    cmnd[4]=0;
    if(!(pid[t++]=fork())){
        SCHED(cmnd);
        exit(0);
    }
    cmnd[4]=(char*)malloc(9);
    sprintf(cmnd[0],"xterm");
    sprintf(cmnd[1],"-e");
    sprintf(cmnd[2],"./mmu");
    sprintf(cmnd[3],"%d",SM1);
    sprintf(cmnd[4],"%d",SM2);
    sprintf(cmnd[5],"%d",MQ2);
    sprintf(cmnd[6],"%d",MQ3);
    cmnd[7]=0;
    if(!(pid[t++]=fork())){
        MMU(cmnd);
        exit(0);
    }
    for(int i=0;i<k;i++){
        usleep(250*1e3);
        sprintf(cmnd[0],"./process");
        free(cmnd[1]);
        cmnd[1]=ref[i];
        sprintf(cmnd[2],"%d",MQ1);
        sprintf(cmnd[3],"%d",MQ3);
        sprintf(cmnd[4],"%d",i);
        cmnd[5]=0;
        if(!(pid[t++]=fork())){
            PROCESS(cmnd);
            exit(0);
        }
    }
    P(semid);
    free(cmnd[1]);
    for(int i=0;i<k+2;i++){
        kill(pid[i],SIGINT);
        waitpid(pid[i],0,0);
    }
    for(int i=0;i<5;i++) {
        if(i!=1)free(cmnd[i]);
    }
    for(int i=0;i<k;i++){
        shmctl(sm1[i],IPC_RMID,0);
    }
    sm2=(struct list*)shmat(SM2,0,0);
    id=SM2;
    while(id!=-1){
        prev=sm2;
        if(sm2->next>-1) sm2=(struct list*)shmat(sm2->next,0,0);
        id1=prev->next;
        shmdt(prev);
        shmctl(id,IPC_RMID,0);
        id=id1;
    }
    shmctl(SM1,IPC_RMID,0);
    msgctl(MQ1,IPC_RMID,0);
    msgctl(MQ2,IPC_RMID,0);
    msgctl(MQ3,IPC_RMID,0);
    semctl(semid,0,IPC_RMID,0);
    semctl(semid1,0,IPC_RMID,0);
    shmdt(valid_size);
    shmctl(shmid,IPC_RMID,0);
    return 0;
}