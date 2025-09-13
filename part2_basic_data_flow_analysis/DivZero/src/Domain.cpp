#include "Domain.h"

//===----------------------------------------------------------------------===//
// Abstract Domain Implementation
//===----------------------------------------------------------------------===//

/* Add your code here */


Domain* Domain::add(Domain* E1, Domain* E2){
    if(E1->Value == MaybeZero || E2->Value == MaybeZero || E1->Value == Uninit || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == Zero) return E2;
    if(E2->Value == Zero) return E1;

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &MZ;

    return &NZ;

}

Domain* Domain::sub(Domain* E1, Domain* E2){
    if(E1->Value == MaybeZero || E2->Value == MaybeZero || E1->Value == Uninit || E2->Value == Uninit)
        return &MZ;

    if(E1->Value == Zero) return E2;
    if(E2->Value == Zero) return E1;

    if(E1->Value == NonZero && E2->Value == NonZero)
        return &MZ;

    return &NZ;
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