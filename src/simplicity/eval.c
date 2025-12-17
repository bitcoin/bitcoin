#include "eval.h"

#include <string.h>
#include "bounded.h"
#include "limitations.h"
#include "simplicity_alloc.h"
#include "simplicity_assert.h"

/* We choose an unusual representation for frames of the Bit Machine.
 *
 * An 'n'-bit frame is stored in the array of 'UWORD's of length 'l' where 'l' is the least value such that 'n <= l * UWORD_BIT'.
 * Thus there may be extra "padding" bits in the array when 'n < l * UWORD_BIT'.
 *
 * We choose to store the frames bits in a sequence with the first bits in the last element of the array and
 * the last bits in the first element of the array.
 * Within a 'UWORD' array element, the bits of the frame are stored with the first bits in the most significant positions
 * and the last bits in the least significant positions.
 * We choose to put padding bits entirely within the most significant bits of the last element of the array.
 *
 * Thus the last bit of the frame will always be the least significant bit of the first element of the array.
 * When there are no padding bits, the first bit of the frame will be the most significant bit of the last element of the array.
 * When there are padding bits, the first bit of the frame will occur at the most significant non-padding bit.
 *
 * More precisely, bit 'm' of an 'n'-bit frame (with '0 <= m < n') is the bit at position '1 << ((n-m-1) % UWORD_BIT)'
 * of the element of the array at index '(n-m-1 / UWORD_BIT)'.
 *
 * 0-bit frames are allowed, in which case the array will have length 0.
 *
 * Rationale:
 *
 * The Bit Machine's standard library of jets operates using a "big endian" representation of integers
 * from the Bit Machine's perspective.
 * It is often the case that we encounter types that are sums of various integers sizes.
 * For example, the Bitcoin primitive 'outputValue : TWO^32 |- ONE + TWO^64' has a target type
 * that is the sum of 64-bit integer (with a 0-bit integer).
 *
 * When a frame is generated from a type such as 'ONE + TWO^64' our representation places the tag for this type
 * by itself as the least significant bit of the last element of the frame's array (as long as 'UWORD_BIT' divides 64).
 * When this frame contains a value of the right-hand type, 'TWO^64', this value entirely fits perfectly within
 * the within the first elements of the array (again, as long as 'UWORD_BIT' divides 64).
 * Furthermore, if 'UWORD_BIT == 8', then this representation place this value of type 'TWO^64'
 * into the machine's memory in little endian byte order.
 *
 * All of the above means that when jets need to marshal values from the Bit Machine's representation
 * to the architecture's representation, it will often be the case that the data is already byte-aligned and
 * in the correct order for little endian processors.
 * When a jet marshals a architecture-sized word, and 'UWORD' is the architecture's native integer size, then
 * it will often be the case that the data is word-aligned (for both big and little endian processors).
 * Only the case when 'UWORD_BIT == 8' and architecture's processor is big-endian will the compiler need to emit
 * byte-swapping instructions.
 *
 * Nevertheless, our implementation is independent of architecture and will function correctly on all architectures
 * for any value of UWORD_BIT.
 *
 * Note: while we do attempt make the fast path for marshaling values for jets common, when assigning discounts to jets
 * it is important to only consider the worst case, slow path, behaviour, as good byte or bit alignment is not guaranteed in
 * presence of oddly shaped pairs of values.
 */

/* The main memory used by the Bit Machine during execution is contained in a single allocation of an array of 'UWORD's
 * named 'cells'.
 * The read and write frames used by the Bit Machine during execution are slices of this single array allocation.
 * We represent the read frame and write frame stacks within 'cells' using a [gap buffer](https://en.wikipedia.org/wiki/Gap_buffer).
 * The frames of the read frame stack are assigned to the beginning of the cell array
 * with the active read frame occurring as the last of these frames.
 * The frames of the write frame stack are assigned to the end of the cell array
 * with the active write frame occurring as the first of these frames.
 * This leaves a (possibly empty) gap of unused UWORDs between the '.edge' of the active read frame
 * and the '.edge' of the active write frame.
 * This gap will shrink / grow / move during the execution of the Bit Machine.
 * Thus whether a particular UWORD from 'cells' belongs to some read frame or write frame will vary during the execution.
 * Static analysis determines a safe size that is acceptable for the 'cells' array.
 */

/* To keep track of the individual frames of the read frame and write frame stacks we another single allocation of
 * an array of 'frameItem's called 'frames'.
 * This 'frames' array is another instance of a [gap buffer](https://en.wikipedia.org/wiki/Gap_buffer).
 * The read frames are tracked by 'frameItem's occurring at the beginning of the 'frames' array
 * with the active read frame tracked the last of these 'frameItem's.
 * The write frames are tracked by 'frameItem's occurring at the end of the 'frames' array
 * with the active write frame tracked the first of these 'frameItem's.
 * This leaves a (possibly empty) gap of unused 'frameItem's between the item that tracks active read frame
 * and the item that tracks the active write frame.
 * This gap will shrink / grow / move during the execution of the Bit Machine.
 * Thus whether a particular 'frameItem' from 'frames' tracks a read frame or write frame will vary during the execution.
 * The 'frameItem' that tracks the active read frame is located at 'state.activeReadFrame'.
 * The 'frameItem' that tracks the active write frame is located at 'state.activeWriteFrame'.
 * There is always an active read frame and an active write frame, though these frames are initially of size 0
 * when evaluating Simplicity programs.
 * Static analysis determines a safe size that is acceptable for the 'frames' array.
 */

