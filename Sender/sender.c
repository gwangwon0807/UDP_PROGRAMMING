#include <signal.h>
#include "../Packet/packet.h"

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

  //setting SIGALRM
  struct sigaction sa;
  sa.sa_handler = resend;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);
  srand(time(NULL));
  clock_t t;

  // 파일 열기
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("file open failed");
    exit(EXIT_FAILURE);
  }

  //log 파일 열기 및 저장 
  log_fp = fopen("log.txt", "wb");
  fwrite("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n", 1, strlen("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n"), log_fp);

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
  

  // 3-way Handshaking
  while(1)
  {
    memset(&signal_packet, 0, sizeof(Packet));
    memset(&log_content, 0, sizeof(Log));

    signal_packet = create_signal_packet(SYN, SYN_FLAG, seqNum, 0);
    log_content.log_flag = SYN_FLAG;
    log_content.log_type = SYN;
    log_content.log_seq = seqNum++;

    strncpy(signal_packet.data, filename, strlen(filename));
    t = clock();
    sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    memset(&signal_packet, 0, sizeof(Packet));

    log_event("SEND", &log_content, 0, 0);

    recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);

    if(signal_packet.flag == SYN_ACK)
    {
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
      log_content.log_flag = SYN_FLAG;
      log_content.log_type = SYN;
      log_event("RECV", &log_content, 0, log_content.log_time_taken);

      printf("Receiver: OK\n");
  
      memset(&signal_packet, 0, sizeof(Packet));
      signal_packet = create_signal_packet(ACK, NONE, seqNum, 0);

      log_content.log_type = ACK;
      log_content.log_flag = NONE;
      log_content.log_seq = seqNum++;

      sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
      log_event("SEND", &log_content, 0, 0);

      memset(&signal_packet, 0, sizeof(Packet));
      break;
    }
  }
  fwrite("\n", 1, strlen("\n"), log_fp);



  // 파일 전송
  char buffer[BUF_SIZE];
  int size[1];
  fseek(fp, 0, SEEK_END);
  size[0] = (int)ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  

  printf("Pleas wait..\n");


  sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  size_t bytesRead;
  memset(&packet.data, 0, sizeof(packet.data));
  memset(&log_content, 0, sizeof(Log));
  log_content.log_ack = 0;
  int length = size[0];
  int count = 1;

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

     if (size[0] - length >= size[0] *  (0.1 * count))
    {
      printf("%d%%\n", count++ * 10);
    }

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

    while(recvfrom(sockfd, &packet, sizeof(Packet),0, NULL, NULL) == -1){
      continue;
    }
    
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
  memset(&log_content, 0, sizeof(Log));
  fwrite("\n", 1, strlen("\n"), log_fp);
  printf("100%%\n");

  // 4-way Handshaking
  signal_packet = create_signal_packet(FIN, FIN_FLAG, seqNum , 0);
  log_content.log_flag = FIN_FLAG;
  log_content.log_type = FIN;
  log_content.log_seq = seqNum++;

  packet.type = FIN;
  packet.flag = FIN_FLAG;
  log_event("SEND", &log_content, 0, 0);
  t = clock();
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  memset(&signal_packet, 0, sizeof(Packet));

  t = clock() - t;
  log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
  
  recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);
  log_event("RECV", &log_content, 0, log_content.log_time_taken);
  memset(&signal_packet, 0, sizeof(Packet));

  recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);
  log_event("RECV", &log_content, 0, 0);
  memset(&signal_packet, 0, sizeof(Packet));

  signal_packet = create_signal_packet(ACK, NONE, seqNum , 0);
  log_content.log_seq = seqNum;
  log_content.log_ack = signal_packet.ackNum;
  log_content.log_flag = signal_packet.flag;

  log_event("SEND", &log_content, 0, 0);
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  memset(&signal_packet, 0, sizeof(Packet));
  printf("Receiver: Welldone\n");
  sleep(1);

  // 파일 닫기
  fclose(fp);
  fclose(log_fp);
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
