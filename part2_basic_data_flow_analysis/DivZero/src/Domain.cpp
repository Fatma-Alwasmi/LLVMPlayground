#include "Domain.h"

//===----------------------------------------------------------------------===//
// Abstract Domain Implementation
//===----------------------------------------------------------------------===//

namespace dataflow {

/* Add your code here */

// Constructors
Domain::Domain() : Value(Uninit) {}
Domain::Domain(Element V) : Value(V) {}

static Domain MZ(Domain::MaybeZero);  // Maybe Zero
static Domain Z(Domain::Zero);        // Zero
static Domain NZ(Domain::NonZero);    // Non Zero
static Domain U(Domain::Uninit);      // Uninitialized

Domain* Domain::add(Domain* E1, Domain* E2){
    if(E1->Value == MaybeZero || E2->Value == MaybeZero || E1->Value == Uninit || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == Zero) return E2;
    if(E2->Value == Zero) return E1;

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &NZ;

    return &MZ;

}

Domain* Domain::sub(Domain* E1, Domain* E2){
    if(E1->Value == MaybeZero || E2->Value == MaybeZero || E1->Value == Uninit || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == Zero){
        if(E2->Value == Zero) return &Z; // 0 - 0 = 0
        if(E2->Value == NonZero) return &NZ; // 0 - x = -x (non zero)
    }

    if(E1->Value == Zero) return E1; // x - 0 = x

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &MZ; // x - y = maybe zero 
    return &MZ;
}

Domain* Domain::mul(Domain* E1, Domain* E2){
    if(E1->Value == Zero || E2->Value == Zero)
        return &Z;

    if(E1->Value == MaybeZero || E2->Value == MaybeZero || E1->Value == Uninit || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &NZ;    

    return &MZ;
}

Domain* Domain::div(Domain* E1, Domain* E2){
    if(E2->Value == Zero || E2->Value == MaybeZero || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == Zero) return &Z;

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &NZ;    

    return &MZ;
}

Domain* Domain::join(Domain* E1, Domain* E2){
    if(E1->Value == Uninit) return E2;
    if(E2->Value == Uninit) return E1;
    if(E1->Value == E2->Value) return E1;
    
    if((E1->Value == Zero && E2->Value == NonZero) ||
        (E1->Value == NonZero && E2->Value == Zero))
        return &MZ;
    return &MZ;
}

} // namespace dataflow