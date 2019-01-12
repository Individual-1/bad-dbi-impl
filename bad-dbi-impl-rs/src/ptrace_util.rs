use libc::{c_long, c_uchar};
use nix::Error;
use nix::sys::ptrace;
use nix::sys::uio::{IoVec, RemoteIoVec, process_vm_readv, process_vm_writev};
use nix::unistd::{Pid};
use std::ffi::{c_void};
use std::{mem};

struct Breakpoint {
    orig: c_long,
    addr: ptrace::AddressType,
}

struct Breakpoint_v {
    orig: c_uchar,
    addr: usize,
}

fn set_bp(pid: Pid, addr: ptrace::AddressType) -> Result<Breakpoint, Error> {
    match ptrace::read(pid, addr) {
        Ok(word) => {
            // Create a mask that zeroes out the top byte
            let mut patched: c_long = !0 >> 8;
            let patched_ptr: *mut c_void = &mut patched as *mut _ as *mut c_void;
            patched &= word;
            // Shift `int3` to the top byte and OR it into the masked word
            patched |= (0xCC as c_long) << ((mem::size_of::<c_long>() - 1) * 8);
            // Write our patched word in
            match ptrace::write(pid, addr, patched_ptr) {
                Ok(_) => {
                    return Ok(Breakpoint { orig: word, addr: addr });
                },
                Err(e) => {
                    return Err(e);
                }
            }
        },
        Err(e) => {
            return Err(e);
        }
    }
}

fn unset_bp(pid: Pid, bp: Breakpoint) -> Result<u8, nix::Error> {
    let bp_orig_ptr: *mut c_void = &mut bp.orig as *mut _ as *mut c_void;
    match ptrace::write(pid, bp.addr, bp_orig_ptr) {
        Ok(_) => {
            return Ok(0);
        },
        Err(e) => {
            return Err(e);
        }
    }
}

fn set_bp_v(pid: Pid, addr: usize) -> Result<Breakpoint_v, &'static str> {
    // First, read the original byte value
    let mut orig = vec![0; 1];
    let remote_iov = RemoteIoVec { base: addr, len: 1 };
    let rnum = process_vm_readv(pid,
                                &[IoVec::from_mut_slice(&mut orig)],
                                &[remote_iov])
                                .expect("Unexpected error in process_vm_readv");
    if rnum != 1 {
        return Err("Failed to read target byte");
    }
    
    // Now write 0xCC into the spot to trigger an int3
    let interrupt = vec![0xCC, 1];
    let wnum = process_vm_writev(pid,
                                 &[IoVec::from_slice(&interrupt)],
                                 &[remote_iov])
                                 .expect("Unexpected error in process_vm_writev");

    if wnum != 1 {
        return Err("Failed to write interrupt to target addr");
    }

    return Ok(Breakpoint_v { orig: orig[0], addr: addr });
}

fn unset_bp_v(pid: Pid, bp: Breakpoint_v) -> Result<u8, &'static str> {
    let orig = vec![bp.orig; 1];
    let remote_iov = RemoteIoVec { base: bp.addr, len: 1 };
    let wnum = process_vm_writev(pid,
                                 &[IoVec::from_slice(&orig)],
                                 &[remote_iov])
                                 .expect("Unexpected error in process_vm_writev");

    if wnum != 1 {
        return Err("Failed to write original byte to target addr");
    }

    return Ok(0);
}

pub fn read_mem_v(pid: Pid, addr: usize, dst: &mut [u8]) -> Result<usize, &'static str> {
    let remote_iov = RemoteIoVec { base: addr, len: dst.len() };
    let rnum = process_vm_readv(pid,
                                &[IoVec::from_mut_slice(dst)],
                                &[remote_iov])
                                .expect("Unexpected error in process_vm_readv");
    
    if rnum <= 0 {
        return Err("Failed to read target");
    }

    return Ok(rnum);
}

pub fn write_mem_v(pid: Pid, addr: usize, src: &[u8], len: usize) -> Result<usize, &'static str> {
    let mut wlen: usize = len;
    if len == 0 {
        wlen = src.len();
    }

    let remote_iov = RemoteIoVec { base: addr, len: wlen };
    let wnum = process_vm_writev(pid,
                                 &[IoVec::from_slice(src)],
                                 &[remote_iov])
                                 .expect("Unexpected error in process_vm_writev");

    if wnum != wlen {
        return Err("Failed to write full buffer");
    }

    return Ok(wnum);
}