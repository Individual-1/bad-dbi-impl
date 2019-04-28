#pragma once

#include <stdint.h>
#include <sys/uio.h>

#include <map>

#include "breakpoint.hpp"

class ptrace_utils {
    private:
        map<uint16_t, breakpoint *> bps;
        uint16_t bp_id_ctr;
        pid_t pid;

    public:
        ptrace_utils(pid_t pid);

        uint16_t set_bp(void *addr);
        bool unset_bp(uint16_t bp_id);
        bool delete_bp(uint16_t bp_id);

        void print_bps();

        bool set_bp_cc(breakpoint &bp);
        bool unset_bp_cc(breakpoint &bp);
};