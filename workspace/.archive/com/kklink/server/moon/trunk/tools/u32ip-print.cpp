#include <stdint.h>
#include <stdio.h>
#include <iostream>

int main()
{
    union {
        uint32_t ival;
        unsigned char v[4];
    } u;

    while (std::cin >> u.ival)
        printf("%d.%d.%d.%d\n", u.v[3], u.v[2], u.v[1], u.v[0]);
}

