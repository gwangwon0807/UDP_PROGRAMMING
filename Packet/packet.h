#define BUF_SIZE 200

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

//socket통신을 위한 headerfile
#include <arpa/inet.h>

typedef enum{
  DATA = 0,
  ACK = 1,
  SYN = 2,
  SYN_ACK = 3,
  FIN = 4
} PacketType;

typedef enum{
  NONE = 0,
  SYN_FLAG = 1,
  FIN_FLAG = 2
} PacketFlag;

typedef struct {
    int type;
    int seqNum;
    int ackNum;
    int length;
    char data[BUF_SIZE];
} Packet;

typedef struct {
  int log_mode;
  int log_type;
  int log_seq;
  int log_ack;
  int log_length;
  int log_loss;
  int log_timeout;
  float log_time_taken;
} Log;

