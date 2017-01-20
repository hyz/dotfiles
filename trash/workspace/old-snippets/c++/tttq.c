#ifdef _WIN32
#  ifdef _CRTDBG_MAP_ALLOC
#    include <stdlib.h>
#    include <crtdbg.h>
#  endif
#  include <windows.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "sys/strutils.h"
#include "sys/sysdecl.h"
#include "sys/rsevt.h"


#ifdef WIN32
# ifdef NO_W32_PTHREAD
    typedef int pthread_attr_t;
    typedef int pthread_t;
#   define pthread_attr_init(...) ((void)0)
#   define pthread_attr_setdetachstate(...) ((void)0)
#   define pthread_create(...) ((void)0)
#   define pthread_attr_destroy(...) ((void)0)
    typedef int pthread_mutex_t;
#   define pthread_mutex_init(...) ((void)0)
#   define pthread_mutex_lock(...) ((void)0)
#   define pthread_mutex_unlock(...) ((void)0)
    typedef int pthread_cond_t;
#   define pthread_cond_init(...) ((void)0)
#   define pthread_cond_wait(...) ((void)0)
#   define pthread_cond_signal(...) ((void)0)
# else
#   include "hksys/pthread.h"
# endif
#endif

struct response
{
    response_func_t func;
    struct args_t args;
};

struct timedq
{
    // unsigned long int seconds;
    void* last_f;
    struct timedexc* head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t thread;
};

struct timedexc
{
    struct timedexc* next;
    struct timespec reltime;
    func_argv_t func;
    void* self;
    unsigned int length;
    char* argv[1];
};

int append_response(struct responser* respr, response_func_t cf, struct args_t* args)
{
    assert(respr);
    if (cf)
    {
        if ((respr->length & 0x07) == 0)
        {
            struct response* vec = respr->vec;
            vec = realloc(vec, sizeof(vec[0]) * (respr->length + 0x08));
            if (!vec)
            {
                return -1;
            }
            respr->vec = vec;
        }
        respr->vec[respr->length].func = cf;
        respr->vec[respr->length].args = *args;
        return (respr->length++);
    }
    return 0;
}

int respond(struct responser* respr)
{
    unsigned int x = 0;
    assert(respr);
    for (; x < respr->length; ++x)
    {
        respr->vec[x].func(&respr->vec[x].args);
    }
    if (x > 0)
    {
        free(respr->vec);
        respr->length = 0;
        respr->vec = 0;
    }
    return (int)x;
}

#ifdef unix
#  define TS_GET_CURRENT(ts) clock_gettime(CLOCK_REALTIME, ts)
#else
#  define TIMESPEC_TO_FILETIME_OFFSET (((__int64)27111902 << 32) + (__int64)3577643008)
#  define TS_GET_CURRENT(ts) do{ \
    FILETIME ft; GetSystemTimeAsFileTime(&ft); \
    (ts)->tv_sec = (int)((*(__int64*)&ft - TIMESPEC_TO_FILETIME_OFFSET) / 10000000); \
    (ts)->tv_nsec = (int)((*(__int64*)&ft - TIMESPEC_TO_FILETIME_OFFSET - ((__int64)(ts)->tv_sec * (__int64)10000000)) * 100); \
}while(0)
//GetTickCount()
#endif

#define TS_PLUS_MSEC(ts, msec) do{ \
    unsigned int n = (msec); \
    (ts)->tv_sec += n / 1000; \
    n =  (n % 1000) * 1000000 + (ts)->tv_nsec; \
    (ts)->tv_sec += n / 1000000000; \
    (ts)->tv_nsec = n % 1000000000; \
}while(0)
#define TS_LESS_THAN(x, y) ((x)->tv_sec < (y)->tv_sec || ((x)->tv_sec == (y)->tv_sec && (x)->tv_nsec < (y)->tv_nsec))

static struct timedexc* insort(struct timedexc* head, struct timedexc* xc)
{
    struct timedexc* p = head;
    if (!head || TS_LESS_THAN(&xc->reltime, &head->reltime))
    {
        xc->next = head;
        return xc;
    }
    while (p->next && !TS_LESS_THAN(&xc->reltime, &p->next->reltime))
    {
        p = p->next;
    }
    xc->next = p->next;
    p->next = xc;
    return head;
}

static void exit_f(int msec, void* my, int len, char* argv[])
{
}

static void invoke(struct timedexc* head, struct timespec ts)
{
    while (head)
    {
        unsigned int msec;
        struct timedexc* x = head;
        head = head->next;
        msec = 1000 * (x->reltime.tv_sec - ts.tv_sec);
        msec += (x->reltime.tv_nsec - ts.tv_nsec) / 1000000;
        x->func(msec, x->self, x->length, x->argv);
        free(x);
    }
}

