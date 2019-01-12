use capstone::{InsnGroupIter, InsnGroupType, Instructions};
use capstone::prelude::{Capstone, InsnDetail};
use nix::unistd::{Pid};

use ptrace_util::{read_mem_v, write_mem_v};

// Read 1,000 bytes at a time
const INSN_BUFSIZE: usize = 1000;

struct BasicBlock<'a> {
    insns_vec: Vec<Instructions<'a>>,
    start_addr: usize,
}

impl<'a> BasicBlock<'a> {
    fn new() -> BasicBlock<'a> {
        BasicBlock {
            insns_vec: Vec::new(),
            start_addr: 0,
        }
    }
}

fn disas_basic_block<'a>(pid: Pid, cs: &Capstone, addr: usize) -> Result<BasicBlock<'a>, &'static str> {
    let mut insn_buf: Vec<u8> = vec![0; INSN_BUFSIZE];
    let mut basic_block: BasicBlock = BasicBlock::new();
    let mut cur_addr: usize = addr;

    basic_block.start_addr = addr;

    while true {
        let mut rnum: usize = read_mem_v(pid, cur_addr, &mut insn_buf[..]).expect("Failed to read program memory");
        let rval: (Instructions<'a>, usize) = check_for_basic_block(cs, &insn_buf[0 .. rnum], addr);

        /*
        * If rval.1 is non-zero, then we need to cache the instructions and do another read + basic block check loop
        * Otherwise, we have our basic block
        */
        basic_block.insns_vec.push(rval.0);
        if rval.1 == 0 {
            break;
        } else {
            cur_addr = rval.1;
            continue;
        }
    }

    return Ok(basic_block);
}

fn check_for_basic_block<'a>(cs: &Capstone, code: &[u8], addr: usize) -> (Instructions<'a>, usize) {
    let insns = cs.disasm_all(code, addr as u64).expect("Failed to disassemble instructions");
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
    for insn in insns.iter() {
        let detail: InsnDetail  = cs.insn_detail(&insn).expect("Failed to get instruction detail");
        let groups: InsnGroupIter = detail.groups();
        cnt += 1;

        for group in groups {
            found = match group.0 as u32 {
                InsnGroupType::CS_GRP_IRET => true,
                InsnGroupType::CS_GRP_JUMP => true,
                InsnGroupType::CS_GRP_RET => true,
                _ => false,
            }
        }

        /*
        * If we find the end of a basic block, return just those instructions along with 0 to indicate we found the end
        * If not then just return our currently parsed instructions and the addr of the next to read in
        */
        if found {
            return (cs.disasm_count(code, addr as u64, cnt).expect("Failed to disassemble instructions"), 0 as usize);
        } else if cnt == insns.len() {
            return (insns, (insn.address() + insn.bytes().len() as u64) as usize);
        }
    }

    // Should be impossible to reach this point
    return (insns, 0 as usize);
}