#include <stdio.h>

#include "num.h"
#include "consts.h"
#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"

using namespace secp256k1;

int main() {
    Context ctx;
    FieldElem f1,f2;
    f1.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846115");
//    f2.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846115");
    printf("%s\n",f1.ToString().c_str());
//    printf("%s\n",f2.ToString().c_str());
    for (int i=0; i<1000000; i++) {
        f1.SetInverse_Number(ctx,f1);
//        f2.SetInverse_(f2);
//        if (!(f1 == f2)) {
//          printf("f1 %i: %s\n",i,f1.ToString().c_str());
//          printf("f2 %i: %s\n",i,f2.ToString().c_str());
//        }
//        f1 *= 2;
//        f2 *= 2;
    }
    printf("%s\n",f1.ToString().c_str());
//    printf("%s\n",f2.ToString().c_str());
    return 0;
}
