extern crate errno;
extern crate libc;
extern crate nix;

use errno::Errno;
use libc::{c_long, execve, pid_t, uint64_t};
use nix::Error;
use nix::sys::ptrace;
use nix::sys::signal::Signal;
use nix::sys::wait::{waitpid, WaitPidFlag, WaitStatus};
use nix::unistd::{fork, ForkResult};
use std::ffi::{CString, c_void};
use std::path::Path;
use std::{mem, ptr};

struct Breakpoint {
    orig: c_long,
    addr: AddressType,
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
                    return Breakpoint { orig: word, addr: addr };
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
            return Ok(());
        },
        Err(e) => {
            return Err(e);
        }
    }
}

pub fn run_to_bp(pid: pid_t)
