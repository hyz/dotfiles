#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include "bencode_detail.h"
#include "utils/HKCmdPacket.h"

#if defined(_WIN32) && !defined(snprintf)
#  define snprintf sprintf_s
#endif

typedef const char* (*decode_func_t)(void*, const char*, const char*);

static const char* decode_int(IntType* val, const char* beg, const char* end);
static const char* decode_string(struct StrType* s, const char* beg, const char* end);
static const char* decode_list(struct List* l, const char* beg, const char* end);
static const char* decode_any(struct Any* any, const char* beg, const char* end);

static int encode_string(char buf[], size_t len, struct StrType* s);
static int encode_list(char buf[], size_t len, struct List* l);
static int encode_any(char buf[], size_t len, struct Any* any);

static struct Any* copy_any(struct Any* a, struct Any* src);
static struct StrType* copy_string(struct StrType* s, struct StrType* src);
static struct List* copy_list(struct List* l, struct List* src);

static void destroy_list(struct List* l);
static void destroy_string(struct StrType* s);
static void destroy_any(struct Any* a);

const char* decode_int(IntType* val, const char* beg, const char* end)
{
    const char *e, *savbeg = beg;
    if (*beg != 'i')
        return beg;
    ++beg;
    e = (char*)memchr(beg, 'e', end-beg);
    if (!e)
        return savbeg;
    *val = atoi(beg);
    return e+1;
}

const char* decode_size(size_t *size, const char* beg, const char* end)
{
    const char* c = (char*)memchr(beg, ':', end-beg);
    if (!c)
        return beg;
    *size = atoi(beg);
    if (*size > (size_t)(end-(c+1)))
        return beg;
    return c+1;
}

const char* decode_string(struct StrType* s, const char* beg, const char* end)
{
    size_t len;
    const char* e = decode_size(&len, beg, end);
    if (e != beg)
    {
        str_assign(s, e, len);
        return e + len;
    }
    return beg;
}

struct Any* list_append(struct List* l, struct Any* a)
{
    if (l->length % 8 == 0)
    {
        l->buf = (struct Any*)realloc(l->buf, sizeof(l->buf[0]) * (l->length + 8));
    }
    memset(&l->buf[l->length], 0, sizeof(l->buf[0]));
    return copy_any(&l->buf[l->length++], a);
}

static int _dict_index(struct Dict* d, const char* key)
{
    unsigned int x;
    for (x = 0; x < d->length; ++x)
    {
        if (strcmp(key, d->buf[x].key) == 0)
            return (int)x;
    }
    return -1;
}

static struct Pair* dict_find(struct Dict* d, const char* key)
{
    int x = _dict_index(d, key);
    return (x<0 ? 0 : &d->buf[x]);
}

struct Any* dict_get(struct Dict* d, const char* key)
{
    struct Pair* p = dict_find(d, key);
    return (p ? &p->val : 0);
}

//struct Any* dict_setdefault(struct Dict* d, const char* key, struct Any* a) { }

struct Any* dict_set(struct Dict* d, const char* key, struct Any* a)
{
    struct Pair* p = dict_find(d, key);
    if (p)
    {
        return copy_any(&p->val, a);
    }
    if (d->length % 8 == 0)
    {
        d->buf = (struct Pair*)realloc(d->buf, sizeof(d->buf[0]) * (d->length + 8));
    }
    p = &d->buf[d->length];
    p->key = strdup(key);
    memset(&p->val, 0, sizeof(p->val));
    copy_any(&p->val, a);
    ++d->length;
    return &p->val;
}

struct Dict* dict_delete(struct Dict* d, const char* key)
{
    int x = _dict_index(d, key);
    if (x >= 0)
    {
        free(d->buf[x].key);
        destroy_any(&d->buf[x].val);
        d->buf[x] = d->buf[--d->length];
    }
    return d;
}

struct Dict* dict_add(struct Dict* d, struct Dict* src)
{
    unsigned int x;
    for (x = 0; x < src->length; ++x)
    {
        dict_set(d, src->buf[x].key, &src->buf[x].val);
    }
    return d;
}

#define N_B10(x) ((x)<10?1:((x)<100?2:((x)<1000?3:((x)<10000?4:((x)<100000?5:(6))))))

static int strlen_of_b10(int num)
{
    int n = 0;
    if (num < 0)
    {
        ++n;
        num = -num;
    }
    for (++n, num /= 10; num > 0; num /= 10)
        ++n;
    return n;
}

