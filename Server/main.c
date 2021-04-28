#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include "queue.h"
#include "database.h"
#include "helpfunc.h"

#define PORT 2024
#define SERVER_BACKLOG 100
#define THREAD_POOL_SIZE 10
#define LOGIN_REQUEST 1
#define SIGNUP_REQUEST 2
#define PROBLEM_SET_REQUEST 3
#define P1_REQUEST 4
#define P2_REQUEST 5
#define P3_REQUEST 6
#define TOTAL_REQUEST 7
#define INVALID_REQUEST -1
#define REQUEST_SIZE 8192

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

typedef struct sockaddr SA;
typedef struct sockaddr_in SA_in;

void * handle_connection (void *socket);
void * thread_function (void *args);
char * handle_request (char * client_request);
int get_request (int SOCKET, char ** result, int * logged_in);
int get_request_type (char * request);
char * create_response (char * request, int * logged_in, int * p1_solution_sent, int * p2_solution_sent, int * p3_solution_sent);
char * login_response (char * request, int * logged_in);
char * signup_response (char * request);
char * problem_set_response (char * request);
char * p1_response (char * request, int uid, int * solution_sent);
char * p2_response (char * request, int uid, int * solution_sent);
char * p3_response (char * request, int uid, int * solution_sent);
char * total_response(char * request, int p1_points, int p2_points, int p3_points);
char * invalid_request_response ();
char * create_source_file (char * request, int uid, const char problem_number[3]);
char * create_executable (int uid, const char problem_number[3], char * source_file_path);
void execute_source_code (int uid, const char problem_number[2], char * executable_path);
int get_solution_result (int uid, const char problem_number[2]);
int remove_file (char * file_path);
int write_response (int SOCKET, char * response, int * problems_sent);

extern int errno;
sqlite3 * DB; // Database object


int main() {

    // Define structures for server and client
    SA_in server;
    SA_in client;

    // Socket file descriptor
    int SOCKET;


    // Connect to database
    if (sqlite3_open(DB_PATH, &DB) == SQLITE_OK) {
        printf("[server]Connected to database\n");
    } else {
        perror("[server]Error occured while connecting to database");
        return errno;
    }


    // Initialize the thread pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }


    // Attemp to create the socket
    if ((SOCKET = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Error creating socket");
        return errno;
    }


    // Prepare client and server structures
    bzero(&server, sizeof(SA_in));
    bzero(&client, sizeof(SA_in));


    // Fill in server structure
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);


    // Bind socket
    if (bind(SOCKET, (SA *) &server, sizeof(server)) == -1) {
        perror("[server]Error binding");
        return errno;
    }


    // Listen to connections
    if (listen(SOCKET, SERVER_BACKLOG) == -1) {
        perror("[server]Error listening");
        return errno;
    }


    // Serving clients
    while (1) {
        int client_socket;
        int length = sizeof(client);
        printf("[server]Waiting for connection on port %d...\n", PORT);


        // Accepting connections and verifing for errors
        client_socket = accept(SOCKET, (SA *) &client, &length);
        if (client_socket < 0) {
            perror("[server]Error accepting connection");
        }


        // Handle the connection
        int *new_client = malloc(sizeof(int));
        *new_client = client_socket;

        // Locking queue push to avoid race condition
        pthread_mutex_lock(&mutex);
        push(new_client);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&condition_var);
    }

}


void * thread_function(void * arg) {

    while(1) {
        // Verify if there are clients in the queue
        // If there are clients, handle their requests

        // Locking queue pop to avoid race condition
        int * queue_client;
        pthread_mutex_lock(&mutex);
        if ((queue_client = pop()) == NULL) {
            pthread_cond_wait(&condition_var, &mutex);
            queue_client = pop();
        }
        pthread_mutex_unlock(&mutex);

        if (queue_client != NULL) {
            handle_connection(queue_client);
        }
    }
}


