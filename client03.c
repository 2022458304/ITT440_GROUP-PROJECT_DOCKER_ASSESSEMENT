// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8082
#define SERVER_IP "172.19.0.4"

void displayOptions() {
    printf("\nOptions:\n");
    printf("1. Add User\n");
    printf("2. Delete User\n");
    printf("3. Show Database\n");
    printf("4. End Connection\n");
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    char option[256];
    char request[256];
    char response[256];

    while (1) {
        displayOptions();

        printf("\nEnter an option: ");
        fgets(option, sizeof(option), stdin);

        option[strcspn(option, "\n")] = '\0';

        if (strcmp(option, "1") == 0) {
            char username[256];
            char pointsStr[256];

            printf("Enter the username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            printf("Enter the points: ");
            fgets(pointsStr, sizeof(pointsStr), stdin);
            pointsStr[strcspn(pointsStr, "\n")] = '\0';

            sprintf(request, "ADD_USER %s %s", username, pointsStr);
            send(sock, request, strlen(request), 0);

            recv(sock, response, sizeof(response), 0);
            printf("Server response: %s\n", response);
        } else if (strcmp(option, "2") == 0) {
            char username[256];

            printf("Enter the username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            sprintf(request, "DELETE_USER %s", username);
            send(sock, request, strlen(request), 0);

            recv(sock, response, sizeof(response), 0);
            printf("Server response: %s\n", response);
        } else if (strcmp(option, "3") == 0) {
            sprintf(request, "SHOW_DB");
            send(sock, request, strlen(request), 0);

            printf("\nServer response:\n");
            while (1) {
                memset(response, 0, sizeof(response));
                ssize_t bytesRead = recv(sock, response, sizeof(response) - 1, 0);
                if (bytesRead <= 0) {
                    break;
                }
                printf("%s", response);
            }

            continue;
        } else if (strcmp(option, "4") == 0) {
            sprintf(request, "END_CONN");
            send(sock, request, strlen(request), 0);
            break;
        } else {
            printf("Invalid option. Please try again.\n");
        }
    }

    close(sock);

    return 0;
}

