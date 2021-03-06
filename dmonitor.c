#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>

#define mutexNum 100
#define threadNum 10

struct Mnode//node for monitor
{       
        pthread_t thid;
        pthread_mutex_t *mutex;
};
struct Edge
{
	pthread_t thid;
	pthread_mutex_t *src;
	pthread_mutex_t *dest;
};

pthread_mutex_t localmutex = PTHREAD_MUTEX_INITIALIZER;
struct Mnode monitor[threadNum][mutexNum];
pthread_mutex_t*mArr[mutexNum];
int ** adjArray;

void addToMonitor( pthread_t thid,pthread_mutex_t *mutex){//add to monitor array
    for(int i=0; i<threadNum; i++){
        if(monitor[i][0].thid == thid){
            for(int j=0; j<mutexNum; j++){
                //thid는 같을때 마지막 칸을 찾아서 넣어주면 된다.
                if(monitor[i][j].thid == 0){//마지막 칸 발견
                    monitor[i][j].thid = thid;
                    monitor[i][j].mutex = mutex;
                    return;
                }        
            }
        }else if(monitor[i][0].thid == 0){
            //같은 thid가 없을때 -> 새롭게 시작
            monitor[i][0].thid = thid;
            monitor[i][0].mutex = mutex;
            return;
        }
    }
}

// We return the pointer
void **create2darray(int count) /* Allocate the array */
{
    /* Check if allocation succeeded. (check for NULL pointer) */
    int i;
    adjArray = malloc(count*sizeof(int *));
    for(i = 0 ; i < count ; i++)
        adjArray[i] = malloc( count*sizeof(int) );
}


//now have to find graph adjArray
int mutexArray(){
    
    //mutex array 안에 서로 다른 mutex를 넣어줘야함.
    int lastPoint=0;
    //배열을 반복문을 돌리는데 tid=0을 만나지않을 때 까지
    for(int i=0 ;i<threadNum; i++){
        if(monitor[i][0].thid == 0){
            return lastPoint;
        }
	for(int j=0; j<mutexNum; j++){
            if(monitor[i][j].thid == 0)    break;
            else{
                int diff =1;// 새로운 것인지 아닌 것인지를 알려주는 변수-> 새롭다고 초기화
                for(int k=0; k<lastPoint; k++){//지금의 mutex가 기존에 있는 것인지 검사.
                    if(mArr[j]!=NULL && monitor[i][j].mutex == mArr[k])//새롭지 않다-> diff=0 으로 변경
                        diff=0;
                }
                if(diff ==1){
                    mArr[lastPoint] = monitor[i][j].mutex;
                    lastPoint++;
                }
            }
        }
    }

	//ㅏ㈀瘦沮� 하면 총 몇개의 mutex종류가 있는 지 알 수 있다.
	return lastPoint;
	
}
int getNum(pthread_mutex_t *mutex, int length){
	for(int i=0; i<length; i++){
        if(mArr[i] == mutex){
            return i;
	}
    }
}

void MakeAdjArray(){
    int count = mutexArray();
    create2darray(count);
   // int adjArray[count][count];
    
    for(int i=0; i<count; i++){
        for(int j=0; j<count; j++){
            adjArray[i][j] =0;
        }
    }
    
    for(int i=0 ;i<threadNum; i++){
        if(monitor[i][0].thid == 0){
            return;
        }
    for(int j=0; j<mutexNum; j++){
        if(monitor[i][j+1].mutex == NULL)    break;//a->b 정보가 필요하기 때문에 다음 인덱스 정보인 j+1의 유무도 확인해야함.
        else{   
                //src->dest에 관련 인덱스를 찾음
                int src = getNum(monitor[i][j].mutex, count);
                int dest = getNum(monitor[i][j+1].mutex, count);
                //adjArray에 표시
                adjArray[src][dest] = 1;
            }
        }
    }
    
   
    return;
}
//find cycle
int cycleFinder(int* checker,int i, int count){
  //checker[i] = index;
    int j=0;
    for(j=0; j<count; j++){
        if(adjArray[i][j] == 1){
            if(checker[j] == 1){
                printf("|");
                for (int j=0; j<count; j++){
                    printf("%d ", checker[j]);
                }
                printf("|");
                return 1;
            }
            else{
		checker[i] = j;
                return cycleFinder(checker, j,count);
            }
            
        }
    }
    if(j==count){
	return 0;
    }
}

