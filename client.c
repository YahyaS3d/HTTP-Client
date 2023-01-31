//By: Yahya Saad ID: 322944869
//Ex2: client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
//-------macros--------
#define WRONG_COMMAND printf("Usage: client [-p <text>] [-r n < pr1=value1 pr2=value2 …>] <URL>\n");
#define MAX_PORT 32 // 2^5 set a random default
#define MAX_PORT_VAL 65535 // 2^16  - 1
#define HTTP_SIZE 7 //"http://" size as a string
#define COMMAND 255 //2^8 - 1
#define BODY_LENGTH 16 // 2^4
//---------private function-------
void error(char *);
int onlyNum(char *);
void freeAll(char*, char*, char**, int);
//---------main program-------
int main(int argc, char *argv[]){
    int p = 0;                // -p segmentation
    int r = 0;                //  -r segmentation
    int R_count=0;            // counter for -r parameters
    int check_host=0;        // check host status
    char* command;           // finalize command pattern
    char** R_para;          // array contains -r parameters and their values
    char buffer[100];// test for buffer size 100' an example
    char* host=NULL;//set the host value here
    char* path;// set the path value
    int size=argc-1; // command length
    // set port allocated memory to zero
    char* port=(char*)calloc(MAX_PORT,sizeof(char));       //array to hold port with max size =  65535
    if(port==NULL){
        fprintf(stderr, "problem with allocating memory \n");
        exit(EXIT_FAILURE) ;
    }
    //check if command length is suitable
    if (argc < 2) {
        fprintf(stderr, "command is too short \n");
        free(port);
        exit(EXIT_FAILURE) ;
    }


    char* body;
        //request parsing
    for(int i=1; i<argc; i++){

        // check -p situation
        if(strcmp(argv[i],"-p")==0 && p==0){
            if(i+1<argc){
                body=argv[i+1];
                size=size-2;
                p=1;
                i++;
            }
        }

            // check -r situation
        else if(strcmp(argv[i],"-r")==0 && r==0){
            if(i+1<argc){
                if(onlyNum(argv[i+1])==-1)
                    break;
                R_count=atoi(argv[i+1]);
                if(R_count<0)
                    break;

                i+=2;
                //init allocated memory to zero - make -r array with r-count size
                //2d array to save r parameters
                R_para=(char**)calloc(R_count,sizeof(char*));
                if(R_para==NULL){
                    fprintf(stderr, "problem with allocating memory \n");
                    free(port);
                    if(check_host==1)
                        free(host);
                    exit(EXIT_FAILURE) ;
                }
                r=1;
                int flag=R_count;//set a temporary flag
                int index=0;
                int j=i;
                while(flag>0){
                    if(argv[j]==NULL)
                        break;
                    else if(strchr(argv[j],'=')==NULL)
                        break;
                    else if(strcmp(strchr(argv[j],'='),"=")==0 || strcmp(strchr(argv[j],'='),argv[j])==0)
                        break;
                    R_para[index]=argv[j];
                    index++;
                    flag--;
                    j++;
                }
                if(flag==0)
                    size=size-2-R_count;

                i+=R_count-1;
            }
        }

            //after http
        else if(strstr(argv[i],"http://")!=NULL && check_host==0){
            char* tmp;
            tmp=strstr(argv[i],"http://");
            tmp+=HTTP_SIZE;
            size--;
            //check port
            if(strstr(tmp,":")==NULL){
                //set default port to 80
                port[0]='8';
                port[1]='0';
                port[2]='\0';
            }
            else
            {
                char* tmp;
                tmp=strstr(tmp,":");
                tmp++;
                if(tmp[0]=='/')
                    size=-1;
                int h;
                for(h=0; h< MAX_PORT-1; h++){
                    if(tmp[h]=='\0' || tmp[h]=='/')
                        break;
                    port[h]=tmp[h];
                }
                port[MAX_PORT-1]='\0';

                tmp+=h;
                if(tmp[0]!='\0' && tmp[0]!='/')
                    size=-1;

            }

            host=(char*)calloc(strlen(tmp)+1,sizeof(char));
            if(host==NULL){
                fprintf(stderr, "problem with allocating memory \n");
                free(port);
                if(r==1)
                    free(R_para);
                exit(EXIT_FAILURE) ;
            }
            check_host=1;
            path=strstr(tmp,"/");
            for(int i=0; i<strlen(tmp); i++){
                if(tmp[i]=='\0' || tmp[i]=='/' || tmp[i]==':')
                    break;
                host[i]=tmp[i];
            }
        }
            //usage error state
        else{
            WRONG_COMMAND;
            if(check_host==1)
                freeAll(host,port,R_para,r);

            exit(EXIT_FAILURE) ;
        }

    }
    if(size!=0 || onlyNum(port)==-1 || atoi(port)>MAX_PORT_VAL || host==NULL){
        WRONG_COMMAND;
        if(check_host==1)
            freeAll(host,port,R_para,r);
        exit(EXIT_FAILURE) ;
    }
    //----------------connect to the server------------------
    int mk_connect; // in order to connect with the server 
    int s;             // compute socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0){
        free(port);
        error("socket failed");
    }
    struct hostent *server;
    server = gethostbyname(host);
    if (server == NULL) {
        herror("host doesn't exist");
        if(check_host==1)
            freeAll(host,port,R_para,r);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(atoi(port));

    mk_connect = connect(s, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (mk_connect < 0){
        freeAll(host,port,R_para,r);
        error("connect failed:");
    }

    int pcount;//path count length
    if(path==NULL)
      pcount = 0;
    else
       pcount=strlen(path);

    int command_length=strlen(host)+pcount+COMMAND;
    command=(char*)calloc(command_length,sizeof(char));
    if(command==NULL){
        fprintf(stderr, "problem with allocating memory \n");
        freeAll(host,port,R_para,r);
        exit(EXIT_FAILURE);
    }
    if(p==1)
        strcat(command,"POST ");
    else
        strcat(command,"GET ");
    if(path==NULL)
        strcat(command,"/");
    else
        strcat(command,path);

    if(r==1 && R_count!=0){
        strcat(command,"?");
        for(int i=0;i<R_count;i++){
            strcat(command,R_para[i]);
            if(i+1!=R_count)
                strcat(command,"&");
        }
    }//request is ready for printing
    strcat(command," HTTP/1.0\r\nHost: ");
    strcat(command,host);
    if(p==1){
        strcat(command,"\r\nContent-length:");
        char body_length[BODY_LENGTH];
        sprintf(body_length,"%ld",strlen(body));
        strcat(command,body_length);
    }
    strcat(command,"\r\n\r\n");
    if(p==1){
        strcat(command,body);
    }
    printf("HTTP request =\n%s\nLEN = %ld\n",command,strlen(command));
    // send and then receive response from the server
    write(s, command, strlen(command)+1) ;
    int bytes_read=0;
    do{
        mk_connect = read(s, buffer,  100) ;
        bytes_read+=mk_connect;
        buffer[mk_connect]='\0';
        if(mk_connect > 0)
            printf("%s", buffer) ;
        else if(mk_connect<0)
            error("read() failed") ;
    }while(mk_connect>0);

    printf("\nTotal received response bytes: %d\n",bytes_read);
    close(s);
    freeAll(host,port,R_para,r);
    free(command);
    return EXIT_SUCCESS;
}
//private func : In any case of a failure in one of the system calls, use perror(<sys_call>) and exit the program
void error(char* str){
    perror(str);
    exit(EXIT_FAILURE) ;
}
//private func: Checking allocations state and free all if needed
void freeAll(char* host, char* port, char** R_para, int R_flag){
    if(host)
        free(host);
    if(port)
        free(port);
    if(R_flag==1)
        free(R_para);
}
//private func: Return 0 if the string includes only numbers
int onlyNum(char* str){
    int length = strlen(str);
    for(int i=0; i<length ;i++){
        if(str[i]<='9' && str[i]>='0')
            continue;
        return -1;
    }
    return 0;
}

