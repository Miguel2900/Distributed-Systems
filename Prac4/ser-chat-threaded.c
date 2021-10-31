/* -------------------------------------------------------------------------- */
/* chat server                                                                */
/*                                                                            */
/* server program that works with datagram-type sockets by receiving the mes- */
/* sages of a set of clients, displaying them, and returning the message sent */
/* by each client to the rest, until the last client sends an "exit" command  */
/*                                                                            */
/* the messages are stored  in an array that will  work in a similar way to a */
/* message queue                                                              */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* libraries needed for the program execution                                 */
/* -------------------------------------------------------------------------- */
#include <netinet/in.h> /* TCP/IP library                      */
#include <arpa/inet.h>  /* Newer functionality for  the TCP/IP */
                        /* library                             */
#include <sys/socket.h> /* sockets library                     */
#include <sys/types.h>  /* shared data types                   */
#include <stdio.h>      /* standard input/output               */
#include <unistd.h>     /* unix standard functions             */
#include <string.h>     /* text handling functions             */
#include <pthread.h>    /* libraries for thread handling       */
#include <time.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------- */
/* global definitions                                                         */
/* -------------------------------------------------------------------------- */
#define BUFFERSIZE 1024  /* buffer size                         */
#define MAX_MEMBERS 20   /* maximum number of members in room   */
#define MAX_MESSAGES 100 /* maximum number of mesgs in queue    */
#define MAX_THREADS 5    /* maximum number of threads in pool   */
#define MAX_PLAYERS 4
#define MAX_CARDS_HAND 5
#define MAX_CARDS_DECK 52
#define STRING_SIZE 3
#define MAX_ROUNDS 5

/* -------------------------------------------------------------------------- */
/* global variables and structures                                            */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* structure used to send and receive data between the client and the server  */
struct data
{
  int data_type;                                  /* type of data sent         */
  int chat_id;                                    /* chat id sent by server    */
  char data_text[BUFFERSIZE - (sizeof(int) * 2)]; /* data sent                 */
};

/* -------------------------------------------------------------------------- */
/* structure used to store the information of all the clients logged          */
struct member
{
  time_t t;
  int chat_id;                                /* chat id                      */
  char alias[BUFFERSIZE - (sizeof(int) * 2)]; /* member alias                 */
  struct sockaddr_in address;                 /* address of the member        */
  int game_id;
};

/* -------------------------------------------------------------------------- */
/* structure used as a  message queue to store  the messages that  need to be */
/* processed                                                                  */
struct msg
{
  int chat_id;
  char data_text[BUFFERSIZE - (sizeof(int) * 2)]; /* data sent                 */
  int deck_id;
};

struct card
{
  int value;
  char rank[STRING_SIZE];
  char suit[STRING_SIZE];
  int deck_id;
};

struct deck
{
  struct card card;
  int available;
};

struct player
{
  struct deck hand[MAX_CARDS_HAND];
  struct member member;
  int game_id;
  int card_chosen;
  int points;
};

/* -------------------------------------------------------------------------- */
/* global lists to be shared between all of the threads                       */
struct member part_list[MAX_MEMBERS];                  /* list of members in room             */
struct msg queue[MAX_MESSAGES];                        /* list of messages to process         */
int sfd;                                               /* socket descriptor                   */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER; /* Mutual exclusion */
int participants;                                      /* number of participats in the chat   */
struct player players_list[4];
int game_participants;
struct deck deck[MAX_CARDS_DECK];

/* -------------------------------------------------------------------------- */
/* send_message()                                                             */
/*                                                                            */
/* this function will constantly look for a new  message to send in the queue */
/* in order to send it to the proper recipient                                */
/* void *ptr  - pointer that will receive the parameters for the thread       */
/*                                                                            */
/* -------------------------------------------------------------------------- */

void deal_cards(int game_id);
void return_cards(int game_id);
void ins_in_queue(struct data message, char text1[BUFFERSIZE]);
void show_available_cards(int game_id);
void reset_players();

