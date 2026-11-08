// Генератор тестових файлів: write <поле> <значення> / read <поле> / string
#include <array>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

using namespace std;

static void generate(const string &name, const array<double, 7> &w, size_t n, unsigned seed)
{
    mt19937 gen(seed);
    discrete_distribution<int> dist(w.begin(), w.end());
    ofstream out(name);
    for (size_t k = 0; k < n; ++k)
    {
        int c = dist(gen);
        if (c == 6)
            out << "string\n";
        else if (c % 2 == 0)
            out << "read " << c / 2 << '\n';
        else
            out << "write " << c / 2 << " 1\n";
    }
    cout << "Створено " << name << " (" << n << " операцій)\n";
}

int main(int argc, char *argv[])
{
    size_t n = argc > 1 ? stoull(argv[1]) : 3'000'000;
    string dir = argc > 2 ? argv[2] : ".";
    generate(dir + "/a_variant.txt", {10, 5, 10, 5, 10, 20, 40}, n, 1);
    
    generate(dir + "/b_equal.txt", {1, 1, 1, 1, 1, 1, 1}, n, 2);
    generate(dir + "/c_skewed.txt", {1, 80, 2, 9, 0.5, 7, 0.5}, n, 3);
}
