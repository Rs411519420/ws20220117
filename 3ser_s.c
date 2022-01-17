#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include <pthread.h>


#define N 256
#define PORT 8899
#define IP "192.168.1.2"

#define ERR_MSG(msg) do{\
	printf("__%d__ __%s__\n",__LINE__,__func__);\
	perror(msg);\
}while(0)

typedef struct
{
	char type;
	char admin[5];
	char user_n[20];
	char user_p[20];
	char age[5];
	char tellphone[20];
	char address[20];
	char salary[20];
	char text[N];
	char flag[5];
}__attribute__((packed)) recv_msg;//取消结构体对齐

typedef struct 
{
	int newfd;
	struct sockaddr_in cin;
	sqlite3* db;
}con;


void* recv_msg_cli(void* argc);
int init_socket(int* psfd);
int init_sqlite(sqlite3 **pdb);
int do_register(int newfd,recv_msg rcv,sqlite3*db);
int do_login(int newfd,recv_msg rcv,sqlite3* db);
int do_quit(int newfd,recv_msg rcv,sqlite3* db);
int do_add(int newfd,recv_msg rcv,sqlite3* db);


int main(int argc, const char *argv[])
{
	//数据库初始化
	sqlite3 *db = NULL;
	if(init_sqlite(&db) < 0)
	{
		printf("数据库初始化失败\n");
		return -1;
	}
	//网络初始化
	int sfd = 0;
	if(init_socket(&sfd) < 0)
	{
		printf("网络初始化失败\n");
		return -1;
	}

	//多线程处理，主线程用于连接
	int newfd;
	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);

	while(1)
	{
		newfd = accept(sfd,(struct sockaddr*)&cin,&addrlen);
		if(newfd < 0)
		{
			ERR_MSG("accept");
			return -1;
		}
		printf("[%s:%d] newfd=%d 客户端连接成功\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),newfd);
		
		//创建线程
		con child = {newfd,cin,db};
		pthread_t tid;
		if(pthread_create(&tid,NULL,recv_msg_cli,&child) < 0)
		{
			ERR_MSG("pthread_create");
			return -1;
		}
	}
	return 0;
}

