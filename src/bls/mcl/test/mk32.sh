g++ -O3 -march=native base_test.cpp ../src/x86.s -m32 -I ~/32/include/ -I ../include/ -I ../../xbyak/ -I ../../cybozulib/include ~/32/lib/libgmp.a ~/32/lib/libgmpxx.a -I ~/32/lib -DNDEBUG
