#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <uv.h>

typedef struct sockaddr SA;
typedef struct sockaddr_in SA_in;

void handle_connection (int SOCKET);
char * create_request (char * command);
int send_request (int SOCKET, char * request);
int get_response (int SOCKET, char ** response);
char * login_request ();
char * signup_request ();
char * problem_set_request ();
char * send_p1_request ();
char * send_p2_request ();
char * send_p3_request ();
char * send_total_request ();

int PORT;           // Server port to connect
char IP_ADDRESS[32];  // Server address to connect
extern int errno;   // Error code


int main(int argc, char *argv[]) {

    // Check if there there are all the arguments at the command line
    if (argc != 3) {
        printf("[client]Not enough arguments. Syntax is:\n%s <server address> <port>\n", argv[0]);
        return -1;
    }


    // Declare variables for the socket and server
    int SOCKET;
    SA_in server;


    // Set port and ip address
    PORT = atoi(argv[2]);
    strcpy(IP_ADDRESS, argv[1]);


    // Create socket
    if ((SOCKET = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[client]Error creating socket");
        return errno;
    }


    // Fill in the structure used for the server
    bzero(&server, sizeof(SA_in));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    server.sin_port = htons (PORT);


    // Connect to the server
    if (connect (SOCKET, (SA *) &server,sizeof (SA)) == -1)
    {
        perror ("[client]Error connecting to server\n");
        return errno;
    }


    // Handle connection
    handle_connection(SOCKET);

    return 0;
}


void handle_connection(int SOCKET) {

    printf("Welcome to International Computer Science Olympiad\n");

    // Message buffers
    char * client_buffer = malloc(1024);
    char * server_buffer = malloc(1024);

    while (1) {

        // If user input is detected, write and read messages
        if (fgets(client_buffer, 1024, stdin)) {

            // Skip if client buffer is \n
            if (strcmp(client_buffer, "\n") == 0) {
                continue;
            }


            // Handle request
            char * request = create_request(client_buffer);
            send_request(SOCKET, request);
            get_response(SOCKET, &server_buffer);
            fflush(stdout);
            printf("%s\n", server_buffer);

            bzero(client_buffer, 1024);
            bzero(server_buffer, 1024);
        }
    }

}


char * create_request (char * command) {

    // Create variable request
    char * composed_request;

    // Treat different request types
    if (strcmp(command, "login\n") == 0) {

        // Login request
        composed_request = login_request();
        return composed_request;

    } else if (strcmp(command, "signup\n") == 0) {

        // Create user request
        composed_request = signup_request();
        return composed_request;

    } else if (strcmp(command, "problemset\n") == 0) {

        // Create request to get the problem set
        composed_request = problem_set_request();
        return composed_request;

    } else if (strcmp(command, "p1\n") == 0) {

        // Create request for sending the first problem
        composed_request = send_p1_request();
        return composed_request;

    } else if (strcmp(command, "p2\n") == 0) {

        // Create request for sending the first problem
        composed_request = send_p2_request();
        return composed_request;

    } else if (strcmp(command, "p3\n") == 0) {

        // Create request for sending the first problem
        composed_request = send_p3_request();
        return composed_request;

    } else if (strcmp(command, "total\n") == 0) {

        // Create request for getting the total number of points
        composed_request = send_total_request();
        return composed_request;

    } else {

        // Command error
        return "header:invalid";

    }

}


char * login_request () {

    // Username and password variables
    char user_name[256];
    char password[256];


    // Get username from user input
    printf("Introduceti usernameul: ");
    scanf("%s", user_name);

    // Get password from user input
    printf("Introduceti parola: ");
    scanf("%s", password);


    // Create request
    char * request = malloc(2048);

    strcpy(request, "header:login\n");
    strcat(request, "results={uname:");
    strcat(request, user_name);
    strcat(request, ",passwd:");
    strcat(request, password);
    strcat(request, "}");

    return request;
}


char * signup_request () {

    // Username and password variables
    char user_name[256];
    char password[256];


    // Get username from user input
    printf("Introduceti usernameul: ");
    scanf("%s", user_name);

    // Get password from user input
    printf("Introduceti parola: ");
    scanf("%s", password);


    // Create request
    char * request = malloc(2048);

    strcpy(request, "header:signup\n");
    strcat(request, "results={uname:");
    strcat(request, user_name);
    strcat(request, ",passwd:");
    strcat(request, password);
    strcat(request, "}");

    return request;
}


char * problem_set_request () {

    // Create request
    char * request = malloc(2048);
    strcpy(request, "header:problemset\n");

    return request;
}


char * send_p1_request() {

    // Get file path for the source file
    char file_path[256] = "/home/vlad/CLionProjects/Client/Solvings/";
    char file_name[64];

    // Get file name from user input
    printf("File name: ");
    scanf("%s", file_name);
    strcat(file_path, file_name);


    // Get source file code in a buffer
    FILE * source_code_file = fopen(file_path, "r");
    char source_code[4096] = "";
    char code_line[512];

    while (fgets(code_line, 512, source_code_file) != NULL) {
        strcat(source_code, code_line);
    }

    fclose(source_code_file);


    // Create request
    char * request = malloc(8192);
    strcpy(request, "header:p1\n");
    strcat(request, "code:\n");
    strcat(request, source_code);


    return request;
}


char * send_p2_request() {

    // Get file path for the source file
    char file_path[256] = "/home/vlad/CLionProjects/Client/Solvings/";
    char file_name[64];

    // Get file name from user input
    printf("File name: ");
    scanf("%s", file_name);
    strcat(file_path, file_name);


    // Get source file code in a buffer
    FILE * source_code_file = fopen(file_path, "r");
    char source_code[4096] = "";
    char code_line[512];

    while (fgets(code_line, 512, source_code_file) != NULL) {
        strcat(source_code, code_line);
    }

    fclose(source_code_file);


    // Create request
    char * request = malloc(8192);
    strcpy(request, "header:p2\n");
    strcat(request, "code:\n");
    strcat(request, source_code);


    return request;
}


char * send_p3_request() {

    // Get file path for the source file
    char file_path[256] = "/home/vlad/CLionProjects/Client/Solvings/";
    char file_name[64];

    // Get file name from user input
    printf("File name: ");
    scanf("%s", file_name);
    strcat(file_path, file_name);


    // Get source file code in a buffer
    FILE * source_code_file = fopen(file_path, "r");
    char source_code[4096] = "";
    char code_line[512];

    while (fgets(code_line, 512, source_code_file) != NULL) {
        strcat(source_code, code_line);
    }

    fclose(source_code_file);


    // Create request
    char * request = malloc(8192);
    strcpy(request, "header:p3\n");
    strcat(request, "code:\n");
    strcat(request, source_code);


    return request;
}


char * send_total_request () {

    // Create request
    char * request = malloc(2048);
    strcpy(request, "header:total\n");

    return request;
}


int send_request (int SOCKET, char * request) {

    size_t msg_len = strlen(request);

    if (write(SOCKET, &msg_len, sizeof(size_t)) < 0) {

        // Error sending message length
        return -1;

    } else {

        // Message length sent succesfully. Now send the message
        if (write(SOCKET, request, msg_len) < 0) {

            // Error sending message
            return -2;

        } else {

            // Message sent successfully
            return 0;

        }
    }
}


int get_response (int SOCKET, char ** response) {

    // Create variable for message length
    size_t message_length;

    if (read(SOCKET, &message_length, sizeof(size_t)) < 0) {

        // Error reading message length
        return -1;

    } else {

        // Message length successfully read. Now read the message
        if (read(SOCKET, *response, message_length)  < 0) {

            // Error reading message
            return -2;

        } else {

            // Message read successfully

            // If we have to recieve the problem set, we read the buffer forwards
            if (strcmp(*response, "Sending problem set\n") == 0) {

                // We loop 3 times because we wait for 3 files
                for (int i = 0; i < 3; i++) {

                    // Get problems text
                    char * problem_text = malloc(2048);
                    bzero(problem_text, 2048);
                    size_t sizeoffile;
                    read(SOCKET, &sizeoffile, sizeof(size_t));
                    read(SOCKET, problem_text, sizeoffile);

                    // Create path to file
                    char path[128] = "/home/vlad/CLionProjects/Client/Problemset/";
                    char file_name[6] = "p";
                    file_name[1] = (char)('0' + i + 1);
                    strcat(file_name, ".txt");
                    strcat(path, file_name);

                    // Create file
                    FILE * new_file = fopen(path, "w");
                    fclose(new_file);

                    // Save problem set locally
                    int text_file = open(path, O_WRONLY);
                    write(text_file, problem_text, strlen(problem_text));
                    close(text_file);
                }
            }
            return 0;

        }
    }
}