void * handle_connection(void *p_client_socket) {

    // Variable that tracks if the user is logged in
    int logged_in = 0;
    // Variable that tracks if the problem set is sent
    int problems_sent = 0;
    // Variable that tracks sent problems
    int p1_solution_sent = 0;
    int p2_solution_sent = 0;
    int p3_solution_sent = 0;


    // Socket
    int client_socket = *((int*)p_client_socket);

    // Mesages buffers
    char * client_buffer = malloc(REQUEST_SIZE);
    char * server_buffer = malloc(REQUEST_SIZE);


    // Waiting and reading mesage
    printf("[server]Connection established. Waiting for message...\n");

    while (1) {
        bzero(client_buffer, REQUEST_SIZE);
        bzero(server_buffer, REQUEST_SIZE);


        // Get the requests and treat them
        if (get_request(client_socket, &client_buffer, &logged_in) < 0) {
            close(client_socket);
            printf("\nClient disconnected\n");
            return NULL;
        }


        // Create a response
        char * response = create_response(client_buffer, &logged_in, &p1_solution_sent, &p2_solution_sent, &p3_solution_sent);
        printf("%s\n", response);


        // Write response
        if (write_response(client_socket, response, &problems_sent) < 0) {
            close(client_socket);
            printf("\nClient disconnected\n");
            return NULL;
        }
    }
}


int get_request (int SOCKET, char ** result, int * logged_in) {

    // Create variable for message length
    size_t message_length;

    if (read(SOCKET, &message_length, sizeof(size_t)) < 0) {

        // Error reading message length
        return -1;

    } else {

        // Message length successfully read. Now read the message
        if (read(SOCKET, *result, message_length)  <= 0) {

            // Error reading message
            return -2;

        } else {

            // Message read successfully
            return 0;

        }
    }
}


int write_response (int SOCKET, char * response, int * problems_sent) {

    // Variable for the length of the response
    size_t response_length = strlen(response);

    if (write(SOCKET, &response_length, sizeof(size_t)) < 0) {

        // Error writing the length of the response
        return -1;

    } else {

        // Length of response sent successfully. Now send the response
        if (write(SOCKET, response, response_length)  < 0) {

            // Error sending the response
            return -2;

        } else {

            // Response sent successfully
            if (strcmp(response, "Sending problem set\n") == 0) {

                // Now we should send the files
                int f1 = open("/home/vlad/CLionProjects/Server/Problems/p1.txt", O_RDONLY);
                int f2 = open("/home/vlad/CLionProjects/Server/Problems/p2.txt", O_RDONLY);
                int f3 = open("/home/vlad/CLionProjects/Server/Problems/p3.txt", O_RDONLY);

                size_t f1_size = file_size("/home/vlad/CLionProjects/Server/Problems/p1.txt");
                size_t f2_size = file_size("/home/vlad/CLionProjects/Server/Problems/p2.txt");
                size_t f3_size = file_size("/home/vlad/CLionProjects/Server/Problems/p3.txt");

                write(SOCKET, &f1_size, sizeof(size_t));
                printf("File 1 sent: %zd\n", sendfile(SOCKET, f1, NULL, f1_size));
                write(SOCKET, &f2_size, sizeof(size_t));
                printf("File 2 sent: %zd\n", sendfile(SOCKET, f2, NULL, f2_size));
                write(SOCKET, &f3_size, sizeof(size_t));
                printf("File 3 sent: %zd\n", sendfile(SOCKET, f3, NULL, f3_size));

                problems_sent = 0;
            }
            return 0;

        }
    }
}


char * create_response (char * request, int * logged_in, int * p1_solution_sent, int * p2_solution_sent, int * p3_solution_sent) {

    // Get request type
    int request_type = get_request_type(request);


    // Model response. It depends on the request type
    if (request_type == LOGIN_REQUEST) {

        // Create response for login request
        if (!(*logged_in)) return login_response(request, logged_in);
        else return "You are already logged in\n";

    } else if (request_type == SIGNUP_REQUEST) {

        // Create response for signup request
        if (!(*logged_in)) return signup_response(request);
        else return "You can't sign up if you are logged in\n";

    } else if (request_type == PROBLEM_SET_REQUEST) {

        // Create response for problem_set request
        if (*(logged_in)) return problem_set_response(request);
        else return "You need to be logged in to get the problem set\n";

    } else if (request_type == P1_REQUEST) {

        // Create response for p1 request
        if (*(logged_in)) return p1_response(request, *logged_in, p1_solution_sent);
        else return "You need to be logged in to send solving\n";

    } else if (request_type == P2_REQUEST) {

        // Create response for p1 request
        if (*(logged_in)) return p2_response(request, *logged_in, p2_solution_sent);
        else return "You need to be logged in to send solving\n";

    } else if (request_type == P3_REQUEST) {

        // Create response for p1 request
        if (*(logged_in)) return p3_response(request, *logged_in, p3_solution_sent);
        else return "You need to be logged in to send solving\n";

    } else if (request_type == TOTAL_REQUEST) {

        // Create response for total request
        if (*(logged_in)) return total_response(request, *p1_solution_sent, *p2_solution_sent, *p3_solution_sent);
        else return "You need to be logged in to get total\n";

    } else {

        // Create response for invalid_request
        return invalid_request_response();

    }

}


