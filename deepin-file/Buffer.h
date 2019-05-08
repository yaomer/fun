#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <sys/uio.h>

class Buffer {
public:
    explicit Buffer() 
        : _buf(INIT_SIZE), _readindex(0), _writeindex(0) { }
    ~Buffer() { }
    static const size_t INIT_SIZE = 1024;
    static const char _crlf[];
    char *begin() { return &*_buf.begin(); }
    char *peek() { return begin() + _readindex; }
    size_t prependable() { return _readindex; }
    size_t readable() { return _writeindex - _readindex; }
    size_t writeable() { return _buf.capacity() - _writeindex; }
    // 将data追加到Buffer中
    void append(const char *data, size_t len)
    {
        makeSpace(len);
        _buf.insert(_buf.begin() + _writeindex, data, data + len);
        _writeindex += len;
    }
    // 内部腾挪
    void makeSpace(size_t len)
    {
        // 有足够的腾挪空间
        if (len < writeable() && writeable() + prependable() > len) {
            size_t readn = readable();
            std::copy(peek(), peek() + readn, begin());
            _readindex = 0;
            _writeindex = _readindex + readn;
        }
    }
    // 返回\r\n在Buffer中第一次出现的位置，没出现返回-1
    int findCrlf(void)
    {
        const char *crlf = std::search(peek(), begin() + _writeindex, _crlf, _crlf + 2);
        return crlf == begin() + _writeindex ? -1 : crlf - peek();
    }
    void retrieve(size_t len)
    {
        if (len < readable())
            _readindex += len;
        else
            _readindex = _writeindex = 0;
    }
    void retrieveAll() { retrieve(readable()); }
    int readFd(int fd);
private:
    std::vector<char> _buf;
    size_t _readindex;
    size_t _writeindex;
};

#endif // _BUFFER_H