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
struct client
{
  struct sockaddr_in sock_write; 
  char name[BUFFERSIZE];
  short int available;
};

int main()
  {
    struct client clients[5];
    int clientCtr = 0;
    int clientID = 0;
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
    sock_read.sin_port   = htons(10200);      /* Port Number to Publish              */
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

    /*makes every client in array available*/
    for (int i = 0; i < 5; i++)
    {
      clients[i].available = 0;
    }
    
    while (strcmp(text1,"shutdown") != 0)
      {
        read_char = recvfrom(sfd,text1,BUFFERSIZE,0,(struct sockaddr *)&(sock_write),&(lenght));

        /* ----------------------------------------------------------------- */
        /*Checks if the message has a ~ at the end, if this is true, 
        it stores a new client*/
        /* ----------------------------------------------------------------- */
        if (text1[read_char - 1] == '~')
        {
          text1[read_char - 1] = '\0';

          for (int i = 0; i < 5; i++)
          {
            if (clients[i].available == 0)
            {
              strcpy(clients[clientCtr].name,text1);
              memcpy(&clients[clientCtr].sock_write, &sock_write, sizeof(sock_write));
              break;
            }
          }

          sprintf(text1, "%s just Joined", clients[clientCtr].name);
          clientCtr++;
        }
        else
          text1[read_char] = '\0';

        /* ----------------------------------------------------------------- */
        /*Makes the message to be sent to the other clients*/
        /* ----------------------------------------------------------------- */
        for (int i = 0; i < clientCtr; i++)
        {
          if (clients[i].sock_write.sin_port == sock_write.sin_port)
          {
            clientID = i;

            /* ----------------------------------------------------------------- */
            /*If the client sent "exit", it will notify the others his departure*/
            /* ----------------------------------------------------------------- */
            if (strcmp(text1, "exit") == 0)
            {
              sprintf(text2,"%s rage quited",clients[clientID].name);
              clients[i].available = 1;
            }

            else
              sprintf(text2,"%s: [%s]",clients[clientID].name,text1);
          }
        }
        
        /* ----------------------------------------------------------------- */
        /* We first display the mmesage received                             */
        /* ----------------------------------------------------------------- */
        printf("Server: Message Received [%s]\n",text1);


        /* ----------------------------------------------------------------- */
        /* Now we display the address of the client that sent the message    */
        /* ----------------------------------------------------------------- */
        client_address = inet_ntoa(sock_write.sin_addr);
        printf("Server: From Client with Address-[%s], Port-[%d], AF-[%d].\n", client_address, sock_write.sin_port, sock_write.sin_family);


        /* ----------------------------------------------------------------- */
        /* cycles between al clients and sends the message, excpet form the one who send it*/
        /* ----------------------------------------------------------------- */
        for (int i = 0; i < clientCtr; i++)
        {
          if (i != clientID)
          {
            sendto(sfd,text2,strlen(text2),0,(struct sockaddr *)&(clients[i].sock_write), sizeof(clients[i].sock_write));
          }
        }
      }
    shutdown(sfd, SHUT_RDWR);
    close(sfd);
    return(0);
  }