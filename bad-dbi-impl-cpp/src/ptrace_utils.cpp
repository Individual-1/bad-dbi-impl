#include "ptrace_utils.hpp"

uint16_t ptrace_utils::set_bp(void *addr) {
    breakpoint *bp = new breakpoint();

    // Set our target breakpoint address
    bp->addr = addr;

    // Pre-emptively set this breakpoint to active
    bp->active = true;

    // Add it to our map of breakpoints and increment our new minimum
    pair<map<uint16_t, breakpoint *>::iterator, bool> res = bps.emplace(bp_id_ctr, bp);

    // If we fail to set the breakpoint, then clean up
    if (!set_bp_cc(*bp)) {
        bps.erase(res->first);
        bp->teardown_buf();
        delete bp;

        return 0;
    }

    bp_id_ctr++;

    return (bp_id_ctr - 1);
}

bool ptrace_utils::unset_bp(uint16_t bp_id) {
    map<uint16_t, breakpoint *>::iterator bit = bps.find(bp_id);
    if (bit == bps.end()) {
        return false;
    }

    if (!unset_bp_cc(*(bit->second))) {
        return false;
    }

    // Set this item to inactive
    bit->second->active = false;

    return true;
}

bool ptrace_utils::delete_bp(uint16_t bp_id) {
    map<uint16_t, breakpoint *>::iterator bit = bps.find(bp_id);
    if (bit == bps.end()) {
        return false;
    }

    if (bit->second->active) {
        if (!unset_bp_cc(*(bit->second))) {
            return false;
        }
    }

    // Clear out our breakpoint buffer
    bit->second->teardown_buf();

    // Delete the item from the bps map
    bps.erase(bit);

    // Finally, delete the item itself
    delete bit->second;

    return true;
}

void ptrace_utils::print_bps() {
    cout << "Current breakpoints:\n";
    for (map<uint16_t, bp *>::iterator it = bps.begin();
         it != bps.end(); it++) {
            if (it->second == NULL) {
                continue;
            }

            cout << "id: " << it->first << " " << *(it->second) << "\n";
         }
    cout << "===\n"
}

/*
    This is a dumb method which doesn't parse instructions to figure out
    what to actually replace, in the future we can write a wrapper around this
    to be more intelligent about not destroying everything
*/
bool ptrace_utils::set_bp_cc(breakpoint &bp) {
    struct iovec local[1];
    struct iovec remote[1];
    uint8_t *cc_buf;
    ssize_t nr;
    bool result = false;

    if (!bp.setup_buf(1)) {
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
        bp.teardown_buf();
    }

    free(cc_buf);
    cc_buf = NULL;

    return result;
}

bool ptrace_utils::unset_bp_cc(breakpoint &bp) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nr;
    bool result = false;

    if (!bp.orig || !bp.addr || !bp.active) {
        cout << "Attempting to unset breakpoint with invalid values\n";
        goto cleanup;
    }

    local[0].iov_base = (void *) bp.orig;
    local[0].iov_len = bp.orig_len;

    remote[0].iov_base = bp.addr;
    remote[0].iov_len = bp.orig_len;

    nr = process_vm_writev(pid, local, 1, remote, 1, 0);
    if (nr != bp.orig_len) {
        cout << "Failed to write original data into process\n";
        goto cleanup;
    }

    result = true;
cleanup:
    return result;
}