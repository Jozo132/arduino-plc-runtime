// runtime-test.h - 1.0.0 - 2022-12-11
//
// Copyright (c) 2022 J.Vovk
//
// This file is part of VovkPLCRuntime.
//
// VovkPLCRuntime is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// VovkPLCRuntime is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VovkPLCRuntime.  If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if defined(__RUNTIME_FULL_UNIT_TEST___)
#warning "RUNTIME FULL UNIT TEST ENABLED - Pleas do not use this in production code. This is only for testing purposes and might cause unexpected behaviour."
#elif defined(__RUNTIME_DEBUG__)
#warning "RUNTIME TEST ENABLED - Pleas do not use this in production code. This is only for testing purposes and might cause unexpected behaviour."
#endif // __RUNTIME_DEBUG__


#ifdef __RUNTIME_DEBUG__
template <typename T> struct RuntimeTestCase {
    const char* name;
    RuntimeError expected_error;
    T expected_result;
    // function pointer for startup that takes a RuntimeProgram & type and returns void 
    void (*build)(RuntimeProgram&);
};

#define REPRINT(count, str) for (uint8_t i = 0; i < count; i++) { Serial.print(str); }
#define REPRINTLN(count, str) REPRINT(count, str); Serial.println();

void print_U64(uint64_t big_number) {
    const uint16_t NUM_DIGITS = log10(big_number) + 1;
    char sz[NUM_DIGITS + 1];
    sz[NUM_DIGITS] = 0;
    for (uint16_t i = NUM_DIGITS; i--; big_number /= 10)
        sz[i] = '0' + (big_number % 10);
    Serial.print(sz);
}
void println_U64(uint64_t big_number) {
    print_U64(big_number);
    Serial.println();
}

void print_S64(int64_t big_number) {
    if (big_number < 0) {
        Serial.print('-');
        big_number = -big_number;
    }
    print_U64(big_number);
}
void println_S64(int64_t big_number) {
    print_S64(big_number);
    Serial.println();
}

struct RuntimeTest {
    static const uint16_t memory_size = 16;
    static const uint16_t stack_size = 32;
    static const uint16_t program_size = 64;
    uint8_t memory[memory_size];
    VovkPLCRuntime runtime = VovkPLCRuntime(stack_size, memory, memory_size);
    RuntimeProgram program = RuntimeProgram(program_size);
    template <typename T> void run(const RuntimeTestCase<T>& test) {
        REPRINTLN(50, '-');
        program.erase();
        test.build(program);
        Serial.print(F("Running test: ")); Serial.println(test.name);
        RuntimeError status = fullProgramDebug(runtime, program);
        T output = runtime.read<T>();
        bool passed = status == test.expected_error && test.expected_result == output;
        Serial.print(F("Program result: ")); println(output);
        Serial.print(F("Expected result: ")); println(test.expected_result);
        Serial.print(F("Test passed: ")); Serial.println(passed ? F("YES") : F("NO - TEST DID NOT PASS !!!"));
    }

    template <typename T> void review(const RuntimeTestCase<T>& test) {
        size_t offset = Serial.print("Test \"");
        offset += Serial.print(test.name);
        offset += Serial.print('"');
        program.erase();
        test.build(program);
        runtime.cleanRun(program);
        T output = runtime.read<T>();
        bool passed = test.expected_result == output;
        for (; offset < 40; offset++) Serial.print(' ');
        Serial.println(passed ? F("Passed") : F("FAILED !!!"));
    }

