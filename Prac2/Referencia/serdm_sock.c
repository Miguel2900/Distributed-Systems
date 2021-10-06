/* -------------------------------------------------------------------------- */
/* serd_sock.c                                                                */
/*                                                                            */
/* server program that works with datagram-type sockets by receiving the mes- */
/* sages of a set of clients, displaying them, and returning a specific mess- */
/* age back to each specific client until the word "exit" is typed.           */
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
    struct sockaddr_in sock_read;      /* structure for the read socket       */
    struct sockaddr_in sock_write;     /* structure for the write socket      */
    char   text1[BUFFERSIZE];          /* reading buffer                      */
    char   text2[BUFFERSIZE];          /* writing buffer                      */
    char   *client_address;            /* address of the sending client       */
    int    sfd;                        /* socket descriptor                   */
    int    read_char;                  /* read characters                     */
    socklen_t lenght;                  /* size of the read socket             */


    /* ---------------------------------------------------------------------- */
    /* structure of the socket that will read what comes from the client      */
    /* ---------------------------------------------------------------------- */
    sock_read.sin_family = AF_INET;    /* AF_INET = TCP Socket                */ 
    sock_read.sin_port   = htons(10400);      /* Port Number to Publish              */
    /* Address of the server's computer in the case of the server             */
    inet_aton("200.13.89.15",(struct in_addr *)&sock_read.sin_addr);
    memset(sock_read.sin_zero, 0, 8);

    /* ---------------------------------------------------------------------- */
    /* Instrucctions for publishing the socket                                */
    /* ---------------------------------------------------------------------- */
    sfd = socket(AF_INET,SOCK_DGRAM,0);
    bind(sfd,(struct sockaddr *)&(sock_read),sizeof(sock_read));

    /* ---------------------------------------------------------------------- */
    /* Inicialization of several variables                                    */
    /* ---------------------------------------------------------------------- */
    text1[0] = '\0';

    /* ---------------------------------------------------------------------- */
    /* The  socket is  read and  the  messages  are  answered until  the word */
    /* shutdown is received                                                   */
    /* ---------------------------------------------------------------------- */
    lenght = sizeof(struct sockaddr);
    while (strcmp(text1,"shutdown") != 0)
      {
        read_char = recvfrom(sfd,text1,BUFFERSIZE,0,(struct sockaddr *)&(sock_write),&(lenght));

        /* ----------------------------------------------------------------- */
        /* We first display the mmesage received                             */
        /* ----------------------------------------------------------------- */
        text1[read_char] = '\0';
        printf("Server: Message Received [%s]\n",text1);


        /* ----------------------------------------------------------------- */
        /* Now we display the address of the client that sent the message    */
        /* ----------------------------------------------------------------- */
        client_address = inet_ntoa(sock_write.sin_addr);
        printf("Server: From Client with Address-[%s], Port-[%d], AF-[%d].\n", client_address, sock_write.sin_port, sock_write.sin_family);


        /* ----------------------------------------------------------------- */
        /* We assemble the message with the answer for the client and send it*/
        /* ----------------------------------------------------------------- */
        sprintf(text2,"Server returning the Message [%s]",text1);
        sendto(sfd,text2,strlen(text2),0,(struct sockaddr *)&(sock_write),sizeof(sock_write));

      }
    shutdown(sfd, SHUT_RDWR);
    close(sfd);
    return(0);
  }

