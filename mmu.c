#define _GNU_SOURCE
#include<sys/sem.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/msg.h>
#include<signal.h>
#include<stdbool.h>
#include<string.h>
#include<fcntl.h>
#include <limits.h>
#define V(semid) semop(semid,&sem_sig,1)
struct table{
    int frame;
    bool valid;
    int lref;
};
struct message
{
    long mtype;
    char mtext[20];
};
struct list{
    int frame;
    int next;
};
int fd,*tfault,*inv,count=0;
struct list *SM2,*SM3;
void print(char output[]){
    printf("%s",output);
    write(fd,output,strlen(output));
}
void pgfltsq(int id,int pg,int type){
    char output[70];
    if(!type){
        sprintf(output,"Page fault sequence - (%d,%d)\n",id,pg);
        tfault[id]++;
    }
    else{
        sprintf(output,"Total no. of faults for process %d=%d\n",id,tfault[id]);
    }
    print(output);
}
void invalid(int id,int pg,int type){
    char output[70];
    if(!type){
        sprintf(output,"Invalid page reference-(%d,%d)\n",id,pg);
        inv[id]++;
    }
    else sprintf(output,"Total invalid references for process %d=%d\n",id,inv[id]);
    print(output);
}
void glord(int id,int pg){
    char output[70];
    sprintf(output,"Global ordering -(%d,%d,%d)\n",++count,id,pg);
    print(output);
}
void PFH(int j,int n,int idx,struct table *SM1[]){
    int loc=0,id;
    int t=INT_MAX;
    if(SM2->next==-1){
        for(int i=0;i<n;i++){
            if(SM1[j][i].valid){
                if((SM1[j][i].lref<=t)) {
                    loc=i;
                    t=SM1[j][i].lref;
                }
            }
        }
        SM1[j][loc].valid=0;
        SM1[j][idx].valid=1;
        SM1[j][idx].lref=count;
        SM1[j][idx].frame=SM1[j][loc].frame;
    }
    else{
        SM3=(struct list*)shmat(SM2->next,0,0);
        SM1[j][idx].frame=SM3->frame;
        SM1[j][idx].valid=1;
        SM1[j][idx].lref=count;
        id=SM3->next;
        shmdt(SM3);
        shmctl(SM2->next,IPC_RMID,0);
        SM2->next=id;
    }

}
void free_frame(int j,int n,struct table *SM1[]){
    int id;
    for(int i=0;i<n;i++){
        if(SM1[j][i].valid){
            id=shmget(IPC_PRIVATE,sizeof(struct list),IPC_CREAT|0777);
            SM3=(struct list*)shmat(id,0,0);
            SM3->next=SM2->next;
            SM2->next=id;
            SM3->frame=SM1[j][i].frame;
            shmdt(SM3);
        }
    }
}
int main(int argc,char* argv[]){
    int MQ3,MQ2,id,SM2_id,SM1_id;
    struct message msg,buff;
    fd=open("result.txt",O_WRONLY|O_TRUNC|O_CREAT,0777);
    struct sembuf sem_sig=(struct sembuf){0,1,0};
    SM1_id=atoi(argv[1]);SM2_id=atoi(argv[2]);MQ2=atoi(argv[3]);MQ3=atoi(argv[4]);
    struct list *prev;
    int *sm1=(int*)shmat(SM1_id,0,0),length,count1=0,id1,n;
    struct shmid_ds buf;
    shmctl(SM1_id, IPC_STAT, &buf);
    int k = (int) buf.shm_segsz / sizeof(int);
    tfault=(int*)malloc(sizeof(int)*k);
    inv=(int*)malloc(sizeof(int)*k);
    //signal(SIGINT,handler);
    int *valid_size=shmat(shmget(ftok("/home",'p'),k*sizeof(int),IPC_CREAT|0777),0,0);
    struct table* SM1[k];
    for(int i=0;i<k;i++){
        shmctl(sm1[i], IPC_STAT, &buf);
        length=buf.shm_segsz/sizeof(struct table);
        SM1[i]=(struct table*)shmat(sm1[i],0,0);
        for(int j=0;j<length;j++){
            SM1[i][j].lref=count;
            SM1[i][j].valid=0;
        }
    }
    shmctl(SM2_id, IPC_STAT, &buf);
    SM3=SM2=(struct list*)shmat(SM2_id,0,0);
    while(1){
        if((n=msgrcv(MQ3,(void*)&buff,13,1,MSG_EXCEPT))==-1){
            perror("recv failed5\n");
            exit(1);
        }
        else{
            // id=SM2_id;
            // SM3=SM2;
            // while(id!=-1){
            //     prev=SM3;
            //     if(SM3->next>-1)SM3=(struct list*)shmat(SM3->next,0,0);
            //     else break;
            //     id1=prev->next;
            //     sprintf(b,"%d ",SM3->frame);
            //     write(fd,b,strlen(b));
            //     id=id1;
            // }
            // write(fd,"\n",1);
            buff.mtext[n]='\0';
            buff.mtype--;
            shmctl(sm1[buff.mtype-1], IPC_STAT, &buf);
            // printf("%ld,%s,%d\n",buff.mtype-1,buff.mtext,length);
            if(atoi(buff.mtext)==-9){
                sprintf(msg.mtext,"TERMINATED");
                pgfltsq(buff.mtype-1,atoi(buff.mtext),1);
                invalid(buff.mtype-1,atoi(buff.mtext),1);
                msg.mtype=buff.mtype;
                count1++;
                free_frame(buff.mtype-1,valid_size[buff.mtype-1],SM1);
                if(msgsnd(MQ2,(void*)&msg,strlen(msg.mtext),0)==-1){
                    perror("send failed4\n");
                    exit(1);
                }
            }
            else{
                glord(buff.mtype-1,atoi(buff.mtext));
                if((atoi(buff.mtext)<valid_size[buff.mtype-1])&&SM1[buff.mtype-1][atoi(buff.mtext)].valid){
                    sprintf(msg.mtext,"%d",SM1[buff.mtype-1][atoi(buff.mtext)].frame);
                    msg.mtype=1;
                    SM1[buff.mtype-1][atoi(buff.mtext)].lref=count;
                    if(msgsnd(MQ3,(void*)&msg,strlen(msg.mtext),0)==-1){
                        perror("send failed6\n");
                        exit(1);
                    }
                }
                else if(atoi(buff.mtext)<valid_size[buff.mtype-1]){
                    sprintf(msg.mtext,"-1");
                    msg.mtype=1;
                    if(msgsnd(MQ3,(void*)&msg,strlen(msg.mtext),0)==-1){
                        perror("send failed7\n");
                        exit(1);
                    }
                    PFH(buff.mtype-1,valid_size[buff.mtype-1],atoi(buff.mtext),SM1);
                    pgfltsq(buff.mtype-1,atoi(buff.mtext),0);
                    sprintf(msg.mtext,"PAGE FAULT HANDLED");
                    msg.mtype=buff.mtype;
                    if(msgsnd(MQ2,(void*)&msg,strlen(msg.mtext),0)==-1){
                        perror("send failed8\n");
                        exit(1);
                    }
                }
                else{
                    sprintf(msg.mtext,"-2");
                    msg.mtype=1;
                    if(msgsnd(MQ3,(void*)&msg,strlen(msg.mtext),0)==-1){
                        perror("send failed9\n");
                        exit(1);
                    }
                    printf("PROCESS %ld TRYING TO ACCESS INVALID PAGE REFERENCE\n",buff.mtype-1);
                    sprintf(msg.mtext,"TERMINATED");
                    invalid(buff.mtype-1,atoi(buff.mtext),0);
                    pgfltsq(buff.mtype-1,atoi(buff.mtext),1);
                    invalid(buff.mtype-1,atoi(buff.mtext),1);
                    msg.mtype=buff.mtype;
                    free_frame(buff.mtype-1,valid_size[buff.mtype-1],SM1);
                    if(msgsnd(MQ2,(void*)&msg,strlen(msg.mtext),0)==-1){
                        perror("send failed10\n");
                        exit(1);
                    }
                }
            }
        }
    }
    for(int i=0;i<k;i++) shmdt(SM1[i]);
    shmdt(sm1);
    id=SM2_id;
    while(id!=-1){
        prev=SM2;
        if(SM2->next>-1)SM2=(struct list*)shmat(SM2->next,0,0);
        id1=prev->next;
        shmdt(prev);
        id=id1;
    }
    free(inv);
    free(tfault);
    close(fd);
    shmdt(valid_size);
    return 0;
}