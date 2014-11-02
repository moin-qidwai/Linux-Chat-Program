Linux-Chat-Program
==================

This is the server and client code in C for a simple chat program for multiple client communication.

DETAILED DESCRIPTION:
=====================

This repository contains code for a very simple chat program implemented on the linux system, using base system libraries and the C programming language. The program consists of two major components, the first being the server program and the second is the Client program.

SERVER PROGRAM:
--------------
The server program maintains a list of clients and also a queue of client messages. It uses semaphores to make sure synchronisation errors do not occur. 

The server program accepts one optional input argument – listen_port_number. If this argument 
is missed, the server will use the default port number 3500 (defined in the provided header file 
chat_server.h). Once it starts, it runs forever. To terminate the server program, user can use 
SIGINT (kill -2 or Ctrl + C) or SIGTERM (kill -15) signals to terminate the chat server and all 
its threads.

CLIENT PROGRAM:
--------------
Once the program started, it accepts commands/instructions from the user. There are total 6 commands available for the user, and their actions are specified as follows:-

1. USER/user [username] – the argument username is used for setting the user identity of all messages posted to the chat server. As the chat server needs to display all messages with their associated usernames, the client program MUST get the user’s,username before setting up the connection to the chat server. A user can rename his/her
username by using this command (if current username clashes with other user’s,username).

2. JOIN/join [server name] [port number] – to connect to a chat server. A TCP connection will be established to the targeted chat server, and then a JOIN message (with user’s username) will be sent to the server. If the chat client has already connected to a server, it is not allowed to join another chat server before termination of current 
TCP connection.

3. SEND/send [message] – to send a message to the chat server. The message will be sent to the server via the established TCP connection, and will be shown in every client’s message window (broadcasted by the server). The client must be connected to the server before sending a message.

4. DEPART/depart – to terminate current chat connection. The user does not exit from the program, and is able to rejoin the server again.

5. EXIT/exit – the user exits from the program.

6. CLEAR/clear – the command window will be cleared.

COMPILING:
=========
To compile both the client and server programs simply run the following command in the project directory:
        
        $ make all

POSSIBLE ERRORS WHILE COMPILING:
================================

1. chat_client.c:4:20: fatal error: curses.h: No such file or directory
-----------------------------------------------------------------------
    In order to solve this one must use this command on fedora/CentOS:
      $ yum install ncurses-devel ncurses
    And this command on Ubuntu/Debian:
      $ sudo apt-get install libncurses5-dev libncursesw5-dev


