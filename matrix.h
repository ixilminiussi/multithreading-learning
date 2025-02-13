#ifndef MATRIX
#define MATRIX

#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

namespace matrix
{

inline int nearestFour(int n)
{
    return (n + 3) & ~3;
}

class Matrix
{
  public:
    Matrix(int cols, int rows) : _rows{rows}, _cols{cols}
    {
        _data.resize(100);
    }
    double &operator()(int i, int j);
    double operator()(int i, int j) const;
    std::vector<double> getCol(int i) const;
    std::vector<double> getRow(int i) const;

    int getColSize() const
    {
        return _cols;
    }
    int getRowSize() const
    {
        return _rows;
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix &m);

  private:
    int _rows;
    int _cols;
    std::vector<double> _data{};
};

inline Matrix parallel(const Matrix &a, const Matrix &b);

inline std::ostream &operator<<(std::ostream &os, const Matrix &m)
{
    for (int i = 0; i < m._rows; i++)
    {
        os << "[ ";
        for (int j = 0; j < m._cols; j++)
        {
            os << m(j, i) << " ";
        }
        os << "]" << std::endl;
    }
    return os;
}

inline double operator*(const std::vector<double> &&a, const std::vector<double> &&b)
{
    double r{0.0};

    if (a.size() != b.size())
    {
        throw("cannot multiply vectors of different sizes");
    }

    for (int i = 0; i < a.size(); i++)
    {
        r += a[i] * b[i];
    }

    return r;
}

inline Matrix operator*(const Matrix &a, const Matrix &b)
{
    return matrix::parallel(a, b);
}

inline std::vector<double> Matrix::getCol(int i) const
{
    std::vector<double> col(_rows);

    for (int j = 0; j < _rows; j++)
    {
        col.push_back((*this)(i, j));
    }

    return col;
}

inline std::vector<double> Matrix::getRow(int j) const
{
    std::vector<double> row(_cols);

    for (int i = 0; i < _cols; i++)
    {
        row.push_back((*this)(i, j));
    }

    return row;
}

inline double &Matrix::operator()(int i, int j)
{
    return _data[i * _cols + j];
}

inline double Matrix::operator()(int i, int j) const
{
    return _data[i * _cols + j];
}

class CalcIndex
{
  public:
    CalcIndex(Matrix *m, const Matrix &a, const Matrix &b, int i, int j) : _m{m}, _a{a}, _b{b}, _i{i}, _j{j} {};

    void operator()()
    {
        (*_m)(_i, _j) = _a.getRow(_i) * _b.getCol(_j);
    }

  private:
    Matrix *_m;
    const Matrix &_a, &_b;
    int _i, _j;
};

inline Matrix sequencial(const Matrix &a, const Matrix &b)
{
    if (a.getColSize() != b.getRowSize() || a.getRowSize() != b.getColSize())
    {
        throw("cannot multiply matrices with incompatible sizes");
    }

    Matrix m(a.getRowSize(), b.getColSize());

    for (int i = 0; i < a.getRowSize(); i++)
    {
        for (int j = 0; j < b.getColSize(); j++)
        {
            m(i, j) = a.getRow(i) * b.getCol(j);
        }
    }

    return m;
}

inline Matrix parallel(const Matrix &a, const Matrix &b)
{
    if (a.getColSize() != b.getRowSize() || a.getRowSize() != b.getColSize())
    {
        throw("cannot multiply matrices with incompatible sizes");
    }

    Matrix m(a.getRowSize(), b.getColSize());

    std::vector<std::thread> threads;

    for (int i = 0; i < a.getRowSize(); i++)
    {
        for (int j = 0; j < b.getColSize(); j++)
        {
            CalcIndex calc(&m, a, b, i, j);
            threads.push_back(std::thread(calc));
        }
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    return m;
}

} // namespace matrix

#endif
