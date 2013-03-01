
struct jscalls
{
    union K { const char *s[2]; int idx; K(){s[0]=s[1]=0;} };

    virtual jscalls() {}
    virtual const char* array(const char* beg, const char* end, union K& k) {}
    virtual const char* value(const char* beg, const char* end, union K& k) {}

    // int key(const char* begin, const char* end);
    // int value(const char* begin, const char* end);
};

struct jsp
{
    jscalls::K key_;
    std::stack<jscalls*> stack_;

    const char *array(const char* beg, const char* end, jscalls::K& k)
    {
        static jscalls jc_;

        jscalls* jc = stack_.top()->array(beg, end, k);
        stack_.push(jc ? jc : &jc_);

        const char* sbeg = beg = skipss(beg, end);

        const char* p = skipss(beg+1, end);
        if (p == end)
            err;
        if (*p == '}')
            goto Retpos;

        do {
            ++beg;

            jscalls::K xk;
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

        if (*beg != '}')
            err;

Retpos:
        stack_.pop();
        return beg + 1;
    }

    const char *walk(const char* beg, const char* end, jscalls::K& k)
    {
        beg = skipss(beg, end);
        if (beg < end)
        {
            const char *vals[2];
            switch (*beg)
            {
                case '{': case '[':
                    return this->array(beg, end, k);
                case '"': case '\'':
                    beg = this->string(beg, end, vals);
                    break;
                case '+': case '-':
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    beg = this->digital(beg, end, vals);
                    break;
                case 'T': case 'F': case 't': case 'f':
                    beg = this->boolean(beg, end, vals);
                    break;
                default:
                    err;
                    return beg;
            }
            stack_.top()->value(vals[0], vals[1], k);
        }

        return beg;
    }

    const char* digital(const char* beg, const char* end, const char* ret[2])
    {
    }

    const char* string(const char* beg, const char* end, const char* ret[2])
    {
    }

    const char* boolean(const char* beg, const char* end, const char* ret[2])
    {
        if not in ("True", "False", "true", "false")
            err;
    }
}

const char* jswrap(const char *ins[2], const char *outs[2])
{
}

int main(int ac, char *const av[])
{
    const char *v[2];
    v[0] = av[1];
    v[1] = av[1] + strlen(av[1]);

    jswalk(calls, v);

    return 0;
}