void *send_message(void *ptr)
{
  int c_i;            /* counter variable                    */
  struct msg chatmsg; /* chat message to process             */

  while (1)
  {
    /* first we  are going to look in the message queue for the next chat */
    /* message to process                                                 */

    /* block message queue to use it in an exclusive manner  ------------ */
    pthread_mutex_lock(&msg_mutex);

    for (c_i = 0; c_i < MAX_MESSAGES; ++c_i)
      if (queue[c_i].chat_id != -1)
        break;

    /* if i < MAX_MESSAGES, it means  that there is a message to process  */
    /* so we  copy  the messae in  the queue to  the  local  structure to */
    /* unblock the queue as quickly as possible ad then  process the chat */
    /* message                                                            */
    if (c_i < MAX_MESSAGES)
    {
      /* copy the mssage from the queue to the local struture           */
      memcpy((struct msg *)&chatmsg, (struct msg *)&queue[c_i], sizeof(struct msg));

      /* change cha_id to -1 to make this record free again             */
      queue[c_i].chat_id = -1;
    }

    /* --- unblock message queue so other threads can use it ------------ */
    pthread_mutex_unlock(&msg_mutex);

    /* If there are messages to process  we send  the chat message to all */
    /* of the participants                                                */
    if (c_i < MAX_MESSAGES)
    {
      for (c_i = 0; c_i < MAX_MEMBERS; ++c_i)
        if ((c_i != chatmsg.chat_id) && (part_list[c_i].chat_id != -1))
          sendto(sfd, chatmsg.data_text, strlen(chatmsg.data_text), 0, (struct sockaddr *)&(part_list[c_i].address), sizeof(struct sockaddr_in));
    }
  }
}

void *check_heartbeat(void *ptr)
{
  time_t t;
  char text1[BUFFERSIZE];

  while (1)
  {
    for (int i = 0; i < MAX_MEMBERS; i++)
    {
      t = time(NULL);
      /*If the participant time is grater or equal to 30 seconds, he will be disconnected*/
      if ((t - part_list[i].t >= 30) && (part_list[i].chat_id != -1))
      {
        part_list[i].chat_id = -1;
        participants--;

        if (part_list[i].game_id != -1)
        {
          return_cards(part_list[i].game_id);
          players_list[part_list[i].game_id].game_id = -1;
          part_list[i].game_id = -1;
          game_participants--;
        }
        sprintf(text1, "Client [%s] is leaving the chat room.", part_list[i].alias);
        /*Notifying the other participants his departure*/
        for (int j = 0; j < MAX_MEMBERS; j++)
        {
          if ((j != i) && (part_list[j].chat_id != -1))
            sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(part_list[j].address), sizeof(struct sockaddr_in));
        }
      }
    }
    sleep(1);
  }
}

