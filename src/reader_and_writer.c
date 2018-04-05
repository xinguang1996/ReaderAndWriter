//reader_and_writer.c
//unistd.h是C和C++程序设计语言中提供对POSIX操作系统API的访问功能的头文件的名称
#include <unistd.h>
//线程相关头文件
#include <pthread.h>
//该函数库用于处理游标的移动和荧幕显示
#include <curses.h>
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
	{"w3",3,6},
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
pthread_mutex_t h_mutex_reader_wait; //静态初始化互斥锁h_mutex_reader_wait
pthread_mutex_t h_mutex_first_reader_wait; //静态初始化互斥锁h_mutex_first_reader_wait
pthread_mutex_t h_mutex_wait; //静态初始化互斥锁h_mutex_wait
pthread_mutex_t h_mutex_r_seq_count;//静态初始化互斥锁h_mutex_r_seq_count，用于变量r_seq_count改变时互斥
pthread_mutex_t h_mutex_o_seq_count;//静态初始化互斥锁h_mutex_o_seq_count，用于变量o_seq_count改变时互斥

time_t base;//定义UNIX时间戳，表示为1970年1月1日0时0分0秒到现在的总秒数

/*
	读者优先算法的读者线程处理函数
*/
void* RF_reader_thread(void *data){
	char thread_name[3]; //线程名称
	struct timeval t;
	//(TEST_INFO *)data->i = (TEST_INFO *)data->require_moment;
	strcpy(thread_name,((TEST_INFO *)data)->thread_name); //将线程的名称复制给thread_name变量

	sleep(((TEST_INFO *)data)->require_moment); //使线程休眠，模拟线程的请求的时间
	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->i = t.tv_sec - base;
	printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, thread_name);
	refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
	strcpy(r_seq[r_seq_count++], thread_name);
	pthread_mutex_unlock(&h_mutex_r_seq_count);
	pthread_mutex_lock(&h_mutex_read_count); //read_count的互斥锁开启，相当于P
	read_count++; //读者的数量加一
	if(read_count==1)
		pthread_mutex_lock(&CS_DATA); //申请资源
	pthread_mutex_unlock(&h_mutex_read_count); //取消read_count的互斥，相当于V
	
	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->b = t.tv_sec - base;//计算开始时间
	printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, thread_name);
	refresh();
	pthread_mutex_lock(&h_mutex_o_seq_count);
	strcpy(o_seq[o_seq_count++], thread_name);
	pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",thread_name);
	//refresh();

	sleep(((TEST_INFO *)data)->persist_time);  //使线程休眠，模拟线程的运行需要时间

	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->e = t.tv_sec - base;//计算结束时间
	printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, thread_name);
	refresh();
	pthread_mutex_lock(&h_mutex_read_count);
	read_count--; //线程运行完毕，读者数量减一
	if(read_count==0)
		pthread_mutex_unlock(&CS_DATA); //释放资源
	pthread_mutex_unlock(&h_mutex_read_count);
	return 0;
}

/*
	读者优先算法的写者线程处理函数
*/

void* RF_writer_thread(void *data){
	struct timeval t;
	sleep(((TEST_INFO *)data)->require_moment);
	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->i = t.tv_sec - base;
	printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, ((TEST_INFO *)data)->thread_name);
	refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
	pthread_mutex_lock(&CS_DATA);  //申请资源
	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->b = t.tv_sec - base;
	printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, ((TEST_INFO *)data)->thread_name);
	refresh();
	pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",((TEST_INFO *)data)->thread_name);
	//refresh();
	sleep(((TEST_INFO *)data)->persist_time);
	gettimeofday(&t, NULL);
	((TEST_INFO *)data)->e = t.tv_sec - base;
	printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, ((TEST_INFO *)data)->thread_name);
	refresh();
	pthread_mutex_unlock(&CS_DATA); //释放资源
	return 0;
}

