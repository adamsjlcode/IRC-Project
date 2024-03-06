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

#define ANSI_STYLE_BOLD   "\e[1m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"



int sockfd;        // Socket file descriptor
char username[32]; // Username for login
// char realname[32]; // Real name of the user
// char password[32]; // Password for login

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

//Startup Usage Menu
void print_usage(char *program_name) {
    fprintf(stderr,
        // "Usage: %s -u username -r realname -p password -a address:port\n"
        "Usage: %s -u username -a address:port\n"
        "  -u  Set the username for the login\n"
        // "  -r  Set the real name of the user\n"
        // "  -p  Set the password for the login\n"
        "  -a  Set the IP address and port of the server in the format address:port\n",
        program_name);
}

// Thread function to handle sending messages
void *send_msg_handler(void *arg) {
  char message[BUFFER_SIZE] = {};
  char buffer[BUFFER_SIZE + 32] = {};

  while (1) {
    str_overwrite_stdout();
    fgets(message, BUFFER_SIZE, stdin);
    str_trim_lf(message, BUFFER_SIZE);

    if (strcmp(message, "/exit") == 0) {
      break;
    } else {
      sprintf(buffer, ANSI_STYLE_BOLD ANSI_COLOR_GREEN "%s" ANSI_RESET ": %s\n", username, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

    bzero(message, BUFFER_SIZE);
    bzero(buffer, BUFFER_SIZE + 32);
  }

  catch_ctrl_c_and_exit(2);
  return NULL;
}

// Timestamp function
void getTimeStamp(char *timestamp) {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Thread function to handle receiving messages
void *recv_msg_handler(void *arg) {
  char message[BUFFER_SIZE] = {};
  while (1) {
    memset(message, 0, BUFFER_SIZE);
    int receive = recv(sockfd, message, BUFFER_SIZE, 0);
    char timestamp[20];
    
    if (receive > 0) {
      message[receive] = '\0';  // Null-terminate the message
      getTimeStamp(timestamp);  // Get the current timestamp
      
      // Check if the message is a server shutdown notice
      if (strcmp(message, "Server is shutting down.\n") == 0) {
        printf("%s - Server is shutting down. Exiting...\n", timestamp);
        exit(EXIT_SUCCESS);  // Exit client program
      }
      
      printf("%s - %s", timestamp, message);  // Print the timestamp and message
      str_overwrite_stdout();  // Overwrite the stdout
    } else if (receive == 0) {
      printf("Server connection closed. Exiting...\n");
      exit(EXIT_SUCCESS);  // Exit client program
    } else {
      perror("recv failed");  // Print the receive error
      exit(EXIT_FAILURE);  // Exit client program due to error
    }
  }
  return NULL;
}

// Function to parse command-line arguments
void parse_args(int argc, char *argv[], char *ip, int *port) {
  if (argc < 2) { // Check if all required parameters are provided
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
  }
  int opt;
  // while ((opt = getopt(argc, argv, "u:r:p:a:")) != -1) {
    while ((opt = getopt(argc, argv, "u:a:")) != -1) {
    switch (opt) {
    case 'u': // Username
      strncpy(username, optarg, 31);
      username[31] = '\0';
      break;
    // case 'r': // Real name
    //   strncpy(realname, optarg, 31);
    //   realname[31] = '\0';
    //   break;
    // case 'p': // Password
    //   strncpy(password, optarg, 31);
    //   password[31] = '\0';
    //   break;
    case 'a': // Address and port in format address:port
      sscanf(optarg, "%14[^:]:%d", ip, port);
      break;
    default:
      fprintf(stderr,
              // "Usage: %s -u username -r realname -p password -a address:port\n",
              // argv[0]);
              "Usage: %s -u username -a address:port\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char **argv) {
  char ip[15] = "";
  int port = -1;

  parse_args(argc, argv, ip, &port);

  if (port == -1) {
        fprintf(stderr, "ERROR: Port not provided or invalid.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
  }
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
    switch (errno) {
        case ECONNREFUSED:
            fprintf(stderr, "ERROR: The connection was refused - is the server running on the specified port?\n");
            break;
        case ETIMEDOUT:
            fprintf(stderr, "ERROR: The connection timed out - check the server IP and port.\n");
            break;
        case ENETUNREACH:
            fprintf(stderr, "ERROR: The network is unreachable - check your network connection.\n");
            break;
        case EADDRNOTAVAIL:
            fprintf(stderr, "ERROR: The requested address is not valid in this context - are you using the correct IP?\n");
            break;
        default:
            perror("ERROR: Failed to connect to the server");
            break;
    }
    close(sockfd);  // Close the socket before exiting
    return EXIT_FAILURE;
  }

  // Send login details (username, real name, password) to the server
  char login_details[128];
  // sprintf(login_details, "%s %s %s", username, realname, password);
  sprintf(login_details, "%s", username);
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
