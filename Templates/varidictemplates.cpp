/*
    Some varidic template implementation
*/

//Compile time nCr computation
template<int N, int R>
constexpr int nCr() {
    static_assert(N >= 0, "nCr: N must be >= 0");
    static_assert(R >= 0, "nCr: R must be >= 0");
    if constexpr (R > N) {
        return 0;
    } else if constexpr(N == R || R == 0) {
        return 1;
    } else {
        return nCr<N-1, R-1>() + nCr<N-1, R>();
    }
}


int main() {
    static_assert(nCr<6, 2>() == 15);
}