static void* tq_thread_func(void* a)
{
    int ec = 0;
    struct timedq* tq = (struct timedq*)a;
    struct timedexc *x, *head = 0;
    struct timespec ts; //unsigned int tv;
    while (tq->last_f != exit_f)
    {
        pthread_mutex_lock(&tq->mutex);
        while (!tq->head)
        {
            ec = pthread_cond_wait(&tq->cond, &tq->mutex);
            if (ec && ec != ETIMEDOUT)
            {
                HKLOG(L_ERR, "pthread_cond_wait: %d\n", ec);
            }
        }
        TS_GET_CURRENT(&ts);
        while (TS_LESS_THAN(&ts, &tq->head->reltime))
        {
            ts = tq->head->reltime;
            ec = pthread_cond_timedwait(&tq->cond, &tq->mutex, &ts);
            if (ec && ec != ETIMEDOUT)
            {
                HKLOG(L_ERR, "pthread_cond_wait: %d\n", ec);
            }
            TS_GET_CURRENT(&ts);
        }
        x = head = tq->head;
        while (x->next && !TS_LESS_THAN(&ts, &x->next->reltime))
        {
            x = x->next;
        }
        tq->head = x->next;
        x->next = 0;
        pthread_mutex_unlock(&tq->mutex);
        invoke(head, ts);
    }
    if (tq->head)
    {
        ts = tq->head->reltime;
        ts.tv_nsec -= 1000000;
        invoke(tq->head, ts);
        tq->head = 0;
    }
    return a;
}

static void* _tq_post(struct timedq* q, unsigned int msec, struct timedexc* tc)
{
    if (tc && tc->func)
    {
        TS_GET_CURRENT(&tc->reltime);
        TS_PLUS_MSEC(&tc->reltime, msec);
        pthread_mutex_lock(&q->mutex);
        if (q->last_f != exit_f)
        {
            q->head = insort(q->head, tc);
            q->last_f = tc->func;
            pthread_cond_signal(&q->cond);
        }
        pthread_mutex_unlock(&q->mutex);
        return tc;
    }
    return 0;
}

static unsigned int length_sum(unsigned int argc, const char* argv[])
{
    unsigned int x, len = 0;
    for (x = 0; x < argc; ++x)
    {
        len += (int)strlen(argv[x]);
    }
    return len;
}

static struct timedexc* newexc(func_argv_t f, void* a, unsigned int ac, unsigned int len)
{
    struct timedexc* tc;
    len += sizeof(struct timedexc) + sizeof(char*) * ac + ac; //length_sum(ac, argv) + ac;
    tc = (struct timedexc*)malloc(len);
    if (tc)
    {
        memset(tc, 0, sizeof(*tc));
        tc->func = f;
        tc->self = a;
        tc->length = ac;
    }
    return tc;
}

struct timedexc* newexc_v(func_argv_t f, void* my, unsigned int c, unsigned int len, va_list ap)
{
    struct timedexc* tc;
    const char* arg = va_arg(ap, const char*);
    if (!arg)
    {
        return newexc(f, my, c, len);
    }
    tc = newexc_v(f, my, c+1, len + strlen(arg), ap);
    if (tc)
    {
        tc->argv[c] = (char*)&tc->argv[tc->length] + len + c;
        strcpy(tc->argv[c], arg);
    }
    return tc;
}

void* tq_post(struct timedq* q, unsigned int msec, func_argv_t f, void* a, unsigned int argc, const char* argv[])
{
    struct timedexc* tc = newexc(f, a, argc, length_sum(argc, argv));
    if (tc)
    {
        unsigned int x, len;
        for (len = x = 0; x < argc; ++x)
        {
            tc->argv[x] = (char*)&tc->argv[tc->length] + len + x;
            strcpy(tc->argv[x], argv[x]);
            len += (int)strlen(argv[x]);
        }
        if (_tq_post(q, msec, tc))
        {
            return tc;
        }
        free(tc);
    }
    return 0;
}

//static int va_list_argv(const char* v[], int len, va_list ap)
//{
//    int x = 0;
//    while (x < len && (v[x] = va_arg(ap, const char*)))
//        ++x;
//    return x;
//}

void* tq_post_v(struct timedq* q, unsigned int msec, func_argv_t f, void* my, ...)
{
    struct timedexc* tc;
    va_list ap;
    va_start(ap, my);
    tc = newexc_v(f, my, 0, 0, ap);
    va_end(ap);
    if (tc)
    {
        if (_tq_post(q, msec, tc))
        {
            return tc;
        }
        free(tc);
    }
    return 0;
}

