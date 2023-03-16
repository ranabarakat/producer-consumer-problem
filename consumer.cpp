#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <time.h>
#include <vector>
#include <cstring>
#include <signal.h>
#include <map>

// #define BUFFER_SIZE 1024
#define SHM_KEY 0x12345
#define SEM_KEY 0x11111

int shmid;
int semid;

/*struct buffer
{
    std::pair<char[11],double> queue[BUFFER_SIZE];
	unsigned front;
	unsigned rear;
};
struct buffer *buf;*/

struct commodity{
    char name [11];
    double price;
};

struct commodity *buf;
int front;

void semSetVal(int id, int value,int semNum){
    struct sembuf s;
    s.sem_num = semNum;
	s.sem_op = value;
	s.sem_flg = 0;
	semop(id, &s, 1);
}


void semSignal(int id, int semNum){
	struct sembuf s;
    s.sem_num = semNum;
	s.sem_op = 1;
	s.sem_flg = SEM_UNDO;
	semop(id, &s, 1);
}

void semWait(int id,int semNum){
	struct sembuf s;
	s.sem_num = semNum;
	s.sem_op = -1;
	s.sem_flg = SEM_UNDO;
	semop(id, &s, 1);
}


void sigHandler(int sigNum)
{
	if(sigNum == SIGINT)
	{
		// detach memory
		shmdt(buf);

		// deallocate semaphores
		semctl(semid, 0, IPC_RMID);

		// deallocate memory
		shmctl(shmid, IPC_RMID, NULL);
        exit(sigNum);
	}
}

struct prices{
    double currentPrice=0;
    double avgPrice=0;
    char *arrow=(char*)" ";
    char *color=(char*)"\033[;36m";
};

std::vector<std::pair<std::string,double>> queue;
std::map<std::string,struct prices> com;

void consume(std::string comName,double price){
    system("clear");
    //printf("\e[1;1H\e[2J");
    for (int x=0; x<comName.length(); x++)
        comName[x]=tolower(comName[x]);
    
    queue.push_back(std::make_pair(comName,price));
    int items = 0;
    double sum = 0;
    double avgPrice;

    for(int i = queue.size() - 1; i >= 0; i--)
    {
        if (items==5) break;
        if(queue[i].first == comName){
            sum+=queue[i].second;
            items++;
        }
       
    }
    avgPrice = sum/items;
    if (price>com[comName].currentPrice){
        com[comName].arrow = (char*)"↑";
        com[comName].color = (char*)"\033[;32m";
    }
    else{
        com[comName].arrow = (char*)"↓";
        com[comName].color = (char*)"\033[;31m";
    }
    com[comName].currentPrice = price;
    com[comName].avgPrice = avgPrice;

    printf("+-------------------------------------+\n");
    printf("| Currency      |  Price   | AvgPrice |\n");
    printf("+-------------------------------------+\n");
    printf("| ALUMINIUM     | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["aluminium"].color,com["aluminium"].currentPrice,com["aluminium"].arrow,com["aluminium"].color,com["aluminium"].avgPrice,com["aluminium"].arrow);
    printf("| COPPER        | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["copper"].color,com["copper"].currentPrice,com["copper"].arrow,com["copper"].color,com["copper"].avgPrice,com["copper"].arrow);
    printf("| COTTON        | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["cotton"].color,com["cotton"].currentPrice,com["cotton"].arrow,com["cotton"].color,com["cotton"].avgPrice,com["cotton"].arrow);
    printf("| CRUDEOIL      | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["crudeoil"].color,com["crudeoil"].currentPrice,com["crudeoil"].arrow,com["crudeoil"].color,com["crudeoil"].avgPrice,com["crudeoil"].arrow);
    printf("| GOLD          | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["gold"].color,com["gold"].currentPrice,com["gold"].arrow,com["gold"].color,com["gold"].avgPrice,com["gold"].arrow);
    printf("| LEAD          | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["lead"].color,com["lead"].currentPrice,com["lead"].arrow,com["lead"].color,com["lead"].avgPrice,com["lead"].arrow);
    printf("| MENTHAOIL     | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["menthaoil"].color,com["menthaoil"].currentPrice,com["menthaoil"].arrow,com["menthaoil"].color,com["menthaoil"].avgPrice,com["menthaoil"].arrow);
    printf("| NATURALGAS    | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["naturalgas"].color,com["naturalgas"].currentPrice,com["naturalgas"].arrow,com["naturalgas"].color,com["naturalgas"].avgPrice,com["naturalgas"].arrow);
    printf("| NICKEL        | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["nickel"].color,com["nickel"].currentPrice,com["nickel"].arrow,com["nickel"].color,com["nickel"].avgPrice,com["nickel"].arrow);
    printf("| SILVER        | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["silver"].color,com["silver"].currentPrice,com["silver"].arrow,com["silver"].color,com["silver"].avgPrice,com["silver"].arrow);
    printf("| ZINC          | %s%7.2lf%s \033[0m| %s%7.2lf%s \033[0m|\n",com["zinc"].color,com["zinc"].currentPrice,com["zinc"].arrow,com["zinc"].color,com["zinc"].avgPrice,com["zinc"].arrow);
    printf("+-------------------------------------+\n");
  
}

void consumer(int semid ,int shmid,int n){
    std::string comName;
    double price;
    if((buf = (struct commodity*)shmat(shmid, NULL, 0)) == (void*)-1)
	{
		perror("shmat failed\n");
		exit(2);
	}
    signal(SIGINT, sigHandler);

    while(true)
	{
		semWait(semid, 1);
		semWait(semid, 0);
        /* Critical Section */
        comName = std::string(((buf+front)->name));
        price = (buf+front)->price;

        // update front 
        front = (front + sizeof(struct commodity)) % n;
	    //buf->front = (buf->front + 1) % (n);
        /* End Critical Section */
		semSignal(semid,0);
        std::cout<<semctl(semid,2,GETVAL)<<"\n";
		semSignal(semid,2);
        std::cout<<semctl(semid,2,GETVAL)<<"\n";
        consume(comName,price);
	}

}

int main(int argc, char *argv[]){

    int n=abs(std::stoi(argv[1]));

    if((shmid = shmget(SHM_KEY,n*sizeof(struct commodity),0666|IPC_CREAT)) == -1){
        perror("shmget failed\n");
        exit(2);
    }

    if((buf = (struct commodity*)shmat(shmid, NULL, 0)) == (void*)-1)
	{
		perror("shmat failed\n");
		exit(2);
	}

   // buf->front=0;
    front = 0;
    semid = semget(SEM_KEY,3,IPC_CREAT | IPC_EXCL | 0666);
    if (semid>=0){
        /* first semaphore in the set for handling critical section entry, initialzed with value = 1 */
        semSetVal(semid,1,0); 

        /* second semaphore in the set for blocking if buffer is empty, initialzed with value = 0 */
        semSetVal(semid,0,1);

        /* third semaphore in the set for blocking if buffer is full, initialzed with value = BUFFER_SIZE */
        semSetVal(semid,n,2);
    }

    else if (errno == EEXIST) // semaphore set already exists, fetch its semid
        semid = semget(SEM_KEY, 3, 0);
        std::cout<<semctl(semid,2,GETVAL)<<"\n";

    consumer(semid,shmid,n);

    return 0;
}