    static RuntimeError fullProgramDebug(VovkPLCRuntime& runtime, RuntimeProgram& program) {
        runtime.clear(program);
        program.println();
        uint16_t program_pointer = 0;
        bool finished = false;
        RuntimeError status = RTE_SUCCESS;
        while (!finished) {
            program_pointer = program.getLine();
            long t = micros();
            status = runtime.step(program);
            t = micros() - t;
            float ms = (float) t * 0.001;
            if (status == RTE_PROGRAM_EXITED) finished = true;
            bool problem = status != RTE_SUCCESS && status != RTE_PROGRAM_EXITED;
            if (problem) {
                const char* error = getRuntimeErrorName(status);
                Serial.print(F("Error at program pointer "));
                Serial.print(program_pointer);
                Serial.print(F(": "));
                Serial.println(error);
                return status;
            }
            Serial.print(F("Stack trace @Program ["));
            Serial.print(program_pointer);
            Serial.print(F("]: "));
            runtime.printStack();
            Serial.print(F("   <= "));
            program.printOpcodeAt(program_pointer);
            Serial.print(F("  (executed in "));
            Serial.print(ms, 3);
            Serial.print(F(" ms)"));
            Serial.println();
            if (program.finished()) finished = true;
        }
        long t = micros();
        runtime.cleanRun(program);
        t = micros() - t;
        float ms = (float) t * 0.001;
        Serial.print(F("Leftover ")); runtime.printStack(); Serial.println();
        Serial.print(F("Time to execute program: ")); Serial.print(ms, 3); Serial.println(F(" ms"));
        if (status != RTE_SUCCESS) { Serial.print(F("Debug failed with error: ")); Serial.println(getRuntimeErrorName(status)); }
        return status;
    }

    template <typename T> void println(T result) { Serial.println(result); }
    void println(uint64_t result) { println_U64(result); }
    void println(int64_t result) { println_S64(result); }
};


#ifdef __RUNTIME_FULL_UNIT_TEST___

RuntimeTest Tester;

RuntimeTestCase<uint8_t> case_add_U8 = { "add_U8 => (1 + 2) * 3", RTE_SUCCESS, 9, [](RuntimeProgram& program) {
    program.pushU8(1);
    program.pushU8(2);
    program.push(ADD, U8);
    program.pushU8(3);
    program.push(MUL, U8);
} };
RuntimeTestCase<uint16_t> case_add_U16 = { "add_U16 => (1 + 2) * 3", RTE_SUCCESS, 9, [](RuntimeProgram& program) {
    program.pushU16(1);
    program.pushU16(2);
    program.push(ADD, U16);
    program.pushU16(3);
    program.push(MUL, U16);
} };

RuntimeTestCase<uint32_t> case_add_U32 = { "add_U32 => (1 + 2) * 3", RTE_SUCCESS, 9, [](RuntimeProgram& program) {
    program.pushU32(1);
    program.pushU32(2);
    program.push(ADD,U32);
    program.pushU32(3);
    program.push(MUL,U32);
} };
RuntimeTestCase<uint64_t> case_add_U64 = { "add_U64 => (1 + 2) * 3", RTE_SUCCESS, 9, [](RuntimeProgram& program) {
    program.pushU64(1);
    program.pushU64(2);
    program.push(ADD, U64);
    program.pushU64(3);
    program.push(MUL, U64);
} };

RuntimeTestCase<int8_t> case_sub_S8 = { "sub_S8 => (1 - 2) * 3", RTE_SUCCESS, -3, [](RuntimeProgram& program) {
    program.pushS8(1);
    program.pushS8(2);
    program.push(SUB,S8);
    program.pushS8(3);
    program.push(MUL,S8);
} };
RuntimeTestCase<int16_t> case_sub_S16 = { "sub_S16 => (1 - 2) * 3", RTE_SUCCESS, -3, [](RuntimeProgram& program) {
    program.pushS16(1);
    program.pushS16(2);
    program.push(SUB,S16);
    program.pushS16(3);
    program.push(MUL,S16);
} };

RuntimeTestCase<int32_t> case_sub_S32 = { "sub_S32 => (1 - 2) * 3", RTE_SUCCESS, -3, [](RuntimeProgram& program) {
    program.pushS32(1);
    program.pushS32(2);
    program.push(SUB,S32);
    program.pushS32(3);
    program.push(MUL,S32);
} };
RuntimeTestCase<int64_t> case_sub_S64 = { "sub_S64 => (1 - 2) * 3", RTE_SUCCESS, -3, [](RuntimeProgram& program) {
    program.pushS64(1);
    program.pushS64(2);
    program.push(SUB,S64);
    program.pushS64(3);
    program.push(MUL,S64);
} };