/* When a 'frameItem' tracks a read frame, its '.edge' field points to the UWORD from 'cell' that is
 * one-past-the-end of the 'cells' slice that makes up that frame.
 * The '.offset' value indirectly tracks the position of the read frame's cursor.
 * A cursor at the beginning of a read frame is denoted by an '.offset' value equal to that frame's padding.
 * When the frame has no padding, a cursor at the beginning of a read frame is denoted by an '.offset' of 0.
 * For each subsequent cursor position within the read frame, the '.offset' increments by one.
 * When the cursor is at (one-cell past) the end of the read frame, the '.offset' value will be equal to the total number of bits
 * allocated for the frame (including padding bits), which is necessarily some multiple of (UWORD_BIT).
 * We say "a read frame is valid for /n/ more cells" when '.edge - ROUND_UWORD(.offset + n)' points to a
 * 'UWORD[ROUND_UWORD(.offset + n)]' array of initialized values.
 * We say "a read frame is valid" if it is valid for 0 more cells.
 *
 * When a 'frameItem' tracks a write frame, its '.edge' field points the UWORD from 'cell' that is
 * the first element of the 'cells' slice that makes up that frame.
 * The '.offset' value indirectly tracks the position of the write frame's cursor.
 * A cursor at the beginning of a read frame is denoted by an '.offset' value equal to
 * that frame's number of bits (excluding padding).
 * For each subsequent cursor position within the write frame, the '.offset' decrements by one.
 * When the cursor is at (one-cell past) the end of the write frame, the '.offset' value will be equal to 0.
 * We say "a write frame is valid for /n/ more cells" when '.edge' points to an 'UWORD[ROUND_UWORD(.offset)]' array of
 * initialized values and 'n <= .offset'.
 * We say "a write frame is valid" if it is valid for 0 more cells.
 *
 * Notice that the interpretation of the fields of a 'frameItem' depends on whether the 'frameItem' is tracking a read frame or
 * a write frame.
 */

/* Given a read frame, advance its cursor by 'n' cells.
 *
 * Precondition: NULL != frame.
 */
static void forward(frameItem* frame, size_t n) {
  frame->offset += n;
}

/* Given a read frame, move its cursor backwards by 'n' cells.
 *
 * Precondition: n <= frame->offset
 */
static void backward(frameItem* frame, size_t n) {
  simplicity_debug_assert(n <= frame->offset);
  frame->offset -= n;
}

/* Given a write frame, advance its cursor by 'n' cells.
 *
 * Precondition: n <= frame->offset
 */
static void skip(frameItem* frame, size_t n) {
  simplicity_debug_assert(n <= frame->offset);
  frame->offset -= n;
}

/* Given a compact bit representation of a value 'v : B', write the value 'v' to a write frame, skipping cells as needed
 * and advance its cursor.
 * Cells in front of the '*dst's cursor's final position may be overwritten.
 *
 * :TODO: Consider writing an optimized version of this function for word type 'TWO^(2^n)' which is a very common case and
 * doesn't need any skipping of cells.
 *
 * Precondition: '*dst' is a valid write frame for 'bitSize(B)' more cells;
 *               'compactValue' is a compact bitstring representation of a value 'v : B';
 *               'type_dag[typeIx]' is a type dag for the type B.
 */
static void writeValue(frameItem* dst, const bitstring* compactValue, size_t typeIx, type* type_dag) {
  size_t cur = typeSkip(typeIx, type_dag);
  size_t offset = 0;
  bool calling = true;
  setTypeBack(cur, type_dag, 0);
  while (cur) {
    if (SUM == type_dag[cur].kind) {
      simplicity_debug_assert(calling);

      /* Write one bit to the write frame and then skip over any padding bits. */
      bool bit = getBit(compactValue, offset);
      offset++;
      writeBit(dst, bit);
      skip(dst, pad(bit, type_dag[type_dag[cur].typeArg[0]].bitSize, type_dag[type_dag[cur].typeArg[1]].bitSize));

      size_t next = typeSkip(type_dag[cur].typeArg[bit], type_dag);
      if (next) {
        setTypeBack(next, type_dag, type_dag[cur].back);
        cur = next;
      } else {
        cur = type_dag[cur].back;
        calling = false;
      }
    } else {
      simplicity_debug_assert(PRODUCT == type_dag[cur].kind);
      size_t next;
      if (calling) {
        next = typeSkip(type_dag[cur].typeArg[0], type_dag);
        /* Note: Because we are using 'typeSkip' we have an invarant on 'cur' such that whenever type_dag[cur].kind == PRODUCT,
           then it is a product of two non-trival types.  This implies that 'next' cannot actually be 0. */
        if (next) {
          /* Traverse the first element of the product type, if it has any data. */
          setTypeBack(next, type_dag, cur);
          cur = next;
          continue;
        }
      }
      next = typeSkip(type_dag[cur].typeArg[1], type_dag);
      /* Note: Because we are using 'typeSkip' we have an invarant on 'cur' such that whenever type_dag[cur].kind == PRODUCT,
         then it is a product of two non-trival types.  This implies that 'next' cannot actually be 0. */
      if (next) {
        /* Traverse the second element of the product type, if it has any data. */
        setTypeBack(next, type_dag, type_dag[cur].back);
        cur = next;
        calling = true;
      } else {
        cur = type_dag[cur].back;
        calling = false;
      }
    }
  }
  /* Note: Above we use 'typeSkip' to skip over long chains of products against trivial types
   * This avoids a potential DOS vulnerability where a DAG of deeply nested products of unit types with sharing is traversed,
   * taking exponential time.
   * While traversing still could take exponential time in terms of the size of the type's dag,
   * at least one bit of witness data is required per PRODUCT type encountered.
   * This ought to limit the total number of times through the above loop to no more that 3 * compactValue->len.
   */
}

/* Our representation of the Bit Machine state consists of a gap buffer of 'frameItem's.
 * The gap buffer is allocated at 'frame'
 * The read frames of the gap buffer extends from the beginning of the buffer to '.activeReadFrame'.
 * The write frames extend from the end of the buffer down to '.activeWriteFrame'.
 */
typedef struct evalState {
  frameItem* activeReadFrame;
  frameItem* activeWriteFrame;
} evalState;

/* 'call' is an item is used to track the "call stack" of the Bit Machine during evaluation.
 * Each call stack frame remembers where to return to after the call and a set of flags to hold various bits of state.
 */
typedef struct call {
  size_t return_to;
  flags_type flags;
} call;

