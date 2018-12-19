extern crate libc;
extern crate nix;

use errno:Errno;
use libc::{execve, pid_t, uint64_t};
use nix::Error;
use nix::sys::ptrace;
use nix::sys::signal::Signal;
use nix::sys::wait::{waitpid, WaitPidFlag, WaitStatus};
use nix::unistd::{fork, ForkResult};
use std::ffi::CString;
use std::{mem, ptr};
use std::path::Path;



pub fn fork_child(filename: &Path, args: &[Str]) -> Result<, Error> {
    match fork() {
        Ok(ForkResult::Parent(pid)) => {
            handle_signals(pid);
            return Ok(pid);            
        },
        Ok(ForkResult::Child) => {
            exec_child(filename, args);
        },
        Err(e) => {
            return Err(e);
        }
    }
}

fn handle_signals(pid: pid_t) -> Result<, Error> {
    loop {
        match waitpid(pid, None) {
            Ok(WaitStatus::Stopped(pid, Signal::SIGTRAP)) => {
                // First we read out registers to determine what address to read from and where to write to
                let regs = defs::x64_regs;
                match ptrace::ptrace(ptrace::Request::PTRACE_GETREGS, pid, ptr::null(), cast::transmute(&regs)) {
                    Errno(0) => {
                        regs.rax
                    },
                    _ => {

                    }
                }
            },
            Ok(_) => {
                panic!("Unhandled stop");
                break;
            },
            Err(e) => {
                panic!("Unhandled error");
                return Err(e);
            }
        }
    }
}

fn exec_child(filename: &Path, args: &[Str]) ->() {
    let c_filename = &CString::new(filename.to_str().unwrap()).unwrap();
    ptrace::traceme();
    // TODO: Add args support
    execve(c_filename, &[]], &[]).ok().expect("execve failed");
    unreachable!();
}