#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

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
	char text[20];
	char flag[5];
}__attribute__((packed)) recv_msg;


int init_socket(int* psfd);
int do_register(int sfd,recv_msg snd);
int do_login(int sfd,recv_msg snd);
int do_quit(int sfd,recv_msg snd);
int second_print(int sfd,recv_msg snd);
int third_print(int sfd,recv_msg snd);
int do_add(int sfd,recv_msg snd);


int main(int argc, const char *argv[])
{
	//网络初始化
	int sfd;
	if(init_socket(&sfd)<0)
	{
		printf("网络初始化失败\n");
		return -1;
	}
    recv_msg snd;
    char choose;
    int res; 

    res = -1;
    while(1)
    {
        printf("********************\n");
        printf("*******1.登录*******\n");
        printf("*******2.注册*******\n");
        printf("*******3.退出*******\n");                            
        printf("********************\n");

        printf("please input your choose>>>");
        scanf("%c",&choose);
        while(getchar() != 10);

		switch(choose)
        {
            case '1':
                res = do_login(sfd,snd);
				if(res == 0)
				{
					if(strcmp(snd.admin,"1") == 1)
					{
						second_print(sfd,snd);//管理员
					}
					else if(strcmp(snd.admin,"0") == 0)
					{
						third_print(sfd,snd);//普通员工
					}
					else
					{
						printf("The admin is error\n");
						break;
					}
				}
                break;
            case '2':
                do_register(sfd,snd);
                break;
            case '3':
            	do_quit(sfd,snd);
               //goto end;
            default:
                printf("! input error,please input again\n");
        }
		//任意键清屏
        printf("input any key to clear>>>");
        while(getchar() != 10);
        system("clear");
	}

	return 0;
}

/*管理员界面*/
int second_print(int sfd,recv_msg snd)
{
	    while(1)
    {
		system("clear");
        printf("------------------------\n");
        printf("-------1.增加信息-------\n");
        printf("-------2.删除信息-------\n");
        printf("-------3.修改信息-------\n");
        printf("-------4.查询信息-------\n");
        printf("-------5.返回上级-------\n");
        printf("------------------------\n");

        printf("please input your choose>>>");
		char choose;
        scanf("%c",&choose);
        while(getchar() != 10);

        switch(choose)
        {
            case '1':                                                
                do_add(sfd,snd);
                break;
				/*
            case '2':
                do_delete();
                break;
            case '3':
                do_update();
                break;
            case '4':
                do_search();
                break;
				*/
            case '5':
                system("clear");
				return 0;
            default:
                printf("! input error,please input again\n");
                break;
        }

		//任意键清屏
        printf("input any key to clear>>>");
        while(getchar() != 10);
        system("clear");
	}

	close(sfd);
	return 0;
}

/*普通员工界面*/
int third_print(int sfd,recv_msg snd)
{
	while(1)
	{
		system("clear");
        printf("------------------------\n");
        printf("-------1.修改信息-------\n");
        printf("-------2.查询信息-------\n");
        printf("-------3.返回上级-------\n");
        printf("------------------------\n");

		printf("please input your choose>>>");
		char choose;
		scanf("%c",&choose);
		while(getchar() != 10);
		switch(choose)
		{
            case '1':
                //do_update();
                break;
            case '2':
                //do_search();
                break;
            case '3':
                //system("clear");
				return 0;
            default:
                printf("! input error,please input again\n");
                break;
		}

		//任意键清屏
        printf("input any key to clear>>>");
        while(getchar() != 10);
        system("clear");
	}

	close(sfd);
	return 0;
}


/*增加信息*/
int do_add(int sfd,recv_msg snd)
{
	int res = 0;
	snd.type = 'A';
	printf("%s-------------------\n",snd.admin);
	if(strcmp(snd.admin,"1") == 0)
	{
		memset(&snd,0,sizeof(snd));
		printf("please input name to add>>");
		scanf("%s",snd.user_n);
		while(getchar() != 10);		
		
		printf("please input age to add>>");
		scanf("%s",snd.age);
		while(getchar() != 10);

		printf("please input tellphone to add>>");
		scanf("%s",snd.tellphone);
		while(getchar() != 10);

		printf("please input address to add>>");
		scanf("%s",snd.address);
		while(getchar() != 10);

		printf("please input salary to add>>");
		scanf("%s",snd.salary);
		while(getchar() != 10);
	}
	else
	{
		printf("! you are not administratortor");
		//system("clear");
	}
	//send
	if(send(sfd,&snd,sizeof(recv_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	//recv
	res = recv(sfd,&snd,sizeof(recv_msg),0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(res == 0)
	{
		printf("服务器关闭\n");
		return -1;
	}
	//分析是否错误包
	printf("%s\n",snd.text);

	return 0;
}


/*退出*/
int do_quit(int sfd,recv_msg snd)
{
	int res = 0;
	snd.type = 'Q';
	res = send(sfd,&snd,sizeof(recv_msg),0);
	if(res < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	exit(0);
	return 0;
}


/*登录*/
int do_login(int sfd,recv_msg snd)
{
	memset(&snd,0,sizeof(snd));
	printf("please input your user name>>");
	scanf("%s",snd.user_n);
	while(getchar() != 10);

	printf("please input your user passwd>>");
	scanf("%s",snd.user_p);
	while(getchar() != 10);

	snd.type = 'L';
	//send
	if(send(sfd,&snd,sizeof(recv_msg),0) < 0)
	{
		ERR_MSG("send");
		return -1;
	}
	//recv
	int res = recv(sfd,&snd,sizeof(recv_msg),0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(res == 0)
	{
		printf("服务器关闭\n");
		return -1;
	}
	//分析是否错误包
	printf("%c\n",snd.type);
	if(snd.type == 'E')
	{
		printf("%s\n",snd.text);
		return -1;
	}
	else
	{
		printf("%s\n",snd.text);
	}
	return 0;
}


/*注册*/
int do_register(int sfd,recv_msg snd)
{
	memset(&snd,0,sizeof(snd));
    printf("please input your user name>>");
    scanf("%s",snd.user_n);
    while(getchar() != 10);

    printf("please input your passwd>>");                            
    scanf("%s",snd.user_p);
    while(getchar() != 10);

    printf("are you administrator? (1 or 0)>>");
    scanf("%s",snd.admin);
	printf("%s!!!!!!!!!!!!!!\n",snd.admin);
    while(getchar() != 10);

    snd.type = 'R';
    //send
    if(send(sfd,&snd,sizeof(recv_msg),0) < 0)
    {
        ERR_MSG("send");
        return -1;
    }
    //recv
    memset(&snd,0,sizeof(snd));
    int res = recv(sfd,&snd,sizeof(recv_msg),0);
    if(res < 0)
    {
        ERR_MSG("recv");
        return -1;
    }
    else if(0 == res)                                                
    {
        printf("服务器关闭\n");
        return -1;
    }
    //分析是否为错误包
    printf("%c\n",snd.type);
    if(snd.type == 'E')
    {
        printf("%s\n",snd.text);
        return -1;
    }
    else
        printf("%s\n",snd.text);
    return 0;
}


/*网络初始化*/
int init_socket(int* psfd)
{
	*psfd = socket(AF_INET,SOCK_STREAM,0);
	if(*psfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}
	//填充地址信息结构体
	struct sockaddr_in sin;
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);

//连接服务器
    if(connect(*psfd,(struct sockaddr*)&sin,sizeof(sin)) < 0)
    {   
        ERR_MSG("connect");
        return -1; 
    }                                                                
    printf("连接成功\n");
	return 0;
}


