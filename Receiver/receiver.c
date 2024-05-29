#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

int main(int argc, char** argv) {
  if (argc != 3) {
        fprintf(stderr, "Usage: %s <receiver port> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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
  char buffer[BUF_SIZE];
  memset(buffer, 0, sizeof(buffer));
  clnt_addr_size = sizeof(client_addr);

  // "Greeting" 메시지 수신
  recvfrom(sockfd,  buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("Sender: %s\n", buffer);
  memset(buffer, 0, sizeof(buffer));
  
  // file name 수신
  recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
  printf("File Name: %s\n", buffer);
  

  // 응답 전송
  sendto(sockfd, "OK", strlen("OK"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

  // 파일 저장하기
  FILE* fp;
  fp = fopen(buffer, "wb");
  memset(buffer, 0, sizeof(buffer));
  int size[1];

  log_fp = fopen("log.txt", "wb");
  fwrite("\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n", 1, strlen("\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n"), log_fp);

  recvfrom(sockfd, size, sizeof(size), 0, (struct  sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
  int ackNum = 1;
  srand(time(NULL));
  clock_t t;

  while(1)
  {
    int percent = rand() % 100;
    memset(&packet, 0, sizeof(Packet));
    t = clock();
    ssize_t num_bytes = recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);

    log_content.log_type = packet.type;
    log_content.log_ack = packet.ackNum;
    log_content.log_seq = packet.seqNum;
    log_content.log_loss = 0;
    
    if(packet.seqNum > 0)
    {
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC;
    }

    if (packet.type == 2)
    {
      log_event("RECV", &log_content, 0, log_content.log_time_taken);
      printf("Sender: Finish, Type: %d\n", packet.type);
      break;
    }
    log_event("RECV", &log_content, 0, log_content.log_time_taken);
    printf("Seq: %d, Ack: %d, Type: %d\n", packet.seqNum, ackNum, packet.type);

    packet.ackNum = ackNum;
    packet.type = 1;
    log_content.log_ack = packet.ackNum;
    log_content.log_type = packet.type;

    if(num_bytes == -1)
    { 
      printf("recv error\n");
      exit(1);
    }

    t = clock();
    if (size[0] > BUF_SIZE)
    { 
      size[0] -= BUF_SIZE;   
      fwrite(packet.data, 1, BUF_SIZE, fp);
    }
    else
    {
      fwrite(packet.data, 1, size[0], fp);
    }
    
    if(percent < (int)(drop_probability * 100))
    {
      log_content.log_loss = 1;
      log_event("SEND", &log_content, 0, log_content.log_time_taken);
      printf("prob\n");
      printf("%d, %d\n", percent, (int)(drop_probability * 100));
      continue;
    }
    else
    {
      log_event("SEND", &log_content, 0, log_content.log_time_taken);
      if(pre_packet.seqNum == packet.seqNum)
      {
        sendto(sockfd, &pre_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
      }    
      else
      {
        sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        
        if ((ackNum % 2)== 0)
        {
          ackNum++;
        }
        else
        {
          ackNum--;
        }
      }
    }
    memset(&pre_packet,0,sizeof(Packet));
    pre_packet = packet;
  }  
  
  sendto(sockfd, "Welldone", strlen("Welldone"), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

  fclose(fp);

  return 0;

}