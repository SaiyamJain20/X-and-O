#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
void print_board(struct sockaddr_in client1, struct sockaddr_in client2, int sockfd) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "Current board:\n %c | %c | %c\n-----------\n %c | %c | %c\n-----------\n %c | %c | %c\n-----------------------------------------------------------------\n",
             board[0][0], board[0][1], board[0][2], 
             board[1][0], board[1][1], board[1][2], 
             board[2][0], board[2][1], board[2][2]);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1, sizeof(client1));
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2, sizeof(client2));
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

int ask_play_again(struct sockaddr_in client1, struct sockaddr_in client2, int sockfd) {
    char response1[1000], response2[1000];
    socklen_t len1 = sizeof(client1);
    socklen_t len2 = sizeof(client2);
    recvfrom(sockfd, response1, sizeof(response1), 0, (struct sockaddr *)&client1, &len1);
    recvfrom(sockfd, response2, sizeof(response2), 0, (struct sockaddr *)&client2, &len2);

    // Check responses
    if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "yes", 3) == 0) {
        return 1; // Both want to play again
    } else if (strncmp(response1, "no", 2) == 0 && strncmp(response2, "no", 2) == 0) {
        return 0; // Both don't want to play again
    } else if (strncmp(response1, "yes", 3) == 0 && strncmp(response2, "no", 2) == 0) {
        sendto(sockfd, "Player 2 doesn't want to play again.\n", 37, 0, (struct sockaddr *)&client1, sizeof(client1));
    } else if (strncmp(response1, "no", 2) == 0 && strncmp(response2, "yes", 3) == 0) {
        sendto(sockfd, "Player 1 doesn't want to play again.\n", 37, 0, (struct sockaddr *)&client2, sizeof(client2));
    }

    return 0; // If one player doesn't want to play again
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client1, client2;
    char buffer[1024];
    int row, col, winner = 0;
    socklen_t len1 = sizeof(client1);
    socklen_t len2 = sizeof(client2);
    int play_again = 1; // Flag to track if they want to play again
    char player_move[100];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Receive "connect" message from Player 1
    recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client1, &len1);
    printf("Player 1 connected.\n");
    sendto(sockfd, "You are Player 1 (X)\n", 21, 0, (struct sockaddr *)&client1, len1);

    // Receive "connect" message from Player 2
    recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client2, &len2);
    printf("Player 2 connected.\n");
    sendto(sockfd, "You are Player 2 (O)\n", 21, 0, (struct sockaddr *)&client2, len2);

    while (play_again) {
        // Initialize the game board
        init_board();
        current_player = 1; // Reset to Player 1 for the new game
        winner = 0; // Reset winner for the new game

        // Game loop
        while (winner == 0) {
            struct sockaddr_in current_client = (current_player == 1) ? client1 : client2;
            struct sockaddr_in opponent_client = (current_player == 1) ? client2 : client1;
            socklen_t current_length = (current_player == 1) ? len1 : len2;
            socklen_t opponent_length = (current_player == 1) ? len2 : len1;

            // Print the board for both players
            print_board(client1, client2, sockfd);

            // Send turn prompt
            snprintf(buffer, sizeof(buffer), "Player %d's turn. Enter your move (row col):\n", current_player);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&current_client, current_length);

            // Receive and process move
            int bytes_received = recvfrom(sockfd, player_move, sizeof(player_move) - 1, 0, (struct sockaddr *)&current_client, &current_length);
            if (bytes_received <= 0) {
                perror("recvfrom failed");
                break;
            }
            player_move[bytes_received] = '\0'; // Null-terminate the string
            sscanf(player_move, "%d %d", &row, &col);

            // Check for valid move
            while (!is_valid_move(row, col)) {
                sendto(sockfd, "Invalid move, try again.\n", 25, 0, (struct sockaddr *)&current_client, current_length);
                snprintf(buffer, sizeof(buffer), "Player %d's turn. Enter your move (row col):\n", current_player);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&current_client, current_length);

                // Wait for a new move
                bytes_received = recvfrom(sockfd, player_move, sizeof(player_move) - 1, 0, (struct sockaddr *)&current_client, &current_length);
                if (bytes_received <= 0) {
                    perror("recvfrom failed");
                    break;
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
        print_board(client1, client2, sockfd);
        if (winner == 1 || winner == 2) {
            snprintf(buffer, sizeof(buffer), "Player %d Wins!\n", winner);
        } else {
            snprintf(buffer, sizeof(buffer), "It's a Draw!\n");
        }
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client1, len1);
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client2, len2);

        play_again = ask_play_again(client1, client2, sockfd);
        if(play_again == 0) break;
    }

    close(sockfd);
    return 0;
}
