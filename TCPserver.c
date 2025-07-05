#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>

#define PORT 8080

char board[3][3];
int current_player = 1; // 1 for Player 1, 2 for Player 2

// Initialize the game board with empty spaces
void init_board() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            board[i][j] = ' ';
}

// Display the current game board
void print_board(int client1, int client2) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "Current board:\n %c | %c | %c\n-----------\n %c | %c | %c\n-----------\n %c | %c | %c\n-----------------------------------------------------------------\n",
             board[0][0], board[0][1], board[0][2], 
             board[1][0], board[1][1], board[1][2], 
             board[2][0], board[2][1], board[2][2]);
    send(client1, buffer, strlen(buffer), 0);
    send(client2, buffer, strlen(buffer), 0);
}

// Check if a move is valid (spot is empty and within bounds)
int is_valid_move(int row, int col) {
    return (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ');
}

// Check if the game is won or drawn
int check_winner() {
    for (int i = 0; i < 3; i++) {
        // Check rows and columns
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return (board[i][0] == 'X') ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return (board[0][i] == 'X') ? 1 : 2;
    }
    // Check diagonals
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return (board[0][0] == 'X') ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return (board[0][2] == 'X') ? 1 : 2;

    // Check if the board is full (draw)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ')
                return 0; // Game is still ongoing

    return 3; // Draw
}

int ask_play_again(int client1, int client2) {
    char response1[1000], response2[1000];
    recv(client1, response1, sizeof(response1), 0);
    recv(client2, response2, sizeof(response2), 0);

    // Check responses
    if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "yes", 3) == 0) {
        return 1; // Both want to play again
    } else if (strncmp(response1, "no", 2) == 0 && strncmp(response2, "no", 2) == 0) {
        return 0; // Both don't want to play again
    } else if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "no", 2) == 0) {
        send(client1, "Player 2 doesn't want to play again.\n", 37, 0);
    } else if (strncmp(response1, "no", 2) == 0 && strncmp(response2, "yes", 3) == 0) {
        send(client2, "Player 1 doesn't want to play again.\n", 37, 0);
    }

    return 0; // If one player doesn't want to play again
}


int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    int opt = 1;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set the socket to be reusable (both address and port)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt(SO_REUSEPORT) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for clients
    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Accept player 1
    if ((new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Player 1 connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected.\n");

    // Accept player 2
    if ((new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Player 2 connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected.\n");

    // Inform players of their roles
    send(new_socket1, "You are Player 1 (X)\n", 21, 0);
    send(new_socket2, "You are Player 2 (O)\n", 21, 0);

    int row, col, winner = 0;
    char player_move[100];
    int play_again = 1; // Flag to track if they want to play again

    while (play_again) {
        // Initialize the game board
        init_board();
        current_player = 1; // Reset to Player 1 for the new game
        winner = 0; // Reset winner for the new game

        // Game loop
        while (winner == 0) {
            int current_socket = (current_player == 1) ? new_socket1 : new_socket2;
            int opponent_socket = (current_player == 1) ? new_socket2 : new_socket1;

            // Print the board for both players
            print_board(new_socket1, new_socket2);

            // Send turn prompt
            snprintf(buffer, sizeof(buffer), "Player %d's turn. Enter your move (row col):\n", current_player);
            send(current_socket, buffer, strlen(buffer), 0);

            // Receive and process move
            int bytes_received = recv(current_socket, player_move, sizeof(player_move) - 1, 0);
            if (bytes_received <= 0) {
                perror("recv failed");
                break; // Connection may have been closed
            }
            player_move[bytes_received] = '\0'; // Null-terminate the string
            sscanf(player_move, "%d %d", &row, &col);

            // Check for valid move
            while (!is_valid_move(row, col)) {
                send(current_socket, "Invalid move, try again.\n", 25, 0);
                snprintf(buffer, sizeof(buffer), "Player %d's turn. Enter your move (row col):\n", current_player);
                send(current_socket, buffer, strlen(buffer), 0);

                // Wait for a new move
                bytes_received = recv(current_socket, player_move, sizeof(player_move) - 1, 0);
                if (bytes_received <= 0) {
                    perror("recv failed");
                    break; // Connection may have been closed
                }
                player_move[bytes_received] = '\0'; // Null-terminate the string
                sscanf(player_move, "%d %d", &row, &col);
            }

            // Update the board
            board[row][col] = (current_player == 1) ? 'X' : 'O';

            // Check for winner or draw
            winner = check_winner();
            if (winner != 0) break;

            // Switch turns
            current_player = (current_player == 1) ? 2 : 1;
        }

        // Game over
        print_board(new_socket1, new_socket2);

        // Prepare and send the result to both players
        if (winner == 1 || winner == 2) {
            snprintf(buffer, sizeof(buffer), "Player %d Wins!\n", winner);
        } else {
            snprintf(buffer, sizeof(buffer), "It's a Draw!\n");
        }
        send(new_socket1, buffer, strlen(buffer), 0);
        send(new_socket2, buffer, strlen(buffer), 0);

        // Ask if they want to play again
        play_again = ask_play_again(new_socket1, new_socket2);
        if(play_again == 0) break;
    }

    // Close connections
    close(new_socket1);
    close(new_socket2);
    close(server_fd);

    return 0;
}
