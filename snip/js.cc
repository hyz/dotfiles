#include <ctype.h>
#include <string.h>
#include <stack>
#include <iostream>

struct jscalls
{
    enum { Ty_Str=1, Ty_Int, Ty_Bool };
    union value_t {
        const char *s[2];
        double d;
        long i;
        bool b;
        value_t(){ s[0]=s[1]=0; d=0; }
    };

    virtual jscalls() {}
    virtual const char* array(value_t& k, const char* beg, const char* end) {}
    virtual const char* value(value_t& k, const char* beg, const char* end, int ty) {}
};

struct jsax
{
    std::stack<jscalls*> stack_;

    const char *parse(jscalls* initial, const char* beg, const char* end)
    {
        stack_.push(initial);

        jscalls::value_t k;
        beg = this->walk(beg, end, k);

        stack_.pop();
        //assert (stack_.empty());

        return beg;
    }

    const char *array(const char* beg, const char* end, jscalls::value_t& k)
    {
        const char* sbeg = beg = skipss(beg, end);

        const char* p = skipss(beg+1, end);
        if (p == end)
            err;
        if (*p == *sbeg+2)
            return p + 1;

        do {
            ++beg;

            jscalls::value_t xk;
            if (*sbeg == '{')
            {
                beg = this->string(beg, end, xk.s);
                beg = skipss(beg, end);
                if (beg == end || *beg != ':')
                {
                    err;
                }
            }
            else
            { xk.i++; }

            beg = this->walk(beg, end, xk);

            beg = skipss(beg, end);
        } while (*beg == ',');

        if (*beg != *sbeg+2)
            err;

        return beg + 1;
    }

    const char *walk(const char* beg, const char* end, jscalls::value_t& k)
    {
        beg = skipss(beg, end);
        if (beg < end)
        {
            static jscalls jc_;
            jscalls* jc;

            int ty = 0;
            const char *vals[2];
            switch (*beg)
            {
                case '{': case '[':
                    jc = stack_.top()->array(k, beg, end);
                    stack_.push(jc ? jc : &jc_);
                    beg = this->array(beg, end);
                    stack_.pop();
                    return beg;

                case '"': case '\'':
                    beg = this->string(beg, end, vals);
                    ty = jscalls::Ty_Str;
                    break;

                case '+': case '-':
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    beg = this->digital(beg, end, vals);
                    ty = jscalls::Ty_Int;
                    break;

                case 'T': case 'F': case 't': case 'f':
                    beg = this->boolean(beg, end, vals);
                    ty = jscalls::Ty_Bool;
                    break;

                default:
                    err;
                    return beg;
            }

            stack_.top()->value(k, vals[0], vals[1], ty);
        }

        return beg;
    }

    const char* digital(const char* beg, const char* end, const char* ret[2])
    {
        const char* sbeg = beg = skipss(beg, end);
        if (*sbeg == '-' || *sbeg == '+')
            ++beg;
        for (; beg < end; ++beg)
            if (!isdigit(*beg) && *beg != '.')
                break;
        if (std::count(sbeg, beg, '.') > 1)
            err;
        ret[0] = sbeg;
        ret[1] = beg;
        return beg;
    }

    const char* string(const char* beg, const char* end, const char* ret[2])
    {
        const char* sbeg = beg = skipss(beg, end);
        beg = std::find(beg, end, *sbeg);
        if (beg == end)
            err;
        ret[0] = sbeg;
        ret[1] = beg;
        return beg+1;
    }

    const char* boolean(const char* beg, const char* end, const char* ret[2])
    {
        const char* sbeg = beg = skipss(beg, end);
        if (*sbeg == 'T' || *sbeg == 't')
        {
            if (end - beg < 4
                    || beg[1] != 'r' || beg[2] != 'u' || beg[3] != 'e')
                err;
            beg += 4;
        }
        else if (*sbeg == 'F' || *sbeg == 'f')
        {
            if (end - beg < 5
                    || beg[1] != 'a' || beg[2] != 'l' || beg[3] != 's' || beg[4] != 'e')
                err;
            beg += 5;
        }
        else
        {
            err;
        }

        ret[0] = sbeg;
        ret[1] = beg;
        return beg;
    }
}

int main(int ac, char *const av[])
{
    const char *v[2];
    v[0] = av[1];
    v[1] = av[1] + strlen(av[1]);

    jswalk(calls, v);

    return 0;
}