void *card_game(void *ptr)
{
  time_t t1, t2;
  int i, j;
  int result;
  int winner;
  int draw;
  int game_winner;
  int players_ready = 0;
  int game_running = 0;
  char text1[BUFFERSIZE];

  for (i = 0; i < MAX_CARDS_DECK; i++)
  {
    deck[i].available = 0;
    deck[i].card.deck_id = i;

    result = i % 13 + 1;

    deck[i].card.value = result;
    if (result == 1)
      strcpy(deck[i].card.rank, "A");
    else if (result == 11)
      strcpy(deck[i].card.rank, "J");
    else if (result == 12)
      strcpy(deck[i].card.rank, "Q");
    else if (result == 13)
      strcpy(deck[i].card.rank, "K");
    else
      sprintf(deck[i].card.rank, "%d", result);

    result = i / 13;
    if (result == 0)
      strcpy(deck[i].card.suit, "♥");
    else if (result == 1)
      strcpy(deck[i].card.suit, "♦");
    else if (result == 2)
      strcpy(deck[i].card.suit, "♣");
    else
      strcpy(deck[i].card.suit, "♠");
  }

  for (i = 0; i < MAX_CARDS_DECK; i++)
  {
    printf("|v: %d r: %s s: %s|", deck[i].card.value, deck[i].card.rank, deck[i].card.suit);
    if (i % 13 == 12)
      printf("\n");
  }

  while (1)
  {
    if (game_participants >= 1 && game_running == 0)
    {
      printf("Starting game\n");
      game_running = 1;
      t1 = time(NULL);
    }

    if (game_participants >= 1 && game_running == 1)
    {
      t2 = time(NULL);
      if (game_participants == 1 && (t2 - t1) >= 30)
      {
        sprintf(text1, "Not enough players to start game");
        printf("%s\n", text1);
        sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
        reset_players();
      }
      if (game_participants > 1)
      {
        while (game_running == 1 && game_participants > 0)
        {
          for (j = 0; j < MAX_ROUNDS && game_participants > 0; j++)
          {
            while (players_ready < game_participants && game_participants > 0)
            {
              players_ready = 0;
              for (i = 0; i < MAX_PLAYERS; i++)
              {
                if (players_list[i].card_chosen != -1)
                  players_ready++;
              }
            }

            result = 0;
            draw = 0;
            for (i = 0; i < MAX_PLAYERS; i++)
            {
              if (players_list[i].card_chosen >= result && players_list[i].game_id != -1)
              {
                winner = i;
                result = players_list[i].card_chosen;
              }
            }
            for (i = 0; i < MAX_PLAYERS; i++)
            {
              if (players_list[i].card_chosen == result && players_list[i].game_id != -1)
                draw++;
              players_list[i].card_chosen = -1;
            }
            if (draw > 1)
              sprintf(text1, "It's a draw between %d players", draw);
            else
            {
              players_list[winner].points++;
              sprintf(text1, "Round winner: %s", players_list[winner].member.alias);
            }
            for (i = 0; i < MAX_PLAYERS; i++)
            {
              if (players_list[i].game_id != -1)
              {
                printf("Player: %s Points: %d", players_list[i].member.alias, players_list[i].points);
                sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
              }
            }

            players_ready = 0;
          }

          result = 0;
          draw = 0;
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (players_list[i].points >= result && players_list[i].game_id != -1)
            {
              winner = i;
              result = players_list[i].points;
            }
          }
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (players_list[i].points == result && players_list[i].game_id != -1)
              draw++;
            players_list[i].points = 0;
          }
          if (draw > 1)
            sprintf(text1, "It's a draw between %d players", draw);
          else
            sprintf(text1, "Game winner: %s", players_list[winner].member.alias);
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (players_list[i].game_id != -1)
              sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
          }
          reset_players();
        }
      }
    }

    if (game_participants == 0 && game_running == 1)
      game_running = 0;
    sleep(1);
  }
}

void deal_cards(int game_id)
{
  int random;

  for (int i = 0; i < MAX_CARDS_HAND; i++)
  {
    while (1)
    {
      random = rand() % MAX_CARDS_DECK;
      if (deck[random].available == 0)
        break;
    }
    players_list[game_id].hand[i].card = deck[random].card;
    players_list[game_id].hand[i].available = 0;
    deck[random].available = 1;
  }
  show_available_cards(game_id);
}

void show_available_cards(int game_id)
{
  char text1[BUFFERSIZE];
  sprintf(text1, "-----------------------------------------------------");
  sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[game_id].member.address), sizeof(struct sockaddr_in));
  for (int i = 0; i < MAX_CARDS_HAND; i++)
  {
    if (players_list[game_id].hand[i].available == 0)
    {
      sprintf(text1, "%d: |%s %s|", i + 1, players_list[game_id].hand[i].card.rank, players_list[game_id].hand[i].card.suit);
      sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[game_id].member.address), sizeof(struct sockaddr_in));
    }
  }
  sprintf(text1, "Choose a card with his id:");
  sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[game_id].member.address), sizeof(struct sockaddr_in));
}

void return_cards(int game_id)
{
  for (int i = 0; i < MAX_CARDS_HAND; i++)
    deck[players_list[game_id].hand[i].card.deck_id].available = 0;
}

void reset_players()
{
  game_participants = 0;
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (players_list[i].game_id != -1)
      return_cards(players_list[i].game_id);
    players_list[i].game_id = -1;
    players_list[i].card_chosen = -1;
    players_list[i].points = 0;
  }
}

