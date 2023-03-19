#include "buffer.h"

buffer::buffer(size_t init_size, size_t max_size): read_pos(0), write_pos(0)
{
    _buf.resize(init_size, 0);
    this->max_size = max_size;
}


buffer::~buffer()
{

}

size_t buffer::preWriteAbleBytes() const
{
    return read_pos;
}

size_t buffer::readAbleBytes() const
{
    return write_pos - read_pos;
}

size_t buffer::writeAbleBytes() const
{
    return _buf.size() - write_pos;
}

int buffer::find(char target)
{   
    for(size_t i = read_pos; i < write_pos; i++)
    {
        if(_buf[i] == target)
        {
            return i;
        }
    }
    return -1;
}


int buffer::find(std::string target)
{   
    size_t len = target.size();
    if(readAbleBytes() < len)
    {
        return -1;
    }

    for(size_t i = read_pos; i <= write_pos - len; i++)
    {
        if(strncmp(beginPtr() + i, &target[0], len) == 0)
        {
            return i;
        }
    }
    return -1;
}


size_t buffer::getReadPos()
{
    return read_pos;
}

size_t buffer::getWritePos()
{
    return write_pos;
}

size_t buffer::size() const
{
    return _buf.size();
}

void buffer::append(const std::string& str)
{   
    if(str.size() != 0)
    {
        append(&str[0], str.size());
    }
}

void buffer::append(const void* data, size_t len)
{
    if(data != nullptr)
    {
        append((char*)data, len);
    }
}

void buffer::append(const buffer& buf)
{
    size_t readn = buf.readAbleBytes();
    if(readn != 0)
    {
        append(buf.peek(), readn);
    }
}

void buffer::append(const char* str, size_t len)
{
    assert(str);
    // 检查是否需要扩容
    if(len > writeAbleBytes())
    {
        _makeSpace(len);
    }
    assert(writeAbleBytes() >= len);

    std::copy(str, str + len, beginWrite());
    hasWrite(len);
}


void buffer::deln(size_t len)
{
    assert(len > 0 && len <= readAbleBytes());
    read_pos += len;
}

void buffer::delAll()
{   
    bzero(&_buf[0], _buf.size());
    read_pos = write_pos = 0;
}

bool buffer::empty() const
{
    return read_pos == write_pos;
}

std::string buffer::subsepstr(std::string sep, bool del)
{
    int pos = this->find(sep);
    if(pos == -1){
        return "";
    }
    std::string ret = this->substr(pos - read_pos + sep.size(), del);
    return ret;
}

std::string buffer::substr(size_t len, bool del)
{
    assert(len > 0 && len <= readAbleBytes());
    std::string ret(peek(), peek() + len);
    if(del){
        deln(len);
    }
    return ret;
}


void buffer::hasWrite(size_t len)
{
    write_pos += len;
    _buf[write_pos] = '\0';
}

const char* buffer::peek() const
{
    return beginPtr() + read_pos;
}

const char* buffer::end() const
{
    return beginPtr() + write_pos;
}

char* buffer::beginPtr()
{
    return &*_buf.begin();
}

const char* buffer::beginPtr() const
{
    return &*_buf.begin();
}


char* buffer::beginWrite()
{
    return beginPtr() + write_pos;
}

// 从文件描述符读数据到buf
int buffer::readFd(int fd, int& save_errno)
{
    const size_t writeable = writeAbleBytes();
    int ret = read(fd, beginPtr(), writeable);
    if(ret < 0)
    {
        save_errno = errno;
    }else if(ret <= writeable)
    {
        write_pos += ret;
    }
    // 当从fd写满且buf_size < max_size时 扩容两倍
    if(ret == writeable && _buf.size() < max_size)
    {
        _buf.resize(2 * _buf.size()); 
    }
    return ret;
}


int buffer::writeFd(int fd, int& save_errno)
{
    int ret = write(fd, peek(), readAbleBytes());
    if(ret < 0)
    {
        save_errno = errno;
    }else{
        deln(ret);
    }
    return ret;
}


buffer& buffer::operator+(std::string str)
{
    append(str);
    return *this;
}

buffer& buffer::operator+=(std::string str)
{
    append(str);
    return *this;
}

std::ostream& operator<<(std::ostream& os, const buffer& buf)
{
    if(!buf.empty())
    {
        for(size_t pos = buf.read_pos; pos < buf.write_pos; pos++)
        {
            os<<buf[pos];
        }
    }else
    {
        os<<"empty";
    }
    return os;
}

char buffer::operator[](size_t pos) const
{
    assert(pos >= read_pos && pos < write_pos);
    return _buf[pos];
}

// 扩容
void buffer::_makeSpace(size_t len)
{
    // 检查当前尾部可写和头部的长度
    if(preWriteAbleBytes() + writeAbleBytes() < len)
    {
        // 进行扩容
        _buf.resize(write_pos + len + 1);
    }else
    {
        size_t readables = readAbleBytes();
        // 将未读的数据搬移到begin处
        std::copy(_buf.begin() + read_pos, _buf.begin() + write_pos, _buf.begin());
        read_pos = 0;
        write_pos = read_pos + readables;
        assert(write_pos == readAbleBytes());
    }
}