#define FLAG_TCO        0x01 // Whether TCO is on (1) or off (0).
#define FLAG_LAST_CASE  0x02 // For case combinators, last branch executed was right (1) or left (0).
#define FLAG_EXEC       0x10 // Whether this combinator has ever been executed (1) or not (0).
#define FLAG_CASE_LEFT  0x20 // For case combinators, whether the left branch has ever been executed (1) or not (0).
#define FLAG_CASE_RIGHT 0x40 // For case combinators, whether the right branch has ever been executed (1) or not (0).

static inline bool get_tco_flag(const call *stack) {
  return FLAG_TCO == (stack->flags & FLAG_TCO);
}

static inline void set_tco_flag(call *stack, bool flag) {
  if (flag) {
     stack->flags |= FLAG_TCO;
  } else {
     stack->flags &= (flags_type)(~FLAG_TCO);
  }
}

static inline bool get_case_last_flag(const call *stack) {
  return FLAG_LAST_CASE == (stack->flags & FLAG_LAST_CASE);
}

static inline void set_case_last_flag(call *stack, bool flag) {
  if (flag) {
     stack->flags |= FLAG_LAST_CASE;
  } else {
     stack->flags &= (flags_type)(~FLAG_LAST_CASE);
  }
}

/* Starting from the Bit Machine 'state',
 * run the machine with the TCO (off) program generated by the well-typed Simplicity expression 'dag[len]' of type 'A |- B'.
 * If some jet execution fails, returns 'SIMPLICITY_ERR_EXEC_JET'.
 * If some 'assertr' or 'assertl' combinator fails, returns 'SIMPLICITY_ERR_EXEC_ASSERT'.
 * Otherwise returns 'SIMPLICITY_NO_ERROR'.
 *
 * The 'state' of the Bit Machine is whatever the state is after the last successfully executed Bit Machine instruction.
 *
 * ** No heap allocations are allowed in 'runTCO' or any of its subroutines. **
 *
 * Precondition: The gap between 'state.activeReadFrame' and 'state.activeWriteFrame' is sufficient for execution of 'dag'
 *                 and the values are initialized;
 *               The gap between 'activeReadFrame(state)->edge' and 'activeWriteFrame(state)->edge'
 *                 is sufficient for execution of 'dag';
 *               '*activeReadFrame(state)' is a valid read frame for 'bitSize(A)' more cells.
 *               '*activeWriteFrame(state)' is a valid write frame for 'bitSize(B)' more cells.
 *               call stack[len];
 *               for all i < len, stack[i].flags = 0;
 *               dag_node dag[len] and 'dag' is well-typed with 'type_dag';
 *               if 'dag[len]' represents a Simplicity expression with primitives then 'NULL != env';
 */
