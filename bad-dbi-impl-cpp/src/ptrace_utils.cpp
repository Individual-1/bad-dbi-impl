#include "ptrace_utils.hpp"

/*
    This is a dumb method which doesn't parse instructions to figure out
    what to actually replace, in the future we can write a wrapper around this
    to be more intelligent about not destroying everything
*/
bool ptrace_utils::set_bp_cc(pid_t pid, struct bp &bp) {
    struct iovec local[1];
    struct iovec remote[1];
    uint8_t *cc_buf;
    ssize_t nr;
    bool result = false;

    bp.orig = (uint8_t *) malloc(bp.orig_len);
    if (!bp.orig) {
        cout << "Failed to allocate memory for original memory\n";
        goto cleanup;
    }

    local[0].iov_base = (void *) bp.orig;
    local[0].iov_len = bp.orig_len;

    remote[0].iov_base = bp.addr;
    remote[0].iov_len = bp.orig_len;

    nr = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nr != 1) {
        cout << "Failed to read original instruction\n";
        goto cleanup;
    }

    // TODO: determine what is more expensive, looping with a single byte buffer or 
    // allocating a potentially massive buffer and filling it with 0xCC
    uint8_t *cc_buf = (uint8_t *) malloc(bp.orig_len);
    if (!cc_buf) {
        cout << "Failed to allocate memory for 0xCC buffer\n";
        goto cleanup;
    }

    memset(cc_buf, 0xCC, bp.orig_len);
    local[0].iov_base = (void *) cc_buf;

    nr = process_vm_writev(pid, local, 1, remote, 1, 0);
    if (nr != bp.orig_len) {
        cout << "Failed to write breakpoint (0xCC) into remote memory\n";
        goto cleanup;
    }

    result = true;

cleanup:
    if (!result) {
        free(bp.orig);
        bp.orig = NULL;
    }

    free(cc_buf);
    cc_buf = NULL;

    return result;
}