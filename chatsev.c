#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/getopt_core.h>

#define MAX_CLIENTS 10  // Maximum number of clients the server can handle concurrently
#define BUFFER_SIZE 1024  // Size of the buffer used for sending and receiving messages
#define VERBOSE 0  // Verbose mode flag (0 for off, 1 for on)
int listenfd = 0;  // Global variable for the listening socket
// Structure to represent a client
typedef struct {
    struct sockaddr_in address;  // Client's address
    int sockfd;  // Socket file descriptor for the client
    int uid;  // Unique identifier for the client
    char username[32];  // Client's username
} client_t;

client_t *clients[MAX_CLIENTS];  // Array of pointers to clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for synchronizing access to clients array

// Function Prototypes
void add_client(client_t *cl);
void remove_client(int uid);
void send_message(char *s, int uid);
void *handle_client(void *arg);
void execute_command(client_t *cli, char *cmd);
int is_command(const char *message);
void getTimeStamp(char *timestamp);
void handle_sigint(int sig);
void shutdown_server();

// Add Client to the server
void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);  // Locking the mutex for exclusive access to clients array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Unlocking the mutex
}

// Remove Client from the server
void remove_client(int uid) {
    pthread_mutex_lock(&clients_mutex);  // Locking the mutex for exclusive access to clients array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid == uid) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Unlocking the mutex
}
// Signal handler for SIGINT and SIGTERM
void handle_sigint(int sig) {
    printf("Shutting down server...\n");
    shutdown_server();
    exit(EXIT_SUCCESS);
}

// Function to close all client connections and the listening socket
void shutdown_server() {
    char *shutdown_msg = "Server is shutting down.\n";

    // Lock the mutex to safely modify the clients array
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            // Inform each client that the server is shutting down
            send(clients[i]->sockfd, shutdown_msg, strlen(shutdown_msg), 0);

            // Close the client socket
            close(clients[i]->sockfd);

            // Free the memory allocated for the client
            free(clients[i]);

            // Set the client pointer to NULL
            clients[i] = NULL;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // Close the listening socket
    close(listenfd);
}

// Send message to all clients except the sender
void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);  // Locking the mutex for exclusive access to clients array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && clients[i]->uid != uid) {
            write(clients[i]->sockfd, s, strlen(s));
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Unlocking the mutex
}

// Thread function to handle all communication with a client
void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int length = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
        if (length <= 0) {
            break; // Client disconnected
        }

        buffer[length] = '\0';
        if (is_command(buffer)) {
            execute_command(cli, buffer);
        } else {
            send_message(buffer, cli->uid); // Broadcast message
        }

        memset(buffer, 0, BUFFER_SIZE);
    }

    close(cli->sockfd);
    remove_client(cli->uid);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

// Check if a message is a command
int is_command(const char *message) {
    return message[0] == '/';
}

// Get current time stamp in the format YYYY-MM-DD HH:MM:SS
void getTimeStamp(char *timestamp) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Execute command received from a client
void execute_command(client_t *cli, char *cmd) {
    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, "Command '%s' used by %s\n", cmd, cli->username);
    printf("%s", log_msg);  // Logging the command and user who executed it

    // Basic commands implementation
    if (strcmp(cmd, "/help") == 0) {
        char help_msg[BUFFER_SIZE];
        snprintf(help_msg, BUFFER_SIZE, "\nHelp commands:\n""/help - Show this help message\n""/exit - Disconnect from the chat\n");
        // Log the command usage
        char log_msg[BUFFER_SIZE];
        getTimeStamp(log_msg);
        snprintf(log_msg + strlen(log_msg), BUFFER_SIZE - strlen(log_msg)," - %s used /help command\n", cli->username);
        printf("%s", log_msg);
        write(cli->sockfd, help_msg, strlen(help_msg));
    } else if (strcmp(cmd, "/list") == 0) {
        // List clients implementation
    } else if (strcmp(cmd, "/exit") == 0) {
        // Disconnect the client
    } else {
        char *unknown_cmd_msg = "Unknown command.\\n";
        write(cli->sockfd, unknown_cmd_msg, strlen(unknown_cmd_msg));
    }
}

// Main server function
int main(int argc, char **argv) {
    int port = 0;
    int max_clients = MAX_CLIENTS;
    int verbose = VERBOSE;
    int opt;
    char timestamp[20];

    // Process command-line arguments using getopt
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p <port> [options]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Process optional arguments for max clients and verbose mode
    for (int i = optind; i < argc; i++) {
        if (strcmp(argv[i], "--max-clients") == 0 && i + 1 < argc) {
            max_clients = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        }
    }

    if (port == 0) {
        fprintf(stderr, "Port must be specified with -p option\n");
        return EXIT_FAILURE;
    }

    printf("Starting server on port %d\n", port);
    printf("Max clients: %d\n", max_clients);
    if (verbose) {
        printf("Verbose mode enabled\n");
    }

    int connfd = 0;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t tid;
    char cli_ip[INET_ADDRSTRLEN]; // Buffer to store the string form of the client's IP
    socklen_t cli_len = sizeof(cli_addr);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE to prevent termination when writing to a closed socket

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    if (listen(listenfd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("<[ SERVER STARTED ]>\n");

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
        if (connfd < 0) {
            perror("ERROR: Accepting connection failed");
            continue;
        }

        // Convert IP addresses from binary to text form
        inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_ip, INET_ADDRSTRLEN);
        printf("Connection accepted from %s:%d\n", cli_ip, ntohs(cli_addr.sin_port));

        // Allocate memory for a new client and set its attributes
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        if (!cli) {
            perror("ERROR: Unable to allocate memory for new client");
            continue;
        }
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = connfd;  // Using connfd as a simple UID, but should be unique per connection

        // Add new client to the clients array
        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);  // Create a thread to handle the new client

        getTimeStamp(timestamp);  // Get the current timestamp
        printf("%s: New client connected.\n", timestamp);
    }

    // Close the listening socket and cleanup
    close(listenfd);

    return EXIT_SUCCESS;
}