static simplicity_err runTCO(evalState state, call* stack, const dag_node* dag, type* type_dag, size_t len, const txEnv* env) {
/* The program counter, 'pc', is the current combinator being interpreted.  */
  size_t pc = len - 1;

/* 'stack' represents the interpreter's call stack.
 * However, the stack is not directly represented as an array.
 * Instead, the bottom of the call stack is located at 'stack[len - 1]' and the top of the call stack is located at 'stack[pc]'.
 * The intermediate call stack items are somewhere between 'pc' and 'len - 1'.
 * The each call stack item references the one below it through the 'stack[i].return_to' values.
 * The bottom of the stack's '.return_to' value is set to 'len' which is an out-of-bounds index.
 *
 * During CALLs, a new 'stack[i]' value is created where 'i' is index of the combinator being called,
 * and 'stack[i].return_to' is set to the current 'pc' value.
 * During TAIL_CALLs, 'stack[i].return_to' is instead set to the 'stack[pc].return_to' value.
 * During RETURNs, the 'pc' is set to the 'stack[pc].return_to' value.
 * TAIL_CALLs allows for faster returns within the interpreter (unrelated to Simplicity's TCO),
 * skipping over intermediate combinators.
 */
  stack[pc].return_to = len;
  set_tco_flag(&stack[pc], false);

/* 'calling' lets us know if we are entering a CALL or returning from a CALL. */
  bool calling = true;

  /* :TODO: Use static analysis to limit the number of iterations through this loop. */
  while(pc < len) {
    stack[pc].flags |= FLAG_EXEC;
    tag_t tag = dag[pc].tag;
    simplicity_debug_assert(state.activeReadFrame < state.activeWriteFrame);
    simplicity_debug_assert(state.activeReadFrame->edge <= state.activeWriteFrame->edge);
    if (dag[pc].jet) {
      if(!dag[pc].jet(state.activeWriteFrame, *state.activeReadFrame, env)) return SIMPLICITY_ERR_EXEC_JET;
      /* Like IDEN and WITNESS, we want to "fallthrough" to the UNIT case. */
      tag = UNIT;
    }
    switch (tag) {
     case COMP:
      if (calling) {
        /* NEW_FRAME(BITSIZE(B)) */
        *(state.activeWriteFrame - 1) = initWriteFrame(type_dag[COMP_B(dag, type_dag, pc)].bitSize, state.activeWriteFrame->edge);
        state.activeWriteFrame--;

        /* CALL(dag[pc].child[0], SAME_TCO) */
        stack[dag[pc].child[0]].return_to = pc;
        set_tco_flag(&stack[dag[pc].child[0]], get_tco_flag(&stack[pc]));
        pc = dag[pc].child[0];
      } else {
        /* MOVE_FRAME */
        simplicity_debug_assert(0 == state.activeWriteFrame->offset);
        memmove( state.activeReadFrame->edge, state.activeWriteFrame->edge
               , (size_t)((state.activeWriteFrame + 1)->edge - state.activeWriteFrame->edge) * sizeof(UWORD)
               );
        *(state.activeReadFrame + 1) = initReadFrame(type_dag[COMP_B(dag, type_dag, pc)].bitSize, state.activeReadFrame->edge);
        state.activeWriteFrame++; state.activeReadFrame++;

        /* TAIL_CALL(dag[pc].child[1], true) */
        calling = true;
        stack[dag[pc].child[1]].return_to = stack[pc].return_to;
        set_tco_flag(&stack[dag[pc].child[1]], true);
        pc = dag[pc].child[1];
      }
      break;
     case ASSERTL:
     case ASSERTR:
     case CASE:
      if (calling) {
        bool bit = peekBit(state.activeReadFrame);

        if (bit) {
           stack[pc].flags |= FLAG_CASE_RIGHT;
        } else {
           stack[pc].flags |= FLAG_CASE_LEFT;
        }

        /* FWD(1 + PADL(A,B) when bit = 0; FWD(1 + PADR(A,B) when bit = 1 */
        forward(state.activeReadFrame, 1 + pad( bit
                                     , type_dag[CASE_A(dag, type_dag, pc)].bitSize
                                     , type_dag[CASE_B(dag, type_dag, pc)].bitSize));

        /* CONDITIONAL_TAIL_CALL(dag[pc].child[bit]); */
        stack[dag[pc].child[bit]].return_to = get_tco_flag(&stack[pc]) ? stack[pc].return_to : pc;
        set_tco_flag(&stack[dag[pc].child[bit]], get_tco_flag(&stack[pc]));

        /* Remember the bit we peeked at for the case when we return. */
        set_case_last_flag(&stack[pc], bit);

        pc = dag[pc].child[bit];
      } else {
        /* BWD(1 + PADL(A,B) when bit = 0; BWD(1 + PADR(A,B) when bit = 1 */
        backward(state.activeReadFrame, 1 + pad( get_case_last_flag(&stack[pc])
                                      , type_dag[CASE_A(dag, type_dag, pc)].bitSize
                                      , type_dag[CASE_B(dag, type_dag, pc)].bitSize));

        /* RETURN; */
        pc = stack[pc].return_to;
      }
      break;
     case PAIR:
      if (calling) {
        /* CALL(dag[pc].child[0], false); */
        stack[dag[pc].child[0]].return_to = pc;
        set_tco_flag(&stack[dag[pc].child[0]], false);
        pc = dag[pc].child[0];
      } else {
        /* TAIL_CALL(dag[pc].child[1], SAME_TCO); */
        calling = true;
        stack[dag[pc].child[1]].return_to = stack[pc].return_to;
        set_tco_flag(&stack[dag[pc].child[1]], get_tco_flag(&stack[pc]));
        pc = dag[pc].child[1];
      }
      break;
     case DISCONNECT:
      if (calling) {
        /* NEW_FRAME(BITSIZE(WORD256 * A)) */
        *(state.activeWriteFrame - 1) = initWriteFrame(type_dag[DISCONNECT_W256A(dag, type_dag, pc)].bitSize,
                                                       state.activeWriteFrame->edge);
        state.activeWriteFrame--;

        /* WRITE_HASH(dag[dag[pc].child[1]].cmr) */
        write32s(state.activeWriteFrame, dag[dag[pc].child[1]].cmr.s, 8);

        /* COPY(BITSIZE(A)) */
        simplicity_copyBits(state.activeWriteFrame, state.activeReadFrame, type_dag[DISCONNECT_A(dag, type_dag, pc)].bitSize);

        if (get_tco_flag(&stack[pc])) {
          /* DROP_FRAME */
          state.activeReadFrame--;
        }

        /* MOVE_FRAME */
        simplicity_debug_assert(0 == state.activeWriteFrame->offset);
        memmove( state.activeReadFrame->edge, state.activeWriteFrame->edge
               , (size_t)((state.activeWriteFrame + 1)->edge - state.activeWriteFrame->edge) * sizeof(UWORD)
               );
        *(state.activeReadFrame + 1) = initReadFrame(type_dag[DISCONNECT_W256A(dag, type_dag, pc)].bitSize,
                                                     state.activeReadFrame->edge);
        state.activeWriteFrame++; state.activeReadFrame++;

        /* NEW_FRAME(BITSIZE(B * C)) */
        *(state.activeWriteFrame - 1) = initWriteFrame(type_dag[DISCONNECT_BC(dag, type_dag, pc)].bitSize,
                                                       state.activeWriteFrame->edge);
        state.activeWriteFrame--;

        /* CALL(dag[pc].child[0], true) */
        stack[dag[pc].child[0]].return_to = pc;
        set_tco_flag(&stack[dag[pc].child[0]], true);
        pc = dag[pc].child[0];
      } else {
        /* MOVE_FRAME */
        simplicity_debug_assert(0 == state.activeWriteFrame->offset);
        memmove( state.activeReadFrame->edge, state.activeWriteFrame->edge
               , (size_t)((state.activeWriteFrame + 1)->edge - state.activeWriteFrame->edge) * sizeof(UWORD)
               );
        *(state.activeReadFrame + 1) = initReadFrame(type_dag[DISCONNECT_BC(dag, type_dag, pc)].bitSize,
                                                     state.activeReadFrame->edge);
        state.activeWriteFrame++; state.activeReadFrame++;

        /* COPY(BITSIZE(B)) */
        simplicity_copyBits(state.activeWriteFrame, state.activeReadFrame, type_dag[DISCONNECT_B(dag, type_dag, pc)].bitSize);

        /* FWD(BITSIZE(B)) */
        forward(state.activeReadFrame, type_dag[DISCONNECT_B(dag, type_dag, pc)].bitSize);

        /* TAIL_CALL(dag[pc].child[1], true) */
        calling = true;
        stack[dag[pc].child[1]].return_to = stack[pc].return_to;
        set_tco_flag(&stack[dag[pc].child[1]], true);
        pc = dag[pc].child[1];
      }
      break;
     case INJL:
     case INJR:
      /* WRITE(0) when INJL; WRITE(1) when INJR */
      writeBit(state.activeWriteFrame, INJR == dag[pc].tag);

      /* SKIP(PADL(A,B)) when INJL; SKIP(PADR(A,B)) when INJR */
      skip(state.activeWriteFrame, pad( INJR == dag[pc].tag
                                 , type_dag[INJ_B(dag, type_dag, pc)].bitSize
                                 , type_dag[INJ_C(dag, type_dag, pc)].bitSize));
      /*@fallthrough@*/
     case TAKE:
      simplicity_debug_assert(calling);
      /* TAIL_CALL(dag[pc].child[0], SAME_TCO); */
      stack[dag[pc].child[0]].return_to = stack[pc].return_to;
      set_tco_flag(&stack[dag[pc].child[0]], get_tco_flag(&stack[pc]));
      pc = dag[pc].child[0];
      break;
     case DROP:
      if (calling) {
        /* FWD(BITSIZE(A)) */
        forward(state.activeReadFrame, type_dag[PROJ_A(dag, type_dag, pc)].bitSize);

        /* CONDITIONAL_TAIL_CALL(dag[pc].child[0]); */
        stack[dag[pc].child[0]].return_to = get_tco_flag(&stack[pc]) ? stack[pc].return_to : pc;
        set_tco_flag(&stack[dag[pc].child[0]], get_tco_flag(&stack[pc]));
        pc = dag[pc].child[0];
      } else {
        /* BWD(BITSIZE(A)) */
        backward(state.activeReadFrame, type_dag[PROJ_A(dag, type_dag, pc)].bitSize);

        /* RETURN; */
        pc = stack[pc].return_to;
      }
      break;
     case IDEN:
     case WORD:
     case WITNESS:
      if (IDEN == tag) {
        /* COPY(BITSIZE(A)) */
        simplicity_copyBits(state.activeWriteFrame, state.activeReadFrame, type_dag[IDEN_A(dag, type_dag, pc)].bitSize);
      } else {
        writeValue(state.activeWriteFrame, &dag[pc].compactValue, dag[pc].targetType, type_dag);
      }
      /*@fallthrough@*/
     case UNIT:
      simplicity_debug_assert(calling);
      if (get_tco_flag(&stack[pc])) {
        /* DROP_FRAME */
        state.activeReadFrame--;
      }

      /* RETURN; */
      calling = false;
      pc = stack[pc].return_to;
      break;
     case HIDDEN: return SIMPLICITY_ERR_EXEC_ASSERT; /* We have failed an 'ASSERTL' or 'ASSERTR' combinator. */
     case JET:
      /* Jets (and primitives) should already have been processed by dag[i].jet already */
      SIMPLICITY_UNREACHABLE;
    }
  }
  simplicity_assert(pc == len);

  return SIMPLICITY_NO_ERROR;
}

