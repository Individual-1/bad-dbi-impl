extern crate errno;
extern crate libc;
extern crate nix;

use errno::Errno;
use libc::{c_long, c_uchar, execve, pid_t, uint64_t};
use nix::Error;
use nix::sys::ptrace;
use nix::sys::signal::Signal;
use nix::sys::uio::{IoVec, RemoteIoVec, process_vm_readv, process_vm_writev}
use nix::sys::wait::{waitpid, WaitPidFlag, WaitStatus};
use nix::unistd::{fork, ForkResult};
use std::ffi::{CString, c_void};
use std::path::Path;
use std::{mem, ptr};

struct Breakpoint {
    orig: c_long,
    addr: AddressType,
}

struct Breakpoint_v {
    orig: c_uchar,
    addr: usize,
}

fn set_bp(pid: pid_t, addr: ptrace::AddressType) -> Result<Breakpoint, Error> {
    match ptrace::read(pid, addr) {
        Ok(word) => {
            // Create a mask that zeroes out the top byte
            let mut patched: c_long = !0 >> 8;
            let patched_ptr: *mut c_void = &mut patched as *mut _ as *mut c_void;
            patched &= word;
            // Shift `int3` to the top byte and OR it into the masked word
            patched |= (0xCC as c_long << ((mem::size_of<c_long>() - 1) * 8));
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

fn unset_bp(pid: pid_t, bp: Breakpoint) -> Result<(), Error)> {
    let bp_orig_ptr: *mut c_void = &mut bp.orig as *mut _ as *mut c_void;
    match ptrace::write(pid, bp.addr, bp_orig_ptr) {
        Ok(_) => {
            return Ok();
        },
        Err(e) => {
            return Err(e);
        }
    }
}

fn set_bp_v(pid: pid_t, addr: usize) -> Result<Breakpoint_v, Error)> {
    // First, read the original byte value
    let mut orig = vec![0; 1];
    let remote_iov = RemoteIoVec { base: addr, len: 1 };
    let rnum = process_vm_readv(pid,
                                &[IoVec::from_mut_slice(&mut orig)],
                                &[remote_iov]);
    if (rnum != 1) {
        return Err("Failed to read target byte");
    }
    
    // Now write 0xCC into the spot to trigger an int3
    let interrupt = vec![0xCC, 1];
    let wnum = process_vm_writev(pid,
                                 &[IoVec::from_slice(&interrupt)],
                                 &[remote_iov]);

    if (wnum != 1) {
        return Err("Failed to write interrupt to target addr");
    }

    return Ok(Breakpoint_v { orig: orig[0], addr: addr });
}

fn unset_bp_v(pid: pid_t, bp: Breakpoint_v) -> Result<(), Error)> {
    let orig = vec![bp.orig; 1];
    let remote_iov = RemoteIoVec { base: addr, len: 1 };
    let wnum = process_vm_writev(pid,
                                 &[IoVec::from_slice(&orig)],
                                 &[remote_iov]);

    if (wnum != 1) {
        return Err("Failed to write original byte to target addr");
    }

    return Ok();
}