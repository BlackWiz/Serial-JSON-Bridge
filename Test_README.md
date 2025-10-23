# test.c - UART Testing Framework

## Purpose

This file exists to validate UART communication under real-world conditions. Not simulation. Not theory. Actual byte-by-byte transmission and reception on bare-metal hardware.

The goal is simple: prove the driver works before integrating it into the JSON parser pipeline.

## What It Does Now

Currently implements two testing modes via `UART_CONFIG`:

**UART_NORMAL Mode:**
- Transmits "Hello I am Iron Man" every 5 seconds
- Tests basic TX functionality with timing control
- Validates interrupt-driven transmission state machine
- Enables RX buffer readiness for future bidirectional tests

**UART_ECHO Mode (RX_B2B):**
- Receives data over UART
- Immediately echoes it back
- Tests full duplex communication
- Validates RX→TX state transitions without data loss

Both modes verify:
- Interrupt enable/disable sequences work correctly
- State machines don't hang or race
- Buffer management under continuous operation

## Test Scenarios to Implement

### 1. Basic Protocol Validation
- **Single Byte Echo**: Verify minimal payload handling
- **Fixed Length Packets**: Test 10, 50, 100 byte transmissions
- **Null Termination Handling**: Ensure proper string termination detection
- **Line Ending Variations**: Test `\r`, `\n`, `\r\n` detection

### 2. Buffer Boundary Tests
- **Max Buffer Fill (99 bytes)**: Test up to `RX_BUFF_SIZE - 1`
- **Buffer Overflow Trigger**: Send 101+ bytes and verify graceful failure
- **Partial Receive Timeout**: Incomplete packet handling
- **Zero-Length Transmission**: Edge case validation

### 3. Stress & Timing Tests
- **Rapid Fire Transmission**: Back-to-back sends without delays
- **Sustained High Throughput**: Continuous 1KB/sec for 60 seconds
- **Variable Packet Sizes**: Random lengths from 1-100 bytes
- **Transmission Bursts**: Idle→burst→idle patterns

### 4. Error Injection & Recovery
- **Simulated Overrun Errors**: Overwhelm RX buffer intentionally
- **Framing Error Detection**: Test baud rate mismatch scenarios
- **Noise Error Handling**: Validate error flag clearing via `error_reset()`
- **State Machine Recovery**: Verify IDLE return after errors

### 5. JSON Integration Prep
- **Send Raw JSON Strings**: Transmit `{"test": "value"}` and verify reception
- **Large JSON Payloads**: Test with 200+ char JSON documents
- **Escaped Characters**: Validate `\"`, `\\`, `\n` in JSON strings
- **Fragmented JSON Reception**: Multi-interrupt JSON assembly

### 6. Interrupt Behavior Validation
- **ISR Execution Time**: Profile interrupt handler duration
- **Nested Interrupt Handling**: Verify TX/RX interrupt priority
- **Critical Section Integrity**: Test `__disable_irq()` / `__enable_irq()` pairs
- **State Consistency Under Load**: Concurrent TX and RX operations

### 7. Real-World Scenarios
- **Command-Response Pattern**: Send command, wait for ACK, proceed
- **Bidirectional Streaming**: Simultaneous TX and RX active
- **Connection Timeout**: No data for N seconds, system behavior
- **Power Cycle Recovery**: Resume communication after MCU reset

## Testing Approach

**Unit Level**: Each UART function tested in isolation (init, transmit, receive, error handling)

**Integration Level**: Full TX→RX→echo loops with actual data payloads

**Stress Level**: Push hardware limits—max baud, max buffer, max duration

**Failure Analysis**: Intentionally break things to verify error paths work

## Future Additions

- Automated test runner with pass/fail reporting over UART
- Baud rate switching tests (9600 → 115200 → 9600)
- DMA-based UART comparison (interrupt vs DMA throughput)
- Multi-device communication (STM32 ↔ STM32)
- Integration with `jsonprocess.c` for end-to-end pipeline validation

## Current Status

- Basic echo working
- Normal mode transmission validated
- Error detection infrastructure in place
- Ready for systematic test case expansion

---

**Next Step**: Implement packet size stress tests, starting with 10/50/100 byte fixed transmissions.
