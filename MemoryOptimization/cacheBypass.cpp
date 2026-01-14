#include <emmintrin.h>
#include <smmintrin.h>

/*
Cache bypass on write, use non temporal writes:
    Write to write combining buffer of the processor,
    which will flush 64 bytes directly to RAM, 
    without bring the 64bytes to Cache
 */
void setbytes(char *p, int c)
{
    // __m128i is a 16byte regiister
    // _mm_set_epi8 inits the register to 'c'
    __m128i i = _mm_set_epi8(c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c);
    // Stream the 64 bytes to Main Memory
    _mm_stream_si128((__m128i *)&p[0], i);
    _mm_stream_si128((__m128i *)&p[16], i);
    _mm_stream_si128((__m128i *)&p[32], i);
    _mm_stream_si128((__m128i *)&p[48], i);
}

/*
Cache Bypass on Read:
    Stream 64 bytes from Main Memory without polluting the Cache
*/
inline void read(char* p) {
    __m128i r1 = _mm_stream_load_si128((__m128i*)&p[0]);    // Loads line into buffer
    __m128i r2 = _mm_stream_load_si128((__m128i*)&p[16]); // Serviced from buffer only 
    __m128i r3 = _mm_stream_load_si128((__m128i*)&p[32]); // Serviced from buffer only 
    __m128i r4 = _mm_stream_load_si128((__m128i*)&p[48]); // Serviced from buffer only 
}

int main() {

}