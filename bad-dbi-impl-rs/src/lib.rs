extern crate capstone;
extern crate errno;
extern crate keystone;
extern crate libc;
extern crate nix;

mod defs;
mod instrument_util;
mod ptrace_util;

use libc::{execve};
use nix::Error;
use nix::sys::ptrace;
use nix::sys::signal::Signal;
use nix::sys::wait::{waitpid, WaitStatus};
use nix::unistd::{fork, ForkResult, Pid};
use std::ffi::CString;
use std::{ptr};
use std::path::Path;

pub fn fork_child(filename: &Path, args: &[str]) -> Result<Pid, Error> {
    match fork() {
        Ok(ForkResult::Parent{ child }) => {
            handle_signals(child);
            return Ok(child);            
        },
        Ok(ForkResult::Child) => {
            exec_child(filename, args);
            return Ok(Pid::from_raw(0));
        },
        Err(e) => {
            return Err(e);
        }
    }
}

fn handle_signals(pid: Pid) -> Result<_, uint32> {
    loop {
        match waitpid(pid, None) {
            Ok(WaitStatus::Stopped(pid, Signal::SIGTRAP)) => {
                // First we read out registers to determine what address to read from and where to write to
                let regs = defs::x64_regs;
                match ptrace::ptrace(ptrace::Request::PTRACE_GETREGS, pid, ptr::null(), cast::transmute(&regs)) {
                    Ok(_) => {
                        regs.rax
                    },
                    Err(_) => {

                    }
                }
            },
            Ok(_) => {
                panic!("Unhandled stop");
            },
            Err(e) => {
                panic!("Unhandled error");
            }
        }
    }
}

fn exec_child(filename: &Path, args: &[Str]) ->() {
    let c_filename = &CString::new(filename.to_str().unwrap()).unwrap();
    ptrace::traceme();
    // TODO: Add args support
    execve(c_filename, &[], &[]).ok().expect("execve failed");
    unreachable!();
}
