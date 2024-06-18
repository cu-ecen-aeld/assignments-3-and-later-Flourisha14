#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>


#define PORT "9000"
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 500

static bool shutdown_server = false;

struct sockaddr_storage their_addr;
int new_connection;
int sockfd;
int fd;

void daemonize(void);
void register_signal_handler(void);
void setup_server(struct addrinfo *, struct addrinfo **);
void socket_create_bind(struct addrinfo **, struct addrinfo **);
void socket_listen(void);
int accept_new_connection(void);
void log_connection(char *);


static void signal_handler (int signo)
{
    if ((signo == SIGINT) || (signo == SIGTERM))
    {
        //printf("Shutting Down Server...\n");

        close(fd);
        close(new_connection);
        close(sockfd);

        log_connection("Closed");
        syslog(LOG_INFO, "Caught signal, exiting\n");

        if (remove("/var/tmp/aesdsocketdata") != 0) 
        {
            perror("remove");
        }
        exit (EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "d")) != -1) 
    {
        switch (opt) 
        {
            case 'd':
                daemonize();
                break;
            default:
                //printf("Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    int num_bytes;
    char recv_buffer[MAXDATASIZE];
    struct addrinfo  server_data, *server_info, *p;
    int cur_file_pos;

    register_signal_handler();
    
    setup_server(&server_data, &server_info);

    socket_create_bind(&server_info, &p);

    freeaddrinfo(server_info);

    socket_listen();

    fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_APPEND | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    new_connection = accept_new_connection();

    while (!shutdown_server) 
    {
        num_bytes = recv(new_connection, recv_buffer, sizeof(recv_buffer), 0);

        if(!num_bytes)
        {
            shutdown_server = true;
            continue;
        }
        syslog(LOG_INFO,"%s\n", recv_buffer);
        if(recv_buffer[num_bytes - 1]=='\n')
        {
            if (write(fd, recv_buffer, num_bytes) == -1) 
            {
                perror("Failed to write to file");
                exit(1);
            }
            cur_file_pos = lseek (fd, 0, SEEK_CUR);

            memset(&recv_buffer, 0, sizeof recv_buffer);

            lseek(fd, 0, SEEK_SET);
            while ((num_bytes = read(fd, recv_buffer, sizeof(recv_buffer) - 1)) > 0) 
            {
                num_bytes = send(new_connection, recv_buffer, strlen(recv_buffer), 0);
                memset(&recv_buffer, 0, sizeof recv_buffer);
            }
            lseek(fd, 0, cur_file_pos);
            
            new_connection = accept_new_connection();
        } 
        else
        {
            if (write(fd, recv_buffer, num_bytes) == -1) 
            {
                perror("Failed to write to file");
                exit(1);
            }
            memset(&recv_buffer, 0, sizeof recv_buffer);
        }
    }

    return 0;
}

void daemonize(void)
{
    pid_t pid;
    int i;
    struct rlimit rlim;

    /*Get Soft Limit for the number of open files*/
    if (getrlimit(RLIMIT_NOFILE, &rlim) == -1) {
        perror("getrlimit");
        exit(EXIT_FAILURE);
    }

    //printf("Current soft limit for number of open files: %ld\n", rlim.rlim_cur);


    /*Create a new process*/
    pid = fork();
    if(pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if(pid!=0)
        exit(EXIT_SUCCESS);

    /*Create a new session & process group*/
    if(setsid() == -1)
    {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    /*Set working directory to the root directory*/
    if(chdir ("/") == -1)
    {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    /*Close all open files*/
    for(i=0; i<rlim.rlim_cur; i++)
    {
        close(i);
    }

    /*Redirect FDs 0, 1 & 2 to /dev/null*/
    open("/dev/null", O_RDWR); /*stdin*/
    dup(0); /*stdout*/
    dup(0); /*stderr*/
}

void register_signal_handler(void)
{
    if (signal (SIGINT, signal_handler) == SIG_ERR) 
    {
        exit (EXIT_FAILURE);
    }

    if (signal (SIGTERM, signal_handler) == SIG_ERR) 
    {
        exit (EXIT_FAILURE);
    }
}

void setup_server(struct addrinfo *server_data, struct addrinfo **server_info)
{
    int status;

    memset(server_data, 0, sizeof (struct addrinfo));

    server_data->ai_family = AF_INET;
    server_data->ai_socktype = SOCK_STREAM;
    server_data->ai_protocol = 0;
    server_data->ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, server_data, &(*server_info)) != 0)) 
    {
        //printf("getaddrinfo: %s\n", gai_strerror(status));
        exit(-1);
    }
}

void socket_create_bind(struct addrinfo **server_info, struct addrinfo **p)
{
    for (*p = *server_info; *p != NULL; *p = (*p)->ai_next) 
    {
        sockfd = socket((*p)->ai_family, (*p)->ai_socktype, (*p)->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue;
        }

        if (bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }
}

void socket_listen(void)
{
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //printf("Waiting for connections...\n");
}

int accept_new_connection(void)
{
    int new_con;
    socklen_t addr_size;

    addr_size = sizeof their_addr;
    new_con = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_con == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    log_connection("Accepted");

    return new_con;
}

void log_connection(char *connection_status)
{
    char addr[INET_ADDRSTRLEN];

    inet_ntop(their_addr.ss_family, &(((struct sockaddr_in *)&their_addr)->sin_addr), addr, sizeof addr);
    //printf("%s connection from %s\n",connection_status, addr);
    syslog(LOG_INFO, "%s connection from %s\n",connection_status, addr);
}