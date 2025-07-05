#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[5126] = {0};
    char move[100];
    char play_again_response[10];
    const char *server_ip = "127.0.0.1"; // Default IP address

    // If an IP address is provided as a command-line argument, use it
    if (argc == 2) {
        server_ip = argv[1];
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    while (1) {
        // Receive and print server messages (including board updates and prompts)
        memset(buffer, 0, sizeof(buffer));
        recv(sock, buffer, sizeof(buffer), 0);
        printf("%s", buffer);
        fflush(stdout);

        // Check if the game is over (contains "Wins" or "Draw")
        if (strstr(buffer, "Wins") != NULL || strstr(buffer, "Draw") != NULL) {
            // Get player response for playing again
            printf("Do you want to play again? (yes/no): ");
            fflush(stdout);
            scanf("%s", play_again_response);
            
            // Send response to server
            send(sock, play_again_response, strlen(play_again_response), 0);

            // If the player said "no", break the loop and close the connection
            if (strncmp(play_again_response, "no", 2) == 0) {
                break;
            }
        } else if (strstr(buffer, "turn") != NULL) {
            int row, col;
            scanf("%d %d", &row, &col);
            sprintf(move, "%d %d", row, col); // Convert row and col to string for sending to server
            // Send move to the server
            send(sock, move, strlen(move), 0);
        } else if (strstr(buffer, "doesn't want to play again.") != NULL) {
            break;
        }
    }

    // Close the socket connection
    close(sock);

    return 0;
}