/* Inspects the stack contents after a successful runTCO execution to verify anti-DOS properties:
 * 1. If 'checks' includes 'CHECK_EXEC', then check that all non-HIDDEN dag nodes were executed at least once.
 * 2. If 'checks' includes 'CHECK_CASE', then check that both branches of every CASE node were executed.
 *
 * If these are violated, it means that the dag had unpruned nodes.
 *
 * Returns 'SIMPLICITY_ERR_ANTIDOS' if any of the anti-DOS checks fail.
 * Otherwise returns 'SIMPLICITY_NO_ERR'.
 *
 * Precondition: call stack[len];
 *               dag_node dag[len];
 */
static simplicity_err antiDos(flags_type checks, const call* stack, const dag_node* dag, size_t len) {
  static_assert(CHECK_EXEC == FLAG_EXEC, "CHECK_EXEC does not match FLAG_EXEC");
  static_assert(CHECK_CASE == (FLAG_CASE_LEFT | FLAG_CASE_RIGHT), "CHECK_CASE does not match FLAG_CASE");
  simplicity_assert(CHECK_CASE == (checks & CHECK_CASE) || 0 == (checks & CHECK_CASE));

  if (!checks) return SIMPLICITY_NO_ERROR;

  for(size_t i = 0; i < len; ++i) {
    /* All non-HIDDEN nodes must be executed at least once. */
    /* Both branches of every case combinator must be executed at least once. */
    flags_type test_flags = (HIDDEN != dag[i].tag ? CHECK_EXEC : 0)
                          | (CASE == dag[i].tag ? CHECK_CASE : 0);

    /* Only enable requested checks */
    test_flags &= checks;
    if (test_flags != (test_flags & stack[i].flags)) {
      return SIMPLICITY_ERR_ANTIDOS;
    }
  }

  return SIMPLICITY_NO_ERROR;
}

/* This structure is used by the static analysis that computes bounds on the working memory that suffices for
 * the Simplicity interpreter, and the CPU cost bounds in milliWU
 */
typedef struct boundsAnalysis {
  ubounded extraCellsBound[2];
  ubounded extraUWORDBound[2];
  ubounded extraFrameBound[2]; /* extraFrameBound[0] is for TCO off and extraFrameBound[1] is for TCO on */
  ubounded cost; /* milliWU */
} boundsAnalysis;

/* :TODO: Document extraFrameBound in the Tech Report (and implement it in Haskell) */
/* Given a well-typed dag representing a Simplicity expression, compute the memory and CPU requirements for evaluation.
 *
 * If 'malloc' fails, then returns SIMPLICITY_ERR_MALLOC.
 * When maxCells < UBOUNDED_MAX, if the bounds on the number of cells needed for evaluation of 'dag' on an idealized Bit Machine exceeds maxCells,
 * then return SIMPLICITY_ERR_EXEC_MEMORY.
 * When maxCost < UBOUNDED_MAX, if the bounds on the dag's CPU cost exceeds 'maxCost', then return SIMPLICITY_ERR_EXEC_BUDGET.
 * If the bounds on the dag's CPU cost is less than or equal to 'minCost', then return SIMPLICITY_ERR_OVERWEIGHT.
 * Otherwise returns SIMPLICITY_NO_ERR.
 *
 * Precondition: NULL != cellsBound
 *               NULL != UWORDBound
 *               NULL != frameBound
 *               NULL != costBound
 *               dag_node dag[len] and 'dag' is well-typed with 'type_dag'.
 * Postcondition: if the result is 'SIMPLICITY_NO_ERR'
 *                then if maxCost < UBOUNDED_MAX then '*costBound' bounds the dag's CPU cost measured in milli weight units
 *                 and if maxCells < UBOUNDED_MAX then '*cellsBound' bounds the number of cells needed for evaluation of 'dag' on an idealized Bit Machine
 *                 and if maxCells < UBOUNDED_MAX then '*UWORDBound' bounds the number of UWORDs needed for the frames during evaluation of 'dag'
 *                 and if maxCells < UBOUNDED_MAX then '*frameBound' bounds the number of stack frames needed during execution of 'dag'.
 */
