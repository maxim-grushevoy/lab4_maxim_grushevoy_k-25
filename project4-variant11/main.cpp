// Вимірювання часу виконання послідовностей дій над GuardedFields (варіант №11)
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <latch>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "guarded_fields.hpp"

using namespace std;
using Clock = chrono::steady_clock;

enum class OpType : unsigned char { Read, Write, Str };
struct Op
{
    OpType type;
    unsigned char field;
    int value;
};

static string pad(const string &s, size_t width)
{
    size_t chars = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80)
            ++chars;
    return s + string(width > chars ? width - chars : 0, ' ');
}

static vector<Op> load(const string &path)
{
    ifstream in(path);
    if (!in)
    {
        cerr << "Не вдалося відкрити " << path << endl;
        exit(1);
    }
    vector<Op> ops;
    string cmd;
    while (in >> cmd)
    {
        Op op{OpType::Str, 0, 0};
        if (cmd == "read")
        {
            int f;
            in >> f;
            op = {OpType::Read, static_cast<unsigned char>(f), 0};
        }
        else if (cmd == "write")
        {
            int f, v;
            in >> f >> v;
            op = {OpType::Write, static_cast<unsigned char>(f), v};
        }
        else if (cmd != "string")
        {
            cerr << "Невідома команда: " << cmd << endl;
            exit(1);
        }
        ops.push_back(op);
    }
    return ops;
}

static size_t worker(GuardedFields &data, const vector<Op> &ops, size_t from, size_t to,
                     latch &ready, latch &go)
{
    size_t checksum = 0;
    ready.count_down();
    go.wait();
    for (size_t i = from; i < to; ++i)
    {
        const Op &op = ops[i];
        switch (op.type)
        {
        case OpType::Read:
            checksum += static_cast<size_t>(data.get(op.field));
            break;
        case OpType::Write:
            data.set(op.field, op.value);
            break;
        case OpType::Str:
            checksum += string(data).size();
            break;
        }
    }
    return checksum;
}

static double run(const vector<Op> &ops, unsigned threads, size_t &sink, string &finalState)
{
    GuardedFields data; 
    latch ready(threads), go(1);
    vector<size_t> sums(threads, 0);
    vector<jthread> pool;
    size_t n = ops.size();
    for (unsigned t = 0; t < threads; ++t)
    {
        size_t from = n * t / threads, to = n * (t + 1) / threads;
        pool.emplace_back([&, t, from, to]
                          { sums[t] = worker(data, ops, from, to, ready, go); });
    }
    ready.wait();
    auto t0 = Clock::now();
    go.count_down();
    for (auto &th : pool)
        th.join();
    auto t1 = Clock::now();
    for (size_t s : sums)
        sink += s;
    finalState = string(data); 
    return chrono::duration<double, milli>(t1 - t0).count();
}

int main(int argc, char *argv[])
{
    string dir = argc > 1 ? argv[1] : ".";
    int repeats = argc > 2 ? atoi(argv[2]) : 5;
    const char *files[3] = {"a_variant.txt", "b_equal.txt", "c_skewed.txt"};
    const char *labels[3] = {"а) за умовою", "б) рівні частоти", "в) дуже відмінні"};
    double table[3][3];
    size_t sink = 0;

    for (int f = 0; f < 3; ++f)
    {
        vector<Op> ops = load(dir + "/" + files[f]);
        cout << "Файл " << files[f] << ": " << ops.size() << " операцій" << endl;
        for (unsigned th = 1; th <= 3; ++th)
        {
            double sum = 0;
            for (int r = 0; r < repeats; ++r)
            {
                string state;
                double ms = run(ops, th, sink, state);
                sum += ms;
                cout << "  потоків: " << th << ", запуск " << r + 1 << ": " << fixed
                     << setprecision(2) << ms << " мс, фінальний стан " << state << endl;
            }
            table[f][th - 1] = sum / repeats;
        }
    }

    cout << "\nСередній час (мс) за " << repeats << " запусків\n";
    cout << pad("Файл", 20) << pad("1 потік", 12) << pad("2 потоки", 12) << pad("3 потоки", 12) << endl;
    ofstream csv("results.csv");
    for (int f = 0; f < 3; ++f)
    {
        cout << pad(labels[f], 20);
        for (int t = 0; t < 3; ++t)
        {
            ostringstream cell;
            cell << fixed << setprecision(2) << table[f][t];
            cout << pad(cell.str(), 12);
            csv << cell.str() << (t < 2 ? ',' : '\n');
        }
        cout << endl;
    }
    cout << "(контрольна сума: " << sink << ")" << endl;
    return 0;
}
