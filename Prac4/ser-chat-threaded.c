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
  time_t t;                                   /* time of the memeber          */
  int chat_id;                                /* chat id                      */
  char alias[BUFFERSIZE - (sizeof(int) * 2)]; /* member alias                 */
  struct sockaddr_in address;                 /* address of the member        */
  int game_id;                                /* game id                      */
};

/* -------------------------------------------------------------------------- */
/* structure used as a  message queue to store  the messages that  need to be */
/* processed                                                                  */
struct msg
{
  int chat_id;                                    /* chat id                  */
  char data_text[BUFFERSIZE - (sizeof(int) * 2)]; /* data sent                */
  int deck_id;                                    /* deck id                  */
};

/* -------------------------------------------------------------------------- */
/* structure used to save card info such as rank, numeric value, suit and his */
/* positon on the deck                                                        */
struct card
{
  int value;              /* numeric value                */
  char rank[STRING_SIZE]; /* card rank                    */
  char suit[STRING_SIZE]; /* card suit                    */
  int deck_id;            /* deck id                      */
};

/* -------------------------------------------------------------------------- */
/* structure used to make the card deck                                       */
struct deck
{
  struct card card; /* card                         */
  int available;    /* availability of the card     */
};

struct player
{
  struct deck hand[MAX_CARDS_HAND]; /* player hand                  */
  struct member member;             /* player chat member           */
  int game_id;                      /* game id                      */
  int card_chosen;                  /* card chosen in the round     */
  int points;                       /* player points                */
};

/* -------------------------------------------------------------------------- */
/* global lists to be shared between all of the threads                       */
struct member part_list[MAX_MEMBERS];                  /* list of members in room             */
struct msg queue[MAX_MESSAGES];                        /* list of messages to process         */
int sfd;                                               /* socket descriptor                   */
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER; /* Mutual exclusion                    */
int participants;                                      /* number of participats in the chat   */
struct player players_list[4];                         /* list of players in game             */
int game_participants;                                 /* number of players in the game       */
struct deck deck[MAX_CARDS_DECK];                      /* card deck                           */
int game_end;                                          /* flag of the game's end              */

/* -------------------------------------------------------------------------- */
/* this function will deal cards to a player                                  */
void deal_cards(int game_id);
/* -------------------------------------------------------------------------- */
/* this function will return and make available player's cards                */
void return_cards(int game_id);
/* -------------------------------------------------------------------------- */
/* this function will insert a meessage into the queue                        */
void ins_in_queue(struct data message, char text1[BUFFERSIZE]);
/* -------------------------------------------------------------------------- */
/* this fucntion will send the player his available cards                     */
void show_available_cards(int game_id);
/* -------------------------------------------------------------------------- */
/* this function will reset all players, in order to finish the game appropriately*/
void reset_players();
/* -------------------------------------------------------------------------- */
/* this function will reset the game, to play a new round                     */
void reset_game();

/* -------------------------------------------------------------------------- */
/* send_message()                                                             */
/*                                                                            */
/* this function will constantly look for a new  message to send in the queue */
/* in order to send it to the proper recipient                                */
/* void *ptr  - pointer that will receive the parameters for the thread       */
/*                                                                            */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/* check_heartbeat()                                                          */
/* this function checks if a memeber has sent a heartbeat                     */
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

