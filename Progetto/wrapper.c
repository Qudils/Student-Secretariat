#include "wrapper.h"

int Socket(int family, int type, int protocol)
{
    int n;
    if ((n = socket(family, type, protocol)) < 0)
    {
        perror("socket error");
        exit(1);
    }
    return n;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (bind(sockfd, addr, addrlen) < 0)
    {
        perror("bind error");
        exit(1);
    }
}

void Ascolta(int sockfd, int queueLen)
{
    if (listen(sockfd, queueLen) < 0)
    {
        perror("listen error");
        exit(1);
    }
}

int Accetta(int sockfd, struct sockaddr *clientaddr, socklen_t *addr_dim)
{
    int n;
    if ((n = accept(sockfd, clientaddr, addr_dim)) < 0)
    {
        perror("accept error");
        exit(1);
    }
    return n;
}

void Connetti(int sockfd, const struct sockaddr *servaddr, socklen_t addr_dim)
{
    if (connect(sockfd, (const struct sockaddr *)servaddr, addr_dim) < 0)
    {
        fprintf(stderr, "connect error\n");
        exit(1);
    }
}