RuntimeTestCase<float> case_sub_F32 = { "sub_F32 => (0.1 + 0.2) * -1", RTE_SUCCESS, -0.3, [](RuntimeProgram& program) {
    program.pushF32(0.1);
    program.pushF32(0.2);
    program.push(ADD,F32);
    program.pushF32(-1);
    program.push(MUL,F32);
} };
RuntimeTestCase<double> case_sub_F64 = { "sub_F64 => (0.1 + 0.2) * -1", RTE_SUCCESS, -0.3, [](RuntimeProgram& program) {
    program.pushF64(0.1);
    program.pushF64(0.2);
    program.push(ADD,F64);
    program.pushF64(-1);
    program.push(MUL,F64);
} };

// Bitwise operations
RuntimeTestCase<uint8_t> case_bitwise_and_X8 = { "bitwise_and_X8", RTE_SUCCESS, 0b00000101, [](RuntimeProgram& program) {
    program.pushU8(0b00001111);
    program.pushU8(0b01010101);
    program.push(BW_AND_X8);
} };
RuntimeTestCase<uint16_t> case_bitwise_and_X16 = { "bitwise_and_X16", RTE_SUCCESS, 0x000F, [](RuntimeProgram& program) {
    program.pushU16(0x00FF);
    program.pushU16(0xF00F);
    program.push(BW_AND_X16);
} };
RuntimeTestCase<uint32_t> case_bitwise_and_X32 = { "bitwise_and_X32", RTE_SUCCESS, 0x0F0F0000, [](RuntimeProgram& program) {
    program.pushU32(0x0F0F0F0F);
    program.pushU32(0xFFFF0000);
    program.push(BW_AND_X32);
} };
RuntimeTestCase<uint64_t> case_bitwise_and_X64 = { "bitwise_and_X64", RTE_SUCCESS, 0b00000101, [](RuntimeProgram& program) {
    program.pushU64(0b00001111);
    program.pushU64(0b01010101);
    program.push(BW_AND_X64);
} };

// Logic (boolean) operations
RuntimeTestCase<bool> case_logic_and = { "logic_and => true && false", RTE_SUCCESS, false, [](RuntimeProgram& program) {
    program.pushBool(true);
    program.pushBool(false);
    program.push(LOGIC_AND);
} };
RuntimeTestCase<bool> case_logic_and_2 = { "logic_and => true && true", RTE_SUCCESS, true, [](RuntimeProgram& program) {
    program.pushBool(true);
    program.pushBool(true);
    program.push(LOGIC_AND);
} };
RuntimeTestCase<bool> case_logic_or = { "logic_or => true || false", RTE_SUCCESS, true, [](RuntimeProgram& program) {
    program.pushBool(true);
    program.pushBool(false);
    program.push(LOGIC_OR);
} };
RuntimeTestCase<bool> case_logic_or_2 = { "logic_or => false || false", RTE_SUCCESS, false, [](RuntimeProgram& program) {
    program.pushBool(false);
    program.pushBool(false);
    program.push(LOGIC_OR);
} };

// Comparison operations
RuntimeTestCase<bool> case_cmp_eq = { "cmp_eq => 1 == 1", RTE_SUCCESS, true, [](RuntimeProgram& program) {
    program.pushBool(1);
    program.pushBool(1);
    program.push(CMP_EQ, BOOL);
} };
RuntimeTestCase<bool> case_cmp_eq_2 = { "cmp_eq => 0.3 == 0.3", RTE_SUCCESS, true, [](RuntimeProgram& program) {
    program.pushF32(0.3);
    program.pushF32(0.3);
    program.push(CMP_EQ, F32);
} };
RuntimeTestCase<bool> case_cmp_eq_3 = { "cmp_eq => 0.29 == 0.31", RTE_SUCCESS, false, [](RuntimeProgram& program) {
    program.pushF32(0.29);
    program.pushF32(0.31);
    program.push(CMP_EQ, F32);
} };

