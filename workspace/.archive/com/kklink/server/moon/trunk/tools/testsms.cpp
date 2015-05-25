#include "util.h"

int main(int argc, char* const argv[])
{
    if (argc != 3)
        exit(3);
    return sendsms(argv[1], argv[2]);
}

