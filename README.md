# X-and-O

A simple networked Tic Tac Toe game with both TCP and UDP implementations, allowing two players to connect remotely and play against each other.

## Overview

This project provides a complete implementation of the classic Tic Tac Toe game with networking support. Players can connect to a central server and play against each other over a network. The game supports both TCP and UDP connection protocols.

## Features

- Play Tic Tac Toe against another player over a network
- Support for both TCP and UDP protocols
- Interactive gameplay with real-time board updates
- Input validation to prevent illegal moves
- Game state tracking (wins, draws)
- Option to play again after a game ends

## Components

- **TCPserver.c**: Server implementation using TCP
- **TCPclient.c**: Client implementation using TCP
- **UDPserver.c**: Server implementation using UDP
- **UDPclient.c**: Client implementation using UDP

## How to Play

### Compiling the Code

First, compile the server and client code:

```bash
# For TCP
gcc TCPserver.c -o tcp_server
gcc TCPclient.c -o tcp_client

# For UDP
gcc UDPserver.c -o udp_server
gcc UDPclient.c -o udp_client
```

## Running the Game
1. Start the server:
```bash
# For TCP
./tcp_server

# For UDP
./udp_server
```

2. Connect two clients:
```bash
# For TCP
./tcp_client [server_ip]

# For UDP
./udp_client [server_ip]
```

If no IP is specified, it defaults to localhost (127.0.0.1).

3. Players take turns entering their moves in the format row col (both 0-indexed):
```
0 0    # Top-left corner
1 1    # Center
2 2    # Bottom-right corner
```

4. After a game ends, both players will be asked if they want to play again.

## Game Board Layout
```
 0,0 | 0,1 | 0,2
---------------
 1,0 | 1,1 | 1,2
---------------
 2,0 | 2,1 | 2,2
```

## Protocol Information
- Port: Both servers use port 8080 by default
- Player Identification: First connected client is Player 1 (X), second is Player 2 (O)
- Move Format: Row and column as integers separated by a space

## Technical Details
- Written in C for maximum compatibility
- Uses standard socket programming APIs
- Handles network communication, game state, and player turns
- Validates moves to ensure fair gameplay
- Supports graceful connection handling

## Requirements
- GCC or compatible C compiler
- Linux/Unix environment
- Network connectivity between server and clients