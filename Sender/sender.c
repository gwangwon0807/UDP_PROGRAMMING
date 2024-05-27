#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

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

Packet packet;
Packet pre_packet;

struct sockaddr_in server_addr;
int sockfd;
int timeout_interval;

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
  

  if(!(strcmp(buffer, "OK")))
  {
    sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    size_t bytesRead;
    memset(&packet.data, 0, sizeof(packet.data));

    while (bytesRead = fread(packet.data, 1, BUF_SIZE, fp) > 0)
    {

      packet.seqNum = seqNum;
      packet.type = 1;
      pre_packet.type = 1;
      sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
      
      //timeout 발생시 재전송
      alarm(timeout_interval);
      recvfrom(sockfd, &packet, sizeof(Packet),0, NULL, NULL);
      alarm(0);

      printf("Seq: %d, Ack: %d Type:%d preAck: %d\n", seqNum++, packet.ackNum, packet.type, pre_packet.ackNum);
      memset(&pre_packet, 0, sizeof(Packet));
      pre_packet = packet;
      printf("preAck: %d\n", pre_packet.ackNum);
      memset(&packet, 0, sizeof(Packet));
    }

    // 전송 완료 메시지 전송
    packet.type = 0;
    packet.seqNum = seqNum;
    sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  }
  

  // 응답 대기
  memset(buffer, 0, sizeof(buffer));
  recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
  printf("Receiver: %s\n", buffer);

  // 파일 닫기
  fclose(fp);
  close(sockfd);
  
  return 0;
}

void resend()
{
  sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  printf("resend\n");
  alarm(timeout_interval);
}