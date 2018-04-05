//reader_and_writer.c
#include<stdio.h>
//unistd.h是C和C++程序设计语言中提供对POSIX操作系统API的访问功能的头文件的名称
#include <unistd.h>
//线程相关头文件
#include <pthread.h>
#include<fcntl.h>
//standard library标准头文件，定义了一些宏和通用工具函数
#include <stdlib.h>
//定义了一些字符串处理的函数
#include <string.h>
//时间相关的头文件
#include <time.h>
#define MAX_THREAD 10  //宏定义最大线程数量

//定义结构体，并为该结构体定义别名TEST_INFO
typedef struct{
	char thread_name[3]; //线程名称
	unsigned int require_moment; //申请时间
	unsigned int persist_time; //持续使用时间
	unsigned int i; //与require_moment相同
	unsigned int b; //开始时间
	unsigned int e;//结束时间
}TEST_INFO;

//定义测试数据
TEST_INFO test_data[MAX_THREAD]={
	{"r1",5,9},
	{"w1",12,15},
	{"w2",3,6},
	{"r2",9, 12},
	{"w3",0,3},
	{"w4",6,10},
	{"r3",13,0},
	{"r4",4,7},
	{"w5",10,13},
	{"r5",1,4}
};

char studentID[8];
char r_seq[MAX_THREAD][3];//请求队列
char o_seq[MAX_THREAD][3];//实际运行队列
int read_count=0; //读者数量
int write_count=0; //写者数量
int r_seq_count;//请求队列数量
int o_seq_count;//实际运行队列数量
pthread_mutex_t CS_DATA; //静态初始化互斥锁CS_DATA，代表资源互斥
pthread_mutex_t h_mutex_read_count; //静态初始化互斥锁h_mutex_rwad_count,用于变量read_count改变时互斥
pthread_mutex_t h_mutex_write_count; //静态初始化互斥锁h_mutex_write_cout，用于变量write_count改变时互斥
//pthread_mutex_t h_mutex_reader_wait; //静态初始化互斥锁h_mutex_reader_wait
//pthread_mutex_t h_mutex_first_reader_wait; //静态初始化互斥锁h_mutex_first_reader_wait
pthread_mutex_t h_mutex_wait; //静态初始化互斥锁h_mutex_wait
pthread_mutex_t h_mutex_r_seq_count;//静态初始化互斥锁h_mutex_r_seq_count，用于变量r_seq_count改变时互斥
pthread_mutex_t h_mutex_o_seq_count;//静态初始化互斥锁h_mutex_o_seq_count，用于变量o_seq_count改变时互斥

