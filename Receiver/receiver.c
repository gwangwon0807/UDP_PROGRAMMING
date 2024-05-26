#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../Packet/PACKET.h"

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

int main(int argc, char** argv) {
  if (argc != 3) {
        fprintf(stderr, "Usage: %s <receiver port> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

  Packet packet;
  int receiver_port = atoi(argv[1]);
  float drop_probability = atof(argv[2]);

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
  server_addr.sin_port = htons(receiver_port);
  
  // 소켓과 주소 바인딩
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // 인사, 파일명 수신
  char file_buffer[BUF_SIZE];
  memset(file_buffer, 0, sizeof(file_buffer));
  clnt_addr_size = sizeof(client_addr);

  // "Greeting" 메시지 수신
  recvfrom(sockfd,  file_buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("Sender: %s\n", file_buffer);
  memset(file_buffer, 0, sizeof(file_buffer));
  
  // file name 수신
  recvfrom(sockfd, file_buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("File Name: %s\n", file_buffer);
  

  // 응답 전송
  sleep(1);
  sendto(sockfd, "OK", strlen("OK"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

  // 파일 저장하기
  FILE* fp;
  fp = fopen(file_buffer, "wb");
  memset(file_buffer, 0, sizeof(file_buffer));
  memset(&packet, 0, sizeof(Packet));
  int size[1];

  recvfrom(sockfd, size, sizeof(size), 0, (struct  sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
  while(1)
  {
    //ssize_t num_bytes = recvfrom(sockfd, file_buffer, BUF_SIZE, 0, (struct sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
    ssize_t num_bytes = recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);

    if(num_bytes == -1)
    { 
      printf("recv error\n");
      exit(1);
    }

    /*if (num_bytes > 0 && strcmp(packet.data, "Finish") == 0)
    {
      printf("Sender: %s", packet.data);
      break;
    }*/

    if (packet.type == 1)
    {
      printf("Sender: Finish\n");
      break;
    }

    if (size[0] > BUF_SIZE)
    { 
      size[0] -= BUF_SIZE;   
      fwrite(packet.data, 1, BUF_SIZE, fp);
    }
    else
    {
      fwrite(packet.data, 1, size[0], fp);
    }
    printf("%s\n", packet.data);
    memset(&packet.data, 0, sizeof(packet.data));
    /*if (num_bytes > 0 && strcmp(file_buffer, "Finish") == 0)
    {
      printf("Sender: %s", file_buffer);
      break;
    }

    if (size[0] > num_bytes)
    { 
      size[0] -= num_bytes;   
      fwrite(file_buffer, 1, num_bytes, fp);
    }
    else
    {
      fwrite(file_buffer, 1, strlen(file_buffer), fp);
    }
    memset(file_buffer, 0, sizeof(file_buffer));*/
  }
  memset(file_buffer, 0, sizeof(file_buffer));

  sleep(1);
  sendto(sockfd, "Welldone", strlen("Welldone"), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

  fclose(fp);

  return 0;

}