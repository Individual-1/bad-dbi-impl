extern crate capstone;
extern crate keystone;

use capstone::{InsnGroupIter, InsnGroupType, Instructions};
use capstone::prelude::{Capstone, CsResult, InsnDetail};

struct basic_block {
    instrs: Vec<>

}

fn disas_basic_block(cs: &mut Capstone, code: &[u8], addr: u64, instr: &mut Vec<String>) -> (Instructions<'a>, usize) {
    let insns = cs.disasm_all(code, addr)?;
    let mut cnt: usize = 0;
    let mut found: bool = false;

    /*
    * Iterate over our disassembled block to find the end of this basic block
    * 
    * Three possible cases
    * 1. We find a jmp/ret inside the block - Return the block of instructions up to and including that instruction
    * 2. We find a jmp/ret at the end of the block - Return the whole block of instructions
    * 3. We don't find a jmp/ret in the block - Return the address the last instruction ends on so parent can read next chunk starting from that address
    */
    while (cnt < insns.len()) {
        let instr = insns[cnt];
        
        let detail: InsnDetail  = cs.insn_detail(&instr).expect(("Failed to get instruction detail");
        let groups: InsnGroupIter = detail.groups();

        for group in groups {
            found = match group {
                InsnGroupType.CS_GRP_IRET => true,
                InsnGroupType.CS_GRP_JUMP => true,
                InsnGroupType.CS_GRP_RET => true,
                _ => false,
            }
        }

        match found {
            true => break,
            false => cnt += 1,
        }
    }

    match found {
        // Handle case where we find an end to the basic block
        true => {
            return (insns[0 .. cnt], 0);
        },
        // Handle case where we don't find an end
        false => {
            let instr = insns[cnt];
            let end_addr: usize = instr.address + instr.bytes.len();
            return (insns, end_addr);
        }
    }
}