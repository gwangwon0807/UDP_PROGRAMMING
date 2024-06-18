#include <signal.h>
#include <math.h>
#include "../Packet/packet.h"

struct sockaddr_in server_addr;
Packet last_recv_pck;

FILE *fp;

int sockfd;
int timeout_interval;

int cwnd = 1;
int send_cnt = 0;
int ssthresh = 10000;
int dup_ack = 0;
int temp;

void resend();
void transform(Packet*);

int main(int argc, char** argv) 
{
  if (argc != 7) {
        fprintf(stderr, "Usage: %s <sender port> <receiver IP> <receiver port> <timeout interval> <filename> <drop probability>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

  memset(&pre_packet, 0, sizeof(Packet));
  int seqNum = 1;
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
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("file open failed");
    exit(EXIT_FAILURE);
  }

  //log 파일 열기 및 저장 
  log_fp = fopen("log.txt", "wb");
  log_cwnd = fopen("cwnd.txt", "wb");
  fwrite("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n", 1, strlen("\t\tFlag\t\tType\t\tSeq\t\tAck\t\tLength\t\tLoss\t\tTimeout\t\ttime\n"), log_fp);
  fwrite("Event      \t\tCwnd\t\tSsthresh\n", 1, strlen("Event      \t\tCwnd\t\tSsthresh\n"), log_cwnd);

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
    strncpy(signal_packet.data, filename, strlen(filename));
    t = clock();
    sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    pre_packet = signal_packet;
    log_event("SEND", &log_content, &signal_packet, 0, 0);

    memset(&signal_packet, 0, sizeof(Packet));
    recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);
    if(signal_packet.flag == SYN_ACK)
    {
      t = clock() - t;
      log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
      
      log_event("RECV", &log_content, &signal_packet, 0, log_content.log_time_taken);

      printf("Receiver: OK\n");

      last_recv_pck = signal_packet;
      transform(&signal_packet);
      sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
      log_event("SEND", &log_content, &signal_packet,0, 0);
      
      memset(&pre_packet, 0, sizeof(Packet));
      memset(&signal_packet, 0, sizeof(Packet));
      break;
    }
  }
  fwrite("\n", 1, strlen("\n"), log_fp);



  // 파일 전송
  int size[1];
  fseek(fp, 0, SEEK_END);
  size[0] = (int)ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  printf("Pleas wait..\n");

  sendto(sockfd, size, sizeof(size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  size_t bytesRead = 1;
  memset(&packet.data, 0, sizeof(packet.data));
  memset(&log_content, 0, sizeof(Log));
  log_content.log_ack = 0;

  int count = 1;
  int cur_seq;

  while((int)ftell(fp) != size[0])
  {
    for(int i = 0; i < cwnd; i++)
    {
      int start_offset;

      if(i == 0)
      {
        cur_seq = last_recv_pck.ackNum;
        start_offset = last_recv_pck.ackNum - 2;
        fseek(fp, (long)start_offset, SEEK_SET);
      }
      else
      {
        cur_seq += pre_packet.length;
      }
      packet.seqNum = cur_seq;
      packet.ackNum = last_recv_pck.seqNum + 1;
      bytesRead = fread(packet.data, 1, BUF_SIZE, fp);
      
      int percent = rand() % 100;
      
      packet.length = strlen(packet.data);
      
      if(i == cwnd -1)
      {
        alarm(timeout_interval);
        t = clock();
      }
      

      if(percent < (int)(drop_probability * 100))
      { 
        log_content.log_loss = 1;
        log_event("SEND", &log_content, &packet, log_content.log_timeout, log_content.log_time_taken);
      }
      else
      {
        sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        log_event("SEND", &log_content, &packet,log_content.log_timeout, log_content.log_time_taken);
      }

      send_cnt++;
      pre_packet = packet;
      memset(&log_content, 0, sizeof(Log));
      memset(&packet, 0, sizeof(Packet));

      if(bytesRead<= 0)
      {
        break;
      }
    }
    log_event_cwnd("SEND      ", cwnd, ssthresh);

    for(int i = 0; i < send_cnt; i++)
    {
      recvfrom(sockfd, &last_recv_pck, sizeof(Packet),0, NULL, NULL);
      if((int)((float)last_recv_pck.ackNum / (float)size[0] * 100) >= 10 * count)
      {
        printf("%d%%\n", 10 * count++);
      }
      if(log_content.log_timeout == 1)
      {
        break;
      }

      if(i == send_cnt - 1)
      { 
        alarm(0);
        t = clock() - t;  
        log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
      }
      else
      {
        t = clock() - t;  
        log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC;
      }
      
      log_event("RECV", &log_content, &last_recv_pck, 0, log_content.log_time_taken);

      if (pre_packet.ackNum == last_recv_pck.ackNum)
      {
        if(++dup_ack == 3)
        {
          ssthresh = (int)ceil(cwnd / 2);
          cwnd = ssthresh + 3;
          dup_ack = 0;
          log_event_cwnd("DUP_ACK   ", cwnd, ssthresh);
          break;
        }
        continue;
      }
      if (cwnd >= ssthresh)
      {
        cwnd += 1/cwnd;
        log_event_cwnd("Cong-avoid", cwnd, ssthresh);
      }
      else
      {
        cwnd += 1;
        log_event_cwnd("Slow-Start", cwnd, ssthresh);
      }
      pre_packet = last_recv_pck;
    }
    
    send_cnt = 0;
  }

  memset(&log_content, 0, sizeof(Log));
  fwrite("\n", 1, strlen("\n"), log_fp);

  // 4-way Handshaking  
  signal_packet = create_signal_packet(FIN, FIN_FLAG, pre_packet.seqNum , pre_packet.ackNum);
  
  transform(&signal_packet);
  log_event("SEND", &log_content, &signal_packet, 0, 0);
  t = clock();
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  memset(&signal_packet, 0, sizeof(Packet));

  t = clock() - t;
  log_content.log_time_taken = ((float)t) / CLOCKS_PER_SEC; //Send and Receive Time
  
  recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);
  log_event("RECV", &log_content, &signal_packet, 0, log_content.log_time_taken);
  memset(&signal_packet, 0, sizeof(Packet));

  recvfrom(sockfd, &signal_packet, sizeof(Packet), 0, NULL, NULL);
  log_event("RECV", &log_content, &signal_packet, 0, 0);
  
  transform(&signal_packet);
  signal_packet.type = ACK;
  signal_packet.flag = NONE;

  log_event("SEND", &log_content, &signal_packet, 0, 0);
  sendto(sockfd, &signal_packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  memset(&signal_packet, 0, sizeof(Packet));
  printf("Receiver: Welldone\n");
  sleep(1);

  // 파일 닫기
  fclose(fp);
  fclose(log_fp);
  fclose(log_cwnd);
  close(sockfd);
  return 0;
}

void resend()
{
  log_content.log_timeout = 1;
  ssthresh = (int)ceil(cwnd / 2);
  cwnd = 1;
  dup_ack = 0;
  fseek(fp, last_recv_pck.ackNum - 2, SEEK_SET);
  fwrite("\n<----------------------RTO---------------------->\n\n", 1, strlen("\n<----------------------RTO---------------------->\n\n"), log_fp);
  fwrite("\n<----------------------RTO---------------------->\n\n", 1, strlen("\n<----------------------RTO---------------------->\n\n"), log_cwnd);
  alarm(0);
}

void transform(Packet* pck)
{
  temp = pck->seqNum + 1;
  pck->seqNum = pck->ackNum;
  pck->ackNum = temp;
}