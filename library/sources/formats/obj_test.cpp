#error "FIX ME"

#include <cstring>
#include <cassert>
#include <array>
#include <cctype>


template<size_t BufSize, size_t ResSize>
void parseItems(std::array<char, BufSize> & in,
                std::array<char const *, ResSize> & out)
{
    size_t ptr = 0;
    for(size_t i(0); i < out.size(); ++i)
    {
        // skip whitespace
        while (ptr < in.size() && in[ptr] && ::isspace(in[ptr])) ++ptr;
        
        // not EOF and not whitespace

        if (ptr >= in.size() || !in[ptr]) throw std::runtime_error("tooshort at " + std::to_string(ptr));

        // skip token
        out[i] = &(in[ptr]);
        while (ptr < in.size() && in[ptr] && !::isspace(in[ptr])) ++ptr;
        in[ptr++] = '\0';
    }
}

template<typename T>
void expectThrow(T t)
{
    try {
        t();
    } catch(...) {
        return;
    }
    assert(0);
}

int main()
{
    std::array<char, 512> buf;
    std::array<char const *, 4> out;

    strcpy(buf.data(), "");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), " ");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), "    ");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), "\n");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), "\n\n");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), "\r\n");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), " \n \r\n \n\r ");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), "a");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), " a\n");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), " a b\n");
    expectThrow([&buf, &out]{parseItems(buf, out);});

    strcpy(buf.data(), " a b\tcsdfsdf  \n");
    expectThrow([&buf, &out]{parseItems(buf, out);});


    strcpy(buf.data(), " a\tb csdfsdf  sdffg \n");
    parseItems(buf, out);
    assert(!strcmp(out[0], "a"));
    assert(!strcmp(out[1], "b"));
    assert(!strcmp(out[2], "csdfsdf"));
    assert(!strcmp(out[3], "sdffg"));

    strcpy(buf.data(), "v 1 2 3");
    parseItems(buf, out);
    assert(!strcmp(out[0], "v"));
    assert(!strcmp(out[1], "1"));
    assert(!strcmp(out[2], "2"));
    assert(!strcmp(out[3], "3"));

}
