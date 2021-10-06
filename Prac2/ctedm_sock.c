/* -------------------------------------------------------------------------- */
/* cted_sock.c                                                                */
/*                                                                            */
/* client program that works with datagram type sockets sending entries typed */
/* by a user and receiving an  answer for them until  the word "exit" is wri- */
/* tten.                                                                      */
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

#define  BUFFERSIZE 1024               /* buffer size                         */

/* -------------------------------------------------------------------------- */
/* main ()                                                                    */
/*                                                                            */
/* main function of the system                                                */
/* -------------------------------------------------------------------------- */
int main()
  {
    struct sockaddr_in sock_write;     /* structure for the write socket      */
    char   text1[BUFFERSIZE];          /* reading buffer                      */
    char   text2[BUFFERSIZE];          /* writing buffer                      */
    char name[BUFFERSIZE];
    char   *auxptr;                    /* auxiliar char pointer               */
    int    sfd;                        /* socket descriptor                   */
    int    read_char;                  /* read characters                     */

    /* ---------------------------------------------------------------------- */
    /* structure of the socket that the client will use to send information   */
    /* the IP address is the one of the server waiting for our messages       */
    /* ---------------------------------------------------------------------- */
    sock_write.sin_family = AF_INET;    /* AF_INET = TCP Socket               */
    sock_write.sin_port = htons(10200); /* Port Number to Publish             */
    /* Address of the computer to connect to in the case of a client          */
    inet_aton("200.13.89.15", (struct in_addr *)&sock_write.sin_addr);
    memset(sock_write.sin_zero, 0, 8);   

    /* ---------------------------------------------------------------------- */
    /* Instrucctions for connecting to the server's socket                    */
    /* ---------------------------------------------------------------------- */
    sfd = socket(AF_INET,SOCK_DGRAM,0);

    /* ---------------------------------------------------------------------- */
    /* Inicialization of several variables                                    */
    /* ---------------------------------------------------------------------- */
    text1[0] = '\0';

    /* ---------------------------------------------------------------------- */
    /* text typed by the user isread and sent to the server.  The client then */
    /* waits for  an answer and  displays it. The  cycle  continues until the */
    /* word "exit" is written.                                                */
    /* ---------------------------------------------------------------------- */
    
    /*Ask for a name and sends it to server*/
    printf("Input ID:\n");
    fgets(name, BUFFERSIZE, stdin);
    for(auxptr = name; *auxptr != '\n'; ++auxptr);
	    *auxptr = '\0';
    strcat(name, "~");
    sendto(sfd,name,strlen(name),0,(struct sockaddr *)&(sock_write),sizeof(sock_write));
    
    while ((strcmp(text1,"exit") != 0) && (strcmp(text1,"shutdown") != 0))
      {
        printf("$ ");
        fgets(text1, BUFFERSIZE, stdin);
        for(auxptr = text1; *auxptr != '\n'; ++auxptr);
	        *auxptr = '\0';
        
        sendto(sfd,text1,strlen(text1),0,(struct sockaddr *)&(sock_write),sizeof(sock_write));
        read_char = recvfrom(sfd,text2,BUFFERSIZE,0,NULL,NULL);

        text2[read_char] = '\0';
        printf("Message Received by Client:[%s]\n",text2);
      }

    shutdown(sfd, SHUT_RDWR);
    close(sfd);  
    return(0);
  }

