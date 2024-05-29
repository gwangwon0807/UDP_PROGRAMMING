#temrpj#1
// Compiling and Running process
1. You have to open 2 terminals to execute this project and go to the directory saving these files.
(Reciver's role is server, Sender's role is client)
2. First of all, enter the Reciver directory and type up this sentence "gcc -o server server.c".
3. And then, enter another terminal and type up this sentence "gcc -o client client.c".
4. If you've finished that, go to the Receiver terminal and type up "./server".
5. The Sender terminal requires additional typing that is file name to send to the Receiver. so, you have to type up this sentence "./client hello.txt" ("hello.txt" is just example file).

// If you want to go to the this directory then, you can type up this command.
1. cd ./Receiver and cd ./Sender 
2. "./" is current directory path so, "./" must be Termpj that is name of directory for executing this project

// Check
1. Receiver will copy file that received file.
2. If you want to check the file, go to the Receiver directory or type up this command "vi hello.txt" on Receiver terminal

#termpj#2
//Compiling and Runnung process
1. You have to open 2 terminals to execute this project and go to the directory saving these files.
(Reciver's role is server, Sender's role is client)
2. First of all, enter the Reciver directory and type up this sentence "gcc -o receiver receiver.c".
3. And then, enter another terminal and type up this sentence "gcc -o sender sender.c".
4. If you've finished that, go to the Receiver terminal and type up "./receiver <receiver's port> <drop_probability>. ex. "./receive 12345 0.1".
5. Go to the Sender terminal and you must type up "./sender <sender' port> <receiver's IP> <receiver's port> <timout interval> <file name> <drop probability>. ex. "./sender 12346 127.0.0.1 12345 1 hello.txt 0.1".

// If you want to go to the this directory then, you can type up this command.
1. cd ./Receiver and cd ./Sender 
2. "./" is current directory path so, "./" must be Termpj that is name of directory for executing this project

// Check
1. You can check received file, log.txt file in Receiver directory and You can also check log.txt file in Sender directory.
2. If you want to check the file, go to the Receiver or Sender directory or type up this command "vi filename(log or hello).txt" on terminal

//Description
1. Sender will send (file / packet) to Receiver. I set packet's size is '216' but buffer size is '200'. Because, I wanted to check more easy. 
2. Receiver will receive packet. And then, if Receiver received packet, send Ack to Sender.
3. This program has drop probability. If dropped packet, occur timeout and resend.
4. Be made log.txt file each directory. That include (type, seq, ack, length, loss, timout, time).
