# How Bitcoin's Script Virtual Machine Works

## Overview

Bitcoin's Script is a stack-based virtual machine that processes transactions and determines whether they are valid for spending. It operates as a simple, deterministic, and deliberately limited programming language designed for expressing spending conditions on Bitcoin outputs.

The Script VM is implemented primarily in `/src/script/interpreter.cpp` and related files in the `/src/script/` directory.

## Core Architecture

### Stack-Based Design

Bitcoin Script operates as a **stack machine**, similar to Forth or PostScript:
- Data is stored on a Last-In-First-Out (LIFO) stack
- Operations (opcodes) manipulate data on the stack
- The final result is determined by the top stack element after execution

### Key Components

1. **Main Stack**: Primary data storage during script execution
2. **Alternative Stack**: Secondary stack accessible via `OP_TOALTSTACK`/`OP_FROMALTSTACK`
3. **Script Code**: The bytecode being executed
4. **Execution Context**: Flags, signature version, and validation state

### Virtual Machine Limits

```cpp
// Maximum number of bytes pushable to the stack
static const unsigned int MAX_SCRIPT_ELEMENT_SIZE = 520;

// Maximum number of non-push operations per script
static const int MAX_OPS_PER_SCRIPT = 201;

// Maximum script length in bytes
static const int MAX_SCRIPT_SIZE = 10000;

// Maximum number of values on script interpreter stack
static const int MAX_STACK_SIZE = 1000;
```

## Script Execution Process

### 1. Script Parsing and Validation

The interpreter begins by parsing the script bytecode using `GetScriptOp()`:

```cpp
bool GetOp(const_iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>& vchRet)
```

This function reads opcodes and associated data from the script, advancing the program counter.

### 2. Opcode Categories

#### Push Operations (0x00-0x4e)
- **Direct Push**: Opcodes 0x01-0x4b push 1-75 bytes directly
- **OP_PUSHDATA1**: Pushes up to 255 bytes (length in next byte)
- **OP_PUSHDATA2**: Pushes up to 65535 bytes (length in next 2 bytes)
- **OP_PUSHDATA4**: Pushes up to 4GB bytes (length in next 4 bytes)

#### Constants (0x4f-0x60)
```cpp
case OP_1NEGATE: // Push -1
case OP_1: case OP_2: ... case OP_16: // Push 1-16
```

#### Flow Control (0x63-0x68)
- **OP_IF/OP_NOTIF**: Conditional execution based on stack top
- **OP_ELSE**: Flip execution state
- **OP_ENDIF**: End conditional block
- **OP_VERIFY**: Fail unless stack top is true
- **OP_RETURN**: Immediately fail (used for unspendable outputs)

#### Stack Operations (0x6b-0x7d)
```cpp
case OP_DUP:    // (x -- x x)
case OP_SWAP:   // (x1 x2 -- x2 x1)
case OP_ROT:    // (x1 x2 x3 -- x2 x3 x1)
case OP_DROP:   // (x -- )
```

#### Arithmetic Operations (0x8b-0xa4)
```cpp
case OP_ADD:    // (a b -- a+b)
case OP_SUB:    // (a b -- a-b)
case OP_EQUAL: // (x1 x2 -- bool)
```

#### Cryptographic Operations (0xa6-0xba)
```cpp
case OP_HASH160:   // (in -- RIPEMD160(SHA256(in)))
case OP_CHECKSIG:  // (sig pubkey -- bool)
case OP_CHECKMULTISIG: // Multisignature verification
```

### 3. Condition Stack Management

The VM uses an optimized `ConditionStack` class to handle nested IF/ELSE/ENDIF blocks:

```cpp
class ConditionStack {
    uint32_t m_stack_size = 0;
    uint32_t m_first_false_pos = NO_FALSE;
    
    bool all_true() const { return m_first_false_pos == NO_FALSE; }
    void push_back(bool f);
    void toggle_top();  // For OP_ELSE
};
```

### 4. Script Execution Loop

The main execution happens in `EvalScript()`:

```cpp
bool EvalScript(std::vector<std::vector<unsigned char>>& stack, 
                const CScript& script, 
                unsigned int flags, 
                const BaseSignatureChecker& checker, 
                SigVersion sigversion, 
                ScriptExecutionData& execdata, 
                ScriptError* serror)
```

**Execution Steps:**
1. Initialize stacks and execution context
2. Parse each opcode sequentially
3. Check execution limits (size, ops count, stack depth)
4. Execute opcode if in active execution branch
5. Update condition stack for flow control opcodes
6. Validate final stack state

## Signature Versions and Script Types

### Legacy Scripts (SigVersion::BASE)
- Original Bitcoin script format
- Used in P2PK, P2PKH, and non-segwit P2SH
- Subject to transaction malleability

### Segregated Witness v0 (SigVersion::WITNESS_V0)
- Introduced with BIP 141
- P2WPKH (Pay-to-Witness-PubkeyHash)
- P2WSH (Pay-to-Witness-ScriptHash)
- Fixed transaction malleability

### Taproot/Tapscript (SigVersion::TAPSCRIPT)
- Introduced with BIP 341/342
- Enhanced privacy and efficiency
- New opcodes like `OP_CHECKSIGADD`
- Schnorr signatures

## Script Validation Process

### Transaction Script Verification

