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

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define VERBOSE 0

// Structure to represent a client
typedef struct {
  struct sockaddr_in address;
  int sockfd;
  int uid;
  char username[32];
  // char realname[32];
  // char password[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *cl);
void remove_client(int uid);
void send_message(char *s, int uid);

void *handle_client(void *arg);

void execute_command(client_t *cli, char *cmd);
int is_command(const char *message);

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char username[32];

    // Receive the username first
    int length = recv(cli->sockfd, username, 32, 0);
    if (length > 0) {
        // Null-terminate the username and copy it to the client structure
        username[length] = '\0'; // Ensure null-termination
        strncpy(cli->username, username, 31);
        cli->username[31] = '\0'; // Ensure the username is not too long

        // Now that we have the username, we can add the client
        add_client(cli);
    } else {
        perror("Couldn't receive username from client");
        close(cli->sockfd);
        free(cli);
        return NULL;
    }
    while (1) {
        length = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
        if (length <= 0) {
            break; // Client disconnected
        }

        buffer[length] = '\0';
        char *command = buffer;
        // Check if message contains a colon, indicating it might be a "username: message" format
        char *colon = strchr(buffer, ':');
        if (colon) {
            // Move the pointer past the colon and white space
            command = colon + 2;
        }

        if (is_command(command)) {
            execute_command(cli, command);
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

void add_client(client_t *cl) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (!clients[i]) {
      clients[i] = cl;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] && clients[i]->uid == uid) {
      clients[i] = NULL;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void send_message(char *s, int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] && clients[i]->uid != uid) {
      write(clients[i]->sockfd, s, strlen(s));
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Timestamp function
void getTimeStamp(char *timestamp) {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void execute_command(client_t *cli, char *cmd) {
    printf("execute_command received: %s\n", cmd); // Debug statement
    char *token = strtok(cmd, " ");

    if (strcmp(token, "/list") == 0) {
        // Initialize userlist with a message indicating the start of the user list.
        char userlist[BUFFER_SIZE] = "Connected users:\n";
        int ul_length = strlen(userlist);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i]) {
                // Make sure that we have a proper null-terminated string for the username.
                clients[i]->username[31] = '\0'; // Ensure null-termination
                int needed_space = strlen(clients[i]->username) + 2; // Include space for newline and null-terminator.
                if (ul_length + needed_space < BUFFER_SIZE) {
                    strcat(userlist, clients[i]->username);
                    strcat(userlist, "\n");
                    ul_length += needed_space; // Increase the length of the current user list.
                } else {
                    // If we don't have enough space, inform about it.
                    printf("User list exceeded buffer size.\n");
                    break; // Exit the loop to avoid buffer overflow.
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Send the user list only if we actually added any users to the list.
        if (ul_length > strlen("Connected users:\n")) {
            if (write(cli->sockfd, userlist, ul_length) < 0) {
                perror("Writing user list to client failed");
            }
        } else {
            // If no users are connected, send a different message.
            char *no_users_msg = "No users connected.\n";
            if (write(cli->sockfd, no_users_msg, strlen(no_users_msg)) < 0) {
                perror("Writing no users message to client failed");
            }
        }

        printf("Sent user list to %s\n", cli->username); // Debug statement
    } else if (strcmp(token, "/whisper") == 0) {
        // Handle whisper command
    }
    // Additional command handling can be implemented here.
}

int is_command(const char *message) {
    int isCmd = message[0] == '/';
    printf("is_command: %s -> %d\n", message, isCmd); // Debug statement
    return isCmd;
}

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

  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t tid;
  char cli_ip[INET_ADDRSTRLEN]; // Buffer to store the string form of the client's IP
  socklen_t cli_len = sizeof(cli_addr);

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  signal(SIGPIPE, SIG_IGN);

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

      // Convert IP addresses from binary to text
      inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_ip, INET_ADDRSTRLEN);
      printf("Connection accepted from %s:%d\n", cli_ip, ntohs(cli_addr.sin_port));

      client_t *cli = (client_t *)malloc(sizeof(client_t));
      cli->address = cli_addr;
      cli->sockfd = connfd;
      cli->uid = connfd; // Using connfd as a simple UID, but should be unique per connection

      pthread_create(&tid, NULL, &handle_client, (void *)cli);

      getTimeStamp(timestamp);
	    printf("%s\n", timestamp);
  }

  // Close the listening socket
  close(listenfd);

  return EXIT_SUCCESS;
}
