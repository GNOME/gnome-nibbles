#include <glib.h>
struct _int128
{
    guint64  hi;
    guint64  lo;
    gboolean negative;
    guint64  remainder;
};
void divide_by(struct _int128 *i128, long long i64, struct _int128 *r)
{
    unsigned __int128 n,a;
    n = ((__int128)i128->hi) << 64 | i128->lo;
    if(i64 < 0)
    {
        a = n / -(__int128)i64;
        r->remainder = n % -(__int128)i64;
    }
    else
    {
        a = n / i64;
        r->remainder = n % i64;
    }
    r->hi = a >> 64;
    r->lo = (guint64)a;
    r->negative = (i128->negative && i64 > 0) || (!i128->negative && i64 < 0);
}
