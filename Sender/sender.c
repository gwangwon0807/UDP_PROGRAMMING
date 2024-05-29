#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

//socket통신을 위한 headerfile
#include <arpa/inet.h>

#define BUF_SIZE 200

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


Packet packet;
Packet pre_packet;
Log log_content;

struct sockaddr_in server_addr;
int sockfd;
int timeout_interval;

FILE* log_fp;

void log_event(const char *event, Log *log_content, int is_timeout, double duration) {
    fprintf(log_fp, "%s\t\t%d\t\t%d\t\t%d\t\t%d\t\t%s\t\t%s\t\t%f ms\n", 
            event, 
            log_content->log_type, 
            log_content->log_seq,
            log_content->log_ack,
            log_content->log_length,
            (log_content->log_loss == 1) ? "YES" : "NO", 
            is_timeout ? "YES" : "NO", 
            duration);
    fflush(log_fp);
}

void resend();

int main(int argc, char** argv) 
{
  if (argc != 7) {
        fprintf(stderr, "Usage: %s <sender port> <receiver IP> <receiver port> <timeout interval> <filename> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  memset(&pre_packet, 0, sizeof(Packet));
  int seqNum = 0;
  int sender_port = atoi(argv[1]);
  char *receiver_ip = argv[2];
  int receiver_port = atoi(argv[3]);
  timeout_interval = atoi(argv[4]);
  char *filename = argv[5];
  float drop_probability = atof(argv[6]);

  // 파일 열기
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("file open failed");
    exit(EXIT_FAILURE);
  }

  //log 파일 열기 및 저장 
  log_fp = fopen("log.txt", "wb");
  fwrite("\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n", 1, strlen("\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n"), log_fp);

  // 소켓 생성
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) 
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 서버 주소 설정
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(receiver_ip);
  server_addr.sin_port = htons(receiver_port);
  
  // 파일 이름 전송
  sendto(sockfd, "Greeting", strlen("Greeting"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  sendto(sockfd, filename, strlen(filename) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // 응답 대기
  char buffer[BUF_SIZE];
  recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
  printf("Receiver: %s\n", buffer);
  

  // 파일 전송
  int size[1];
  fseek(fp, 0, SEEK_END);
  size[0] = (int)ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  //setting SIGALRM
  struct sigaction sa;
  sa.sa_handler = resend;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);
  
  srand(time(NULL));
  clock_t t;

  if(!(strcmp(buffer, "OK")))
  {
    sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    size_t bytesRead;
    memset(&packet.data, 0, sizeof(packet.data));
    memset(&log_content, 0, sizeof(Log));
    log_content.log_ack = -999;
    int length = size[0];

    while (bytesRead = fread(packet.data, 1, BUF_SIZE, fp) > 0)
    {
      int percent = rand() % 100;
      packet.seqNum = seqNum;

      log_content.log_seq = packet.seqNum;
      log_content.log_loss = 0;
      log_content.log_timeout = 0;
      log_content.log_type = packet.type;
      if(length > BUF_SIZE)
      {
        length -= BUF_SIZE;
        log_content.log_length = BUF_SIZE;
      }
      else
      {
        log_content.log_length = length;
      }
      log_content.log_time_taken = 0.0;

       //timeout 발생시 재전송
      alarm(timeout_interval);
      t = clock();
      if(percent < (int)(drop_probability * 100))
      { 
        log_content.log_loss = 1;
        log_event("SEND", &log_content, log_content.log_timeout, log_content.log_time_taken);
      }
      else
      {
        sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        log_event("SEND", &log_content, log_content.log_timeout, log_content.log_time_taken);
      }
      recvfrom(sockfd, &packet, sizeof(Packet),0, NULL, NULL);
      log_content.log_ack = packet.ackNum;
      log_content.log_type = packet.type;

      alarm(0);
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
      log_event("RECV", &log_content, log_content.log_timeout, log_content.log_time_taken);
      seqNum++;
      memset(&pre_packet, 0, sizeof(Packet));
      pre_packet = packet;
      memset(&packet, 0, sizeof(Packet));
    }

    // 전송 완료 메시지 전송
    packet.type = 2;
    packet.seqNum = seqNum;


    t = clock();
    log_content.log_seq = packet.seqNum;
    log_content.log_type = packet.type;
    log_content.log_loss = 0;
    sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    log_event("SEND", &log_content, log_content.log_loss, 0.0);
  }
  

  // 응답 대기
  memset(buffer, 0, sizeof(buffer));
  recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
  t = clock() - t;
  float f_log_time_taken = ((float)t) / CLOCKS_PER_SEC; 
  printf("Receiver: %s \n", buffer);

  // 파일 닫기
  fclose(fp);
  //fclose(log_fp);
  close(sockfd);
  return 0;
}

void resend()
{
  log_content.log_timeout = 1;
  sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  log_event("SEND", &log_content, log_content.log_timeout, log_content.log_time_taken);
  alarm(timeout_interval);
  packet.type =1;
}