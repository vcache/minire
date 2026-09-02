#pragma once

#include <physfs.h>

#include <cstddef>
#include <istream>
#include <streambuf>
#include <string_view>
#include <vector>

namespace minire::content::manager
{
    class PhysFSStreambuf
        : public std::streambuf
    {
        PhysFSStreambuf(const PhysFSStreambuf &) = delete;
        PhysFSStreambuf& operator=(const PhysFSStreambuf &) = delete;
        PhysFSStreambuf(PhysFSStreambuf &&) = delete;
        PhysFSStreambuf & operator=(PhysFSStreambuf &&) = delete;

    public:
        explicit PhysFSStreambuf(PHYSFS_File* file = nullptr,
                                 size_t buffer_size = 4096)
            : _file(file)
            , _buffer(buffer_size == 0 ? 1 : buffer_size)
        {
            setg(_buffer.data(), _buffer.data(), _buffer.data());
        }

        ~PhysFSStreambuf() override { close(); }

    public:
        bool is_open() const noexcept { return _file != nullptr; }

        void close()
        {
            if (_file)
            {
                ::PHYSFS_close(_file);
                _file = nullptr;
            }
            setg(nullptr, nullptr, nullptr);
        }

    protected:
        int_type underflow() override
        {
            if (!_file)
            {
                return traits_type::eof();
            }

            // if there's still unread data in the get area
            if (gptr() < egptr())
            {
                return traits_type::to_int_type(*gptr());
            }

            ::PHYSFS_sint64 bytes_read = ::PHYSFS_readBytes(_file, _buffer.data(), _buffer.size());
            if (bytes_read <= 0)
            {
                return traits_type::eof();
            }

            setg(_buffer.data(), _buffer.data(), _buffer.data() + bytes_read);
            return traits_type::to_int_type(static_cast<unsigned char>(*gptr()));
        }

        // optimized bulk read
        std::streamsize xsgetn(char_type * s,
                               std::streamsize count) override
        {
            if (!_file || count <= 0)
            {
                return 0;
            }

            std::streamsize total_read = 0;

            // consume any bytes currently left in the internal buffer
            if (std::streamsize in_buffer = egptr() - gptr();
                in_buffer > 0)
            {
                std::streamsize to_copy = (count < in_buffer) ? count : in_buffer;
                traits_type::copy(s, gptr(), to_copy);
                gbump(static_cast<int>(to_copy));
                s += to_copy;
                count -= to_copy;
                total_read += to_copy;
            }

            // if remaining count is larger than buffer size, read directly to target destination
            if (count >= static_cast<std::streamsize>(_buffer.size()))
            {
                if (::PHYSFS_sint64 direct_read = ::PHYSFS_readBytes(_file, s, count);
                    direct_read > 0)
                {
                    total_read += direct_read;
                }
                return total_read;
            }

            // otherwise refill buffer if more bytes are needed
            if (count > 0 && underflow() != traits_type::eof())
            {
                std::streamsize remainder = egptr() - gptr();
                std::streamsize to_copy = (count < remainder) ? count : remainder;
                traits_type::copy(s, gptr(), to_copy);
                gbump(static_cast<int>(to_copy));
                total_read += to_copy;
            }

            return total_read;
        }

        pos_type seekoff(off_type off,
                         std::ios_base::seekdir dir,
                         std::ios_base::openmode which) override
        {
            if (!_file || !(which & std::ios_base::in))
            {
                return pos_type(off_type(-1));
            }

            // optimization for tellg() queries: avoid invalidating buffer
            if (dir == std::ios_base::cur && off == 0)
            {
                ::PHYSFS_sint64 physfs_pos = ::PHYSFS_tell(_file);
                if (physfs_pos < 0) return pos_type(off_type(-1));
                return pos_type(physfs_pos - (egptr() - gptr()));
            }

            PHYSFS_sint64 target_pos = 0;
            if (std::ios_base::beg == dir)
            {
                target_pos = off;
            }
            else if (std::ios_base::cur == dir)
            {
                ::PHYSFS_sint64 physfs_pos = ::PHYSFS_tell(_file);
                if (physfs_pos < 0) return pos_type(off_type(-1));
                target_pos = physfs_pos - (egptr() - gptr()) + off;
            }
            else if (std::ios_base::end == dir)
            {
                ::PHYSFS_sint64 length = ::PHYSFS_fileLength(_file);
                if (length < 0) return pos_type(off_type(-1));
                target_pos = length + off;
            }

            if (target_pos < 0 ||
                ::PHYSFS_seek(_file, target_pos) == 0)
            {
                return pos_type(off_type(-1));
            }

            // reset get area pointers because file position jumped
            setg(_buffer.data(), _buffer.data(), _buffer.data());
            return pos_type(target_pos);
        }

        pos_type seekpos(pos_type sp,
                         std::ios_base::openmode which) override
        {
            return seekoff(off_type(sp), std::ios_base::beg, which);
        }

    private:
        ::PHYSFS_File   * _file;
        std::vector<char> _buffer;
    };

    class PhysFSIStream
        : public std::istream
    {
    public:
        PhysFSIStream()
            : std::istream(&_buf)
            , _buf(nullptr)
        {}

        explicit PhysFSIStream(std::string const & filename)
            : std::istream(nullptr)
            , _buf(::PHYSFS_openRead(filename.c_str()))
        {
            rdbuf(&_buf);
            if (!_buf.is_open())
            {
                setstate(std::ios_base::failbit);
            }
        }

        bool is_open() const noexcept { return _buf.is_open(); }

        void close() { _buf.close(); }

    private:
        PhysFSStreambuf _buf;
    };
}