void ins_in_queue(struct data message, char text1[BUFFERSIZE])
{
  /* insert  the new message and  originator client  in the message */
  /* queue                                                          */

  /* first we find an empty  spot for the next  message. if we find */
  /* it, we put  a copy of the  message there, if  we don't we only */
  /* un-block the queue and try again later                         */
  int i;
  int freespot = 0; /* flag that marks an open queue spot  */

  while (freespot == 0)
  {
    /* --- block message queue --- */
    pthread_mutex_lock(&msg_mutex);
    printf("Main Bloqueando mutex\n");

    for (i = 0; i < MAX_MESSAGES; ++i)
      if (queue[i].chat_id == -1)
      {
        freespot = 1;
        break;
      }
    if (freespot == 1)
    {
      queue[i].chat_id = message.chat_id;
      if (message.data_type == 3)
      {
        queue[i].deck_id = players_list[part_list[message.chat_id].game_id].hand[atoi(text1) - 1].card.deck_id;
        players_list[part_list[message.chat_id].game_id].hand[atoi(text1) - 1].available = 1;
        players_list[part_list[message.chat_id].game_id].card_chosen = players_list[part_list[message.chat_id].game_id].hand[atoi(text1) - 1].card.value;
        show_available_cards(part_list[message.chat_id].game_id);
        sprintf(text1, "[%s]:%s %s", part_list[message.chat_id].alias, deck[queue[i].deck_id].card.rank, deck[queue[i].deck_id].card.suit);
      }
      else
        queue[i].deck_id = -1;
      strcpy(queue[i].data_text, text1);
      printf("Message located in position [%d]\n", i);
      printf("queue[%d].chat_id=[%d]\n", i, queue[i].chat_id);
      printf("queue[%d].data_text=[%s]\n", i, queue[i].data_text);
    }

    /* --- unblock message queue --- */
    pthread_mutex_unlock(&msg_mutex);
    printf("Main Desbloqueando mutex\n");
  }
}

