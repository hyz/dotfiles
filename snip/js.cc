#include <ctype.h>
#include <string.h>
#include <string>
#include <stack>
#include <algorithm>
#include <iostream>

struct jsnode
{
    enum { Ty_Str=1, Ty_Int, Ty_Bool };

    virtual jsnode* Enter(const std::string& k, const char* beg, const char* end) { return 0; }
    virtual void Exit(const std::string& k, jsnode* nd) {}

    virtual jsnode* Enter(int k, const char* beg, const char* end) { return 0; }
    virtual void Exit(int k, jsnode* nd) {}

    virtual void Set(const std::string& k, const char* beg, const char* end, int ty) {}
    virtual void Set(int k, const char* beg, const char* end, int ty) {}

    virtual ~jsnode() {}
};

class jsax
{
    struct key_t {
        union {
            const char *s[2];
            int i;
        } u;
        int ty;

        key_t(int y=0) { u.s[0]=u.s[1]=0; ty = y; }
    };

    struct pxfail : std::exception
    {
        int errno;
        const char *pos;

        pxfail(const char *p=0, int ec = 1) {
            pos = p;
            errno = ec;
        }
    };

// #define ERR_R(p) do{err_ = 1; return p;}while(0)
#define ERR_R(p) do{ throw pxfail(p, 1); }while(0)

    // std::stack<jsnode*> stack_;
    // int err_;

public:
    const int parse(jsnode* nd, const char* beg, const char* end, const char **pos)
    {
        // stack_.push(initial);
        int ec = 0;

        try {
            beg = this->_Array(nd, beg, end);
        } catch (const pxfail& e) {
            beg = e.pos;
            ec = e.errno;
        } catch (...) {
            ec = -1;
        }

        if (pos)
            *pos = beg;
        return ec;

        // jsnode::value_t xk;
        // beg = this->walk(nd, beg, end, xk);

        // stack_.pop();
        //assert (stack_.empty());

        // return beg;
    }

    const char *_Array(jsnode* nd, const char* beg, const char* end)
    {
        const char* const sbeg = beg = skipss(beg, end);

        if (*beg != '{' && *beg != '[')
            ERR_R(beg);

        const char* p = skipss(beg+1, end);
        if (p == end)
            ERR_R(sbeg);
        if (*p == *sbeg+2)
            return p + 1;

        do {
            ++beg;

            key_t xk;
            if (*sbeg == '{')
            {
                xk.ty = '{';
                beg = this->_string(beg, end, xk.u.s);
                beg = skipss(beg, end);
                if (beg == end || *beg != ':')
                {
                    ERR_R(beg);
                }
                ++beg;
            }
            else
            {
                xk.ty = '[';
                xk.u.i++;
            }
            beg = this->walk(nd, beg, end, xk);

            beg = skipss(beg, end);
        } while (*beg == ',');

        if (*beg != *sbeg+2)
            ERR_R(beg);

        return beg + 1;
    }

private:
    static const char* skipss(const char* beg, const char* end)
    {
        for (; beg != end; ++beg)
            if (!isspace(*beg))
                break;
        return beg;
    }

    const char *walk(jsnode* pnd, const char* beg, const char* end, const key_t& xk)
    {
        beg = skipss(beg, end);
        if (beg < end)
        {
            static jsnode jc_;
            jsnode* jc;

            int ty = 0;
            const char *vals[2];
            switch (*beg)
            {
                case '{': case '[':
                    if ( (jc = this->_New(pnd, xk, beg, end)) == 0)
                        jc = &jc_;
                    beg = this->_Array(jc, beg, end);
                    this->_Done(pnd, xk, jc); // stack_.pop();
                    return beg;

                case '"': case '\'':
                    beg = this->_string(beg, end, vals);
                    ty = jsnode::Ty_Str;
                    break;

                case '+': case '-':
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    beg = this->_digital(beg, end, vals);
                    ty = jsnode::Ty_Int;
                    break;

                case 'T': case 'F': case 't': case 'f':
                    beg = this->_boolean(beg, end, vals);
                    ty = jsnode::Ty_Bool;
                    break;

                default:
                    ERR_R(beg);
            }

            this->_Set(pnd, xk, vals, ty);
        }

        return beg;
    }

