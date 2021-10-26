/* ------------------------------------------------------------------------- */
/* cte_sock.c                                                                */
/*                                                                           */
/* client program that works with tcp sockets, sending everything typed by a */
/* user to a server.                                                         */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* libraries needed for the process execution                                */
/* ------------------------------------------------------------------------- */
#include <netinet/in.h> /* Internet Network                   */
#include <netinet/in.h> /* Socket structure definition         */
#include <sys/socket.h> /* Socket functions and data           */
#include <arpa/inet.h>  /* Makes   available   in_port_t   and */
                        /* in_addr_t types                     */
#include <sys/types.h>  /* Other data Types                    */
#include <stdio.h>      /* Stdio operations                    */
#include <stdlib.h>     /* Unix system libraries               */
#include <unistd.h>     /* Unix standard linux OS API          */
#include <string.h>     /* String and byte manipulation        */
#include <errno.h>      // System error catalog and library
#include <sys/stat.h>
#include <fcntl.h>

/* ------------------------------------------------------------------------- */
/* system constants                                                          */
/* ------------------------------------------------------------------------- */
#define LINELENGTH 4096 /* Lenght of the line to transfer      */

/* ------------------------------------------------------------------------- */
/* main ()                                                                   */
/*                                                                           */
/* start of the application                                                  */
/* ------------------------------------------------------------------------- */
int main()
{
  long int counter = 0, readSize;
  int ptr;
  struct sockaddr_in sock_write; /* socket structure                   */
  char text[LINELENGTH];         /* read/write buffer                  */
  char *auxptr;                  /* character pointer                  */
  int i;                         /* counter                            */
  int sfd;                       /* socket descriptor                  */

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
  sock_write.sin_family = AF_INET;    /* AF_INET = TCP Socket               */
  sock_write.sin_port = htons(10200); /* Port Number to Publish             */
  /* Address of the computer to connect to in the case of a client          */
  inet_aton("200.13.89.15", (struct in_addr *)&sock_write.sin_addr);
  memset(sock_write.sin_zero, 0, 8);

  /* ---------------------------------------------------------------------- */
  /* Creation of the Socket                                                 */
  /*                                                                        */
  /* #include <sys/socket.h>                                                */
  /* int socket(int domain, int type, int protocol);                        */
  /* Returns: file (socket) descriptor if OK, �1 on error                   */
  /* ---------------------------------------------------------------------- */
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
  {
    perror("Problem creating the socket");
    fprintf(stderr, "errno = %d\n", errno);
    exit(1);
  }

  /* ---------------------------------------------------------------------- */
  /* Connection to the remote Socket                                        */
  /*                                                                        */
  /* #include <sys/socket.h>                                                */
  /* int connect(int sockfd, const struct sockaddr *serv_addr)              */
  /* Returns: file (socket) descriptor if OK, �1 on error                   */
  /* ---------------------------------------------------------------------- */
  if (connect(sfd, (struct sockaddr *)&(sock_write), sizeof(sock_write)) == -1)
  {
    perror("Problem connecting to remote socket");
    fprintf(stderr, "errno = %d\n", errno);
    exit(1);
  }

  /* ---------------------------------------------------------------------- */
  /* we write to the socket until we receive the word "exit".               */
  /* Note: we are writing to the sfd descriptor, which is the value return- */
  /* ed by the socket() function.                                           */
  /* ---------------------------------------------------------------------- */
  text[0] = '\0';

  printf("Input file name:\n");
  fgets(text, LINELENGTH, stdin);
  for (auxptr = text; *auxptr != '\n'; ++auxptr)
    ;
  *auxptr = '\0';
  ptr = open(text, O_RDWR);
  if (ptr != -1)
  {
    struct stat file_stat;
    if (!stat(text, &file_stat))
    {
      fprintf("Size in bytes:[%ld]\n", file_stat.st_size);
      write(sfd, text, LINELENGTH); //send file name

      write(sfd, (long int *)&file_stat.st_size, sizeof(long int)); //send file size
      while (counter < file_stat.st_size)
      {
        lseek(ptr, SEEK_SET, counter);
        readSize = read(ptr, text, LINELENGTH);
        write(sfd, text, readSize);
        counter += readSize;
      }
    }
  }
  /* ---------------------------------------------------------------------- */
  /* Now we close the socket                                                */
  /* ---------------------------------------------------------------------- */
  close(ptr);
  close(sfd);
  return (0);
}
