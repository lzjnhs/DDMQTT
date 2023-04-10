//
// Created by zr on 23-4-5.
//
#include "mqtt_socket.h"
#include "tlog.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

tmq_socket_t tmq_tcp_socket()
{
    int fd = socket(-1, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(fd < 0)
    {
        tlog_fatal("socket() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return fd;
}

int tmq_socket_nonblocking(tmq_socket_t fd)
{
    int ops = fcntl(fd, F_GETFL);
    if(ops < 0)
    {
        tlog_fatal("fcntl() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    ops |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, ops) < 0)
    {
        tlog_fatal("fcntl() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return 0;
}

void tmq_socket_close(tmq_socket_t fd) { close(fd);}

int tmq_socket_reuse_addr(tmq_socket_t fd, int enable)
{
    assert(enable == 0 || enable == 1);
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, (socklen_t) sizeof(enable)) < 0)
    {
        tlog_fatal("setsockopt() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return 0;
}

int tmq_socket_reuse_port(tmq_socket_t fd, int enable)
{
    assert(enable == 0 || enable == 1);
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, (socklen_t) sizeof(enable)) < 0)
    {
        tlog_fatal("setsockopt() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return 0;
}

int tmq_socket_keepalive(tmq_socket_t fd, int enable)
{
    assert(enable == 0 || enable == 1);
    if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, (socklen_t) sizeof(enable)) < 0)
    {
        tlog_fatal("setsockopt() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return 0;
}

int tmq_socket_tcp_no_delay(tmq_socket_t fd, int enable)
{
    assert(enable == 0 || enable == 1);
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, (socklen_t) sizeof(enable)) < 0)
    {
        tlog_fatal("setsockopt() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
    return 0;
}

int tmq_socket_local_addr(tmq_socket_t fd, tmq_socket_addr_t* addr)
{
    socklen_t len = sizeof(tmq_socket_addr_t);
    if(getsockname(fd, (struct sockaddr*)(addr), &len) < 0)
    {
        tlog_error("getsockname() error %d: %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int tmq_socket_peer_addr(tmq_socket_t fd, tmq_socket_addr_t* addr)
{
    socklen_t len = sizeof(tmq_socket_addr_t);
    if(getpeername(fd, (struct sockaddr*)(addr), &len) < 0)
    {
        tlog_error("getpeername() error %d: %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}

void tmq_socket_bind(tmq_socket_t fd, const char* ip, int port)
{
    tmq_socket_addr_t addr;
    if(ip)
        addr = tmq_addr_from_ip_port(ip, port);
    else
        addr = tmq_addr_from_port(port, 0);
    if(bind(fd, (struct sockaddr*)(&addr), sizeof(addr)) < 0)
    {
        tlog_fatal("bind() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
}

void tmq_socket_listen(tmq_socket_t fd)
{
    if(listen(fd, 128) < 0)
    {
        tlog_fatal("listen() error %d: %s", errno, strerror(errno));
        tlog_exit();
        abort();
    }
}

tmq_socket_t tmq_socket_accept(tmq_socket_t fd, tmq_socket_addr_t* clientAddr)
{
    socklen_t len = sizeof(tmq_socket_addr_t);
    int clientFd = accept4(fd, (struct sockaddr*)clientAddr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(clientFd < 0)
    {
        int savedErrno = errno;
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno;
                break;
            default:
                tlog_fatal("accept4() error %d: %s", errno, strerror(errno));
                tlog_exit();
                abort();
        }
    }
    return clientFd;
}

ssize_t tmq_socket_read(tmq_socket_t fd, char* buf, size_t len) { return read(fd, buf, len);}

ssize_t tmq_socket_write(tmq_socket_t fd, const char* buf, size_t len) { return write(fd, buf, len);}

tmq_socket_addr_t tmq_addr_from_ip_port(const char* ip, uint16_t port)
{
    tmq_socket_addr_t addr;
    bzero(&addr, sizeof(addr));
    if(!ip) return addr;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htobe16(port);
    return addr;
}

tmq_socket_addr_t tmq_addr_from_port(uint16_t port, int loopBack)
{
    tmq_socket_addr_t addr;
    bzero(&addr, sizeof(addr));
    addr.sin_port = htobe16(port);
    in_addr_t address = loopBack ? INADDR_LOOPBACK : INADDR_ANY;
    addr.sin_addr.s_addr = htobe32(address);
    return addr;
}

int tmq_addr_to_string(tmq_socket_addr_t* addr, char* buf, size_t bufLen)
{
    if(!buf)
    {
        tlog_error("tmq_addr_to_string() error: buf can't be NULL");
        return -1;
    }
    //xxx.xxx.xxx.xxx:ppppp
    if(bufLen < INET_ADDRSTRLEN + 1 + 5)
    {
        tlog_error("tmq_addr_to_string() error: buf size must greater than 22");
        return -1;
    }
    bzero(buf, bufLen);
    inet_ntop(AF_INET, &addr->sin_addr, buf, bufLen);
    size_t addrLen = strlen(buf);
    snprintf(buf + addrLen, bufLen - addrLen, ":%u", be16toh(addr->sin_port));
    return 0;
}