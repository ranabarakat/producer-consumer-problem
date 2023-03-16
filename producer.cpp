#include <iostream>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <random>
#include <vector>
#include <chrono>
#include <time.h>

// #define BUFFER_SIZE 1024 
#define SHM_KEY 0x12345
#define SHM_KEY2 0x56789
#define SEM_KEY 0x11111

int shmid;
int shmid2;
int semid;

int hour,min,day,month,year,sec;
long nanosec;
double seconds;
struct timespec ts;
time_t now;
struct tm *local;

/*struct buffer
{
    std::pair<char[11],double> queue[BUFFER_SIZE];
	unsigned front;
	unsigned rear;
};*/

struct commodity{
    char name [11];
    double price;
};

int rear;
int *r;


//struct buffer *buf;
struct commodity *buf;
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
		//shmdt(buf);
        exit(sigNum);
	}
}

double produce(double mean, double sd){
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::normal_distribution<double> dist(mean,sd);
    double num = dist(generator);
    return num;
}

void append(std::string comName,double price,int n){
    
    // append commodity name and price to buffer
    strcpy((buf+(rear))->name,comName.c_str());
    (buf+(rear))->price = price;
    //buf = (buf + sizeof(struct commodity));

    //print details
    //std::cout<<"rear= "<<rear<<"\n";
    //std::cout<<buf->queue[buf->rear].first<<" "<<buf->queue[buf->rear].second<<"\n";
    //update rear
    rear = (rear + sizeof(struct commodity)) % n;
    r = &rear;
    //buf->rear = (buf->rear + 1) % (n);
}

void getTime(){
    clock_gettime(CLOCK_REALTIME,&ts);
    time(&now);
    local = localtime(&now);
    hour = local->tm_hour;
    min = local->tm_min;
    day = local->tm_mday;
    month = local->tm_mon+1;
    year = local->tm_year+1900;
    sec = local->tm_sec;
    nanosec = ts.tv_nsec/1000000;
}

void producer(int semid,int shmid,std::string comName,double mean,double sd,int interval,int n){
     
    // detach, deallocate memory and deallocate semaphores if ctrl+C pressed
	signal(SIGINT, sigHandler);
    
    if((buf = (struct commodity*)shmat(shmid, NULL, 0)) == (void*)-1)
	{
		perror("shmat failed\n");
		exit(2);
	}
    r = &rear;
    if((r = (int*)shmat(shmid2, NULL, 0)) == (void*)-1)
	{
		perror("shmat failed\n");
		exit(2);
	}
    
	while(true)
	{   
        double price = produce(mean,sd);
        getTime();
        fprintf(stderr,"[%02d/%02d/%d %02d:%02d:%02d.%3ld] %s: generating a new value %lf\n",day,month,year,hour,min,sec,nanosec,comName.c_str(),price);
        
        semWait(semid,2);
        std::cout<<semctl(semid,2,GETVAL)<<"\n";
        
        getTime();
        fprintf(stderr,"[%02d/%02d/%d %02d:%02d:%02d.%3ld] %s: trying to get mutex on shared buffer\n",day,month,year,hour,min,sec,nanosec,comName.c_str());
		
        semWait(semid,0);

        /* Critical Section */
        getTime();
        fprintf(stderr,"[%02d/%02d/%d %02d:%02d:%02d.%3ld] %s: placing %lf on shared buffer\n",day,month,year,hour,min,sec,nanosec,comName.c_str(),price);
        append(comName,price,n);
        /* End Critical Section */
		semSignal(semid,0);
		semSignal(semid,1);
        getTime();
        fprintf(stderr,"[%02d/%02d/%d %02d:%02d:%02d.%3ld] %s: sleeping for %d ms\n",day,month,year,hour,min,sec,nanosec,comName.c_str(),interval);
		usleep(interval*1000); // microseconds
	}
}

int main(int argc, char *argv[]){

    if(argc != 6){
        std::cout<<"Invalid number of arguments, terminating...\n";
        exit(2);
    }
    std::vector<std::string> commodities;
    commodities.insert(commodities.end(), { "aluminium","copper","cotton","crudeoil","gold","lead","menthaoil","naturalgas","nickel","silver","zinc" });
    std::string comName = argv[1];
    double mean = std::stod(argv[2]);
    double stddev=std::stod(argv[3]);
    int interval=abs(std::stoi(argv[4]));
    int n=abs(std::stoi(argv[5]));
    int check = 0;
    for(auto x:commodities){
        if(x == comName)
            check = 1;
    }
    if (!check){
        std::cout<<"Invalid commodity name, terminating...\n";
        exit(2);
    }
    /* std::string comName;
    double mean;
    double stddev;
    int interval; */

    shmid = shmget(SHM_KEY,n*sizeof(struct commodity),0666|IPC_CREAT| IPC_EXCL);
    if(shmid>=0){ // if shared memory did not exist before, initialize rear
        //buf->rear=0;
    }
    else if(errno = EEXIST) // else fetch the shared memory with its previous values
        shmid = shmget(SHM_KEY,n*sizeof(struct commodity),0);


    shmid2 = shmget(SHM_KEY2,sizeof(int),0666|IPC_CREAT| IPC_EXCL);
    if(shmid2>=0){ // if shared memory did not exist before, initialize rear
        //buf->rear=0;
        rear=0;
    }
    else if(errno = EEXIST) // else fetch the shared memory with its previous values
        shmid2 = shmget(SHM_KEY2,sizeof(int),0);
      
    
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

    producer(semid,shmid,comName,mean,stddev,interval,n);
    
    return 0;
}