The main entry point is `VerifyScript()`:

```cpp
bool VerifyScript(const CScript& scriptSig,      // Input script
                  const CScript& scriptPubKey,   // Output script
                  const CScriptWitness* witness, // Witness data
                  unsigned int flags,            // Validation flags
                  const BaseSignatureChecker& checker,
                  ScriptError* serror)
```

**Validation Steps:**

1. **Input Script (scriptSig) Execution**
   - Must be push-only for P2SH and Segwit
   - Executed first, leaves data on stack

2. **Output Script (scriptPubKey) Execution**
   - Executed with stack from scriptSig
   - Must leave exactly one `true` value on stack

3. **Pay-to-Script-Hash (P2SH) Handling**
   - If output is P2SH, pop and execute the redeemScript
   - Enables more complex spending conditions

4. **Witness Program Verification**
   - Handle Segwit v0 and Taproot witness programs
   - Separate witness data processing

### Signature Verification

Digital signatures are verified through the `BaseSignatureChecker` interface:

```cpp
class BaseSignatureChecker {
    virtual bool CheckECDSASignature(...);     // Legacy ECDSA
    virtual bool CheckSchnorrSignature(...);   // Taproot Schnorr
    virtual bool CheckLockTime(...);           // Timelock verification
    virtual bool CheckSequence(...);           // Sequence verification
};
```

## Error Handling

The VM uses a comprehensive error system defined in `script_error.h`:

```cpp
typedef enum ScriptError_t {
    SCRIPT_ERR_OK = 0,
    SCRIPT_ERR_EVAL_FALSE,           // Script evaluated to false
    SCRIPT_ERR_OP_RETURN,            // OP_RETURN encountered
    SCRIPT_ERR_SCRIPT_SIZE,          // Script too large
    SCRIPT_ERR_PUSH_SIZE,            // Push data too large
    SCRIPT_ERR_OP_COUNT,             // Too many operations
    SCRIPT_ERR_STACK_SIZE,           // Stack size limit exceeded
    SCRIPT_ERR_INVALID_STACK_OPERATION,
    SCRIPT_ERR_UNBALANCED_CONDITIONAL,
    // ... many more specific error types
} ScriptError;
```

## Advanced Features

### Taproot Script Path Spending

Taproot enables complex scripts while maintaining privacy:

1. **Merkle Tree Construction**: Scripts organized in a Merkle tree
2. **Control Block**: Proves script inclusion in the tree
3. **Leaf Hash Verification**: Validates the executed script
4. **Tweaked Public Key**: Commits to the script tree

### Validation Weight (Tapscript)

Tapscript introduces validation weight to limit computation:

```cpp
static constexpr int64_t VALIDATION_WEIGHT_PER_SIGOP_PASSED{50};
static constexpr int64_t VALIDATION_WEIGHT_OFFSET{50};

// Weight decreases with each signature verification
execdata.m_validation_weight_left -= VALIDATION_WEIGHT_PER_SIGOP_PASSED;
```

### Multi-signature Verification

`OP_CHECKMULTISIG` implements m-of-n signature verification:

```cpp
// ([sig ...] num_sigs [pubkey ...] num_pubkeys -- bool)
```

**Process:**
1. Pop number of public keys and signatures
2. Iterate through keys and signatures
3. Match signatures to public keys in order
4. Succeed if enough valid signatures found

## Security Considerations

### Consensus Rules vs. Policy Rules

- **Consensus Rules**: Must be enforced by all nodes (hard failures)
- **Policy Rules**: Optional, used to prevent DoS and improve standardness

### Disabled Opcodes

Several opcodes are permanently disabled for security:
```cpp
case OP_CAT: case OP_SUBSTR: case OP_LEFT: case OP_RIGHT:
case OP_INVERT: case OP_AND: case OP_OR: case OP_XOR:
case OP_2MUL: case OP_2DIV: case OP_MUL: case OP_DIV: case OP_MOD:
case OP_LSHIFT: case OP_RSHIFT:
    return set_error(serror, SCRIPT_ERR_DISABLED_OPCODE);
```

### Validation Flags

Validation behavior is controlled by flags like:
- `SCRIPT_VERIFY_P2SH`: Enable P2SH validation
- `SCRIPT_VERIFY_WITNESS`: Enable Segwit validation
- `SCRIPT_VERIFY_TAPROOT`: Enable Taproot validation
- `SCRIPT_VERIFY_STRICTENC`: Require strict signature encoding

## Performance Optimizations

### Condition Stack Optimization

Instead of maintaining a full boolean stack for IF/ELSE conditions, Bitcoin Core uses an optimized representation tracking only the stack size and position of the first false value.

### Signature Caching

The signature verification process can cache results to avoid redundant ECDSA/Schnorr operations during block validation.

### Script Code Separation

For signature verification, the script code is processed to remove signature data, preventing certain types of attacks while maintaining compatibility.

## Conclusion

Bitcoin's Script VM is a carefully designed, security-focused virtual machine that balances functionality with safety. Its stack-based architecture, comprehensive error handling, and evolving feature set (from legacy scripts to Taproot) make it a robust foundation for Bitcoin's programmable money functionality.

The implementation demonstrates how a relatively simple virtual machine can enable complex financial applications while maintaining the security and consensus requirements of a distributed monetary system.