int get_request_type (char * request) {

    char request_copy[REQUEST_SIZE];
    strcpy(request_copy, request);
    char * header_line = strtok(request_copy, "\n");
    char * request_type = strtok(header_line, ":");
    request_type = strtok(NULL, ":");

    if (strcmp(request_type, "login") == 0) return LOGIN_REQUEST;
    else if (strcmp(request_type, "signup") == 0) return SIGNUP_REQUEST;
    else if (strcmp(request_type, "problemset") == 0) return PROBLEM_SET_REQUEST;
    else if (strcmp(request_type, "p1") == 0) return P1_REQUEST;
    else if (strcmp(request_type, "p2") == 0) return P2_REQUEST;
    else if (strcmp(request_type, "p3") == 0) return P3_REQUEST;
    else if (strcmp(request_type, "total") == 0) return TOTAL_REQUEST;
    else return INVALID_REQUEST;

}


char * login_response (char * request, int * logged_in) {

    char request_copy[REQUEST_SIZE];
    strcpy(request_copy, request);
    char * results_line = strtok(request_copy, "\n");
    results_line = strtok(NULL, "\n");

    char user_name[256];
    char password[256];
    char * token = strtok(results_line, "{}:,=");
    while (token != NULL) {
        if (strcmp(token, "uname") == 0) {
            token = strtok(NULL, "{}:,=");
            strcpy(user_name, token);
        } else if (strcmp(token, "passwd") == 0) {
            token = strtok(NULL, "{}:,=");
            strcpy(password, token);
        }
        token = strtok(NULL, "{}:,=");
    }

    char * err_msg;
    int query_result = user_password_match(DB, user_name, password, &err_msg);

    if (query_result == -1) {
        return "Error\n";
    } else if (query_result == 0) {
        return "Couldn't log in. Try again\n";
    } else {
        *logged_in = query_result;
        return "Logged in\n";
    }
}


char * signup_response (char * request) {

    char request_copy[REQUEST_SIZE];
    strcpy(request_copy, request);
    char * results_line = strtok(request_copy, "\n");
    results_line = strtok(NULL, "\n");

    char user_name[256];
    char password[256];
    char * token = strtok(results_line, "{}:,=");
    while (token != NULL) {
        if (strcmp(token, "uname") == 0) {
            token = strtok(NULL, "{}:,=");
            strcpy(user_name, token);
        } else if (strcmp(token, "passwd") == 0) {
            token = strtok(NULL, "{}:,=");
            strcpy(password, token);
        }
        token = strtok(NULL, "{}:,=");
    }


    char * err_msg;
    if (user_exists(DB, user_name, &err_msg)) {
        return "User already exists. Try another one\n";
    }

    int query_result = create_normal_user(DB, user_name, password, err_msg);
    if (query_result == SQLITE_OK) {
        return "User created\n";
    } else {
        return "Couldn't create user\n";
    }
}


char * p1_response (char * request, int uid, int * solution_sent) {

    if (*solution_sent) return "You already sent a solving for p1";

    char * source_file_path = create_source_file(request, uid, "p1");
    char * executable_path = create_executable(uid, "p1", source_file_path);
    execute_source_code(uid, "1", executable_path);
    int result = get_solution_result(uid, "1");
    *solution_sent = result;

    char * result_string = malloc(3);
    sprintf(result_string, "%d", result * 6);


    remove(source_file_path);
    remove(executable_path);

    return result_string;
}


char * p2_response (char * request, int uid, int * solution_sent) {

    if (*solution_sent) return "You already sent a solving for p2";

    char * source_file_path = create_source_file(request, uid, "p2");
    char * executable_path = create_executable(uid, "p2", source_file_path);
    execute_source_code(uid, "2", executable_path);
    int result = get_solution_result(uid, "2");
    *solution_sent = result;

    char * result_string = malloc(3);
    sprintf(result_string, "%d", result * 6);


    remove(source_file_path);
    remove(executable_path);


    return result_string;
}


char * p3_response (char * request, int uid, int * solution_sent) {

    if (*solution_sent) return "You already sent a solving for p3";

    char * source_file_path = create_source_file(request, uid, "p3");
    char * executable_path = create_executable(uid, "p3", source_file_path);
    execute_source_code(uid, "3", executable_path);
    int result = get_solution_result(uid, "3");
    *solution_sent = result;

    char * result_string = malloc(3);
    sprintf(result_string, "%d", result * 6);


    remove(source_file_path);
    remove(executable_path);

    return result_string;
}


