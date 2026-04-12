# Networked Chat Application

## Overview
This project implements a multi-client networked chat application written in C.  
A central server accepts incoming connections, manages client state, and routes messages between users.  
Clients connect through a command-line interface, send messages, receive broadcasts, and use chat commands.

The system demonstrates:
- Socket programming  
- Concurrency using pthreads  
- Inter-process communication  
- Synchronization and shared-state management  
- Signal handling and shutdown  
- Logging and debugging support 

## Features
- Multi-client support using pthreads  
- Broadcast and private messaging  
- Commands: `/nick`, `/msg`, `/quit`, `/help`  
- Timestamped message formatting  
- Thread-safe client registry and per-client send queues  
- Asynchronous server logging  
- Graceful shutdown via signal handling  
- Robust error handling for partial reads/writes and disconnects  

## File Structure Example
server.c        – main server logic, listener, client handlers  
client.c        – client CLI, network I/O  
router.c        – command parsing and message routing  
registry.c      – client registry with mutex protection  
logger.c        – asynchronous logging  
utils.h        – shared definitions  
Makefile

## Building
make


## Running Server
./server <port>


### Client
./client <server_ip> <port>
