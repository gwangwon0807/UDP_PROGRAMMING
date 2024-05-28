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
  char log_mode[10];
  char log_type[10];
  char log_seq[10];
  char log_ack[10];
  char log_length[10];
  char log_loss[10];
  char log_timeout[10];
  char log_time_taken[15];
} Log;

int i_log_type, i_log_seq, i_log_ack, i_log_length, i_log_loss, i_log_timeout;
float f_log_time_taken;

Packet packet;
Packet pre_packet;
Log log_content;

struct sockaddr_in server_addr;
int sockfd;
int timeout_interval;

void resend();
void change_n_to_s(int, FILE*);

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
  FILE *log_fp = fopen("log.txt", "wb");
  fwrite("\t\tType\t\tSeq\t\tAck\t\tLength\tLoss\tTimeout\t\ttime\n", 1, strlen("\t\tType\t\tSeq\t\tAck\t\tLength\tLoss\tTimeout\t\ttime\n"), log_fp);

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
    i_log_ack = -999;
    printf("1th\n");
    while (bytesRead = fread(packet.data, 1, BUF_SIZE, fp) > 0)
    {
      int percent = rand() % 100;
      packet.seqNum = seqNum;
      packet.type = 1;
      pre_packet.type = 1;

      i_log_seq = packet.seqNum;
      i_log_loss = 0;
      i_log_timeout = 0;
      i_log_type = 1;
      i_log_length = bytesRead;
      f_log_time_taken = 0.0;

       //timeout 발생시 재전송
      alarm(timeout_interval);
      t = clock();
      printf("2th\n");
      if(percent < (int)(drop_probability * 100))
      { 
        i_log_loss = 1;
        printf("prob\n");
        change_n_to_s(1, log_fp);
        fwrite(&log_content, sizeof(log_content), 1, log_fp);
      }
      else
      {
        sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        change_n_to_s(1, log_fp);
        fwrite(&log_content, sizeof(log_content), 1, log_fp);
      }
      printf("4Th");
      recvfrom(sockfd, &packet, sizeof(Packet),0, NULL, NULL);
      i_log_ack = packet.ackNum;
      i_log_type = packet.type;   

      alarm(0);
      t = clock() - t;
      f_log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
      change_n_to_s(0, log_fp);
      fwrite(&log_content, sizeof(log_content), 1, log_fp);

      printf("Seq: %d, Ack: %d Type:%d preAck: %d, time: %f\n", seqNum++, packet.ackNum, packet.type, pre_packet.ackNum, f_log_time_taken);
      memset(&pre_packet, 0, sizeof(Packet));
      pre_packet = packet;
      memset(&packet, 0, sizeof(Packet));
    }

    // 전송 완료 메시지 전송
    packet.type = 0;
    packet.seqNum = seqNum;


    t = clock();
    sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  }
  

  // 응답 대기
  memset(buffer, 0, sizeof(buffer));
  recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
  t = clock() - t;
  f_log_time_taken = ((float)t) / CLOCKS_PER_SEC; 
  printf("Receiver: %s, %f\n", buffer, f_log_time_taken);

  // 파일 닫기
  fclose(fp);
  fclose(log_fp);
  close(sockfd);
  
  return 0;
}

void resend()
{
  FILE *log_fp = fopen("log.txt", "ab");
  i_log_timeout = 1;
  sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  change_n_to_s(1, log_fp);
  fwrite(&log_content, sizeof(log_content), 1, log_fp);
  fclose(log_fp);
  printf("resend\n");
  alarm(timeout_interval);
}

void change_n_to_s(int mode, FILE* log_fp)
{
  memset(&log_content, 0, sizeof(Log));
  printf("6");
  sprintf(log_content.log_ack, "%d\t\t", i_log_ack);
  sprintf(log_content.log_length, "%d\t\t", i_log_length);
  sprintf(log_content.log_loss, "%d\t\t", i_log_loss);

  if(mode == 1)
  {
    strcpy(log_content.log_mode, "send\t\t");
  }
  else
  {
    strcpy(log_content.log_mode, "recv\t\t");
  }
  sprintf(log_content.log_seq, "%d\t\t", i_log_seq);
  sprintf(log_content.log_time_taken, "%f\n", f_log_time_taken);
  sprintf(log_content.log_timeout, "%d\t\t", i_log_timeout);
  sprintf(log_content.log_type, "%d\t\t", i_log_type);
}