#if 0
int tq_post_binary(struct timedq* q, unsigned int msec, void (*f)(int,void*,void*,unsigned int), void* a, void* pvoid, unsigned int len)
{
    if (q->seconds > 0)
    {
        struct timedexc* tc = (struct timedexc*)calloc(sizeof(*tc) + len, 1);
        if (tc)
        {
            tc->func = f;
            tc->self = a;
            tc->length = len;
            tc->argv = (char**)&tc->memory[1];
            if (len > 0)
                memcpy(&tc->memory[1], pvoid, len);
            tc->memory[0] = 'b';
            if (!_tq_post(q, msec, tc))
            {
                free(tc);
                return 0;
            }
            return 1;
        }
    }
    return 0;
}
#endif

struct SyncArg
{
    func_argv_t real_func;
    void* real_arg;
    pthread_mutex_t mutex;
};

static void sync_f(int msec, void* a, int argc, char* argv[])
{
    struct SyncArg* self = a;
    self->real_func(msec, self->real_arg, argc, argv);
    pthread_mutex_unlock(&self->mutex);
}

void* tq_send(struct timedq* q, unsigned int msec, func_argv_t f, void* a0, ...)
{
    struct timedexc* tc;
    va_list vas;
    struct SyncArg self;
    memset(&self, 0, sizeof(self));
    self.real_func = f;
    self.real_arg = a0;
    va_start(vas, a0);
    tc = newexc_v(sync_f, &self, 0, 0, vas); //x = va_list_argv(v, 128, vas)
    va_end(vas);
    if (tc)
    {
        pthread_mutex_init(&self.mutex, 0);
        pthread_mutex_lock(&self.mutex);
        if (!(a0 = _tq_post(q, msec, tc)))
            free(tc);
        pthread_mutex_lock(&self.mutex);
        pthread_mutex_unlock(&self.mutex);
        pthread_mutex_destroy(&self.mutex);
        return a0;
    }
    return 0;
}
// int tq_send_v(struct timedq* q, unsigned int msec, func_argv_t f, void* a, unsigned int argc, char* argv[])
// {
//     int x = 0;
//     struct timedexc* tc = calloc(sizeof(*tc) + sizeof(char*) * argc, 1);
//     if (tc)
//     {
//         struct SyncArg self;
//         memset(&self, 0, sizeof(self));
//         self.real_func = f;
//         self.real_arg = a;
//         pthread_mutex_init(&self.mutex, 0);
//         pthread_mutex_lock(&self.mutex);
//         tc->func = sync_f;
//         tc->self = &self;
//         tc->length = argc;
//         memcpy(tc->argv, argv, sizeof(char*) * argc);
//         if (!(x = _tq_post(q, msec, tc)))
//             free(tc);
//         pthread_mutex_lock(&self.mutex);
//         pthread_mutex_unlock(&self.mutex);
//         pthread_mutex_destroy(&self.mutex);
//     }
//     return x;
// }

struct MixArg
{
    // void*(*pred)(void*, void (*f)(void*,unsigned int, char**), void*, unsigned int, char**);
    func_pred_t pred;
    void* self;
    struct timedq* q;
};

static void _cancel_if(int msec, void* pvoid, int len, char* argv[])
{
    struct MixArg* a = pvoid;
    if (a->q->head)
    {
        struct timedexc yield = { 0 };
        pthread_mutex_lock(&a->q->mutex);
        {
            struct timedexc h, *p = &h, *x = &yield;
            h.next = a->q->head;
            while (p->next)
            {
                if (a->pred(a->self, p->next->func, p->next->self, p->next->length, p->next->argv))
                {
                    x->next = p->next;
                    x = x->next;
                    p->next = x->next;
                }
                else
                {
                    p = p->next;
                }
            }
            x->next = 0;
            a->q->head = h.next;
        }
        pthread_mutex_unlock(&a->q->mutex);
        if (yield.next)
        {
            struct timespec ts = yield.next->reltime;
            ts.tv_nsec -= 1000000;
            invoke(yield.next, ts);
        }
    }
}

void tq_cancel_if(struct timedq* q, func_pred_t pred, void* self)
{
    struct MixArg a = { pred, self, q };
    tq_send(q, 0, _cancel_if, &a, 0, 0);
}

struct timedq* tq_create()
{
    struct timedq* q = malloc(sizeof(*q));
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mutex, 0);
    pthread_cond_init(&q->cond, 0);
    {
        pthread_attr_t a;
        pthread_attr_init(&a);
        // pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
        pthread_attr_setdetachstate(&a, PTHREAD_CREATE_JOINABLE);
        pthread_create(&q->thread, &a, tq_thread_func, q);
        pthread_attr_destroy(&a);
    }
    return q;
}

void tq_destroy(struct timedq* q)
{
    //pthread_mutex_lock(&mutex);
    //tq_send(q, 0, exit_f, &mutex, 0, 0);
    tq_post_v(q, 0, exit_f, NULL, NULL);
    pthread_join(q->thread, 0);
    pthread_cond_destroy(&q->cond);
    pthread_mutex_destroy(&q->mutex);
    free(q);
}

