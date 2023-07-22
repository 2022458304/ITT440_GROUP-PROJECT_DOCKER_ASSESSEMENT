// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <time.h>

#define DB_HOST "server01-mysql"
#define DB_USER "myuser"
#define DB_PASSWORD "my-secret-pw"
#define DB_NAME "gp_db"
#define PORT 8082

void addUserToDatabase(MYSQL *conn, const char *username, int points, const char
 *datetime) {
    char insertQuery[512];
    sprintf(insertQuery, "INSERT INTO usertable (username, userpoints, datetime_
stamps) VALUES ('%s', %d, '%s')", username, points, datetime);
    if (mysql_query(conn, insertQuery) != 0) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
}

void deleteUserFromDatabase(MYSQL *conn, const char *username) {
    char deleteQuery[512];
    sprintf(deleteQuery, "DELETE FROM usertable WHERE username = '%s'", username);
    if (mysql_query(conn, deleteQuery) != 0) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
}

void showDatabase(MYSQL *conn, int client_sock) {
    if (mysql_query(conn, "SELECT * FROM usertable") != 0) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    int numFields = mysql_num_fields(result);
    MYSQL_ROW row;

    char response[256] = {0};
    sprintf(response, "\nDatabase Contents:\n");
    send(client_sock, response, strlen(response), 0);

    printf("\nDatabase Contents:\n");
    printf("---------------------------------\n");
    while ((row = mysql_fetch_row(result)) != NULL) {
        memset(response, 0, sizeof(response));
        for (int i = 0; i < numFields; i++) {
            printf("%s\t", row[i] ? row[i] : "NULL");
            strcat(response, row[i] ? row[i] : "NULL");
            strcat(response, "\t");
        }
        printf("\n");
        strcat(response, "\n");
        send(client_sock, response, strlen(response), 0);
    }
    printf("---------------------------------\n");

    mysql_free_result(result);
}

char* getCurrentDatetime() {
    time_t now;
    struct tm *timeinfo;
    char *datetime = (char *)malloc(20 * sizeof(char));

    time(&now);
    timeinfo = localtime(&now);
    strftime(datetime, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

    return datetime;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running...\n");

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS usertable (username VARCHAR(50), userpoints INT, datetime_stamps DATETIME)") != 0) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Client connected\n");

        char request[256] = {0};
        char response[256] = {0};

        recv(new_socket, request, sizeof(request), 0);
        printf("Received request: %s\n", request);

        char *token = strtok(request, " ");
        if (token != NULL) {
            if (strcmp(token, "ADD_USER") == 0) {
                char *username = strtok(NULL, " ");
                char *pointsStr = strtok(NULL, " ");
                int points = atoi(pointsStr);
                char *datetime = getCurrentDatetime();

                addUserToDatabase(conn, username, points, datetime);
                sprintf(response, "User added successfully");

                free(datetime);
            } else if (strcmp(token, "DELETE_USER") == 0) {
                char *username = strtok(NULL, " ");

                deleteUserFromDatabase(conn, username);
                sprintf(response, "User deleted successfully");
            } else if (strcmp(token, "SHOW_DB") == 0) {
                showDatabase(conn, new_socket);
            } else if (strcmp(token, "END_CONN") == 0) {
                sprintf(response, "Connection ended");
                send(new_socket, response, strlen(response), 0);
                close(new_socket);
                break;
            } else {
                sprintf(response, "Invalid request");
            }
        } else {
            sprintf(response, "Invalid request");
        }

        send(new_socket, response, strlen(response), 0);
        printf("Response sent: %s\n", response);
        printf("Client disconnected\n");
    }

    mysql_close(conn);

    return 0;
}
