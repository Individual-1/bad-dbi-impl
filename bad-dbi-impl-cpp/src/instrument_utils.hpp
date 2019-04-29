#pragma once

#include <stdint.h>

extern "C" {
    #include <capstone/capstone.h>
}

// Read 1000 bytes at a time
const size_t INSN_BUFSIZE = 1000;

class basic_block {
    public:
        vector<cs_insn> insns;
        void *start_addr;

        basic_block() : start_addr(0);
}

class instrument_utils {
    private:
        csh cs_handle;
        pid_t pid;
    
    public:
        instrument_utils(pid_t pid);
        bool init_capstone();

        basic_block * disas_basic_block(void *addr);
        void * check_for_basic_block(basic_block &bb, uint8_t *insns, size_t insns_len, void *addr);
}