// Jump operations
RuntimeTestCase<uint8_t> case_jump = { "jump => 1", RTE_PROGRAM_EXITED, 1, [](RuntimeProgram& program) {
    program.pushU8(1); // 0 [+2]
    program.pushJMP(13); // 2 [+3]
    program.pushU8(1); // 5 [+2]
    program.push(ADD, U8); // 7 [+2]
    program.pushU8(3); // 9 [+2]
    program.push(MUL, U8); // 11 [+2]
    program.push(EXIT); // 13 [+1]
} };


void runtime_test() {
    Serial.println(F("--------------------------------------------------"));
    Serial.println(F("Executing Runtime Unit Tests..."));
    Tester.run(case_add_U8);
    Tester.run(case_add_U16);
    Tester.run(case_add_U32);
    Tester.run(case_add_U64);
    Tester.run(case_sub_S8);
    Tester.run(case_sub_S16);
    Tester.run(case_sub_S32);
    Tester.run(case_sub_S64);
    Tester.run(case_sub_F32);
    Tester.run(case_sub_F64);

    Tester.run(case_bitwise_and_X8);
    Tester.run(case_bitwise_and_X16);
    Tester.run(case_bitwise_and_X32);
    Tester.run(case_bitwise_and_X64);

    Tester.run(case_logic_and);
    Tester.run(case_logic_and_2);
    Tester.run(case_logic_or);
    Tester.run(case_logic_or_2);

    Tester.run(case_cmp_eq);
    Tester.run(case_cmp_eq_2);
    Tester.run(case_cmp_eq_3);
    Tester.run(case_jump);

    Serial.println(F("Runtime Unit Tests Completed."));
    Serial.println(F("--------------------------------------------------"));
    Serial.println(F("Report:"));
    Serial.println(F("--------------------------------------------------"));
    Tester.review(case_add_U8);
    Tester.review(case_add_U16);
    Tester.review(case_add_U32);
    Tester.review(case_add_U64);
    Tester.review(case_sub_S8);
    Tester.review(case_sub_S16);
    Tester.review(case_sub_S32);
    Tester.review(case_sub_S64);
    Tester.review(case_sub_F32);
    Tester.review(case_sub_F64);

    Tester.review(case_bitwise_and_X8);
    Tester.review(case_bitwise_and_X16);
    Tester.review(case_bitwise_and_X32);
    Tester.review(case_bitwise_and_X64);

    Tester.review(case_logic_and);
    Tester.review(case_logic_and_2);
    Tester.review(case_logic_or);
    Tester.review(case_logic_or_2);

    Tester.review(case_cmp_eq);
    Tester.review(case_cmp_eq_2);
    Tester.review(case_cmp_eq_3);
    Tester.review(case_jump);

    Serial.println(F("--------------------------------------------------"));
}

#else // __RUNTIME_DEBUG__

bool runtime_test_called = false;
void runtime_test() {
    if (runtime_test_called) return;
    Serial.println(F("Unit tests are disabled."));
    runtime_test_called = true;
}

#endif


#else // Production mode


    bool runtime_test_called = false;
void runtime_test() {
    if (runtime_test_called) return;
    Serial.println(F("Unit tests are disabled in production mode."));
    runtime_test_called = true;
}
class RuntimeTest {
public:
    static RuntimeError fullProgramDebug(VovkPLCRuntime& runtime, RuntimeProgram& program) {
        program.print();
        Serial.println(F("Runtime working in production mode. Full program debugging is disabled."));
        runtime.clear(program);
        runtime.cleanRun(program);
        return program.status;
    }
};

#endif