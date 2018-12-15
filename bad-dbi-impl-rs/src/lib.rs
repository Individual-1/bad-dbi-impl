extern crate libc;
extern crate nix;

use libc::execve;
use nix::Error;
use nix::sys::ptrace;
use nix::unistd::{
    fork,
    ForkResult,
};
use std::ffi::CString;
use std::path::Path;

pub fn fork_child(filename: &Path, args: &[Str]) -> Result<, Error> {
    match fork() {
        Ok(ForkResult::Parent { child, .. }) => {

        }
        Ok(ForkResult::Child) => {

        }
        Err(_) => {

        }
    }
}

fn trace_child(filename: &Path, args: &[Str]) ->() {
    let c_filename = &CString::new(filename.to_str().unwrap()).unwrap();
    ptrace::traceme();
    // TODO: Add args support
    match execve(c_filename, &[]], &[]) {
        Ok(_) => {

        }
        Err(_) => {

        }
    }
    unreachable!();
}