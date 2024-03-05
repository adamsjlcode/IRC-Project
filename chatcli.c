#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int sockfd;        // Socket file descriptor
char username[32]; // Username for login
char realname[32]; // Real name of the user
char password[32]; // Password for login

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
      sprintf(buffer, "%s: %s\n", username, message);
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
    memset(message, 0, BUFFER_SIZE);
    int receive = recv(sockfd, message, BUFFER_SIZE, 0);
    if (receive > 0) {
      message[receive] = '\0'; 
      printf("%s", message);
      str_overwrite_stdout();
    } else if (receive == 0) {
      break;
    } else {
      // Handle error
    }
  }
  return NULL;
}

// Function to parse command-line arguments
void parse_args(int argc, char *argv[], char *ip, int *port) {
  int opt;
  while ((opt = getopt(argc, argv, "u:r:p:a:")) != -1) {
    switch (opt) {
    case 'u': // Username
      strncpy(username, optarg, 31);
      username[31] = '\0';
      break;
    case 'r': // Real name
      strncpy(realname, optarg, 31);
      realname[31] = '\0';
      break;
    case 'p': // Password
      strncpy(password, optarg, 31);
      password[31] = '\0';
      break;
    case 'a': // Address and port in format address:port
      sscanf(optarg, "%14[^:]:%d", ip, port);
      break;
    default:
      fprintf(stderr,
              "Usage: %s -u username -r realname -p password -a address:port\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char **argv) {
  char ip[15] = "";
  int port;

  parse_args(argc, argv, ip, &port);

  // Register signal handler for Ctrl+C
  signal(SIGINT, catch_ctrl_c_and_exit);

  // Create socket and set up connection
  struct sockaddr_in server_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  // Connect to the server
  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
      perror("ERROR: Failed to connect to the server");
      close(sockfd);  // Close the socket before exiting
      return EXIT_FAILURE;
  }

  // Send login details (username, real name, password) to the server
  char login_details[128];
  sprintf(login_details, "%s %s %s", username, realname, password);
  send(sockfd, login_details, strlen(login_details), 0);

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