simplicity_err simplicity_analyseBounds( ubounded *cellsBound, ubounded *UWORDBound, ubounded *frameBound, ubounded *costBound
                                       , ubounded maxCells, ubounded minCost, ubounded maxCost, const dag_node* dag, const type* type_dag, const size_t len) {
  static_assert(DAG_LEN_MAX <= SIZE_MAX / sizeof(boundsAnalysis), "bound array too large.");
  static_assert(1 <= DAG_LEN_MAX, "DAG_LEN_MAX is zero.");
  static_assert(DAG_LEN_MAX - 1 <= UINT32_MAX, "bound array index does not fit in uint32_t.");
  simplicity_assert(1 <= len);
  simplicity_assert(len <= DAG_LEN_MAX);
  boundsAnalysis* bound = simplicity_malloc(len * sizeof(boundsAnalysis));
  if (!bound) return SIMPLICITY_ERR_MALLOC;

  /* Sum up the total costs.
   * The computations for extraCells and cost are clipped at UBOUNDED_MAX,
   * so a result of UBOUNDED_MAX means "UBOUNDED_MAX or larger".
   *
   * The extraUWORD computation may produce unsigned overflow.
   * However the extraUWORD true value is less than the true value of extraCells.
   * As long as extraCells is strictly less than UBOUNDED_MAX, extraUWORD will be too.
   *
   * The extraFrame computation is bounded by DAG_LEN, and cannot overflow.
   */
  for (size_t i = 0; i < len; ++i) {
    switch (dag[i].tag) {
     case ASSERTL:
     case ASSERTR:
     case CASE:
      bound[i].extraCellsBound[0] = bounded_max( bound[dag[i].child[0]].extraCellsBound[0]
                                       , bound[dag[i].child[1]].extraCellsBound[0] );
      bound[i].extraCellsBound[1] = bounded_max( bound[dag[i].child[0]].extraCellsBound[1]
                                       , bound[dag[i].child[1]].extraCellsBound[1] );

      bound[i].extraUWORDBound[0] = bounded_max( bound[dag[i].child[0]].extraUWORDBound[0]
                                       , bound[dag[i].child[1]].extraUWORDBound[0] );
      bound[i].extraUWORDBound[1] = bounded_max( bound[dag[i].child[0]].extraUWORDBound[1]
                                       , bound[dag[i].child[1]].extraUWORDBound[1] );

      bound[i].extraFrameBound[0] = bounded_max( bound[dag[i].child[0]].extraFrameBound[0]
                                       , bound[dag[i].child[1]].extraFrameBound[0] );
      bound[i].extraFrameBound[1] = bounded_max( bound[dag[i].child[0]].extraFrameBound[1]
                                       , bound[dag[i].child[1]].extraFrameBound[1] );
      bound[i].cost = bounded_add(overhead, bounded_max( bound[dag[i].child[0]].cost
                                               , bound[dag[i].child[1]].cost ));
      break;
     case DISCONNECT:
      bound[i].extraCellsBound[1] = type_dag[DISCONNECT_W256A(dag, type_dag, i)].bitSize;
      bound[i].extraCellsBound[0] = bounded_max(
        bounded_add( type_dag[DISCONNECT_BC(dag, type_dag, i)].bitSize
                   , bounded_max( bounded_add(bound[i].extraCellsBound[1], bound[dag[i].child[0]].extraCellsBound[1])
                                , bounded_max(bound[dag[i].child[0]].extraCellsBound[0], bound[dag[i].child[1]].extraCellsBound[1]))),
        bound[dag[i].child[1]].extraCellsBound[0]);
      bound[i].extraUWORDBound[1] = (ubounded)ROUND_UWORD(type_dag[DISCONNECT_W256A(dag, type_dag, i)].bitSize);
      bound[i].extraUWORDBound[0] = bounded_max(
          (ubounded)ROUND_UWORD(type_dag[DISCONNECT_BC(dag, type_dag, i)].bitSize) +
          bounded_max( bound[i].extraUWORDBound[1] + bound[dag[i].child[0]].extraUWORDBound[1]
             , bounded_max(bound[dag[i].child[0]].extraUWORDBound[0], bound[dag[i].child[1]].extraUWORDBound[1])),
        bound[dag[i].child[1]].extraUWORDBound[0]);

      bound[i].extraFrameBound[1] = bounded_max( bound[dag[i].child[0]].extraFrameBound[1] + 1
                                       , bound[dag[i].child[1]].extraFrameBound[1]);
      bound[i].extraFrameBound[0] = bound[i].extraFrameBound[1] + 1;
      bound[i].cost = bounded_add(overhead
                    , bounded_add(type_dag[DISCONNECT_W256A(dag, type_dag, i)].bitSize
                    , bounded_add(type_dag[DISCONNECT_W256A(dag, type_dag, i)].bitSize /* counted twice because the frame is both filled in and moved. */
                    , bounded_add(type_dag[DISCONNECT_BC(dag, type_dag, i)].bitSize
                    , bounded_add(type_dag[DISCONNECT_B(dag, type_dag, i)].bitSize
                    , bounded_add(bound[dag[i].child[0]].cost, bound[dag[i].child[1]].cost))))));
      break;
     case COMP:
      bound[i].extraCellsBound[0] = bounded_max( bounded_add( type_dag[COMP_B(dag, type_dag, i)].bitSize
                                                            , bounded_max( bound[dag[i].child[0]].extraCellsBound[0]
                                                                         , bound[dag[i].child[1]].extraCellsBound[1] ))
                                               , bound[dag[i].child[1]].extraCellsBound[0] );
      bound[i].extraCellsBound[1] = bounded_add( type_dag[COMP_B(dag, type_dag, i)].bitSize
                                               , bound[dag[i].child[0]].extraCellsBound[1] );
      bound[i].extraUWORDBound[0] = bounded_max( (ubounded)ROUND_UWORD(type_dag[COMP_B(dag, type_dag, i)].bitSize) +
                                         bounded_max( bound[dag[i].child[0]].extraUWORDBound[0]
                                            , bound[dag[i].child[1]].extraUWORDBound[1] )
                                       , bound[dag[i].child[1]].extraUWORDBound[0] );
      bound[i].extraUWORDBound[1] = (ubounded)ROUND_UWORD(type_dag[COMP_B(dag, type_dag, i)].bitSize)
                                  + bound[dag[i].child[0]].extraUWORDBound[1];

      bound[i].extraFrameBound[0] = bounded_max( bound[dag[i].child[0]].extraFrameBound[0]
                                       , bound[dag[i].child[1]].extraFrameBound[1] )
                                  + 1;
      bound[i].extraFrameBound[1] = bounded_max( bound[dag[i].child[0]].extraFrameBound[1] + 1
                                       , bound[dag[i].child[1]].extraFrameBound[1] );
      bound[i].cost = bounded_add(overhead
                    , bounded_add(type_dag[COMP_B(dag, type_dag, i)].bitSize
                    , bounded_add(bound[dag[i].child[0]].cost, bound[dag[i].child[1]].cost)));
      break;
     case PAIR:
      bound[i].extraCellsBound[0] = bound[dag[i].child[1]].extraCellsBound[0];
      bound[i].extraCellsBound[1] = bounded_max( bound[dag[i].child[0]].extraCellsBound[0]
                                       , bounded_max( bound[dag[i].child[0]].extraCellsBound[1]
                                            , bound[dag[i].child[1]].extraCellsBound[1] ));

      bound[i].extraUWORDBound[0] = bound[dag[i].child[1]].extraUWORDBound[0];
      bound[i].extraUWORDBound[1] = bounded_max( bound[dag[i].child[0]].extraUWORDBound[0]
                                       , bounded_max( bound[dag[i].child[0]].extraUWORDBound[1]
                                            , bound[dag[i].child[1]].extraUWORDBound[1] ));

      bound[i].extraFrameBound[0] = bounded_max( bound[dag[i].child[0]].extraFrameBound[0]
                                       , bound[dag[i].child[1]].extraFrameBound[0] );
      bound[i].extraFrameBound[1] = bounded_max( bound[dag[i].child[0]].extraFrameBound[0]
                                       , bound[dag[i].child[1]].extraFrameBound[1] );
      bound[i].cost = bounded_add(overhead, bounded_add( bound[dag[i].child[0]].cost
                                                       , bound[dag[i].child[1]].cost ));
      break;
     case INJL:
     case INJR:
     case TAKE:
     case DROP:
      bound[i].extraCellsBound[0] = bound[dag[i].child[0]].extraCellsBound[0];
      bound[i].extraCellsBound[1] = bound[dag[i].child[0]].extraCellsBound[1];

      bound[i].extraUWORDBound[0] = bound[dag[i].child[0]].extraUWORDBound[0];
      bound[i].extraUWORDBound[1] = bound[dag[i].child[0]].extraUWORDBound[1];

      bound[i].extraFrameBound[0] = bound[dag[i].child[0]].extraFrameBound[0];
      bound[i].extraFrameBound[1] = bound[dag[i].child[0]].extraFrameBound[1];
      bound[i].cost = bounded_add(overhead, bound[dag[i].child[0]].cost);
      break;
     case IDEN:
     case UNIT:
     case HIDDEN:
     case WITNESS:
     case JET:
     case WORD:
      bound[i].extraCellsBound[0] = bound[i].extraCellsBound[1] = 0;
      bound[i].extraUWORDBound[0] = bound[i].extraUWORDBound[1] = 0;
      bound[i].extraFrameBound[0] = bound[i].extraFrameBound[1] = 0;
      bound[i].cost = IDEN == dag[i].tag ? bounded_add(overhead, type_dag[IDEN_A(dag, type_dag, i)].bitSize)
                    : WITNESS == dag[i].tag || WORD == dag[i].tag ? bounded_add(overhead, type_dag[dag[i].targetType].bitSize)
                    : JET == dag[i].tag ? bounded_add(overhead, dag[i].cost)
                    : HIDDEN == dag[i].tag ? 0
                    : overhead;
    }
  }

  {
    const ubounded inputSize = type_dag[dag[len-1].sourceType].bitSize;
    const ubounded outputSize = type_dag[dag[len-1].targetType].bitSize;
    *cellsBound = bounded_add( bounded_add(inputSize, outputSize)
                             , bounded_max(bound[len-1].extraCellsBound[0], bound[len-1].extraCellsBound[1])
                             );
    *UWORDBound = (ubounded)ROUND_UWORD(inputSize) + (ubounded)ROUND_UWORD(outputSize)
                + bounded_max(bound[len-1].extraUWORDBound[0], bound[len-1].extraUWORDBound[1]);
    *frameBound = bound[len-1].extraFrameBound[0] + 2; /* add the initial input and output frames to the count. */
    *costBound = bound[len-1].cost;
  }
  simplicity_free(bound);
  /* Note that the cellsBound and costBound computations have been clipped at UBOUNDED_MAX.
   * Therefore setting maxCells or maxCost to UBOUNDED_MAX will disable the corresponding error check.
   */
  return (maxCells < *cellsBound) ? SIMPLICITY_ERR_EXEC_MEMORY
       : (maxCost < *costBound) ? SIMPLICITY_ERR_EXEC_BUDGET
       : (*costBound <= minCost) ? SIMPLICITY_ERR_OVERWEIGHT
       : SIMPLICITY_NO_ERROR;
}