/* -------------------------------------------------------------------------- */
/* card_game()                                                                */
/* this function will run and control the card game                           */
void *card_game(void *ptr)
{
  time_t t1, t2;                             /* two time variables to measure time           */
  int i, j;                                  /* variables for cycles                         */
  int result;                                /* stores results                               */
  int winner;                                /* saves the winner position                    */
  int draw;                                  /* counter to check if there is a draw          */
  int players_ready = 0;                     /* counter to check how many players are ready  */
  int game_running = 0;                      /* flag to check if game is running             */
  char text1[BUFFERSIZE], text2[BUFFERSIZE]; /* strings to send messages                     */

  /* cycle to fill the card deck          */
  for (i = 0; i < MAX_CARDS_DECK; i++)
  {
    deck[i].available = 0;
    deck[i].card.deck_id = i;

    /* this operation returns the column number if the deck is considered as a matrix */
    result = i % 13 + 1;

    deck[i].card.value = result; /* stores the numeric value of the card     */

    /* the poker deck doesn't have 1, 11, 12 or 13 ranks, it's necessary to change    */
    /* those values into the corresponding letter rank A, J, Q or K respectively      */
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

    /* there are 4 suits in poker, and each of them has 13 cards, here is saved the   */
    /*  suit of each card                                                             */
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

  game_end = 0;
  while (1)
  {
    /* if there is one or more participants and the game hasn't started, the game     */
    /* starts and the time is stored                                                  */
    if (game_participants >= 1 && game_running == 0)
    {
      printf("Starting game\n");
      game_running = 1;
      t1 = time(NULL);
    }

    if (game_participants >= 1 && game_running == 1)
    {
      t2 = time(NULL);

      /* if there is only one participant and has been alone for more than 30 seconds */
      /* the game is finished                                                         */
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
          /* cycle to make the game last 5 rounds, to make players use all their cards*/
          for (j = 0; j < MAX_ROUNDS && game_participants > 0; j++)
          {

            /* cycle to wait for players to choose a card   */
            while (players_ready < game_participants && game_participants > 0)
            {
              players_ready = 0;
              for (i = 0; i < MAX_PLAYERS; i++)
              {
                if (players_list[i].card_chosen != -1 && part_list[i].game_id != -1)
                  players_ready++;
              }
            }

            /* gets the highest value from the chosen cards and the player's game_id   */
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

            /* counts if there are multiple cards with a value equal to the highest    */
            /* value amongst the chosen cards to check if there is a draw              */
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

            /* notifies the players the result of the round  */
            for (i = 0; i < MAX_PLAYERS; i++)
            {
              if (players_list[i].game_id != -1)
                sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
            }

            players_ready = 0;
          }

          /* gets the highest point score  */
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
          /* counts if there are multiple players with the highest score  */
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (players_list[i].points == result && players_list[i].game_id != -1)
              draw++;
            players_list[i].card_chosen = -1;
          }
          if (draw > 1)
            sprintf(text1, "It's a draw between %d players", draw);
          else
            sprintf(text1, "Game winner: %s", players_list[winner].member.alias);
          sprintf(text2, "0: Leave 1: Continue");

          /* notifies the players the result of the game  */
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (players_list[i].game_id != -1)
            {
              sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
              sendto(sfd, text2, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
            }
          }

          players_ready = 0;
          game_end = 1;

          /* cycle to wait for players to decide if they want to leave or continue     */
          /* with the game                                                             */
          while (players_ready < game_participants && game_participants > 0)
          {
            players_ready = 0;
            for (i = 0; i < MAX_PLAYERS; i++)
            {
              if (players_list[i].card_chosen != -1 && part_list[i].game_id != -1)
                players_ready++;
            }
          }

          /* counts the player's votes  */
          result = 0;
          for (i = 0; i < MAX_PLAYERS; i++)
          {
            if (part_list[i].game_id != -1)
              result += players_list[i].card_chosen;
          }

          /* if more than half the players chose to end the game, the game will end    */
          if (result < (game_participants / 2))
          {
            game_running = 0;
            reset_players();
          }

          /* if players want to continue, they will get a new hand    */
          else
            reset_game();
          players_ready = 0;
          game_end = 0;
        }
      }
    }

    /*if there are no participants and  the gamme is running, the game will end  */
    if (game_participants == 0 && game_running == 1)
    {
      game_running = 0;
      reset_players();
    }
    sleep(1);
  }
}