    const char* _string(const char* beg, const char* end, const char* ret[2])
    {
        const char* const sbeg = beg = skipss(beg, end);
        if ((*beg != '"' && *beg != '\'') || beg+1 >= end)
            ERR_R(sbeg);
        beg = std::find(beg+1, end, *sbeg);
        if (beg == end)
            ERR_R(sbeg);
        ret[0] = sbeg+1;
        ret[1] = beg;
        return beg+1;
    }

    const char* _digital(const char* beg, const char* end, const char* ret[2])
    {
        const char* const sbeg = beg = skipss(beg, end);
        if (*sbeg == '-' || *sbeg == '+')
            ++beg;
        if (beg == end || !isdigit(*beg))
            ERR_R(sbeg);
        for (++beg; beg < end; ++beg)
            if (!isdigit(*beg) && *beg != '.')
                break;
        if (std::count(sbeg, beg, '.') > 1)
            ERR_R(sbeg);
        ret[0] = sbeg;
        ret[1] = beg;
        return beg;
    }

    const char* _boolean(const char* beg, const char* end, const char* ret[2])
    {
        const char* const sbeg = beg = skipss(beg, end);
        if (*sbeg == 'T' || *sbeg == 't')
        {
            if (end - beg < 4
                    || beg[1] != 'r' || beg[2] != 'u' || beg[3] != 'e')
                ERR_R(beg);
            beg += 4;
        }
        else if (*sbeg == 'F' || *sbeg == 'f')
        {
            if (end - beg < 5
                    || beg[1] != 'a' || beg[2] != 'l' || beg[3] != 's' || beg[4] != 'e')
                ERR_R(beg);
            beg += 5;
        }
        else
        {
            ERR_R(beg);
        }

        ret[0] = sbeg;
        ret[1] = beg;
        return beg;
    }

    jsnode* _New(jsnode *pnd, const key_t& xk, const char* beg, const char* end)
    {
        if (xk.ty == '[')
            return pnd->Enter(xk.u.i, beg, end);
        return pnd->Enter(std::string(xk.u.s[0], xk.u.s[1]), beg, end);
    }
    void _Done(jsnode *pnd, const key_t& xk, jsnode *nd)
    {
        if (xk.ty == '[')
            pnd->Exit(xk.u.i, nd);
        else
            pnd->Exit(std::string(xk.u.s[0], xk.u.s[1]), nd);
    }
    void _Set(jsnode *pnd, const key_t& xk, const char* v[2], int ty)
    {
        if (xk.ty == '[')
            pnd->Set(xk.u.i, v[0], v[1], ty);
        else
            pnd->Set(std::string(xk.u.s[0], xk.u.s[1]), v[0], v[1], ty);
    }
};

struct P : jsnode
{
    //enum { Ty_Str=1, Ty_Int, Ty_Bool };

    virtual jsnode* Enter(const std::string& k, const char* beg, const char* end) { return this; }
    virtual void Exit(const std::string& k, jsnode* nd) {}

    virtual jsnode* Enter(int k, const char* beg, const char* end) { return this; }
    virtual void Exit(int k, jsnode* nd) {}

    virtual void Set(const std::string& k, const char* beg, const char* end, int ty) {
        std::cout << k << "=" << std::string(beg,end) << "\n";
    }
    virtual void Set(int k, const char* beg, const char* end, int ty) {
        std::cout << k << ":" << std::string(beg,end) << "\n";
    }
};

int main(int ac, char *const av[])
{
    P root;
    jsax jsp;

    std::string line;
    while (std::getline(std::cin, line))
    {
        const char* beg = &line[0];
        const char* end = beg + line.size();
        while (beg < end) {
            const char* sbeg = beg;
            int ec = jsp.parse(&root, beg, end, &beg);
            if (ec)
            {
                break;
            }

            std::cout.write(sbeg, beg - sbeg);
            std::cout << "\n";
        }
    }

    return 0;
}