unsigned int dict_sum_encoded_size(struct Dict* d)
{
    unsigned int x, sum = 2;
    for (x = 0; x < d->length; ++x)
    {
        if (*d->buf[x].key)
        {
            int n = (int)strlen(d->buf[x].key);
            sum += strlen_of_b10(n) + 1 + n;
            switch (d->buf[x].val.type)
            {
                case Type_Int:
                    n = d->buf[x].val.u.integer;
                    sum += strlen_of_b10(n) + 2;
                    break;
                case Type_String:
                    n = d->buf[x].val.u.str.length;
                    sum += strlen_of_b10(n) + 1 + n;
                    break;
                case Type_Dict:
                    sum += dict_sum_encoded_size(&d->buf[x].val.u.dict);
                    break;
                case Type_List:
                    assert(0);
                    break;
                default:
                    break;
            }
        }
    }
    return sum;
}

struct Dict* dict_update(struct Dict* d, const char* beg, const char* end)
{
    struct Dict tmp = {0};
    if (end == decode_dict(&tmp, beg, end))
    {
		unsigned int x;
        for (x = 0; x < tmp.length; ++x)
        {
            dict_set(d, tmp.buf[x].key, &tmp.buf[x].val);
        }
        destroy_dict(&tmp);
    }
    return d;
}

struct StrType* str_assign(struct StrType* s, const char* v, size_t len)
{
    s->buf = (char*)realloc(s->buf, len+1);
    if (len)
        memcpy(s->buf, v, len);
    s->buf[s->length = (int)len] = '\0';
    return s;
}

void destroy_string(struct StrType* s)
{
    if (s->buf)
        free(s->buf);
    s->length = 0;
    s->buf = 0;
}

struct StrType* copy_string(struct StrType* s, struct StrType* src)
{
    return str_assign(s, src->buf, src->length);
}

struct List* copy_list(struct List* l, struct List* src)
{
    unsigned int x;
    if (l->length > 0)
        destroy_list(l);
    for (x=0; x < src->length; ++x)
        list_append(l, &src->buf[x]);
    return l;
}

struct Dict* copy_dict(struct Dict* d, struct Dict* src)
{
    unsigned int x;
    if (d->length > 0)
        destroy_dict(d);
    for (x=0; x < src->length; ++x)
        dict_set(d, src->buf[x].key, &src->buf[x].val);
    return d;
}

void destroy_list(struct List* l)
{
    unsigned int x;
    for (x = 0; x < l->length; ++x)
        destroy_any(l->buf + x);
    l->length = 0;
    if (l->buf)
        free(l->buf);
    l->buf = 0;
}

void destroy_dict(struct Dict* d)
{
    unsigned int x;
    for (x = 0; x < d->length; ++x)
    {
        free(d->buf[x].key);
        destroy_any(&d->buf[x].val);
    }
    d->length = 0;
    if (d->buf)
        free(d->buf);
    d->buf = 0;
}

struct Any* copy_any(struct Any* dest, struct Any* src)
{
    destroy_any(dest);
    dest->type = src->type;
    switch (src->type)
    {
        case Type_String:
            copy_string(&dest->u.str, &src->u.str);
            break;
        case Type_List:
            copy_list(&dest->u.list, &src->u.list);
            break;
        case Type_Dict:
            copy_dict(&dest->u.dict, &src->u.dict);
            break;
        default:
            *dest = *src;
    }
    return dest;
}

void destroy_any(struct Any* a)
{
    switch (a->type)
    {
        case Type_String:
            destroy_string(&a->u.str);
            break;
        case Type_List:
            destroy_list(&a->u.list);
            break;
        case Type_Dict:
            destroy_dict(&a->u.dict);
            break;
        default:
            break;
    }
    a->type = Type_Unknown;
}

const char* decode_list(struct List* l, const char* beg, const char* end)
{
    const char* const savbeg = beg;
    if (*beg != 'l')
        return beg;
    ++beg;
    while (beg < end && *beg != 'e')
    {
        struct Any a = { 0 };
        const char *e = decode_any(&a, beg, end);
        if (e != beg)
            list_append(l, &a);
        destroy_any(&a);
        beg = e;
    }
    if (beg >= end)
    {
        destroy_list(l);
        return savbeg;
    }
    return beg+1;
}

const char* decode_dict(struct Dict* d, const char* beg, const char* end)
{
    const char* const savbeg = beg;
    if (*beg != 'd')
        return beg;
    ++beg;
    while (beg < end && *beg != 'e')
    {
        char key[128];
        size_t len;
        const char *e = decode_size(&len, beg, end);
        if (e != beg && len < sizeof(key))
        {
            struct Any a = { 0 };
            if (len)
                memcpy(key, e, len);
            key[len] = '\0';
            beg = e + len;
            e = decode_any(&a, beg, end);
            if (e != beg)
            {
                dict_set(d, key, &a);
                destroy_any(&a);
                beg = e;
                continue;
            }
        }
        beg = savbeg;
        break;
    }
    if (beg >= end || beg == savbeg)
    {
        destroy_dict(d);
        return savbeg;
    }
    return beg+1; // 'e'
}