void deal_cards(int game_id)
{
  int random;

  /* gets a random card in the deck  */
  for (int i = 0; i < MAX_CARDS_HAND; i++)
  {
    /* cylce to verify is the card is available to give to player  */
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

  /* if the card is available in player's hand, it will send it to him   */
  for (int i = 0; i < MAX_CARDS_HAND; i++)
  {
    if (players_list[game_id].hand[i].available == 0)
    {
      sprintf(text1, "%d: |%s %s|", i + 1, players_list[game_id].hand[i].card.rank, players_list[game_id].hand[i].card.suit);
      sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[game_id].member.address), sizeof(struct sockaddr_in));
    }
  }
  sprintf(text1, "Choose a card with the id:");
  sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[game_id].member.address), sizeof(struct sockaddr_in));
}

void return_cards(int game_id)
{
  /* makes all cards in player's hand available in deck again */
  for (int i = 0; i < MAX_CARDS_HAND; i++)
    deck[players_list[game_id].hand[i].card.deck_id].available = 0;
}

void reset_players()
{
  char text1[BUFFERSIZE];
  game_participants = 0;

  sprintf(text1, "Game finished");

  /* clears the players list and notifies them that the game is over  */
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (players_list[i].game_id != -1)
    {
      return_cards(players_list[i].game_id);
      sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(players_list[i].member.address), sizeof(struct sockaddr_in));
    }
    players_list[i].game_id = -1;
    players_list[i].card_chosen = -1;
    players_list[i].points = 0;
  }
}

void reset_game()
{
  /* makes all players return their cards  */
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (players_list[i].game_id != -1)
      return_cards(players_list[i].game_id);
    players_list[i].card_chosen = -1;
    players_list[i].points = 0;
  }

  /* deal new cards to each player         */
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (players_list[i].game_id != -1)
      deal_cards(players_list[i].game_id);
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

        /* if the game isn't in the final state where players vote, stores the card    */
        /* chosen by the player and sends his available cards                          */
        if (game_end == 0)
        {
          players_list[part_list[message.chat_id].game_id].card_chosen = players_list[part_list[message.chat_id].game_id].hand[atoi(text1) - 1].card.value;
          show_available_cards(part_list[message.chat_id].game_id);
        }
        /*if the player is voting to continue or leave the game, his choise is stored  */
        else
        {
          players_list[part_list[message.chat_id].game_id].card_chosen = atoi(text1);
        }
        sprintf(text1, "[%s]: Chose a card", part_list[message.chat_id].alias);
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
  int iret1[MAX_THREADS];         /* thread 1 return values              */
  pthread_t thread1[MAX_THREADS]; /* thread ids                          */
  int iret2;                      /* thread 2 return values              */
  pthread_t thread2;              /* thread ids                          */
  int iret3;                      /* thread 3 return values              */
  pthread_t thread3;              /* thread ids                          */

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
    /*If data type == 2, it means it's a heartbeat  */
    if (message.data_type == 2)
      memcpy(&part_list[message.chat_id].t, &message.data_text, sizeof(time_t));

    /* if data type == 3, it means it's a game related message  */
    if (message.data_type == 3)
    {
      /* if the message is Start the game, adds the player in the players list  */
      if (strcmp(message.data_text, "Start game") == 0)
      {
        /* gets a free postion in the players list   */
        i = 0;
        while ((players_list[i].game_id != -1) && (i < MAX_PLAYERS))
          ++i;

        /* if the positon is correct, the participant is added into the players list  */
        if (i < MAX_PLAYERS)
        {
          part_list[message.chat_id].game_id = i;
          players_list[i].game_id = i;
          players_list[i].member = part_list[message.chat_id];
          game_participants++;
          deal_cards(i);
        }

        /* if there are no available spaces a message is sent to  the participant  */
        else
        {
          strcpy(text1, "Client could not join. Too many players");
          sendto(sfd, text1, strlen(text1), 0, (struct sockaddr *)&(part_list[message.chat_id].address), sizeof(struct sockaddr_in));
        }
      }
      /* inserts in the queue the card chosen by the player  */
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