/* Run the Bit Machine on the well-typed Simplicity expression 'dag[len]' of type A |- B.
 * If bitSize(A) > 0, initialize the active read frame's data with 'input[ROUND_UWORD(bitSize(A))]'.
 *
 * If malloc fails, returns 'SIMPLICITY_ERR_MALLOC'.
 * When a budget is given, if static analysis results determines the bound on cpu requirements exceed the allowed budget, returns 'SIMPLICITY_ERR_EXEC_BUDGET'.
 * If static analysis results determines the bound on cpu requirements is less than or equal to the minCost, returns 'SIMPLICITY_ERR_OVERWEIGHT'.
 * If static analysis results determines the bound on memory allocation requirements exceed the allowed limits, returns 'SIMPLICITY_ERR_EXEC_MEMORY'.
 * If during execution some jet execution fails, returns 'SIMPLICITY_ERR_EXEC_JET'.
 * If during execution some 'assertr' or 'assertl' combinator fails, returns 'SIMPLICITY_ERR_EXEC_ASESRT'.
 *
 * Note that minCost and budget parameters are in WU, while the cost analysis will be performed in milliWU.
 * Thus the minCost and budget specify a half open interval (minCost, budget] of acceptable cost values in milliWU.
 * Setting minCost to 0 effectively disables the minCost check as every Simplicity program has a non-zero cost analysis.
 *
 * If none of the above conditions fail and bitSize(B) > 0, then a copy the final active write frame's data is written to 'output[roundWord(bitSize(B))]'.
 *
 * If 'anti_dos_checks' includes the 'CHECK_EXEC' flag, and not every non-HIDDEN dag node is executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 * If 'anti_dos_checks' includes the 'CHECK_CASE' flag, and not every case node has both branches executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 *
 * Otherwise 'SIMPLICITY_NO_ERROR' is returned.
 *
 * Precondition: dag_node dag[len] and 'dag' is well-typed with 'type_dag' for an expression of type A |- B;
 *               bitSize(A) == 0 or UWORD input[ROUND_UWORD(bitSize(A))];
 *               bitSize(B) == 0 or UWORD output[ROUND_UWORD(bitSize(B))];
 *               if NULL != budget then *budget <= BUDGET_MAX
 *               if NULL != budget then minCost <= *budget
 *               minCost <= BUDGET_MAX
 *               if 'dag[len]' represents a Simplicity expression with primitives then 'NULL != env';
 */