/* -------------------------------------------------------------------------- */
/* main ()                                                                    */
/*                                                                            */
/* main function of the system                                                */
/* -------------------------------------------------------------------------- */
int main()
{
  struct sockaddr_in sock_read;  /* structure for the read socket       */
  struct sockaddr_in sock_write; /* structure for the write socket      */
  struct data message;           /* message to sendto the server        */
  char text1[BUFFERSIZE];        /* reading buffer                      */
  int i;                         /* counter                             */

  int sfd_in;                     /* socket descriptor for read port     */
  int read_char;                  /* read characters                     */
  socklen_t lenght;               /* size of the read socket             */
  int iret1[MAX_THREADS];         /* thread return values                */
  pthread_t thread1[MAX_THREADS]; /* thread ids                          */
  int iret2;
  pthread_t thread2;
  int iret3;
  pthread_t thread3;

  srand(time(NULL));

  /* ---------------------------------------------------------------------- */
  /* structure of the socket that will read what comes from the client      */
  /* ---------------------------------------------------------------------- */
  sock_read.sin_family = AF_INET;    /* AF_INET = TCP Socket                */
  sock_read.sin_port = htons(10201); /* Port Number to Publish              */
  /* Address of the computer to connect to in the case of a client          */
  inet_aton("200.13.89.15", (struct in_addr *)&sock_read.sin_addr);
  memset(sock_read.sin_zero, 0, 8);

  /* ---------------------------------------------------------------------- */
  /* Instrucctions for publishing the socket                                */
  /* ---------------------------------------------------------------------- */
  sfd = socket(AF_INET, SOCK_DGRAM, 0);
  bind(sfd, (struct sockaddr *)&(sock_read), sizeof(sock_read));

  /* ---------------------------------------------------------------------- */
  /* Inicialization of several variables                                    */
  /* ---------------------------------------------------------------------- */
  message.data_text[0] = '\0';
  participants = 0;
  game_participants = 0;

  for (i = 0; i < MAX_MEMBERS; ++i) /* cleaning of participants list       */
  {
    part_list[i].chat_id = -1;
    part_list[i].game_id = -1;
  }

  for (i = 0; i < MAX_MESSAGES; ++i) /* cleaning of message queue          */
    queue[i].chat_id = -1;

  for (i = 0; i < MAX_PLAYERS; i++)
  {
    players_list[i].game_id = -1;
    players_list[i].card_chosen = -1;
    players_list[i].points = 0;
  }

  /* ---------------------------------------------------------------------- */
  /* Creation of thread pool that will check for new mssages to process     */
  /* ---------------------------------------------------------------------- */
  for (i = 0; i < MAX_THREADS; ++i)
    iret1[i] = pthread_create(&(thread1[i]), NULL, send_message, (void *)(&sfd));

  /*Creation of the thread to check heartbeats */
  iret2 = pthread_create(&(thread2), NULL, check_heartbeat, (void *)(&participants));

  iret3 = pthread_create(&(thread3), NULL, card_game, (void *)(&participants));

  /* ---------------------------------------------------------------------- */
  /* The  socket is  read and  the  messages are   answered  until the word */
  /* "shutdown" is received by a member of the chat                         */
  /* ---------------------------------------------------------------------- */
  lenght = sizeof(struct sockaddr);
  while (strcmp(message.data_text, "shutdown") != 0)
  {
    /* first we read the message sent from any client                     */
    read_char = recvfrom(sfd, (struct data *)&(message), sizeof(struct data), 0, (struct sockaddr *)&(sock_write), &(lenght));
    printf("Type:[%d], Part:[%d], Mess:[%s]\n", message.data_type, message.chat_id, message.data_text);

    /* if data_type == 0, it means that the client is logging in          */
    if (message.data_type == 0) /* Add new member to chat room         */
    {
      i = 0;
      while ((part_list[i].chat_id != -1) && (i < MAX_MEMBERS))
        ++i;
      if (i >= MAX_MEMBERS)
        i = -1; /* i = -1 means client rejected        */
      else
      {
        part_list[i].chat_id = i;
        strcpy(part_list[i].alias, message.data_text);
        memcpy((struct sockaddr_in *)&(part_list[i].address), (struct sockaddr_in *)&(sock_write), sizeof(struct sockaddr_in));
        ++participants;
      }
      sendto(sfd, (int *)&(i), sizeof(int), 0, (struct sockaddr *)&(sock_write), sizeof(sock_write));
    }
    /*If data type == 2, it means it's a heartbeat*/
    if (message.data_type == 2)
      memcpy(&part_list[message.chat_id].t, &message.data_text, sizeof(time_t));

    if (message.data_type == 3)
    {
      if (strcmp(message.data_text, "Start game") == 0)
      {
        i = 0;
        while ((players_list[i].game_id != -1) && (i < MAX_PLAYERS))
          ++i;

        if (i < MAX_PLAYERS)
        {
          part_list[message.chat_id].game_id = i;
          players_list[i].game_id = i;
          players_list[i].member = part_list[message.chat_id];
          game_participants++;
          deal_cards(i);
        }
        else
        {
          strcpy(text1, "Client could not join. Too many players");
          sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(part_list[message.chat_id].address), sizeof(struct sockaddr_in));
        }
      }
      else
      {
        sprintf(text1, "%s", message.data_text);
        ins_in_queue(message, text1);
      }
    }
    /* if data_type == 1, it means that this is a message                 */
    if (message.data_type == 1)
    {
      /* if the message received is the word "exit", it  means that the */
      /* client is leaving the room, so we report that to the other cli */
      /* ents, and change the value of its id in the list to -1         */
      if (strcmp(message.data_text, "exit") == 0)
      {
        sprintf(text1, "Client [%s] is leaving the chat room.", part_list[message.chat_id].alias);
        part_list[message.chat_id].chat_id = -1;
        --participants;

        if (part_list[message.chat_id].game_id != -1)
        {
          return_cards(part_list[message.chat_id].game_id);
          players_list[part_list[message.chat_id].game_id].game_id = -1;
          part_list[message.chat_id].game_id = -1;
          game_participants--;
        }
      }

      else
        sprintf(text1, "[%s]:[%s]", part_list[message.chat_id].alias, message.data_text);

      ins_in_queue(message, text1);
    }
  }
  close(sfd);
  pthread_mutex_destroy(&msg_mutex);
  return (0);
}