int
countMutex(int arr[], int index, int count){
	int check[count];
	for(int i=0; i<count; i++)	check[i] =0;
	int mutexNode=0;
	while(1){
		printf("index %d ",index);
		check[index]=1;
		mutexNode++;
		index = arr[index];
		if(check[index] != 0)	break;
	}
	return mutexNode;
}
pthread_t
findThid(pthread_mutex_t *src, pthread_mutex_t *dest){
	for(int i=0 ;i<threadNum; i++){
        if(monitor[i][0].thid == 0){
		printf("find error..\n");
            	return -1;
        }
        for(int j=0; j<mutexNum; j++){
            if(monitor[i][j+1].thid == 0)    break;
            else{
			if(monitor[i][j].mutex == src && monitor[i][j+1].mutex == dest){
				return monitor[i][j].thid;
			} 
		}
	    }
        }
	return -1;
}
void
fillEdges(int* checker, int index, struct Edge* edges,int cycledMutex){
	//a->b, b->c라는 정보를 넣고 싶은데	
	//1->2	를 a->b로 바꾸자
	for(int i=0; i<cycledMutex; i++){
		int srcNum = checker[index];
		int destNum = checker[srcNum];

       		pthread_mutex_t *src = mArr[srcNum];
        	pthread_mutex_t *dest = mArr[destNum];

		//find a-b in array
		int thid = findThid(src, dest);
		//add to edges
		edges[i].src = src;
		edges[i].dest = dest;
		edges[i].thid = thid;

		index = destNum;
		printf("destNum : %d\n", destNum);
	}
	printf("cycled mutex %d\n", cycledMutex);
	for(int i=0; i<cycledMutex; i++){
		printf("%lu ", edges[i].thid);
	}
}
//같은 쓰레드에 ㅐ獵쩝� 검사
int check1(struct Edge edges[], int count){
	int diff =1;
	for(int i=0; i<count-1; i++){
		for(int j=i+1; j<count; j++){
			if(edges[i].thid == edges[j].thid)	diff=0;
			}
		}
	//1 -> 위험하다 0 -> 위험하지 않다
	return diff;

}
//guard가 있는지 검사
int check2(struct Edge edges[],int count){
	return 1;
}
//
int check3(struct Edge edges[],int count){
	return 1;

}
int
pthread_mutex_lock (pthread_mutex_t *mutex)
{
	static __thread int n_mutex = 0 ; //https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Thread-Local.html
	n_mutex += 1 ;

	int  (*lockp)(pthread_mutex_t *mutex) ;
        char * error ;

        lockp = dlsym(RTLD_NEXT, "pthread_mutex_lock") ;
        if ((error = dlerror()) != 0x0)
                exit(1); 

	if (n_mutex == 1) {
		pthread_mutex_lock(&localmutex);
		//add to monitor array
		addToMonitor(pthread_self(), mutex);
        	MakeAdjArray();
        	//adjPrinter(adjArray);
        	int count =mutexArray();
       		int checker[count];
        	
		for(int i=0; i<count; i++){
        	    checker[i] = 0;
	        }
		int index=cycleFinder(checker,0,count);		
		if(index){
			//사이클이 있다면
			int cycledMutex = countMutex(checker,index,count);//사이클에 있는 노드 수
			printf("%d", cycledMutex);
			struct Edge edges[cycledMutex];//위험할 수 있는 node에 대한 정보가 담겨있음
			fillEdges(checker,index,edges,cycledMutex);
			/*edges 안에는 mutex* src, mutex* dest, thid 정보가 있는 배열, count = 문제가 되는 node 수*/ 
			if(check1(edges, count)&&check2(edges, count)&&check3(edges, count)){
				//위험한 사이클임을 감지-> backtrace 호출
				int i ;
                		void * arr[10] ;
                		char ** stack ;


                		size_t sz = backtrace(arr, 10) ;
                		stack = backtrace_symbols(arr, sz) ;
                			
				FILE * fp;
   				/* open the file for writing*/
   				fp = fopen ("dmonitor.trace","w");
 
   				for(i = 0; i < sz;i++){		
       					fprintf (fp, "%s\n", stack[i]);
   				}
 
   				/* close the file*/  
   				fclose (fp);
			}
		}
		pthread_mutex_unlock(&localmutex);
	}	
		
    	
	n_mutex-= 1 ;

	return 0;
}
