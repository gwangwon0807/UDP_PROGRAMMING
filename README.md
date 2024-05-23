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