const char* decode_any(struct Any* any, const char* beg, const char* end)
{
    switch (*beg)
    {
        case 'd':
            any->type = Type_Dict;
            beg = decode_dict(&any->u.dict, beg, end);
            break;
        case 'l':
            any->type = Type_List;
            beg = decode_list(&any->u.list, beg, end);
            break;
        case 'i':
            any->type = Type_Int;
            beg = decode_int(&any->u.integer, beg, end);
            break;
        default:
            any->type = Type_String;
            beg = decode_string(&any->u.str, beg, end);
            break;
    }
    return beg;
}

int encode_string(char buf[], size_t len, struct StrType* s)
{
    int n = snprintf(buf, len, "%u:", s->length);
    if (len-n < s->length)
        return 0;
    memcpy(buf+n, s->buf, s->length);
    return n + (int)s->length;
}

int encode_list(char buf[], size_t len, struct List* l)
{
    int x, n = 1;
    if (len < 2)
        return 0;
    buf[0] = 'l';
    for (x = 0; (unsigned int)x < l->length; ++x)
    {
        int m = n;
        n += encode_any(buf+n, len-1-n, &l->buf[x]);
        if (n == m)
            return 0;
    }
    buf[n] = 'e';
    return n+1;
}

int encode_dict(char buf[], size_t len, struct Dict* d)
{
    int x, m, n = 1;
    if (len < 2)
        return 0;
    buf[0] = 'd';
    for (x = 0; (unsigned int)x < d->length; ++x)
    {
        struct Pair* p = &d->buf[x];
        if (*p->key)
        {
            m = n;
            n += snprintf(buf+n, len-1-n, "%u:%s", strlen(p->key), p->key);
            if (n > m)
            {
                m = n;
                n += encode_any(buf+n, len-1-n, &p->val);
            }
            if (n == m)
                return 0;
        }
    }
    buf[n] = 'e';
    return n+1;
}

int encode_any(char buf[], size_t len, struct Any* any)
{
    switch (any->type)
    {
        case Type_Int:
            return snprintf(buf, len, "i%de", any->u.integer);
        case Type_String:
            return encode_string(buf, len, &any->u.str);
        case Type_List:
            return encode_list(buf, len, &any->u.list);
        case Type_Dict:
            return encode_dict(buf, len, &any->u.dict);
        default: break;
    }
    return 0;
}

// "10:0=%d;1=%u;"
int dict_format(char buf[], size_t len, const char* cfmt, ...)
{
    int x, n, npair = atoi(cfmt);
    va_list ap;
    char *fmt = 0, *fmts[96];
#define RETURN_X(x) { if(fmt)free(fmt); return x; }
    if (npair > sizeof(fmts))
        return 0;
    for (x = 0; x < npair; ++x)
        fmts[x] = "%s";
    if ( (cfmt = strchr(cfmt, ':')) && cfmt[1])
    {
        char *p = fmt = strdup(cfmt+1);
        while (*p)
        {
            if ( (x = atoi(p)) >= npair)
                RETURN_X(0);
            while (isdigit(*p++))
                ;
            if (*p != '=' || *++p != '%')
                RETURN_X(0);
            fmts[x] = p;
            if (!(p = strchr(p, ';')))
                RETURN_X(0);
            *p++ = '\0';
        }
    }
    va_start(ap, cfmt);
    x = n = 0;
    buf[n++] = 'd';
    for (; x < npair; ++x)
    {
        char *k = va_arg(ap, char *);
        if (k)
        {
            int m = n;
            if (fmts[x][1] == 's')
            {
                char *s = va_arg(ap, char *);
                n += snprintf(buf+n, len-n-1, "%d:%s%d:%s", (int)strlen(k), k, (int)strlen(s), s);
            }
            else if (fmts[x][1] == 'd')
            {
                int d = va_arg(ap, int);
                n += snprintf(buf+n, len-n-1, "%d:%si%de", (int)strlen(k), k, d);
            }
            else if (fmts[x][1] == 'u')
            {
                unsigned int u = va_arg(ap, unsigned int);
                n += snprintf(buf+n, len-n-1, "%d:%si%ue", (int)strlen(k), k, u);
            }
            else
                break;
            if (n <= m)
                break;
        }
    }
    va_end(ap);
    if (x < npair)
        RETURN_X(0);
    buf[n++] = 'e';
    RETURN_X(n);
}

