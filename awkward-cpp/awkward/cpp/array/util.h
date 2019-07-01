#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/complex.h>
#include <cinttypes>
#include <stdexcept>
#include <complex>
#include <sstream>
#include <iomanip>

namespace py = pybind11;

std::uint8_t byteswap(std::uint8_t val) {
    return val;
}

std::int8_t byteswap(std::int8_t val) {
    return val;
}

std::uint16_t byteswap(std::uint16_t val) {
    return (val << 8) | (val >> 8);
}

std::int16_t byteswap(std::int16_t val) {
    return (val << 8) | ((val >> 8) & 0xFF);
}

std::uint32_t byteswap(std::uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

std::int32_t byteswap(std::int32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

std::uint64_t byteswap(std::uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

std::int64_t byteswap(std::int64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

bool isNative(py::array input) {
    char ch = input.request().format.at(0);
    union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };
    return ((bint.c[0] == 1 && ch != '<') || (bint.c[0] != 1 && ch != '>'));
}

bool isNativeInt(py::array input) {
    std::string intList = "qQlLhHbB";
    if (intList.find(input.request().format.at(0)) == std::string::npos) {
        throw std::invalid_argument("argument must be of type int");
    }
    return isNative(input);
}

template <typename T>
void makeNative(py::array_t<T> input) {
    if (isNative(input)) {
        return;
    }
    py::buffer_info array_info = input.request();
    auto array_ptr = (T*)array_info.ptr;
    int N = array_info.shape[0] / array_info.itemsize;

    for (ssize_t i = 0; i < array_info.size; i++) {
        array_ptr[i * N] = byteswap(array_ptr[i * N]);
    }
    return;
}

void makeIntNative(py::array input) {
    if (isNativeInt(input)) {
        return;
    }
    char code = input.request().format.at(1);
    if (code == 'q') {
        makeNative(input.cast<py::array_t<std::int64_t>>());
        return;
    }
    if (code == 'Q') {
        makeNative(input.cast<py::array_t<std::uint64_t>>());
        return;
    }
    if (code == 'l') {
        makeNative(input.cast<py::array_t<std::int32_t>>());
        return;
    }
    if (code == 'L') {
        makeNative(input.cast<py::array_t<std::uint32_t>>());
        return;
    }
    if (code == 'h') {
        makeNative(input.cast<py::array_t<std::int16_t>>());
        return;
    }
    if (code == 'H') {
        makeNative(input.cast<py::array_t<std::uint16_t>>());
        return;
    }
    throw std::invalid_argument("argument must be of type int");
}

template <typename T>
py::array_t<T> slice_numpy(py::array_t<T> input, ssize_t start, ssize_t length, ssize_t step = 1) {
    ssize_t arrayLen = input.request().size;
    if (step == 0) {
        throw std::invalid_argument("slice step cannot be 0");
    }
    if (length < 0) {
        throw std::invalid_argument("slice length cannot be less than 0");
    }
    if (start < 0 || start >= arrayLen || start + (length * step) > arrayLen || start + (length * step) < -1) {
        throw std::out_of_range("slice must be in the bounds of the array");
    }
    py::buffer_info temp_info = py::buffer_info(input.request());
    temp_info.ptr = (void*)((T*)(input.request().ptr) + start);
    temp_info.size = length;
    temp_info.strides[0] = temp_info.strides[0] * step;
    temp_info.shape[0] = temp_info.size;
    return py::array_t<T>(temp_info);
}