char * total_response(char * request, int p1_points, int p2_points, int p3_points) {

    char p1_points_string[3];
    char p2_points_string[3];
    char p3_points_string[3];
    char total_points_string[4];

    sprintf(p1_points_string, "%d", p1_points * 6);
    sprintf(p2_points_string, "%d", p2_points * 6);
    sprintf(p3_points_string, "%d", p3_points * 6);
    sprintf(total_points_string, "%d", p1_points * 6 + p2_points * 6 + p3_points * 6 + 10);

    char * response = malloc(512);
    strcpy(response, "P1: ");
    strcat(response, p1_points_string);
    strcat(response, "\n");
    strcat(response, "P2: ");
    strcat(response, p2_points_string);
    strcat(response, "\n");
    strcat(response, "P3: ");
    strcat(response, p3_points_string);
    strcat(response, "\n");
    strcat(response, "TOTAL: ");
    strcat(response, total_points_string);
    strcat(response, "\n");

    return response;
}


char * problem_set_response (char * request) {
    return "Sending problem set\n";
}


char * invalid_request_response () {
    return "Invalid command\n";
}


char * create_source_file(char * request, int uid, const char problem_number[2]) {

    char request_copy[REQUEST_SIZE];
    strcpy(request_copy, request);
    char * results_line = strtok(request_copy, "\n");
    results_line = strtok(NULL, "\n");
    results_line = strtok(NULL, "\n");


    char * file_path = malloc(128);
    strcpy(file_path, "/home/vlad/CLionProjects/Server/InOut/");
    char user_id[6];
    sprintf(user_id, "%d", uid);
    char file_name[64];
    strcpy(file_name, user_id);
    strcat(file_name, "_");
    strcat(file_name, problem_number);
    strcat(file_name, ".c");
    strcat(file_path, file_name);


    FILE * source_code_file = fopen(file_path, "w");

    while (results_line != NULL) {
        for (int i = 0; i < strlen(results_line); i++) {
            char chr = results_line[i];
            char chr_array[3];
            chr_array[0] = chr;
            chr_array[1] = '\0';
            if (chr == '%') {
                chr_array[1] = '%';
                chr_array[2] = '\0';
                fprintf(source_code_file, chr_array);
            } else {
                fprintf(source_code_file, chr_array);
            }
        }
        fprintf(source_code_file, "\n");
        results_line = strtok(NULL, "\n");
    }

    fclose(source_code_file);

    return file_path;
}


char * create_executable(int uid, const char problem_number[3], char * source_file_path) {

    char * file_path = malloc(128);
    strcpy(file_path, "/home/vlad/CLionProjects/Server/InOut/");
    char user_id[6];
    sprintf(user_id, "%d", uid);
    char file_name[64];
    strcpy(file_name, user_id);
    strcat(file_name, "_");
    strcat(file_name, problem_number);
    strcat(file_name, ".exe");
    strcat(file_path, file_name);

    // Create executable
    pid_t PID = fork();

    if (PID < 0) {

        // Error forking
        perror("Error forking");
        return NULL;

    } else if (PID == 0) {

        // Child
        execlp("gcc", "gcc", "-o", file_path, source_file_path, NULL);

    } else {

        // Parent process
        wait(NULL);

    }
    return file_path;
}


void execute_source_code (int uid, const char problem_number[2], char * executable_path) {

    char user_id[6];
    sprintf(user_id, "%d", uid);

    pid_t PID = fork();

    if (PID < 0) {

        // Error forking
        perror("Error forking");

    } else if (PID == 0) {

        // Child
        char my_exec_path[128];
        strcpy(my_exec_path, executable_path);
        execlp("/home/vlad/CLionProjects/Server/execute_file.bash", "/home/vlad/CLionProjects/Server/execute_file.bash", my_exec_path, problem_number, user_id, NULL);

    } else {

        // Parent process
        wait(NULL);

    }
}


int get_solution_result (int uid, const char problem_number[2]) {

    char user_id[6];
    sprintf(user_id, "%d", uid);

    char path[128];
    strcpy(path, "/home/vlad/CLionProjects/Server/Results/");
    strcat(path, user_id);
    strcat(path, "_p");
    strcat(path, problem_number);
    strcat(path, ".txt");

    FILE * file = fopen(path, "r");

    int result = 0;
    fscanf(file, "%d", &result);
    fclose(file);

    return result;
}