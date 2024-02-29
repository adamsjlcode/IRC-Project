// Including necessary libraries
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10 // Define the maximum number of clients that can connect
#define BUFFER_SIZE 1024 // Define the maximum size of messages

// Structure to represent a client
typedef struct {
  struct sockaddr_in address; // Client's address
  int sockfd;                 // Socket file descriptor for client
  int uid;                    // Unique identifier for the client
  char name[32];              // Client's name
} client_t;

client_t *clients[MAX_CLIENTS]; // Array to hold all clients
pthread_mutex_t clients_mutex =
    PTHREAD_MUTEX_INITIALIZER; // Mutex for handling clients array
int uid = 10; // Global variable for unique identifier for each client

// Function to trim newline character from strings
void str_trim_lf(char *arr, int length) {
  for (int i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

// Add client to the clients queue
void queue_add(client_t *cl) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (!clients[i]) {
      clients[i] = cl;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Remove client from the queue
void queue_remove(int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i]) {
      if (clients[i]->uid == uid) {
        clients[i] = NULL;
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Send message to all clients except the sender
void send_message(char *s, int uid) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i]) {
      if (clients[i]->uid != uid) {
        if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
          perror("ERROR: write to descriptor failed");
          break;
        }
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// Function to handle communication with the client
void *handle_client(void *arg) {
  char buffer[BUFFER_SIZE];
  int leave_flag = 0;

  client_t *cli = (client_t *)arg;

  // Receive name from client
  if (recv(cli->sockfd, cli->name, 32, 0) <= 0 || strlen(cli->name) < 2 ||
      strlen(cli->name) >= 32 - 1) {
    printf("Didn't enter the name.\n");
    leave_flag = 1;
  } else {
    sprintf(buffer, "%s has joined\n", cli->name);
    printf("%s", buffer);
    send_message(buffer, cli->uid);
  }

  bzero(buffer, BUFFER_SIZE);

  while (1) {
    if (leave_flag) {
      break;
    }

    int receive = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
    if (receive > 0) {
      if (strlen(buffer) > 0) {
        send_message(buffer, cli->uid);
        str_trim_lf(buffer, strlen(buffer));
        printf("%s\n", buffer);
      }
    } else if (receive == 0 || strcmp(buffer, "exit") == 0) {
      sprintf(buffer, "%s has left\n", cli->name);
      printf("%s", buffer);
      send_message(buffer, cli->uid);
      leave_flag = 1;
    } else {
      perror("ERROR: -1");
      leave_flag = 1;
    }

    bzero(buffer, BUFFER_SIZE);
  }

  // Close the socket and remove client from queue
  close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  pthread_detach(pthread_self());

  return NULL;
}

// Main function to set up the server
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";   // IP on which the server will run
  int port = atoi(argv[1]); // Port on which the server will run
  int option = 1;
  int listenfd = 0, connfd = 0; // Listening and connection file descriptors
  struct sockaddr_in serv_addr; // Server address structure
  pthread_t tid;                // Thread ID

  // Create a socket for the server
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  // Ignore SIGPIPE to prevent crashes on writes to a closed socket
  signal(SIGPIPE, SIG_IGN);

  // Allow socket descriptor to be reusable
  if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT),
                 (char *)&option, sizeof(option)) < 0) {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  // Bind the socket to the server address
  if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Socket binding failed");
    return EXIT_FAILURE;
  }

  // Listen for client connections
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
  }

  printf("<[ SERVER STARTED ]>\n");

  // Main loop to accept client connections
  while (1) {
    socklen_t clilen = sizeof(serv_addr);
    connfd = accept(listenfd, (struct sockaddr *)&serv_addr, &clilen);

    // Allocate memory for a new client
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli->address = serv_addr;
    cli->sockfd = connfd;
    cli->uid = uid++;

    // Add client to the queue and create a thread to handle communication
    queue_add(cli);
    pthread_create(&tid, NULL, &handle_client, (void *)cli);

    // Reduce CPU usage
    sleep(1);
  }

  return EXIT_SUCCESS;
}
