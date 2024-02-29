#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024 // Define the maximum size of messages

int sockfd;    // Socket file descriptor
char name[32]; // Client name

// Function to overwrite the current line in stdout
void str_overwrite_stdout() {
  printf("\r%s", "> ");
  fflush(stdout);
}

// Function to trim the newline character from strings
void str_trim_lf(char *arr, int length) {
  for (int i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

// Function to handle Ctrl+C signal
void catch_ctrl_c_and_exit(int sig) {
  // Handle Ctrl+C here, clean up and close socket
  printf("\nExiting...\n");
  close(sockfd);
  exit(EXIT_SUCCESS);
}

// Thread function to handle sending messages
void *send_msg_handler(void *arg) {
  char message[BUFFER_SIZE] = {};
  char buffer[BUFFER_SIZE + 32] = {};

  while (1) {
    str_overwrite_stdout();
    fgets(message, BUFFER_SIZE, stdin);
    str_trim_lf(message, BUFFER_SIZE);

    if (strcmp(message, "exit") == 0) {
      break;
    } else {
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

    bzero(message, BUFFER_SIZE);
    bzero(buffer, BUFFER_SIZE + 32);
  }

  catch_ctrl_c_and_exit(2);
  return NULL;
}

// Thread function to handle receiving messages
void *recv_msg_handler(void *arg) {
  char message[BUFFER_SIZE] = {};
  while (1) {
    int receive = recv(sockfd, message, BUFFER_SIZE, 0);
    if (receive > 0) {
      printf("%s", message);
      str_overwrite_stdout();
    } else if (receive == 0) {
      break;
    } else {
      // Handle errors
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s <IP> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = argv[1];       // Server IP address
  int port = atoi(argv[2]); // Server port

  // Register signal handler for Ctrl+C
  signal(SIGINT, catch_ctrl_c_and_exit);

  // Prompt user for their name
  printf("Please enter your name: ");
  fgets(name, 32, stdin);
  str_trim_lf(name, strlen(name));

  if (strlen(name) > 32 || strlen(name) < 2) {
    printf("Name must be less than 32 and more than 2 characters.\n");
    return EXIT_FAILURE;
  }

  // Create socket and set up connection
  struct sockaddr_in server_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  // Connect to the server
  int err =
      connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
    printf("ERROR: connect\n");
    return EXIT_FAILURE;
  }

  // Send client's name to the server
  send(sockfd, name, 32, 0);

  printf("=== WELCOME TO THE CHATROOM ===\n");

  // Create send and receive threads
  pthread_t send_msg_thread;
  if (pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL) != 0) {
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, recv_msg_handler, NULL) != 0) {
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  // Main loop to check for socket disconnection
  while (1) {
    if (sockfd == -1) {
      printf("\nDisconnected from server.\n");
      break;
    }
  }

  // Cleanup before exiting
  close(sockfd);

  return EXIT_SUCCESS;
}
