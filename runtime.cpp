// This file is part of the IMP project.

#include <iostream>

#include "runtime.h"
#include "interp.h"

// -----------------------------------------------------------------------------
static void PrintInt(Interp &interp)
{
    auto v = interp.PeekInt();
    std::cout << v;
    interp.Push<int64_t>(v);
}

static void PrintBool(Interp &interp)
{
    auto v = interp.PeekBool();
    std::cout << (v ? "true" : "false");
    interp.Push<bool>(v);
}

// -----------------------------------------------------------------------------
static void ReadInt(Interp &interp)
{
    int64_t val;
    std::cin >> val;
    interp.Push<int64_t>(val);
}

// -----------------------------------------------------------------------------
std::map<std::string, RuntimeFn> kRuntimeFns = {
    {"print_int", PrintInt},
    {"read_int", ReadInt},
    {"print_bool", PrintBool}};
