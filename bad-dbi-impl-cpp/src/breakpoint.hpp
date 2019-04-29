#pragma once

#include <stdint.h>

class breakpoint {
    public:
        uint8_t *orig; // Original bytes
        size_t orig_len; // Length of original byte buffer
        void *addr; // Address of breakpoint
        bool active;

        breakpoint() {};
        breakpoint(void *addr) addr(addr) {};

        friend ostream &operator<<(ostream &os, const breakpoint &bp);

        bool setup_buf(size_t len);
        void teardown_buf();
};