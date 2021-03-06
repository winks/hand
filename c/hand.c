#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>

#define PORT "3490"
#define FDIR "/srv/finger"
#define ENABLE_FEATURE_LIST 0
#define IGNORE_SYSTEM_USERS 0

#define MAXDATASIZE 100
#define MAXFILESIZE 1024
#define BACKLOG 10
#define MSG_LIST_NO "Finger online user list denied"
#define MSG_LIST_YES "Finger online user NYI"
#define MSG_NO_INFO "No information found"

#define LISTEN_ANY 1
#define LISTEN_V4 2
#define LISTEN_V6 3

void sigchld_handler(int s)
{
    s = 2;
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int reader(char *username, char *filename, char *output)
{
    FILE *fp;
    char filepath[MAXDATASIZE];

    if (snprintf(filepath, MAXDATASIZE, "%s/%s/%s", FDIR, username, filename) >= MAXDATASIZE) {
        perror("too long");
        return 1;
    }
    fprintf(stderr, "  read: %s\n", filepath);

    fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("file does not exist");
        return 2;
    } else {
        size_t len = fread(output, sizeof(char), MAXFILESIZE-1, fp);
        if (len == 0) {
            perror("error reading");
            return 3;
        } else {
            output[++len] = '\0';
        }
    }
    fclose(fp);

    return 0;
}

int sender(int socket_fd, char *str, int newline)
{
    char out[(MAXFILESIZE*2)+1];
    int len;

    if (newline) {
        len = snprintf(out, sizeof out, "%s\n", str);
    } else {
        len = snprintf(out, sizeof out, "%s", str);
    }
    if (send(socket_fd, out, len, 0) == -1) {
        perror("send");
    }
    printf("\n");

    return 0;
}

int xfmt(struct passwd *p, char *txt, int len)
{
    char *tpl1 = "Login: %-33sName: %s";
    char *tpl2 = "Directory: %-29sShell: %s";
    char lines[2][160];

    snprintf(lines[0], sizeof lines[0], tpl1, p->pw_name, p->pw_gecos);
    snprintf(lines[1], sizeof lines[1], tpl2, p->pw_dir, p->pw_shell);
    snprintf(txt, len, "%s\n%s\n", lines[0], lines[1]);

    return 0;
}

int handle_input(char *buf, int new_fd)
{
    size_t buflen = strlen(buf);
    fprintf(stderr, "  recv: %zu/%d\n", buflen, MAXDATASIZE);
    // This is a bogus case, ignore
    if (buflen < 2 ) {
        fprintf(stderr, "  bogus\n");
        return 11;
    }

    char *right = strdup(buf);
    char *left = strsep(&right, "\r");
    fprintf(stderr, "  sep : %zu+%zu\n", strlen(left), strlen(right)+1);

    // This is a bogus case, ignore
    if (right == NULL || right[0] != '\n') {
        fprintf(stderr, "  bogus\n");
        return 12;
    // This is {Q1} = "<CRLF>"
    } else if (strlen(left) == 0) {
        if (ENABLE_FEATURE_LIST) {
            sender(new_fd, MSG_LIST_YES, 1);
        } else {
            sender(new_fd, MSG_LIST_NO, 1);
        }
    // This is the default, {Q1} = "INPUT<CRLF>"
    } else {
        char out_hdr[MAXFILESIZE+2];
        char out_plan[MAXFILESIZE+2];
        char out_all[(MAXFILESIZE+2)*2];

        struct passwd *p;
        if ((p = getpwnam(left)) == NULL) {
            printf("SRV : user not found\n");
            sender(new_fd, MSG_NO_INFO, 1);
            return 13;
        }
        if (IGNORE_SYSTEM_USERS) {
            if(p->pw_uid < 1000 || p->pw_uid >= 65534) {
            printf("SRV : user not allowed to be found: %s:%d:%d\n",
                    p->pw_name, p->pw_uid, p->pw_gid);
            sender(new_fd, MSG_NO_INFO, 1);
            return 14;
            }
        }
        if(reader(p->pw_name, ".nofinger", out_plan) != 2) {
            sender(new_fd, MSG_NO_INFO, 1);
            return 15;
        }

        xfmt(p, out_hdr, sizeof out_hdr);
        fprintf(stderr, "   hdr:\n%s\n---\n", out_hdr);

        if(reader(p->pw_name, ".plan", out_plan) != 0) {
            snprintf(out_plan, sizeof out_plan, "No Plan.\n");
        }

        fprintf(stderr, "  plan:\n%s\n---\n", out_plan);

        snprintf(out_all, sizeof out_all, "%s%s", out_hdr, out_plan);
        sender(new_fd, out_all, 0);
    }

    return 0;
}


int main(int argc, char** argv)
{
    int listen_type = 0;
    if (argc == 2) {
        printf("%s\n", argv[1]);
        if (strcmp(argv[1], "-l") == 0) {
            listen_type = LISTEN_ANY;
        } else if (strcmp(argv[1], "-4") == 0) {
            listen_type = LISTEN_V4;
        } else if (strcmp(argv[1], "-6") == 0) {
            listen_type = LISTEN_V6;
        }
    }
    if (listen_type == 0) {
        printf("Missing arguments.\n");
        printf("Usage:\n");
        printf("\t-l\tListen on any interface\n");
        printf("\t-4\tListen on v4 interface only\n");
        printf("\t-6\tListen on v6 interface only\n");
        return 0;
    }

    // listen on sock_fd, new connection on new_fd
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    // connector's address information
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char *ipver;
    char ipstr[INET6_ADDRSTRLEN];
    char hostname[NI_MAXHOST];
    int rv;
    int numbytes;
    char buf[MAXDATASIZE];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "  gai: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("SRV : socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("SRV : setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("SRV : bind");
            continue;
        }

        void *addr;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
            if (listen_type == LISTEN_V6) {
                continue;
            }
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
            if (listen_type == LISTEN_V4) {
                continue;
            }
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("SRV : binding to %s (%s)\n", ipstr, ipver);

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "SRV : failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("SRV : waiting for connections...\n\n");

    while(1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        getnameinfo((struct sockaddr *)&their_addr, sin_size, ipstr, sizeof ipstr, NULL, 0, NI_NUMERICHOST);
        getnameinfo((struct sockaddr *)&their_addr, sin_size, hostname, sizeof hostname, NULL, 0, 0);
        printf("SRV : got connection from %s %s\n", ipstr, hostname);

        // this is the child process
        if (!fork()) {
            // child doesn't need the listener
            close(sockfd);

            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(11);
            }
            buf[numbytes] = '\0';
            handle_input(buf, new_fd);

            close(new_fd);
            exit(0);
        }
        // parent doesn't need this
        close(new_fd);
    }

    return 0;
}
