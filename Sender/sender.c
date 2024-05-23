#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//socket통신을 위한 headerfile
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 1024
#define PORT 12345

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [File Path]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // 파일 열기
  FILE *fp = fopen(argv[1], "rb");
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
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(PORT);
  
  // 파일 이름 전송
  sendto(sockfd, "Greeting", strlen("Greeting"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  sendto(sockfd, argv[1], strlen(argv[1]) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

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

  if(!(strcmp(response, "OK"))){
    fread(buffer, sizeof(char), BUF_SIZE, fp);
    sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    sendto(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

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