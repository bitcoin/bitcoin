package mcl

/*
#cgo bn256 CFLAGS:-DMCLBN_FP_UNIT_SIZE=4
#cgo bn384 CFLAGS:-DMCLBN_FP_UNIT_SIZE=6
#cgo bn384_256 CFLAGS:-DMCLBN_FP_UNIT_SIZE=6 -DMCLBN_FR_UNIT_SIZE=4
#cgo bn256 LDFLAGS:-lmclbn256 -lmcl
#cgo bn384 LDFLAGS:-lmclbn384 -lmcl
#cgo bn384_256 LDFLAGS:-lmclbn384_256 -lmcl
#include <mcl/bn.h>
*/
import "C"
import "fmt"

// Init --
// call this function before calling all the other operations
// this function is not thread safe
func Init(curve int) error {
	err := C.mclBn_init(C.int(curve), C.MCLBN_COMPILED_TIME_VAR)
	if err != 0 {
		return fmt.Errorf("ERR mclBn_init curve=%d", curve)
	}
	return nil
}