simplicity_err simplicity_evalTCOExpression( flags_type anti_dos_checks, UWORD* output, const UWORD* input
                                           , const dag_node* dag, type* type_dag, size_t len, ubounded minCost, const ubounded* budget, const txEnv* env
                                           ) {
  simplicity_assert(1 <= len);
  simplicity_assert(len <= DAG_LEN_MAX);
  if (budget) {
    simplicity_assert(*budget <= BUDGET_MAX);
    simplicity_assert(minCost <= *budget);
  }
  simplicity_assert(minCost <= BUDGET_MAX);
  static_assert(1 <= UBOUNDED_MAX, "UBOUNDED_MAX is zero.");
  static_assert(BUDGET_MAX <= (UBOUNDED_MAX - 1) / 1000, "BUDGET_MAX is too large.");
  static_assert(CELLS_MAX < UBOUNDED_MAX, "CELLS_MAX is too large.");
  ubounded cellsBound, UWORDBound, frameBound, costBound;
  simplicity_err result = simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, CELLS_MAX, minCost*1000, budget ? *budget*1000 : UBOUNDED_MAX, dag, type_dag, len);
  if (!IS_OK(result)) return result;

  /* frameBound is at most 2*len. */
  static_assert(DAG_LEN_MAX <= UBOUNDED_MAX / 2, "2*DAG_LEN_MAX does not fit in size_t.");
  simplicity_assert(frameBound <= 2*len);

  /* UWORDBound * UWORD_BIT, the number of bits actually allocacted, is at most the cellBound count plus (worse case) padding bits in each frame. */
  static_assert(1 <= UWORD_BIT, "UWORD_BIT is zero.");
  static_assert(2*DAG_LEN_MAX <= (SIZE_MAX - CELLS_MAX) / (UWORD_BIT - 1), "cellsBound + frameBound*(UWORD_BIT - 1) doesn't fit in size_t.");
  simplicity_assert(UWORDBound <= (cellsBound + frameBound*(UWORD_BIT - 1)) / UWORD_BIT);

  /* UWORDBound, is also at most the cellsBound, with an entire UWORD per cell (the rest of the UWORD being padding). */
  simplicity_assert(UWORDBound <= cellsBound);

  /* We use calloc for 'cells' because the frame data must be initialized before we can perform bitwise operations. */
  static_assert(CELLS_MAX - 1 <= UINT32_MAX, "cells array index does not fit in uint32_t.");
  UWORD* cells = simplicity_calloc(UWORDBound ? UWORDBound : 1, sizeof(UWORD));
  static_assert(2*DAG_LEN_MAX <= SIZE_MAX / sizeof(frameItem), "frames array does not fit in size_t.");
  static_assert(1 <= DAG_LEN_MAX, "DAG_LEN_MAX is zero.");
  static_assert(2*DAG_LEN_MAX - 1 <= UINT32_MAX, "frames array index does not fit in uint32_t.");
  frameItem* frames = simplicity_malloc(frameBound * sizeof(frameItem));
  call* stack = simplicity_calloc(len, sizeof(call));

  result = cells && frames && stack ? SIMPLICITY_NO_ERROR : SIMPLICITY_ERR_MALLOC;
  if (IS_OK(result)) {
    const ubounded inputSize = type_dag[dag[len-1].sourceType].bitSize;
    const ubounded outputSize = type_dag[dag[len-1].targetType].bitSize;
    simplicity_assert(NULL != input || 0 == inputSize);
    if (inputSize) memcpy(cells, input, ROUND_UWORD(inputSize) * sizeof(UWORD));

    evalState state =
      { .activeReadFrame = frames
      , .activeWriteFrame = frames + (frameBound - 1)
      };
    *(state.activeReadFrame) = initReadFrame(inputSize, cells);
    *(state.activeWriteFrame) = initWriteFrame(outputSize, cells + UWORDBound);

    result = runTCO(state, stack, dag, type_dag, len, env);

    if (IS_OK(result)) {
      simplicity_assert(NULL != output || 0 == outputSize);
      if (outputSize) memcpy(output, state.activeWriteFrame->edge, ROUND_UWORD(outputSize) * sizeof(UWORD));

      result = antiDos(anti_dos_checks, stack, dag, len);
    }
  }

  simplicity_free(stack);
  simplicity_free(frames);
  simplicity_free(cells);
  return result;
}
