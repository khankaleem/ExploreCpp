#include <new>
#include<vector>
#include <chrono> // The timing library
#include<iostream>
using namespace std;

std::size_t CLS = std::hardware_destructive_interference_size; // Cache line size

/*
Cache efficient multiplication of two matrices
Runtime: 5-6 millis
*/

void multiplyCacheEfficient(
              const vector<vector<double>>& A, 
              const vector<vector<double>>& B,
              size_t N,
              vector<vector<double>>& res) {
    size_t Block = CLS / sizeof(double);
    for (size_t i = 0; i < N; i += Block ) {
        for (size_t j = 0; j < N; j+= Block) {
            for (size_t k = 0; k < N; k += Block) {
                // multiply i - k block k - j block to get i-j block
                for (size_t i2 = 0; i2 < Block; i2++) {
                    for (size_t k2 = 0; k2 < Block; k2++) { // Change the order between k and j for Loop Interchange Optimization - saves 3 millis
                        for (size_t j2 = 0; j2 < Block; j2++) {
                            res[i+i2][j+j2] += A[i+i2][k+k2] * B[k+k2][j+j2];
                        }
                    }
                }
            }
        }
    }

}

/*
    Trivial algo: Runtime: 1200-1300 millis
*/
void multiply(const vector<vector<double>>& A, 
              const vector<vector<double>>& B,
              size_t N,
              vector<vector<double>>& res) {
    for (size_t i = 0; i < N; i += 1 ) {
        for (size_t j = 0; j < N; j+= 1) {
            for (size_t k = 0; k < N; k += 1) {
                res[i][j] += A[i][k]*B[k][j];
            }
        }
    }
}

int main() {
    {
        size_t N = 1024;
        vector<vector<double>> A(N, vector<double>(N, 0));
        vector<vector<double>> B(N, vector<double>(N, 0));
        for(auto i = 0; i < N; i++){
            for(auto j = 0; j < N; j++) {
                A[i][j] = i;
                B[i][j] = j;
            }
        }
        auto start = std::chrono::high_resolution_clock::now();
        vector<vector<double>> Res1(N, vector<double>(N, 0));
        multiplyCacheEfficient(A, B, N, Res1);
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        std::cout << "multiplyCacheEfficient took " << ns << " nanoseconds!\n";
    }
    {
        size_t N = 1024;
        vector<vector<double>> A(N, vector<double>(N, 0));
        vector<vector<double>> B(N, vector<double>(N, 0));
        for(auto i = 0; i < N; i++){
            for(auto j = 0; j < N; j++) {
                A[i][j] = i;
                B[i][j] = j;
            }
        }
        auto start = std::chrono::high_resolution_clock::now();
        vector<vector<double>> Res2(N, vector<double>(N, 0));
        multiply(A, B, N, Res2);
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        std::cout << "multiply took " << ns << " nanoseconds!\n";
    }

}
