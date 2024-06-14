#include "../Packet/packet.h"

int recv_seq = 0;
int last_ack;
int temp;

void transform(Packet* pck);

int main(int argc, char** argv) {
  if (argc != 3) {
        fprintf(stderr, "Usage: %s <receiver port> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

  int receiver_port = atoi(argv[1]);
  float drop_probability = atof(argv[2]);
  clock_t t;

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

  log_fp = fopen("log.txt", "wb");
  fwrite("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n", 1, strlen("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n"), log_fp);

  

  //3-way Handshaking
  Packet signal_packet;
  FILE* fp;
  while(1){
    recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);
    if(signal_packet.flag == SYN_FLAG)
    {

      log_event1("RECV", &log_content, &signal_packet, 0, 0);
      printf("Sender: Greeting\n");
      printf("File Namge: %s\n", signal_packet.data);
      fp = fopen(signal_packet.data, "wb");

      signal_packet.flag = SYN_ACK;
      signal_packet.type = ACK;
      transform(&signal_packet);

      t = clock();
      sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
      log_event1("SEND", &log_content, &signal_packet, 0, 0);

      memset(&signal_packet, 0, sizeof(signal_packet));
    }
    else if (signal_packet.type == ACK)
    {
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC;

      log_event1("RECV", &log_content, &signal_packet, 0, log_content.log_time_taken);
      break;
    }
    
    else
    {
      continue;
    }
  }
  fwrite("\n", 1, strlen("\n"), log_fp);
  memset(&log_content, 0, sizeof(Log));   


  // 파일 저장하기
  int size[1];
  recvfrom(sockfd, size, sizeof(size), 0, (struct  sockaddr*)&client_addr,(unsigned int*)&clnt_addr_size);
  int ackNum = 1;
  srand(time(NULL));
  int length = size[0];
  int count = 1;
  printf("Please wait..\n");

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
    
    if (size[0] - length >= size[0] *  (0.1 * count))
    {
      printf("%d%%\n", count++ * 10);
    }
    
    if(packet.seqNum > 0)
    {
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC;
    }

    if (packet.type == FIN)
    {
      signal_packet = create_signal_packet(FIN, FIN_FLAG, packet.seqNum, packet.ackNum);
      log_content.log_flag = FIN_FLAG;
      log_content.log_type = FIN;
      log_content.log_length = 0;

      fwrite("\n", 1, strlen("\n"), log_fp);
      log_event("RECV", &log_content, 0, log_content.log_time_taken);
      printf("100%%\n");
      printf("Sender: Finish\n");

      memset(&log_content, 0, sizeof(Log));
      break;
    }
    log_event("RECV", &log_content, 0, log_content.log_time_taken);

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
    
    if(percent < (int)(drop_probability * 100))
    {
      log_content.log_loss = 1;
      log_event("SEND", &log_content, 0, log_content.log_time_taken);
      continue;
    }
    else
    { 
      log_content.log_loss = 0;
      if (length > BUF_SIZE)
      { 
        length -= BUF_SIZE;
        log_content.log_length = BUF_SIZE;   
        fwrite(packet.data, 1, BUF_SIZE, fp);
      }
      else
      {
        log_content.log_length = length;
        fwrite(packet.data, 1, length, fp);
      }

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

  signal_packet = create_signal_packet(ACK, FIN_FLAG, packet.seqNum, packet.ackNum);
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
  log_content.log_flag = FIN_FLAG;
  log_content.log_type = ACK;
  log_content.log_ack = packet.ackNum;
  log_content.log_seq = packet.seqNum;

  log_event("SEND", &log_content, 0, 0);
  memset(&signal_packet, 0, sizeof(Packet));

  signal_packet = create_signal_packet(FIN, NONE, packet.seqNum, packet.ackNum);
  t = clock();
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
  log_content.log_flag = FIN_FLAG;
  log_content.log_type = FIN;
  log_content.log_ack = packet.ackNum;
  log_content.log_seq = packet.seqNum;

  log_event("SEND", &log_content, 0, 0);

  memset(&signal_packet, 0, sizeof(Packet));

  recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, (unsigned int*)&clnt_addr_size);

  if(signal_packet.type == ACK)
  {
    t = clock() -t;
    log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC;
    log_content.log_type = ACK;
    log_content.log_flag = signal_packet.flag;
    log_event("RECV", &log_content, 0, log_content.log_time_taken);
  }

  fclose(fp);
  fclose(log_fp);
  close(sockfd);

  return 0;

}

void transform(Packet* pck)
{
  last_ack = pck->ackNum;
  pck->ackNum = pck->seqNum + 1;
  pck->seqNum = last_ack;
}
