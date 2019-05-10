#include <ctype.h>
#include <stdlib.h>
#include <string>
#include <sys/dir.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "Channel.h"
#include "Buffer.h"
#include "Coder.h"

void skipSpace(char *p, char *ep)
{
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
}

void parseLine(Request& req, Buffer& buf, int len)
{
    char *p = buf.peek();
    char *ep = p + len;

    skipSpace(p, ep);
    while (p < ep && !isspace(*p))
        req._type.push_back(*p);
    skipSpace(p, ep);
    while (p < ep && !isspace(*p))
        req._path.push_back(*p);
    skipSpace(p, ep);
    while (p < ep && !isspace(*p))
        req._macAddr.push_back(*p);
    req._state = REQ_HEADER;
}

void parseHeader(Request& req, Buffer& buf, int len)
{
    char *p = buf.peek();
    char *ep = p + len;

    if (strcmp(p, "\r\n") == 0)
        req._state = REQ_RECVING;

    if (strncasecmp(p, "filesize:", 9) == 0) {
        p += 9;
        skipSpace(p, ep);
        req._filesize = atoi(p);
    } else {
        ; // 目前头部只有一个字段
    }
}

int dirIsExist(const char *name)
{
    DIR *dir = opendir(name);
    return dir ? (closedir(dir), 1) : 0;
}

// 接收客户端发来的文件以更新本地文件
void recvFile(Request& req)
{
    Buffer buf;
    req._fd = open(req._macAddr.c_str(), O_WRONLY | O_APPEND);
    ssize_t n = write(req._fd, buf.peek(), buf.readable());
    buf.retrieveAll();
    if (n < req._filesize) {
        // 文件还未接收完，下次接收
        req._filesize -= n;
    } else {
        close(req._fd);
        req._fd = -1;
        // 处理下次请求
        req._state = REQ_LINE;
    }
}

// 将备份文件发送给客户
void sendFile(Channel& chl, Request& req)
{
    Buffer buf;
    int fd = open(req._macAddr.c_str(), O_RDONLY);
    while (buf.readFd(fd) > 0) {
        chl.send(buf.peek(), buf.readable());
        buf.retrieveAll();
    }
    close(fd);
}

// 向客户回复响应信息
void replyResponse(void)
{

}

void onMessage(Channel& chl, Buffer& buf, Request& req)
{
    // 解析用户请求
    if (req._state == REQ_RECVING) {

        return;
    }
    // 可能有一行完整的消息
    while (buf.readable() >= 2) {
        int crlf = buf.findCrlf();
        // 至少有一行请求
        if (crlf >= 0) {
            if (req._state == REQ_LINE) {
                parseLine(req, buf, crlf);
                crlf += 2;
            } else if (req._state == REQ_HEADER) {
                parseHeader(req, buf, crlf);
                crlf += 2;
            } else
                break;
            buf.retrieve(crlf);
        } else
            break;
    }
    // 回复响应
    if (req._type == "SAVE") {
        ;
    } else if (req._type == "GET") {
        ;
    }
    replyResponse();
}
