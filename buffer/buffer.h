#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <assert.h>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h>
#include <cstring>
#include <iostream>

class buffer
{
public:
    buffer(int init_size = 1024);
    ~buffer();
    size_t readAbleBytes() const;
    size_t writeAbleBytes() const;
    size_t preWriteAbleBytes() const;
    size_t size() const;
    size_t getReadPos();
    size_t getWritePos();

    int find(char target);
    int find(std::string target);

    bool empty();

    void deln(size_t);
    void delAll();
    std::string dealToString(size_t);

    const char* peek() const;
    const char* end() const;
    char* beginPtr();
    const char* beginPtr() const;
    char* beginWrite();  // 开始写入的位置
    
    void append(const char* str, size_t len);
    void append(const std::string& str);
    void append(const void* str, size_t len);
    void append(const buffer& buf);

    void hasWrite(size_t len);

    int readFd(int, int&);
    int writeFd(int, int&);

    buffer& operator+(std::string);
    buffer& operator+=(std::string);
    char operator[](size_t);

private:
    void _makeSpace(size_t len);
    std::vector<char> _buf;
    size_t write_pos;
    size_t read_pos;
};

#endif