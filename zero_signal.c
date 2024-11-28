#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ifaddrs.h>

#define PORT 8000
#define MAX_BUFFER_SIZE 1024

void start_server();
void join_as_client();
void handle_communication(int socket, char *client_name);
void send_message(int socket, char *client_name);
char* get_local_ip();

int main() {
    int choice;

    printf("Select an option:\n");
    printf("1. Create Server\n");
    printf("2. Join as Client\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    getchar();  // Consume the newline character left by scanf

    if (choice == 1) {
        start_server();
    } else if (choice == 2) {
        join_as_client();
    } else {
        printf("Invalid choice! Exiting...\n");
        exit(1);
    }

    return 0;
}

// Function to start the server
void start_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];
    char *server_ip = get_local_ip(); // Get the local IP address

    // Create the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }
    printf("Server socket created.\n");

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(1);
    }

    // Display server's local IP address and port
    printf("Server is listening on IP: %s, Port: %d...\n", server_ip, PORT);

    // Accept incoming client connection
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket == -1) {
        perror("Client accept failed");
        close(server_socket);
        exit(1);
    }

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("Client connected from %s\n", client_ip);

    // Get client's name
    char client_name[MAX_BUFFER_SIZE];
    printf("Enter your name (Client): ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = 0;  // Remove the newline character

    // Handle communication with the client
    handle_communication(client_socket, client_name);

    // Close server socket
    close(server_socket);
}

// Function for client to join the server
void join_as_client() {
    int client_socket;
    struct sockaddr_in server_addr;
    char server_ip[100];
    char buffer[MAX_BUFFER_SIZE];
    char client_name[MAX_BUFFER_SIZE];

    printf("Enter server IP address: ");
    fgets(server_ip, sizeof(server_ip), stdin);
    server_ip[strcspn(server_ip, "\n")] = 0; // Remove newline character

    // Create the socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        exit(1);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        close(client_socket);
        exit(1);
    }

    printf("Connected to server at %s\n", server_ip);

    // Get client's name
    printf("Enter your name (Client): ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = 0;  // Remove the newline character

    // Handle communication with the server
    handle_communication(client_socket, client_name);

    // Close client socket
    close(client_socket);
}

// Function to handle communication between server and client
void handle_communication(int socket, char *client_name) {
    char buffer[MAX_BUFFER_SIZE];
    int bytes_read;

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 1;  // Timeout for 1 second, to allow automatic checking for messages
    timeout.tv_usec = 0;

    // Loop for receiving and sending messages
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);

        int activity = select(socket + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("Select failed");
            break;
        }

        // If socket is ready to be read from
        if (FD_ISSET(socket, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            bytes_read = read(socket, buffer, sizeof(buffer) - 1);

            // Check if read is successful
            if (bytes_read == 0) {
                printf("Connection closed by the other party.\n");
                break;
            } else if (bytes_read == -1) {
                perror("Read failed");
                break;
            }

            // Only show non-empty messages
            if (strlen(buffer) > 0) {
                printf("\n%s: %s", client_name, buffer);
            }
        }

        // Send a message if the user has input
        send_message(socket, client_name);
    }
}

// Function to send a message to the other party
void send_message(int socket, char *client_name) {
    char buffer[MAX_BUFFER_SIZE];

    // Automatically wait for user input and send it
    printf("%s: ", client_name);
    fgets(buffer, sizeof(buffer), stdin);

    // Only send non-empty messages
    if (strlen(buffer) > 0 && buffer[0] != '\n') {
        write(socket, buffer, strlen(buffer));
    }
}
    
// Function to get the local IP address of the server
char* get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    char *ip = (char*)malloc(INET_ADDRSTRLEN);
    void *tmp_addr;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        exit(1);
    }

    // Loop through interfaces and find the first non-loopback IPv4 address
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            tmp_addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmp_addr, ip, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0) { // Skip loopback interface
                freeifaddrs(ifaddr);
                return ip;
            }
        }
    }

    freeifaddrs(ifaddr);
    return "127.0.0.1";  // Default to localhost if no IP is found
}

