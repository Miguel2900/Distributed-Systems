/* -------------------------------------------------------------------------- */
/* ser_sock.c                                                                 */
/*                                                                            */
/* server program that works with tcp sockets, receiving  entries from a cli- */
/* ent,and printing them in the screen until the word "exit" is typed         */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* libraries needed for the execution of the program                          */
/* -------------------------------------------------------------------------- */
#include <netinet/in.h>                /* Socket structure definition         */
#include <sys/socket.h>                /* Socket functions and data           */
#include <arpa/inet.h>                 /* Makes   available   in_port_t   and */
                                       /* in_addr_t types                     */
#include <sys/types.h>                 /* Other data Types                    */
#include <stdio.h>                     /* Stdio operations                    */
#include <stdlib.h>                    /* Unix system libraries               */
#include <unistd.h>                    /* Unix standard linux OS API          */
#include <string.h>                    /* String and byte manipulation        */
#include <errno.h>                     /* System error catalog and library    */
#include <sys/stat.h>
#include <fcntl.h>
/* -------------------------------------------------------------------------- */
/* system constants                                                           */
/* -------------------------------------------------------------------------- */
#define LINELENGTH 4096                 /* Lenght of the line to transfer      */

/* -------------------------------------------------------------------------- */
/* main ()                                                                    */
/*                                                                            */
/* start of the application                                                   */
/* -------------------------------------------------------------------------- */
int main()
  {
    long int size, counter = 0;
    int ptr;
    struct sockaddr_in sock_read;      /* socket structure                    */
    char   text[LINELENGTH];          /* read/write buffer                   */
    char   *auxptr;                    /* character pointer                   */
    int    i;                          /* counter                             */
    int    sfd;                        /* socket descriptor                   */
    int    sfd_in;                     /* descriptor for the read port        */
    int    read_char;                  /* variable for characters read        */

    /* ---------------------------------------------------------------------- */
    /* definition of the socket                                               */
    /*                                                                        */
    /* AF_INET      - TCP Socket Family                                       */
    /* 10400        - Number of port in  which  the server will  be listening */
    /*                for incoming messages                                   */
    /* 200.13.89.15 - Server Address. This is  the address that  will be pub- */
    /*                lished to receive  information. It is the server's add- */
    /*                ress                                                    */
    /*                                                                        */
    /* This process will publish  the combination of  200.13.89.15 plus  port */
    /* 10400 in order to make the OS listen through it and  bring information */
    /* to the process                                                         */
    /* ---------------------------------------------------------------------- */ 
    sock_read.sin_family = AF_INET;    /* AF_INET = TCP Socket                */ 
    sock_read.sin_port   = htons(10200);      /* Port Number to Publish              */
    /* Address of the server's computer in the case of the server             */
    inet_aton("200.13.89.15",(struct in_addr *)&sock_read.sin_addr);
    memset(sock_read.sin_zero, 0, 8);

    /* ---------------------------------------------------------------------- */
    /* Instruccions to request and publish the socket                         */
    /* ---------------------------------------------------------------------- */
    
    /* ---------------------------------------------------------------------- */
    /* Creation of the Socket                                                 */
    /*                                                                        */
    /* #include <sys/socket.h>                                                */
    /* int socket(int domain, int type, int protocol);                        */
    /* Returns: file (socket) descriptor if OK, -1 on error                   */
    /* ---------------------------------------------------------------------- */
    sfd = socket(AF_INET,SOCK_STREAM,0);
    if (sfd == -1)
      {
        perror("Problem creating the socket");
        fprintf(stderr,"errno = %d\n", errno);
        exit(1);
      }
    
    /* ---------------------------------------------------------------------- */   
    /* Asociation of an O.S. socket with the socket data structure of the pro */
    /* gram.                                                                  */
    /*                                                                        */
    /* #include <sys/socket.h>                                                */
    /* int bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)*/
    /* Returns: 0 if OK, -1 on error                                          */
    /* ---------------------------------------------------------------------- */
    if (bind(sfd, (struct sockaddr *)&(sock_read), sizeof(sock_read)) == -1)
      {
        perror("Problem binding the socket");
        fprintf(stderr,"errno = %d\n", errno);
        exit(1);
      }

    /* ---------------------------------------------------------------------- */
    /* Creation of a listener  process for the socket. This is needed only in */
    /* connection oriented sockets.                                           */
    /*                                                                        */
    /* #include <sys/socket.h>                                                */
    /* int listen(int sockfd, int buffersize)                                 */
    /* Returns: 0 if OK, -1 on error                                          */
    /* ---------------------------------------------------------------------- */
    if (listen(sfd,LINELENGTH) == -1)
      {
        perror("Problem creating listener process for the socket");
        fprintf(stderr,"errno = %d\n", errno);
        exit(1);
      }

    /* ---------------------------------------------------------------------- */
    /* Initialization of the connection. Binding of the socket to a file poin */
    /* ter                                                                    */
    /*                                                                        */
    /* #include <sys/socket.h>                                                */
    /* int accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen)   */
    /* Returns: 0 if OK, -1 on error                                          */
    /* ---------------------------------------------------------------------- */
    i = sizeof(sock_read);
    sfd_in=accept(sfd, (struct sockaddr *)&(sock_read),(int *)&i);
    if (sfd_in == -1)
      {
        perror("Problem starting the accept process of the socket");
        fprintf(stderr,"errno = %d\n", errno);
        exit(1);
      }

    /* ---------------------------------------------------------------------- */
    /* The socket is read until it receives the word "exit"                   */
    /* Note: Reading is performed through the descriptor sfd_in, which is the */
    /* one returned by the function accept.                                   */
    /* ---------------------------------------------------------------------- */

    text[0] = '\0';

    read_char = read(sfd_in,text,LINELENGTH);
    fprintf(stdout, "File name:[%s]\n",text);

    read_char = read(sfd_in,(long int*) & size,sizeof(long int));
    fprintf(stdout, "Size :[%ld]\n",size);

    ptr = open(text, O_RDWR | O_CREAT, 0664);
    while (counter < size)
    {
      read_char = read(sfd_in,text,LINELENGTH);
      counter += write(ptr, text, read_char);
    }

    /* ---------------------------------------------------------------------- */
    /* Now we close the socket and shutdown the connection                    */
    /* ---------------------------------------------------------------------- */
    close(ptr);
    shutdown(sfd_in, SHUT_RDWR);
    close(sfd);

    return(0);
  }


