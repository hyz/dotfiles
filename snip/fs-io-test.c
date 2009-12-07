#include <stdio.h>
#include <time.h>

void init_line(int x)
{
    char fn[96];
    FILE* f;
    snprintf(fn,sizeof(fn), "test-%04d.txt", x);
    if ( (f = fopen(fn, "a")))
    {
        time_t t = time(0);
        fprintf(f, "%04d %s", x, ctime(&t));
        fclose(f);
    }
}

void move_lines(int x, int y)
{
    int size = 0;
    char xfn[96], yfn[96];
    FILE *xf, *yf;
    snprintf(xfn,sizeof(xfn), "test-%04d.txt", x);
    snprintf(yfn,sizeof(yfn), "test-%04d.txt", y);
    xf = fopen(xfn, "r");
    yf = fopen(yfn, "w");
    if (xf && yf)
    {
        int i = 0;
        char buf[2][1024];
        if (fgets(buf[i%2],1024, xf))
        {
            for (++i; fgets(buf[i%2],1024, xf); ++i)
                size += fprintf(yf, "%s", buf[(i-1)%2]);
            if (x < y)
            {
                time_t t = time(0);
                size += fprintf(yf, "%s", buf[(i-1)%2]);
                size += fprintf(yf, "%04d %s", y, ctime(&t));
            }
        }
    }
    if (xf) fclose(xf);
    if (yf) fclose(yf);
    printf("%s <> %s: %d\n", xfn, yfn, size);
    remove(xfn);
}

int main(int argc, char* argv[])
{
    int x = 0, y = 1, interval = atoi(argc>1?argv[1]:"100");
    init_line(x);
    while (1)
    {
        while ((x+=y) % 10)
        {
            move_lines(x-y, x);
            usleep(1000*interval);
        }
        y = -y;
        x += y;
    }
    return 0;
}

