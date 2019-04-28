#pragma once

#include <stdint.h>
#include <sys/uio.h>

#include <map>

class ptrace_utils {
    private:
        typedef struct bp {
            uint8_t *orig; // Original bytes
            size_t orig_len; // Length of original byte buffer
            void *addr; // Address of breakpoint
        } bp_t;

        map<uint16_t, bp_t *> breakpoints;
        uint16_t bp_id_ctr;
        pid_t pid;

    public:
        ptrace_utils(pid_t pid): bp_id_ctr(0), pid(pid) {}

        uint16_t set_breakpoint(void *addr);
        bool unset_breakpoint(uint16_t bp_id);

        void print_breakpoints();

        bool set_bp_cc(bp_t &bp);
        bool unset_bp_cc(bp_t &bp);
};