#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//socket통신을 위한 headerfile
#include <arpa/inet.h>

#define BUF_SIZE 200

int main(int argc, char** argv) {
  if (argc != 7) {
        fprintf(stderr, "Usage: %s <sender port> <receiver IP> <receiver port> <timeout interval> <filename> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int sender_port = atoi(argv[1]);
    char *receiver_ip = argv[2];
    int receiver_port = atoi(argv[3]);
    int timeout_interval = atoi(argv[4]);
    char *filename = argv[5];
    float drop_probability = atof(argv[6]);

  // 파일 열기
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("file open failed");
    exit(EXIT_FAILURE);
  }

  // 소켓 생성
  int sockfd;
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 서버 주소 설정
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(receiver_ip);
  server_addr.sin_port = htons(receiver_port);
  
  // 파일 이름 전송
  sendto(sockfd, "Greeting", strlen("Greeting"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  sendto(sockfd, filename, strlen(filename) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // 응답 대기
  char response[BUF_SIZE];
  recvfrom(sockfd, response, BUF_SIZE, 0, NULL, NULL);
  printf("Receiver: %s\n", response);
  sleep(1);

  // 파일 전송
  char buffer[BUF_SIZE];
  int size[1];
  fseek(fp, 0, SEEK_END);
  size[0] = (int)ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  if(!(strcmp(response, "OK")))
  {
    sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    size_t bytesRead;
    while(bytesRead = fread(buffer, 1, BUF_SIZE, fp) > 0)
    {
      sendto(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
      memset(buffer, 0, sizeof(buffer));
    }
    printf("Finish\n");
    // 전송 완료 메시지 전송
    sendto(sockfd, "Finish", strlen("Finish"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  }
  sleep(1);

  // 응답 대기
  recvfrom(sockfd, response, BUF_SIZE, 0, NULL, NULL);
  printf("Receiver: %s\n", response);

  // 파일 닫기
  fclose(fp);
  close(sockfd);
  
  return 0;
}