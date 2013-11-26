//
//  ElementComparator.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 26/11/13.
//
//
#include <functional>

#ifndef SwivelAutotune_ElementComparator_h
#define SwivelAutotune_ElementComparator_h

template <class ElementType>
class ElementComparator
{
public:
    ElementComparator (std::function<int (ElementType, ElementType)> f) : compare(f) {};
    
    int compareElements ( ElementType first, ElementType second ) const
    {
        return compare(first, second);
    }
private:
    const std::function<int (ElementType, ElementType)> compare;
};

#endif
