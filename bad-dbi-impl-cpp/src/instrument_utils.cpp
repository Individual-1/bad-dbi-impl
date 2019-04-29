#include "instrument_utils.hpp"
#include "debug_utils.hpp"

instrument_utils::instrument_utils(pid_t pid) {
    this->pid = pid;
    
}

bool instrument_utils::init_capstone() {
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &cs_handle) != CS_ERR_OK) {
        return false;
    }

    return true;
}

basic_block * instrument_utils::disas_basic_block(void *addr) {
    basic_block *bb = new basic_block;
    uint8_t *insn_buf = NULL;
    void *cur_addr = addr;
    ssize_t read_amnt = 0;
    bool result = false;

    insn_buf = (uint8_t *) malloc(sizeof(uint8_t) * INSN_BUFSIZE);
    if (!insn_buf) {
        cout << "Failed to allocate instruction buffer\n";
        goto cleanup;
    }

    bb.start_addr = addr;

    while (true) {
        read_amnt = read_mem(cur_addr, insn_buf, INSN_BUFSIZE);
        if (read_amnt == 0) {
            break;
        }

        cur_addr = check_for_basic_block(*bb, insn_buf, read_amnt, cur_addr);

        if (!cur_addr) {
            result = true;
            break;
        }
    }

cleanup:
    if (!result) {
        delete bb;
    }

    free(insn_buf);
    insn_buf = NULL;

    return result;
}

/*
* This one is a bit strange, basically loop over some instruction buffer and see if it has a control flow instr
* Return values:
* Found a cfi - NULL
* Did not find cfi - void * to next address to start reading from
*/
void * instrument_utils::check_for_basic_block(basic_block &bb, uint8_t *insns, size_t insns_len, void *addr) {
    size_t count = 0;
    cs_insn *parsed;
    void *result = NULL;

    /*
    * Iterate over our disassembled block to find the end of this basic block
    * 
    * Three possible cases
    * 1. We find a jmp/ret inside the block - Return the block of instructions up to and including that instruction
    * 2. We find a jmp/ret at the end of the block - Return the whole block of instructions
    * 3. We don't find a jmp/ret in the block - Return the address the last instruction ends on so parent can read next chunk starting from that address
    */

    count = cs_disasm(cs_handle, insns, insns_len, addr, 0, &parsed);
    
    if (count > 0) {
        bool found = false;

        for (size_t i = 0; i < count; i++) {
            bb.insns.push_back(parsed[i]);
            
            // TODO: can we have this not look so terrible?
            if (cs_insn_group(cs_handle, &parsed[i], X86_GRP_JUMP) ||
                cs_insn_group(cs_handle, &parsed[i], X86_GRP_CALL) ||
                cs_insn_group(cs_handle, &parsed[i], X86_GRP_RET)  ||
                cs_insn_group(cs_handle, &parsed[i], X86_GRP_INT)  ||
                cs_insn_group(cs_handle, &parsed[i], X86_GRP_IRET)  ||
                cs_insn_group(cs_handle, &parsed[i], X86_GRP_BRANCH_RELATIVE))
            {
                found = true;
                break;
            }
        }

        if (!found) {
            // TODO: I'm not comfortable with this int math cast to void *
            result = (void *) (parsed[count-1].address + parsed[count-1].size);
        }

        cs_free(parsed, count);
    }

    return result;
}