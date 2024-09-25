//Compiling and Runnung process
1. You have to open 2 terminals to execute this project and go to the directory saving these files.
(Reciver's role is server, Sender's role is client)
2. First of all, enter the Reciver directory and type up this sentence "gcc -o receiver receiver.c".
3. And then, enter another terminal and type up this sentence "gcc -o sender sender.c".
4. If you've finished that, go to the Receiver terminal and type up "./receiver <receiver's port> <drop_probability>. e.g., "./receive 12345 0.1".
5. Go to the Sender terminal and you must type up "./sender <sender' port> <receiver's IP> <receiver's port> <timout interval> <file name> <drop probability>. e.g., "./sender 12346 127.0.0.1 12345 1 hello.txt 0.1".

// If you want to go to the this directory then, you can type up this command.
1. cd ./Receiver and cd ./Sender 
2. "./" is current directory path so, "./" must be Termpj that is name of directory for executing this project

// Check
1. You can check written files in Receiver and Sender directory
2. If you want to check the file, go to the Receiver or Sender directory or type up this command "vi filename(log or hello or cwnd).txt" on terminal

//Description
1. Sender will send (file / packet) to Receiver. I set packet's size is '216' but buffer size is '200'. Because, I wanted to check more easy. 
2. Receiver will receive packet. And then, if Receiver received packet, send Ack to Sender.
3. This program has congestion control system(Slow start, Fast retransmit, Fast recovery). and, was added "3-way handshaking process"
4. This program has drop probability. If dropped packet, occur timeout and resend. Function as pakcet loss.
5. Be written log.txt file in each directory. That include (Flage, type, seq, ack, length, loss, timout, time).
6. Alarm system call must be entered in seconds in C. So minimum timout interval's size is '1'.
7. Drop probability must be decimal type, e.g., 0.1, 0.2....
8. Cwnd is increase exponentialy on slow start mode.
9. If dropped packet and occured 3dup-ack or RTO, ssthresh is cwnd / 2. And then, cwnd will increas slowly(Each Ack -> cwnd + 1 / cwnd)
10. If occured 3dup-ack, CWND size is sshtresh + 3. and, If occured RTO, CWND size is 1.


// Note
1. Type is indicated "DATA = 0, ACK = 1, SYN = 2, SYN_ACK = 3, FIN = 4"
2. Flag is indicated "NONE = 0, SYN_FLAG = 1, FIN_FLAG = 2"
3. "10%, 20% .... 100%" is percentage of loading

* When you compile, you may see warning message. You should ignore that.

