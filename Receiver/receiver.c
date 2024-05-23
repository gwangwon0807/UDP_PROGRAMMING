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
  // 소켓 생성
  int sockfd;
  int clnt_addr_size;
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 서버 주소 설정
  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  // 소켓과 주소 바인딩
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // 인사, 파일명 수신
  char com_buffer[BUF_SIZE];
  char file_buffer[BUF_SIZE];
  clnt_addr_size = sizeof(client_addr);

  // "Greeting" 메시지 수신
  recvfrom(sockfd, com_buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("Sender: %s\n", com_buffer);
  bzero(com_buffer, 100);
  
  // file name 수신
  recvfrom(sockfd, file_buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("File Name: %s\n", file_buffer);
  

  // 응답 전송
  sleep(1);
  sendto(sockfd, "OK", strlen("OK"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

  // 파일 저장하기
  FILE* fp;
  fp = fopen(file_buffer, "wb");
  bzero(file_buffer, 100);
  int size[1];

  recvfrom(sockfd, size, sizeof(size), 0, (struct  sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
  recvfrom(sockfd, file_buffer, BUF_SIZE, 0, (struct  sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
  recvfrom(sockfd, com_buffer, BUF_SIZE, 0 , (struct sockaddr*)&client_addr, (unsigned int*)&clnt_addr_size);

  printf("Sender: %s\n", com_buffer);
  if(!(strcmp(com_buffer, "Finish")))
  {
    fwrite(file_buffer, sizeof(char), size[0], fp);
  }
  file_buffer[0] ='\0';
  com_buffer[0] ='\0';

  sleep(1);
  sendto(sockfd, "Welldone", strlen("Welldone"), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

  fclose(fp);

  return 0;

}