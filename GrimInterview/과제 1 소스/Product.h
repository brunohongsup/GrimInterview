
#include "pch.h"

struct Product
{
    CTime InputTime;
    CString Id;

    Product(const CString& id) : Id(id) {}

    Product() = delete;

    
};