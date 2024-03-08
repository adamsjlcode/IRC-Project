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
#include <limits.h> // For INT_MAX

#define MAX_CLIENTS 10  // Maximum number of clients the server can handle concurrently
#define BUFFER_SIZE 1024  // Size of the buffer used for sending and receiving messages
#define VERBOSE 0  // Verbose mode flag (0 for off, 1 for on)
#define MAX_THREADS 10 // One thread per client

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
pthread_t *thread_ids = NULL; // Pointer for dynamic allocation

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
    // Join all non-null threads
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_ids[i] != 0) {
            pthread_join(thread_ids[i], NULL);
            thread_ids[i] = 0; // Clear the thread ID
        }
    }
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
            printf("Client %s disconnected.\n", cli->username);
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
    // Clear the thread ID
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_equal(pthread_self(), thread_ids[i])) {
            thread_ids[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    pthread_exit(NULL);
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
    char *token = strtok(cmd, " ");
    snprintf(log_msg, sizeof(log_msg), "Command '%s' used by %s\n", cmd, cli->username);
    printf("%s", log_msg); // Logging the command and user who executed it

    if (strcmp(cmd, "/help") == 0) {
        // Help command implementation
        char help_msg[BUFFER_SIZE];
        snprintf(help_msg, sizeof(help_msg), "\nHelp commands:\n/help - Show this help message\n/list - List connected users\n/exit - Disconnect from the chat\n");
        write(cli->sockfd, help_msg, strlen(help_msg));
    } else if (strcmp(cmd, "/list") == 0) {
        char *userlist = malloc(BUFFER_SIZE);
        if (!userlist) {
            perror("Memory allocation for userlist failed");
            return;
        }
        strcpy(userlist, "Connected users:\n");

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i]) {
                clients[i]->username[31] = '\0'; // Ensure null-termination
                char *newline_username = malloc(strlen(clients[i]->username) + 2);
                if (!newline_username) {
                    perror("Memory allocation for newline_username failed");
                    continue;
                }
                sprintf(newline_username, "%s\n", clients[i]->username);

                char *temp = realloc(userlist, strlen(userlist) + strlen(newline_username) + 1);
                if (!temp) {
                    perror("Memory reallocation for userlist failed");
                    free(newline_username);
                    break;
                }
                userlist = temp;
                strcat(userlist, newline_username);
                free(newline_username);
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (write(cli->sockfd, userlist, strlen(userlist)) < 0) {
            perror("Writing user list to client failed");
        }
        free(userlist);
        printf("Sent user list to %s\n", cli->username); // Debug statement
    } else if (strcmp(token, "/whisper") == 0) {
        char *recipient_username = strtok(NULL, " "); // Get the recipient username
        if (recipient_username == NULL) {
            char *msg = "Whisper command format: /whisper <username> <message>\n";
            write(cli->sockfd, msg, strlen(msg));
            return;
        }

        char *whisper_message = strtok(NULL, ""); // Get the rest of the command as the message
        if (whisper_message == NULL) {
            char *msg = "No message specified.\n";
            write(cli->sockfd, msg, strlen(msg));
            return;

        }

        // Construct the whisper message to be sent
        char message_to_send[BUFFER_SIZE];
        snprintf(message_to_send, BUFFER_SIZE, "[Whisper from %s]: %s\n", cli->username, whisper_message);

        // Lock the clients mutex before accessing the clients array
        pthread_mutex_lock(&clients_mutex);
        int recipient_found = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] && strcmp(clients[i]->username, recipient_username) == 0) {
                // Found the recipient, send the message
                if (write(clients[i]->sockfd, message_to_send, strlen(message_to_send)) < 0) {
                    perror("Sending whisper failed");
                } else {
                    recipient_found = 1;
                }
                break; // Stop searching after finding the recipient
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Inform the sender if the recipient was not found
        if (!recipient_found) {
            char *msg = "Recipient not found.\n";
            write(cli->sockfd, msg, strlen(msg));
        }
    } else if (strcmp(cmd, "/exit") == 0) {
        // Exit command implementation
        // Code to disconnect the client
        close(cli->sockfd);
        remove_client(cli->uid);
        free(cli);
    } else {
        // Unknown command implementation
        char *unknown_cmd_msg = "Unknown command.\n";
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
    char *endptr;
    long val;

    // Process command-line arguments using getopt
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                val = strtol(optarg, &endptr, 10);
                if (endptr == optarg || *endptr != '\0' || val <= 0 || val > 65535) {
                    fprintf(stderr, "Invalid port number.\n");
                    return EXIT_FAILURE;
                }
                port = (int) val;
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
    printf("Ready to accept client connections...\n");
    if (verbose) {
        printf("Verbose mode enabled\n");
    }
    // Ensure the system can handle the number of clients
    if (max_clients <= 0 || max_clients > INT_MAX / sizeof(pthread_t)) {
        fprintf(stderr, "Invalid or too large number of max clients.\n");
        return EXIT_FAILURE;
    }

    // Allocate memory for thread_ids based on max_clients
    thread_ids = malloc(max_clients * sizeof(pthread_t));
    if (!thread_ids) {
        perror("Failed to allocate memory for thread IDs");
        return EXIT_FAILURE;
    }
    memset(thread_ids, 0, max_clients * sizeof(pthread_t));
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
        // Save the thread ID
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_THREADS; ++i) {
            if (thread_ids[i] == 0) {
                thread_ids[i] = tid;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    // Close the listening socket and cleanup
    close(listenfd);
    // Free the thread_ids before exiting
    free(thread_ids);
    return EXIT_SUCCESS;
}