/*
	读者优先算法
*/
void reader_first(){
	int i=0; //for循环控制变量i
	r_seq_count = 0;
	o_seq_count = 0;
	pthread_t h_thread[MAX_THREAD]; //用于声明一个线程ID数组
	struct timeval start_t; //用于表示开始事件，是time.h头文件中的结构体，其中tv_sec表示秒，tv_usec表示微秒	
	gettimeofday(&start_t, NULL);//将当前时间记录至start_t中
	base = start_t.tv_sec;//将当前时间的秒数赋值给base
	//列出初始化线程名称
	printw("reader first init queue:\n");
	printw("name\tr_m\tp_t\n");
	for(i=0;i<MAX_THREAD;i++){
		printw("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
	};
	printw("\n");
	printw("reader first running queue:\n");
	refresh();

	pthread_mutex_init(&CS_DATA,NULL); //动态创建互斥锁，NULL代表使用默认互斥锁属性，默认属性为快速互斥锁

	for(i=0;i<MAX_THREAD;i++){
		//判断是读者还是写者
		if(test_data[i].thread_name[0]=='r')
			pthread_create(&h_thread[i],NULL,RF_reader_thread,&test_data[i]); //创建线程，第一个参数为线程标识符，第二个参数为线程属性设置，第三个参数为函数地址，第四个参数为要传递给函数的参数
		else
			pthread_create(&h_thread[i],NULL,RF_writer_thread,&test_data[i]);
	}

	for(i=0;i<MAX_THREAD;i++){
		pthread_join(h_thread[i],NULL); //当pthread_join()返回之后，应用程序可收回与已终止线程关联的任何数据存储空间，（另外也可设置线程attr属性，当线程结束时直接回收资源）如果没有必要等待特定的线程
	}
	printw("\n");
	refresh();
}


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
        printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, thread_name);
        refresh();
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
        printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, thread_name);
        refresh();
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",thread_name);
	//refresh();

	sleep(((TEST_INFO *)data)->persist_time);

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;//计算结束时间
        printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, thread_name);
        refresh();

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
        printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, ((TEST_INFO *)data)->thread_name);
        refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
	pthread_mutex_lock(&h_mutex_wait);
	pthread_mutex_lock(&CS_DATA);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;
        printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, ((TEST_INFO *)data)->thread_name);
        refresh();
	pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	//printw("%s ",((TEST_INFO *)data)->thread_name);
	//refresh();
	sleep(((TEST_INFO *)data)->persist_time);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;
        printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, ((TEST_INFO *)data)->thread_name);
        refresh();
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
	printw("FCFS init queue:\n");
	printw("name\tr_m\tp_t\n");
        for(i=0;i<MAX_THREAD;i++){
                printw("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
        };
	//for(i=0;i<MAX_THREAD;i++){
	//	printw("%s ",test_data[i].thread_name);
	//};
	printw("\n");
	printw("FCFS running queue:\n");
	refresh();

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
    	printw("\n");
	refresh();

}

/*
	写者优先算法的读者处理函数
*/
void* WF_reader_thread(void *data){
	char thread_name[3];
	struct timeval t;

	strcpy(thread_name,((TEST_INFO *)data)->thread_name);

	sleep(((TEST_INFO *)data)->require_moment);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
        printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, thread_name);
        refresh();
	
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);

	pthread_mutex_lock(&h_mutex_reader_wait);//在读者队列中排队等待
	pthread_mutex_lock(&h_mutex_first_reader_wait);//在读者队列中的头队列
	pthread_mutex_lock(&h_mutex_read_count);
	read_count++;
	if(read_count==1)
		pthread_mutex_lock(&CS_DATA);//申请资源
	pthread_mutex_unlock(&h_mutex_read_count);
	pthread_mutex_unlock(&h_mutex_first_reader_wait);
	pthread_mutex_unlock(&h_mutex_reader_wait);

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;//计算开始时间
        printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, thread_name);
        refresh();
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);

	//printw("%s ",thread_name);
	//refresh();

	sleep(((TEST_INFO *)data)->persist_time);

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;//计算结束时间
        printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, thread_name);
        refresh();
	pthread_mutex_lock(&h_mutex_read_count);
	read_count--;
	if(read_count==0)
		pthread_mutex_unlock(&CS_DATA);
	pthread_mutex_unlock(&h_mutex_read_count);
	return 0;
}

