#pragma once

#include <stdint.h>
#include <sys/uio.h>

#include <map>
#include <queue>

#include "breakpoint.hpp"

class debug_utils {
    private:
        map<uint16_t, breakpoint *> bps;
        map<breakpoint *, uint16_t> reverse_bps;
        queue<uint16_t> avail_bp_id;
        bool bp_id_exhausted;
        uint16_t bp_id_ctr;
        pid_t pid;

        bool set_bp_cc(breakpoint &bp);
        bool unset_bp_cc(breakpoint &bp);

    public:
        debug_utils(pid_t pid);

        uint16_t set_bp(void *addr);
        bool unset_bp(uint16_t bp_id);
        bool delete_bp(uint16_t bp_id);

        void print_bps();

        ssize_t read_mem(void *addr, uint8_t *dst, size_t len);
        ssize_t write_mem(void *addr, uint8_t *src, size_t len);
};