time_t base;//定义UNIX时间戳，表示为1970年1月1日0时0分0秒到现在的总秒数
/*
	无优先（先到先服务）算法的读者处理函数
*/
void* FIFO_reader_thread(void *data){
	char thread_name[3];
	struct timeval t;
	strcpy(thread_name,((TEST_INFO *)data)->thread_name);

	sleep(((TEST_INFO *)data)->require_moment);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
        printf("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, thread_name);
        //refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
	pthread_mutex_lock(&h_mutex_wait);//进入申请资源队列，相当于P,主要防止当有读者在读文件时，写者申请不到文件资源，而后面的读者可以获得文件资源
	pthread_mutex_lock(&h_mutex_read_count);
	read_count++;
	if(read_count==1)
		pthread_mutex_lock(&CS_DATA);//是否可以获得文件资源
	pthread_mutex_unlock(&h_mutex_read_count);
	pthread_mutex_unlock(&h_mutex_wait);

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;//计算开始时间
        printf("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, thread_name);
        //refresh();
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",thread_name);
	//refresh();

	sleep(((TEST_INFO *)data)->persist_time);

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;//计算结束时间
        printf("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, thread_name);
        //refresh();

	pthread_mutex_lock(&h_mutex_read_count);
	read_count--;
	if(read_count==0)
		pthread_mutex_unlock(&CS_DATA);
	pthread_mutex_unlock(&h_mutex_read_count);
	return 0;
}

/*
	无优先（先到先服务）算法的写者处理函数
*/
void* FIFO_writer_thread(void *data){
	struct timeval t;
	sleep(((TEST_INFO *)data)->require_moment);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
        printf("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, ((TEST_INFO *)data)->thread_name);
        //refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
	pthread_mutex_lock(&h_mutex_wait);
	pthread_mutex_lock(&CS_DATA);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;
        printf("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, ((TEST_INFO *)data)->thread_name);
        //refresh();
	pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",((TEST_INFO *)data)->thread_name);
	//refresh();
	sleep(((TEST_INFO *)data)->persist_time);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;
        printf("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, ((TEST_INFO *)data)->thread_name);
        //refresh();
	pthread_mutex_unlock(&CS_DATA);
	pthread_mutex_unlock(&h_mutex_wait);
	return 0;
}

/*
	无优先（先来先服务）算法
*/
void first_come_first_served(){
	int i=0;
	r_seq_count = 0;
	o_seq_count = 0;
	pthread_t h_thread[MAX_THREAD];
	struct timeval start_t; //用于表示开始事件，是time.h头文件中的结构体，其中tv_sec表示秒，tv_usec表示微秒 
        gettimeofday(&start_t, NULL);//将当前时间记录至start_t中
        base = start_t.tv_sec;//将当前时间的秒数赋值给base
        //列出初始化线程名称
	printf("FCFS init queue:\n");
	printf("name\tr_m\tp_t\n");
        for(i=0;i<MAX_THREAD;i++){
                printf("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
        };
	//for(i=0;i<MAX_THREAD;i++){
	//	printw("%s ",test_data[i].thread_name);
	//};
	printf("\n");
	printf("FCFS running queue:\n");
	//refresh();

	pthread_mutex_init(&CS_DATA,NULL);

	for(i=0;i<MAX_THREAD;i++){
		if(test_data[i].thread_name[0]=='r')
			pthread_create(&h_thread[i],NULL,FIFO_reader_thread,&test_data[i]);
		else
			pthread_create(&h_thread[i],NULL,FIFO_writer_thread,&test_data[i]);
	}

	for(i=0;i<MAX_THREAD;i++){
		pthread_join(h_thread[i],NULL);
	}
    	printf("\n");
	//refresh();

}

void save_answer(FILE *f, char *file_name, char *problem_name) {
	int i;
	fprintf(f, "\t%s\n\tr/w  problem:%s\n\n", file_name, problem_name);
	fprintf(f, "name\tr_m\tp_t\ti_t\tb_t\te_t\t\n");
	for(i = 0; i < MAX_THREAD; i++) {
		fprintf(f, "%4s\t%3d\t%3d\t%3d\t%3d\t%3d\t\n", test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time, test_data[i].i, test_data[i].b, test_data[i].e);
	}
	fprintf(f, "\n");
	fprintf(f, "r_seq:");
	for(i = 0; i < MAX_THREAD; i++) {
		fprintf(f, "%4s", r_seq[i]);
	}
	fprintf(f, "\n");
	fprintf(f, "o_seq:");
	for(i = 0; i < MAX_THREAD; i++) {
		fprintf(f, "%4s", o_seq[i]);
	}
	fprintf(f, "\n");
	close(f);
	printf("The result is saved in %s\n", file_name);
	//refresh();
}
int main(int argc,char *argv[]){
	char file_name[100];//文件名
	char problem_name[100];//算法名称
	int *f;
	//initscr();
	strcpy(studentID, "20152517");//学号
	first_come_first_served();
        sprintf(file_name, "%s_FCFS_answer.txt", studentID);
        sprintf(problem_name, "FCFS");
        if((f=fopen(file_name, "w")) == NULL) {
        	printf("ERROR: fail to open file:%s\n", file_name);
                //refresh();
                exit(3);
        }
        save_answer(f, file_name, problem_name);
	//endwin();
	return 0;
}
