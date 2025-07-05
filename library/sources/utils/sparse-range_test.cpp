#error "FIX ME"

#include <bznk-lib/sparse-range.hpp>
#include <iostream>

int main()
{
    bznk::SparseRange<float> v;

    v.insert(10, 12 + 1);
    v.insert(13, 14 + 1);
    v.insert(15, 16 + 1);

    std::cout << "input: " << v << std::endl;

    for(auto i : v.tighten())
    {
        std::cout << "[" << i.first << "; " << i.second << "]" << std::endl;
    }

    return 0;
}
