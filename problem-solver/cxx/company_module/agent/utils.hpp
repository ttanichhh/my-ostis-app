#pragma once
#include <sc-memory/sc_memory.hpp>

inline ScAddr resolve_arg(ScMemoryContext & context, const ScAddr & setAddr, const ScAddr & rrelAddr)
{
    ScIterator5Ptr it = context.CreateIterator5(
        setAddr,
        ScType::ConstPermPosArc,
        ScType::Unknown,
        ScType::ConstPermPosArc,
        rrelAddr
    );

    if (it->Next())
        return it->Get(2);

    return ScAddr();
}
