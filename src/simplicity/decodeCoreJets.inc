/* This file has been automatically generated. */

{
  int32_t code;
  code = simplicity_decodeUptoMaxInt(stream);
  if (code < 0) return (simplicity_err)code;
  switch (code) {
    case 1:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = VERIFY; return SIMPLICITY_NO_ERROR;
        case 2:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = LOW_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = LOW_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LOW_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LOW_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LOW_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 3:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = HIGH_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = HIGH_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = HIGH_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = HIGH_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = HIGH_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 4:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = COMPLEMENT_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = COMPLEMENT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = COMPLEMENT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = COMPLEMENT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = COMPLEMENT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 5:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = AND_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = AND_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = AND_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = AND_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = AND_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 6:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = OR_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = OR_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = OR_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = OR_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = OR_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 7:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = XOR_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = XOR_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = XOR_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = XOR_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = XOR_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 8:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = MAJ_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = MAJ_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MAJ_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MAJ_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MAJ_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 9:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = XOR_XOR_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = XOR_XOR_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = XOR_XOR_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = XOR_XOR_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = XOR_XOR_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 10:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = CH_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = CH_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = CH_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = CH_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = CH_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 11:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = SOME_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = SOME_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = SOME_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = SOME_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = SOME_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 12:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = ALL_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = ALL_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = ALL_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = ALL_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 13:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = EQ_1; return SIMPLICITY_NO_ERROR;
            case 3: *result = EQ_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = EQ_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = EQ_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = EQ_64; return SIMPLICITY_NO_ERROR;
            case 8: *result = EQ_256; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 14:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = FULL_LEFT_SHIFT_8_1; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_LEFT_SHIFT_16_1; return SIMPLICITY_NO_ERROR;
                case 5: *result = FULL_LEFT_SHIFT_32_1; return SIMPLICITY_NO_ERROR;
                case 6: *result = FULL_LEFT_SHIFT_64_1; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 2:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 2: *result = FULL_LEFT_SHIFT_8_2; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_LEFT_SHIFT_16_2; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_LEFT_SHIFT_32_2; return SIMPLICITY_NO_ERROR;
                case 5: *result = FULL_LEFT_SHIFT_64_2; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 3:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_LEFT_SHIFT_8_4; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_LEFT_SHIFT_16_4; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_LEFT_SHIFT_32_4; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_LEFT_SHIFT_64_4; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_LEFT_SHIFT_16_8; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_LEFT_SHIFT_32_8; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_LEFT_SHIFT_64_8; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_LEFT_SHIFT_32_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_LEFT_SHIFT_64_16; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_LEFT_SHIFT_64_32; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 15:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = FULL_RIGHT_SHIFT_8_1; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_RIGHT_SHIFT_16_1; return SIMPLICITY_NO_ERROR;
                case 5: *result = FULL_RIGHT_SHIFT_32_1; return SIMPLICITY_NO_ERROR;
                case 6: *result = FULL_RIGHT_SHIFT_64_1; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 2:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 2: *result = FULL_RIGHT_SHIFT_8_2; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_RIGHT_SHIFT_16_2; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_RIGHT_SHIFT_32_2; return SIMPLICITY_NO_ERROR;
                case 5: *result = FULL_RIGHT_SHIFT_64_2; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 3:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_RIGHT_SHIFT_8_4; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_RIGHT_SHIFT_16_4; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_RIGHT_SHIFT_32_4; return SIMPLICITY_NO_ERROR;
                case 4: *result = FULL_RIGHT_SHIFT_64_4; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_RIGHT_SHIFT_16_8; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_RIGHT_SHIFT_32_8; return SIMPLICITY_NO_ERROR;
                case 3: *result = FULL_RIGHT_SHIFT_64_8; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_RIGHT_SHIFT_32_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = FULL_RIGHT_SHIFT_64_16; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = FULL_RIGHT_SHIFT_64_32; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 16:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = LEFTMOST_8_1; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFTMOST_16_1; return SIMPLICITY_NO_ERROR;
                case 5: *result = LEFTMOST_32_1; return SIMPLICITY_NO_ERROR;
                case 6: *result = LEFTMOST_64_1; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 2:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 2: *result = LEFTMOST_8_2; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFTMOST_16_2; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFTMOST_32_2; return SIMPLICITY_NO_ERROR;
                case 5: *result = LEFTMOST_64_2; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 3:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFTMOST_8_4; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFTMOST_16_4; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFTMOST_32_4; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFTMOST_64_4; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFTMOST_16_8; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFTMOST_32_8; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFTMOST_64_8; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFTMOST_32_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFTMOST_64_16; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFTMOST_64_32; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 17:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = RIGHTMOST_8_1; return SIMPLICITY_NO_ERROR;
                case 4: *result = RIGHTMOST_16_1; return SIMPLICITY_NO_ERROR;
                case 5: *result = RIGHTMOST_32_1; return SIMPLICITY_NO_ERROR;
                case 6: *result = RIGHTMOST_64_1; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 2:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 2: *result = RIGHTMOST_8_2; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHTMOST_16_2; return SIMPLICITY_NO_ERROR;
                case 4: *result = RIGHTMOST_32_2; return SIMPLICITY_NO_ERROR;
                case 5: *result = RIGHTMOST_64_2; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 3:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHTMOST_8_4; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHTMOST_16_4; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHTMOST_32_4; return SIMPLICITY_NO_ERROR;
                case 4: *result = RIGHTMOST_64_4; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHTMOST_16_8; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHTMOST_32_8; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHTMOST_64_8; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHTMOST_32_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHTMOST_64_16; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHTMOST_64_32; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 18:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = LEFT_PAD_LOW_1_8; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFT_PAD_LOW_1_16; return SIMPLICITY_NO_ERROR;
                case 5: *result = LEFT_PAD_LOW_1_32; return SIMPLICITY_NO_ERROR;
                case 6: *result = LEFT_PAD_LOW_1_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_LOW_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_PAD_LOW_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFT_PAD_LOW_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_LOW_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_PAD_LOW_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_LOW_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 19:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = LEFT_PAD_HIGH_1_8; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFT_PAD_HIGH_1_16; return SIMPLICITY_NO_ERROR;
                case 5: *result = LEFT_PAD_HIGH_1_32; return SIMPLICITY_NO_ERROR;
                case 6: *result = LEFT_PAD_HIGH_1_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_HIGH_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_PAD_HIGH_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFT_PAD_HIGH_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_HIGH_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_PAD_HIGH_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_PAD_HIGH_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 20:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = LEFT_EXTEND_1_8; return SIMPLICITY_NO_ERROR;
                case 4: *result = LEFT_EXTEND_1_16; return SIMPLICITY_NO_ERROR;
                case 5: *result = LEFT_EXTEND_1_32; return SIMPLICITY_NO_ERROR;
                case 6: *result = LEFT_EXTEND_1_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_EXTEND_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_EXTEND_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = LEFT_EXTEND_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_EXTEND_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = LEFT_EXTEND_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = LEFT_EXTEND_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 21:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = RIGHT_PAD_LOW_1_8; return SIMPLICITY_NO_ERROR;
                case 4: *result = RIGHT_PAD_LOW_1_16; return SIMPLICITY_NO_ERROR;
                case 5: *result = RIGHT_PAD_LOW_1_32; return SIMPLICITY_NO_ERROR;
                case 6: *result = RIGHT_PAD_LOW_1_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_LOW_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_PAD_LOW_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHT_PAD_LOW_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_LOW_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_PAD_LOW_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_LOW_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 22:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 3: *result = RIGHT_PAD_HIGH_1_8; return SIMPLICITY_NO_ERROR;
                case 4: *result = RIGHT_PAD_HIGH_1_16; return SIMPLICITY_NO_ERROR;
                case 5: *result = RIGHT_PAD_HIGH_1_32; return SIMPLICITY_NO_ERROR;
                case 6: *result = RIGHT_PAD_HIGH_1_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_HIGH_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_PAD_HIGH_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHT_PAD_HIGH_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_HIGH_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_PAD_HIGH_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_PAD_HIGH_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 23:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 4:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_EXTEND_8_16; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_EXTEND_8_32; return SIMPLICITY_NO_ERROR;
                case 3: *result = RIGHT_EXTEND_8_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 5:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_EXTEND_16_32; return SIMPLICITY_NO_ERROR;
                case 2: *result = RIGHT_EXTEND_16_64; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 6:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = RIGHT_EXTEND_32_64; return SIMPLICITY_NO_ERROR;
              }
              break;
          }
          break;
        case 24:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = LEFT_SHIFT_WITH_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LEFT_SHIFT_WITH_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LEFT_SHIFT_WITH_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LEFT_SHIFT_WITH_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 25:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = RIGHT_SHIFT_WITH_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = RIGHT_SHIFT_WITH_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = RIGHT_SHIFT_WITH_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = RIGHT_SHIFT_WITH_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 26:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = LEFT_SHIFT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LEFT_SHIFT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LEFT_SHIFT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LEFT_SHIFT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 27:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = RIGHT_SHIFT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = RIGHT_SHIFT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = RIGHT_SHIFT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = RIGHT_SHIFT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 28:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = LEFT_ROTATE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LEFT_ROTATE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LEFT_ROTATE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LEFT_ROTATE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 29:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = RIGHT_ROTATE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = RIGHT_ROTATE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = RIGHT_ROTATE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = RIGHT_ROTATE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
      }
      break;
    case 2:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = ONE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = ONE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = ONE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = ONE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 2:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = FULL_ADD_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = FULL_ADD_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = FULL_ADD_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = FULL_ADD_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 3:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = ADD_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = ADD_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = ADD_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = ADD_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 4:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = FULL_INCREMENT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = FULL_INCREMENT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = FULL_INCREMENT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = FULL_INCREMENT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 5:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = INCREMENT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = INCREMENT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = INCREMENT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = INCREMENT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 7:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = FULL_SUBTRACT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = FULL_SUBTRACT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = FULL_SUBTRACT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = FULL_SUBTRACT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 8:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = SUBTRACT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = SUBTRACT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = SUBTRACT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = SUBTRACT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 9:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = NEGATE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = NEGATE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = NEGATE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = NEGATE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 10:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = FULL_DECREMENT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = FULL_DECREMENT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = FULL_DECREMENT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = FULL_DECREMENT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 11:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = DECREMENT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = DECREMENT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = DECREMENT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = DECREMENT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 12:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = FULL_MULTIPLY_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = FULL_MULTIPLY_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = FULL_MULTIPLY_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = FULL_MULTIPLY_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 13:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = MULTIPLY_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MULTIPLY_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MULTIPLY_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MULTIPLY_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 14:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = IS_ZERO_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = IS_ZERO_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = IS_ZERO_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = IS_ZERO_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 15:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = IS_ONE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = IS_ONE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = IS_ONE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = IS_ONE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 16:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = LE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 17:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = LT_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = LT_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = LT_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = LT_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 18:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = MIN_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MIN_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MIN_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MIN_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 19:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = MAX_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MAX_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MAX_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MAX_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 20:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = MEDIAN_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MEDIAN_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MEDIAN_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MEDIAN_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 21:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 6: *result = DIV_MOD_128_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 22:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = DIV_MOD_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = DIV_MOD_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = DIV_MOD_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = DIV_MOD_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 23:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = DIVIDE_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = DIVIDE_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = DIVIDE_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = DIVIDE_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 24:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = MODULO_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = MODULO_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = MODULO_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = MODULO_64; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 25:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 3: *result = DIVIDES_8; return SIMPLICITY_NO_ERROR;
            case 4: *result = DIVIDES_16; return SIMPLICITY_NO_ERROR;
            case 5: *result = DIVIDES_32; return SIMPLICITY_NO_ERROR;
            case 6: *result = DIVIDES_64; return SIMPLICITY_NO_ERROR;
          }
          break;
      }
      break;
    case 3:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = SHA_256_BLOCK; return SIMPLICITY_NO_ERROR;
            case 2: *result = SHA_256_IV; return SIMPLICITY_NO_ERROR;
            case 3:
              code = simplicity_decodeUptoMaxInt(stream);
              if (code < 0) return (simplicity_err)code;
              switch (code) {
                case 1: *result = SHA_256_CTX_8_ADD_1; return SIMPLICITY_NO_ERROR;
                case 2: *result = SHA_256_CTX_8_ADD_2; return SIMPLICITY_NO_ERROR;
                case 3: *result = SHA_256_CTX_8_ADD_4; return SIMPLICITY_NO_ERROR;
                case 4: *result = SHA_256_CTX_8_ADD_8; return SIMPLICITY_NO_ERROR;
                case 5: *result = SHA_256_CTX_8_ADD_16; return SIMPLICITY_NO_ERROR;
                case 6: *result = SHA_256_CTX_8_ADD_32; return SIMPLICITY_NO_ERROR;
                case 7: *result = SHA_256_CTX_8_ADD_64; return SIMPLICITY_NO_ERROR;
                case 8: *result = SHA_256_CTX_8_ADD_128; return SIMPLICITY_NO_ERROR;
                case 9: *result = SHA_256_CTX_8_ADD_256; return SIMPLICITY_NO_ERROR;
                case 10: *result = SHA_256_CTX_8_ADD_512; return SIMPLICITY_NO_ERROR;
              }
              break;
            case 4: *result = SHA_256_CTX_8_ADD_BUFFER_511; return SIMPLICITY_NO_ERROR;
            case 5: *result = SHA_256_CTX_8_FINALIZE; return SIMPLICITY_NO_ERROR;
            case 6: *result = SHA_256_CTX_8_INIT; return SIMPLICITY_NO_ERROR;
          }
          break;
      }
      break;
    case 4:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = POINT_VERIFY_1; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 2: *result = DECOMPRESS; return SIMPLICITY_NO_ERROR;
        case 3:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = LINEAR_VERIFY_1; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 4:
          code = simplicity_decodeUptoMaxInt(stream);
          if (code < 0) return (simplicity_err)code;
          switch (code) {
            case 1: *result = LINEAR_COMBINATION_1; return SIMPLICITY_NO_ERROR;
          }
          break;
        case 5: *result = SCALE; return SIMPLICITY_NO_ERROR;
        case 6: *result = GENERATE; return SIMPLICITY_NO_ERROR;
        case 7: *result = GEJ_INFINITY; return SIMPLICITY_NO_ERROR;
        case 8: *result = GEJ_NORMALIZE; return SIMPLICITY_NO_ERROR;
        case 9: *result = GEJ_NEGATE; return SIMPLICITY_NO_ERROR;
        case 10: *result = GE_NEGATE; return SIMPLICITY_NO_ERROR;
        case 11: *result = GEJ_DOUBLE; return SIMPLICITY_NO_ERROR;
        case 12: *result = GEJ_ADD; return SIMPLICITY_NO_ERROR;
        case 13: *result = GEJ_GE_ADD_EX; return SIMPLICITY_NO_ERROR;
        case 14: *result = GEJ_GE_ADD; return SIMPLICITY_NO_ERROR;
        case 15: *result = GEJ_RESCALE; return SIMPLICITY_NO_ERROR;
        case 16: *result = GEJ_IS_INFINITY; return SIMPLICITY_NO_ERROR;
        case 17: *result = GEJ_EQUIV; return SIMPLICITY_NO_ERROR;
        case 18: *result = GEJ_GE_EQUIV; return SIMPLICITY_NO_ERROR;
        case 19: *result = GEJ_X_EQUIV; return SIMPLICITY_NO_ERROR;
        case 20: *result = GEJ_Y_IS_ODD; return SIMPLICITY_NO_ERROR;
        case 21: *result = GEJ_IS_ON_CURVE; return SIMPLICITY_NO_ERROR;
        case 22: *result = GE_IS_ON_CURVE; return SIMPLICITY_NO_ERROR;
        case 23: *result = SCALAR_NORMALIZE; return SIMPLICITY_NO_ERROR;
        case 24: *result = SCALAR_NEGATE; return SIMPLICITY_NO_ERROR;
        case 25: *result = SCALAR_ADD; return SIMPLICITY_NO_ERROR;
        case 26: *result = SCALAR_SQUARE; return SIMPLICITY_NO_ERROR;
        case 27: *result = SCALAR_MULTIPLY; return SIMPLICITY_NO_ERROR;
        case 28: *result = SCALAR_MULTIPLY_LAMBDA; return SIMPLICITY_NO_ERROR;
        case 29: *result = SCALAR_INVERT; return SIMPLICITY_NO_ERROR;
        case 30: *result = SCALAR_IS_ZERO; return SIMPLICITY_NO_ERROR;
        case 35: *result = FE_NORMALIZE; return SIMPLICITY_NO_ERROR;
        case 36: *result = FE_NEGATE; return SIMPLICITY_NO_ERROR;
        case 37: *result = FE_ADD; return SIMPLICITY_NO_ERROR;
        case 38: *result = FE_SQUARE; return SIMPLICITY_NO_ERROR;
        case 39: *result = FE_MULTIPLY; return SIMPLICITY_NO_ERROR;
        case 40: *result = FE_MULTIPLY_BETA; return SIMPLICITY_NO_ERROR;
        case 41: *result = FE_INVERT; return SIMPLICITY_NO_ERROR;
        case 42: *result = FE_SQUARE_ROOT; return SIMPLICITY_NO_ERROR;
        case 43: *result = FE_IS_ZERO; return SIMPLICITY_NO_ERROR;
        case 44: *result = FE_IS_ODD; return SIMPLICITY_NO_ERROR;
        case 46: *result = HASH_TO_CURVE; return SIMPLICITY_NO_ERROR;
        case 47: *result = SWU; return SIMPLICITY_NO_ERROR;
      }
      break;
    case 5:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = CHECK_SIG_VERIFY; return SIMPLICITY_NO_ERROR;
        case 2: *result = BIP_0340_VERIFY; return SIMPLICITY_NO_ERROR;
      }
      break;
    case 7:
      code = simplicity_decodeUptoMaxInt(stream);
      if (code < 0) return (simplicity_err)code;
      switch (code) {
        case 1: *result = PARSE_LOCK; return SIMPLICITY_NO_ERROR;
        case 2: *result = PARSE_SEQUENCE; return SIMPLICITY_NO_ERROR;
        case 3: *result = TAPDATA_INIT; return SIMPLICITY_NO_ERROR;
      }
      break;
  }
}