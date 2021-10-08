/* -------------------------------------------------------------------------- */
/* chat client                                                                */
/*                                                                            */
/* client program that works with datagram type sockets sending entries typed */
/* by a user and in order for the server  to receive them and forward them to */
/* all of the nodes connected.                                                */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* libraries needed for the program execution                                 */
/* -------------------------------------------------------------------------- */
#include <netinet/in.h>                /* TCP/IP library                      */
#include <arpa/inet.h>                 /* Newer functionality for  the TCP/IP */
                                       /* library                             */
#include <sys/socket.h>                /* sockets library                     */
#include <sys/types.h>                 /* shared data types                   */
#include <stdio.h>                     /* standard input/output               */
#include <unistd.h>                    /* unix standard functions             */
#include <string.h>                    /* text handling functions             */
#include <pthread.h>                   /* libraries for thread handling       */
#include <time.h>

/* -------------------------------------------------------------------------- */
/* global definitions                                                         */
/* -------------------------------------------------------------------------- */
#define  BUFFERSIZE 1024               /* buffer size                         */

/* -------------------------------------------------------------------------- */
/* global variables and structures                                            */
/* -------------------------------------------------------------------------- */
struct data
  {
    int  data_type;                              /* type of data sent         */
    int  chat_id;                                /* chat id sent by server    */
    char data_text[BUFFERSIZE-(sizeof(int)*2)];  /* data sent                 */
  };

struct thread_args
{
  int sfd;
  int chat_id;
  struct sockaddr_in sock_write;
};


/* ------------------------------------------------------------------------- */
/* print_message()                                                           */
/*                                                                           */
/* this function will constantly look for a new message to print coming from */
/* the server                                                                */
/* void *ptr  - pointer that will receive the parameters for the thread      */
/*                                                                           */
/* ------------------------------------------------------------------------- */
void *print_message(void *ptr)
  {
    int    *sock_desc;                 /* pointer for parameter              */
    int    read_char;                  /* read characters                    */
    char   text1[BUFFERSIZE];          /* reading buffer                     */

    sock_desc = (int *)ptr;            /* pointer initialization             */
    while(1)
      {
        read_char = recvfrom(*sock_desc,text1,BUFFERSIZE,0,NULL,NULL);
        text1[read_char] = '\0';
        printf("%s\n",text1);
      }
}

void *heartbeat_thread(void *ptr)
{
  time_t t;
  struct data message;

  message.chat_id = ((struct thread_args *)ptr)->chat_id;
  message.data_type = 2;
  while (1)
  {
    t = time(NULL);
    memcpy(message.data_text, &t, sizeof(t));
    sendto(((struct thread_args *)ptr)->sfd,(struct data *)&(message),sizeof(struct data),0,(struct sockaddr *)&(((struct thread_args *)ptr)->sock_write),sizeof(((struct thread_args *)ptr)->sock_write));
    sleep(10);
  }    
}
/* -------------------------------------------------------------------------- */
/* main ()                                                                    */
/*                                                                            */
/* main function of the system                                                */
/* -------------------------------------------------------------------------- */
int main()
  {
    struct sockaddr_in sock_write;     /* structure for the write socket      */
    struct data message;               /* message to sendto the server        */
    char   text1[BUFFERSIZE];          /* reading buffer                      */
    char   *auxptr;                    /* auxiliar char pointer               */
    int    read_char;                  /* read characters                     */
    int    i;                          /* counter                             */
    int    sfd;                        /* socket descriptor                   */
    int    chat_id;                    /* identificator in the chat session   */
    int    iret1;                      /* thread return value                 */
    int    iret2;
    pthread_t thread1;                 /* thread id                           */
    pthread_t thread2;
    struct thread_args args;

    /* ---------------------------------------------------------------------- */
    /* structure of the socket that the client will use to send information   */
    /* the IP address is the one of the server waiting for our messages       */
    /* ---------------------------------------------------------------------- */
    sock_write.sin_family = AF_INET;    /* AF_INET = TCP Socket               */
    sock_write.sin_port = htons(10201); /* Port Number to Publish             */
    /* Address of the computer to connect to in the case of a client          */
    inet_aton("200.13.89.15", (struct in_addr *)&sock_write.sin_addr);
    memset(sock_write.sin_zero, 0, 8);   

    /* ---------------------------------------------------------------------- */
    /* Instrucctions for publiching the socket                                */
    /* ---------------------------------------------------------------------- */
    sfd = socket(AF_INET,SOCK_DGRAM,0);

    /* ---------------------------------------------------------------------- */
    /* request and sending of an alias                                        */
    /*                                                                        */
    /* if data_type = 0 we are telling the server that we are logging for the */
    /* first tim and providing an alias. we will receive a integer with a nu- */
    /* meric id that we will send in every next message                       */
    /* ---------------------------------------------------------------------- */
    printf("Please provide an alias: ");
    message.chat_id = 0;
    message.data_type = 0;             /* data_type 0 is used to send alias   */
    message.data_text[0] = '\0'; 
    fgets(message.data_text, BUFFERSIZE-(sizeof(int)*2), stdin);
    for(auxptr = message.data_text; *auxptr != '\n'; ++auxptr);
      *auxptr = '\0';

    /* sending of information to log in chat room                             */
    sendto(sfd,(struct data *)&(message),sizeof(struct data),0,(struct sockaddr *)&(sock_write),sizeof(sock_write));
    read_char = recvfrom(sfd,(int *)&(chat_id),sizeof(int),0,NULL,NULL);    
    if (chat_id == -1)                 /* client rejected                     */
      {
        printf("Client could not join. Too many participants in room\n");
        close(sfd);  
        return(0);
      } 


     args.chat_id = chat_id;
     args.sfd = sfd;
     args.sock_write =sock_write;
     /* Creation of reading thread                                            */
     iret1 = pthread_create( &thread1, NULL, print_message, (void *)(&sfd));
     iret2 = pthread_create( &thread2, NULL, heartbeat_thread,(void *)(&args));
    /* ---------------------------------------------------------------------- */
    /* text typed by the user isread and sent to the server.  The client then */
    /* waits for  an answer and  displays it. The  cycle  continues until the */
    /* word "exit" is written.                                                */
    /* ---------------------------------------------------------------------- */
    while ((strcmp(message.data_text,"exit") != 0) && (strcmp(message.data_text,"shutdown") != 0))
      {
        printf("$ ");
        
        /* assembling of the message to send                                  */
        message.data_type = 1;         /* data_type 1 is used to send message */
        fgets(message.data_text, BUFFERSIZE-(sizeof(int)*2), stdin);
        for(auxptr = message.data_text; *auxptr != '\n'; ++auxptr);
	        *auxptr = '\0';
        message.chat_id = chat_id;
 
        sendto(sfd,(struct data *)&(message),sizeof(struct data),0,(struct sockaddr *)&(sock_write),sizeof(sock_write));
      }
    close(sfd);  
    return(0);
  }
