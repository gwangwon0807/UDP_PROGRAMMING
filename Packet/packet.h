#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#define BUF_SIZE 200

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
    int flag;
    int type;
    int seqNum;
    int ackNum;
    int length;
    char data[BUF_SIZE];
} Packet;

typedef struct {
  int log_flag;
  int log_mode;
  int log_type;
  int log_seq;
  int log_ack;
  int log_length;
  int log_loss;
  int log_timeout;
  float log_time_taken;
} Log;

FILE* log_fp;

Packet packet;
Packet signal_packet;
Packet pre_packet;

Log log_content;

Packet create_signal_packet(int type, int flag, int seq, int ack)
{
  Packet pkt;
  memset(&pkt, 0, sizeof(Packet));
  pkt.flag = flag;
  pkt.type = type;
  pkt.seqNum = seq;
  pkt.ackNum = ack;
  pkt.length = 0;
  return pkt;
}

void log_event(const char *event, Log *log_content, Packet *pck, int is_timeout, double duration) 
{
  log_content->log_ack = pck->ackNum;
  log_content->log_flag = pck->flag;
  log_content->log_seq = pck->seqNum;
  log_content->log_type = pck->type;
  log_content->log_length = pck->length;

  fprintf(log_fp, "%s\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%s\t\t%s\t\t%f s\n", 
          event, 
          log_content->log_flag,
          log_content->log_type, 
          log_content->log_seq,
          log_content->log_ack,
          log_content->log_length,
          (log_content->log_loss == 1) ? "YES" : "NO", 
          is_timeout ? "YES" : "NO", 
          duration);
  fflush(log_fp);
}