void* recv_msg_cli(void* arg)
{
	//线程分离
	pthread_detach(pthread_self());

	con child = *(con *)arg;
	int newfd=child.newfd;
	struct sockaddr_in cin =child.cin;
	sqlite3* db =child.db;
	
	//recv
	int res = 0;
	recv_msg rcv;
	while(1)
	{
		memset(&rcv,0,sizeof(recv_msg));
		res = recv(newfd,&rcv,sizeof(recv_msg),0);
		if(res < 0)
		{
			ERR_MSG("recv");
			return NULL;
		}
		else if(0 == res)
		{
			printf("[%s:%d] newfd=%d 客户端关闭\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port),newfd);
			break;
		}

		//分析数据包类型
		switch(rcv.type)
		{
			case 'L':
				do_login(newfd,rcv,db);
				break;
			case 'R':
				do_register(newfd,rcv,db);
				break;
			case 'A':
				if(strcmp(rcv.admin,"1") == 0)
				{
					do_add(newfd,rcv,db);
				}
				else
				{
					printf("%s~~~~~~~~~~~~~~~\n",rcv.admin);
					return 0;
				}
				break;
				/*
			case 'D':
				do_delete();
				break;
			case 'U':
				do_update();
				break;
			case 'S':
				do_search();
				break;
				*/
			case 'Q':
				do_quit(newfd,rcv,db);
				pthread_exit(NULL);
				break;
			default:
				printf("协议出错\n");
		}
	}
	sqlite3_close(db);
	pthread_exit(NULL);
}



/*增加信息*/
int do_add(int newfd,recv_msg rcv,sqlite3* db)
{
	char sql[256] = "";
	char* errmsg = NULL;
	bzero(sql,256);
	sprintf(sql,"select * from staff where name=\"%s\";",rcv.user_n);
    char** presult = NULL;
    int row,column;
    if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != 0)
    {
        printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
        return -1; 
    }   
    if(row == 0)
    {  
		bzero(sql,256);
		sprintf(sql,"insert into staff values(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");",rcv.user_n,rcv.age,rcv.tellphone,rcv.address,rcv.salary);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0)
		{
			printf("__%d__ %s\n",__LINE__,errmsg);
			return -1;
		}
		strcpy(rcv.text,"add massage successful");
    } 
	else
	{
		strcpy(rcv.text,"the staff has existed");
	}
	if(send(newfd,&rcv,sizeof(recv_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	printf("%s+++++++++++++++++++++++++++++++\n",rcv.admin);
	return 0;
}


/*退出*/
int do_quit(int newfd,recv_msg rcv,sqlite3* db)
{
	char sql[256] = "";
	char* errmsg = NULL;
	sprintf(sql,"update sign set flag=0 where name=\"%s\";",rcv.user_n);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("__%d__ %s\n",__LINE__,errmsg);
		return -1;
	}
	printf("客户端退出\n");
	printf("%s 退出系统\n",rcv.user_n);

	close(newfd);
	return 0;
}


/*登录*/
int do_login(int newfd,recv_msg rcv,sqlite3* db)
{
	printf("%s/////////////////////////\n",rcv.admin);
	char sql[256] = "";
	char* errmsg = NULL;
	bzero(sql,256);
	sprintf(sql,"select * from sign where name=\"%s\";",rcv.user_n);
	char** presult = NULL;
	int row,column;
	if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != 0)
	{
		printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
		return -1;
	}
	if(row == 0)
	{
		rcv.type = 'E';
		strcpy(rcv.text,"! not exist,please register first");
	}
	else
	{
		if(strcmp("0",presult[7]) != 0)
		{
			rcv.type = 'E';
			bzero(rcv.text,sizeof(rcv.text));
			strcpy(rcv.text,"! the user has logined");
		}
		else
		{
			if(strcmp(rcv.user_p,presult[6]) != 0)
			{
				rcv.type = 'E';
				strcpy(rcv.text,"! the passwd is error");
			}
			else
			{
				strcpy(rcv.text,"login successful");
				bzero(sql,256);
				sprintf(sql,"update sign set flag=1 where name=\"%s\";",rcv.user_n);
				if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0)
				{
					printf("__%d__ %s\n",__LINE__,errmsg);
					return -1;
				}
			}
		}
	}
	strcpy(rcv.flag,"1");
	if(send(newfd,&rcv,sizeof(recv_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}


/*注册*/
int do_register(int newfd,recv_msg rcv,sqlite3* db)
{
    char sql[512] = "";
    sprintf(sql,"select * from sign where name = \"%s\";",rcv.user_n);
    char** presult = NULL;
    int row,column;
    char* errmsg = NULL;
    if(sqlite3_get_table(db,sql,&presult,&row,&column,&errmsg) != 0)
    {
        printf("__%d__ errmsg:%s\n",__LINE__,errmsg);
        return -1;
    }

    if(row != 0)
    {
        rcv.type = 'E';
        strcpy(rcv.text,"! the user has registered");
        if(send(newfd,&rcv,sizeof(recv_msg),0) < 0)
        {
            ERR_MSG("send");
            return -1;
        }
    }
    else
    {
        char sql3[566] = "";
        sprintf(sql3,"insert into sign values(\"%s\",\"%s\",\"%s\",0);",rcv.admin,rcv.user_n,rcv.user_p);
        if(sqlite3_exec(db,sql3,NULL,NULL,&errmsg) != 0)
        {
            printf("__%d__ %s\n",__LINE__,sqlite3_errmsg(db));
            return -1;
        }
		bzero(rcv.text,sizeof(recv_msg));
        strcpy(rcv.text,"register success");
        if(send(newfd,&rcv,sizeof(recv_msg),0) < 0)
        {
            ERR_MSG("send");
            return -1;
        }
    }          
	printf("%s++++++++++++++++++++++\n",rcv.admin);
    sqlite3_free_table(presult);
    return 0;
}


/*数据库初始化*/
int init_sqlite(sqlite3 **pdb)
{
	char sql[256] = "";
	char* errmsg = NULL;
	//打开数据库
	if(sqlite3_open("./staffdata.db",pdb) != SQLITE_OK)
	{
		ERR_MSG("open");
		printf("open error\n");
		return -1;
	}
	//创建员工信息表
	bzero(sql,256);
	sprintf(sql,"create table if not exists staff(name char primary key,age int,tellphone char,address char,salary int);");
	if(sqlite3_exec(*pdb,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("errmsg:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}


	//创建状态表
	bzero(sql,256);
	sprintf(sql,"create table if not exists sign(admin char,name char primary key,passwd char,flag char);");
	if(sqlite3_exec(*pdb,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("errmsg:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}

	//初始化登录状态为0
	bzero(sql,256);
	sprintf(sql,"update sign set flag=0;");
	if(sqlite3_exec(*pdb,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("errmsg:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
	return 0;
}


/*网络初始化*/
int init_socket(int* psfd)
{
	//创建字节流式套接字
	*psfd = socket(AF_INET,SOCK_STREAM,0);
	if(*psfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}
	//允许端口快速重用
	int reuse = 1;
	if(setsockopt(*psfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse)) < 0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}
	//填充地址信息结构体
	struct sockaddr_in sin;
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);
	if(bind(*psfd,(struct sockaddr*)&sin,sizeof(sin)) < 0)
	{
		ERR_MSG("bind");
		return -1;
	}
	//设置被动监听状态
	if((listen(*psfd,10)) < 0)
	{
		ERR_MSG("listen");
		return -1;
	}
	return 0;
}
