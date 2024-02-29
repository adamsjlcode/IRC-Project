
# IRC Project

## CSCI 3160

### The IRC application shall consist of a client application and a server application. Clients shall connect to the server—there may be many clients connected to one server. At a minimum, the IRC server shall

* Start by accepting a port number for the listening service via command line argument,
* Accept incoming client connections via POSIX (Berkeley) sockets for an indeterminate amount of time,
* Map a single task on the server to a single connected client in order to manage a client session,
* Receive messages from the client,
* Multicast received messages to other server tasks, and
* Transmit messages from other clients to the connected client.

### The server should complete all of these functionalities with high quality-of-service (i.e., low-latency message handling, high server availability, high server uptime, and clean implementation—no resource leaks). At a minimum, the IRC client shall

* Start by accepting an IP address and a port number for the server’s listening service,
* Accept input messages from the user,
* Transmit messages to the server’s assigned session task, and
* Receive and display messages from the server’s assigned session task.
* A client may disconnect from the IRC service at any time by having the user input “quit” as a message. The client shall close the connection and exit; the server task, on detection of the closed connection, shall exit

This project outlines the creation of a lightweight Internet Relay Chat (IRC) application using a client-server architecture in C. Here's a breakdown of the key components and potential enhancements:

### Server Application

1. **Initialization**: Accepts a port number via command line argument for its listening service.
2. **Connection Management**: Utilizes POSIX sockets to handle multiple client connections indefinitely.
3. **Client Session Management**: Assigns a single task on the server for each connected client.
4. **Message Handling**:
    * Receives messages from clients.
    * Multicasts received messages to other server tasks.
    * Transmits messages from other clients to the connected client.
5. **Quality of Service**:
    * Low-latency message handling.
    * High server availability and uptime.
    * Clean implementation without resource leaks.

### Client Application

1. **Initialization**: Accepts the server's IP address and port number.
2. **Message Handling**:
    * Accepts user input messages.
    * Transmits messages to the server.
    * Receives and displays messages from the server.
3. **Disconnection**: Allows user to disconnect by inputting "quit", closes the connection, and exits.

### Presentation

* Introduce the team and describe the work completed.
* Demonstrate the IRC application with three clients connected.
* Conduct a project retrospective: roles, successes, challenges, and future improvements.
* Reflect on the Computer Systems course and suggest improvements.

### Exceeding Expectations

1. **Enhanced Client Task Handling**: Separate tasks for message reception/display and user input transmission.
2. **Graphical User Interface (GUI)**: Develop a GUI for the client, using C code as a library integrated into a managed language or web application.
3. **Improved Textual User Interface (TUI)**: Implement features like timestamps, usernames, emojis, and multiple client panes.
4. **Registration and Authentication**: Add workflows for client registration and authentication upon connection.
5. **Conversation History**: Server maintains a history of prior conversations, extending the account concept.

### Development Suggestions

* **Modular Design**: Keep the code modular for easy maintenance and upgrades.
* **Testing**: Implement thorough testing for each component to ensure stability and reliability.
* **Documentation**: Maintain clear and comprehensive documentation for future development and usage.
* **Peer Review**: Regular code reviews within the team to ensure code quality and collaborative learning.

This project will require a strong understanding of network programming in C, multi-threading, and possibly GUI development for the advanced features. It's an excellent opportunity to apply and showcase the skills learned in operating system API and communication labs.
