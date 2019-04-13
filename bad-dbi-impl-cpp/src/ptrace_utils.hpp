#pragma once

#include <stdint.h>
#include <sys/uio.h>

class ptrace_utils {
    public:
        struct bp {
            uint8_t *orig; // Original bytes
            size_t orig_len; // Length of original byte buffer
            void *addr; // Address of breakpoint
        };

        bool set_bp_cc(pid_t pid, struct bp &bp);
};