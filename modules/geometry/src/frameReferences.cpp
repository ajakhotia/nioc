////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022.                                                                             /
// Project  : Naksh                                                                                /
// Author   : Anurag Jakhotia                                                                      /
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <naksh/geometry/frameReferences.hpp>

namespace naksh::geometry::helpers
{

std::string frameCompositionErrorMessage(const std::string& lhsFrameName,
                                         const std::string& rhsFrameName)
{
    assert(lhsFrameName != rhsFrameName &&
           "Detected creation of frameCompositionErrorMessage with compatible frame ids. "
           "This is a programming error.");

    return "Composed transforms with mismatched inner frames. Lhs child frame[" + lhsFrameName +
           "] does not match the rhs parent frame[" + rhsFrameName + "].";
}

} // namespace naksh::geometry::helpers