/*
	写者优先算法的写者处理函数
*/
void* WF_writer_thread(void *data){
	struct timeval t;
	sleep(((TEST_INFO *)data)->require_moment);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
        printw("Time:%d:%s is applying for a visit.\n", ((TEST_INFO *)data)->i, ((TEST_INFO *)data)->thread_name);
        refresh();
	pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);

	if(write_count==0)
		pthread_mutex_lock(&h_mutex_first_reader_wait);//进入排队等待读取文件
	write_count++;
	pthread_mutex_unlock(&h_mutex_write_count);

	pthread_mutex_lock(&CS_DATA);//申请资源
	

	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;
        printw("Time:%d:%s start running.\n", ((TEST_INFO *)data)->b, ((TEST_INFO *)data)->thread_name);
        refresh();
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
	
	//printw("%s ",((TEST_INFO *)data)->thread_name);
	//refresh();
	sleep(((TEST_INFO *)data)->persist_time);
	gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;
        printw("Time:%d:%s end.\n", ((TEST_INFO *)data)->e, ((TEST_INFO *)data)->thread_name);
        refresh();
	pthread_mutex_unlock(&CS_DATA);

	pthread_mutex_lock(&h_mutex_write_count);
	write_count--;
	if(write_count==0)
		pthread_mutex_unlock(&h_mutex_first_reader_wait);
	pthread_mutex_unlock(&h_mutex_write_count);
	return 0;
}


/*
	写者优先算法
*/
void writer_first(){
	int i=0;
	r_seq_count = 0;
        o_seq_count = 0;
	pthread_t h_thread[MAX_THREAD];
	struct timeval start_t; //用于表示开始时间，是time.h头文件中的结构体，其中tv_sec表示秒，tv_usec表示微秒 
        gettimeofday(&start_t, NULL);//将当前时间记录至start_t中
        base = start_t.tv_sec;//将当前时间的秒数赋值给base

	printw("writer first init queue:\n");
	printw("name\tr_m\tp_t\n");
        for(i=0;i<MAX_THREAD;i++){
                printw("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
        };
	//for(i=0;i<MAX_THREAD;i++){
	//	printw("%s ",test_data[i].thread_name);
	//}
	printw("\n");
	printw("writer first running queue:\n");
	refresh();

	pthread_mutex_init(&CS_DATA,NULL);

	for(i=0;i<MAX_THREAD;i++){
		if(test_data[i].thread_name[0]=='r')
			pthread_create(&h_thread[i],NULL,WF_reader_thread,&test_data[i]);
		else
			pthread_create(&h_thread[i],NULL,WF_writer_thread,&test_data[i]);
	}
	for(i=0;i<MAX_THREAD;i++){
		pthread_join(h_thread[i],NULL);
	}
    	printw("\n");
	refresh();
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
	printw("The result is saved in %s\n", file_name);
	refresh();
}
int main(int argc,char *argv[]){
	char select;
	bool end=false;
	char file_name[100];//文件名
	char problem_name[100];//算法名称
	int *f;
	initscr();
	strcpy(studentID, "20152517");//学号
	while(!end){
		clear();
		refresh();
		printw("|-----------------------------------|\n");
		printw("|  1:reader first                   |\n");
		printw("|  2.writer first                   |\n");
		printw("|  3:FCFS                           |\n");
		printw("|  4:exit                           |\n");
		printw("|-----------------------------------|\n");
		printw("select a function(1~4):");
		do{
			select=(char)getch();
		}while(select!='1'&&select!='2'&&select!='3'&&select!='4');
		clear();
		refresh();
		switch(select){
		case '1':
			reader_first();
			sprintf(file_name, "%s_reader_first_answer.txt", studentID);
			sprintf(problem_name, "reader_first");
			if((f=fopen(file_name, "w")) == NULL) {
				printw("ERROR: fail to open file:%s\n", file_name);
				refresh();
				exit(1);
			}
			save_answer(f, file_name, problem_name);
			break;
		case '2':
			writer_first();
			sprintf(file_name, "%s_writer_first_answer.txt", studentID);
                        sprintf(problem_name, "writer_first");
                        if((f=fopen(file_name, "w")) == NULL) {
                                printw("ERROR: fail to open file:%s\n", file_name);
                                refresh();
                                exit(1);
                        }
                        save_answer(f, file_name, problem_name);
			break;
		case '3':
			first_come_first_served();
			sprintf(file_name, "%s_FCFS_answer.txt", studentID);
                        sprintf(problem_name, "FCFS");
                        if((f=fopen(file_name, "w")) == NULL) {
                                printw("ERROR: fail to open file:%s\n", file_name);
                                refresh();
                                exit(3);
                        }
                        save_answer(f, file_name, problem_name);
			break;
		case '4':
			end=true;
		}
		printw("\nPress any key to contibue...");
		getch();
		refresh();
	}
	endwin();
	return 0;
}
