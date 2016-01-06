#include <limits>
#include <stdio.h>

int main()
{
    printf("%u\t%u\n%u\t%u\n%u\t%u\n"
            , sizeof(int)  , unsigned(std::numeric_limits<int>::max()/10000)
            , sizeof(float), unsigned(std::numeric_limits<float>::max()/10000)
            , sizeof(long) , unsigned(std::numeric_limits<long>::max()/10000)
            );
}

