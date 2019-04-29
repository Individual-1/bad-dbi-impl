#include "debug_utils.hpp"

debug_utils::debug_utils(pid_t pid) {
    this->bp_id_exhausted = false;
    this->bp_id_ctr = 1;
    this->pid = pid;
}

uint16_t debug_utils::set_bp(void *addr) {
    map<void *, uint16_t>::iterator rit = reverse_bps.find(addr);
    if (rit != reverse_bps.end()) {
        return rit->second;
    }

    breakpoint *bp = new breakpoint();
    uint16_t bp_id = 0;

    /*
        Try to find an available breakpoint id
        We maintain a list of deleted or otherwise available ids before bp_id_ctr
        If none are available, then use bp_id_ctr
        If bp_id_ctr == UINT16_MAX, then we have exhausted our available increments
    */
    if (!avail_bp_id.empty()) {
        bp_id = avail_bp_id.front();
        avail_bp_id.pop();
    } else {
        if (!bp_id_exhausted) {
            bp_id = bp_id_ctr;

            if (bp_id_ctr == UINT16_MAX) {
                bp_id_exhausted = true;
            } else {
                bp_id_ctr++;
            }
        } else {
            cout << "All breakpoint ids exhausted, can't set additional\n";
            return 0;
        }
    }

    // Set our target breakpoint address
    bp->addr = addr;

    // Pre-emptively set this breakpoint to active
    bp->active = true;

    // Add it to our map of breakpoints and increment our new minimum
    bps.emplace(bp_id, bp);
    reverse_bps.emplace(bp, bp_id);

    // If we fail to set the breakpoint, then clean up
    if (!set_bp_cc(*bp)) {
        bps.erase(bp_id);
        reverse_bps.erase(bp);
        bp->teardown_buf();
        delete bp;

        // Return the bp_id we used
        avail_bp_id.push(bp_id);

        return 0;
    }

    return bp_id;
}

bool debug_utils::unset_bp(uint16_t bp_id) {
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

bool debug_utils::delete_bp(uint16_t bp_id) {
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

    // Delete the reverse mapping
    reverse_bps.delete_bp(bit->second);

    // Delete the item itself
    delete bit->second;

    // Delete the item from the bps map
    bps.erase(bit);

    avail_bp_id.push(bp_id);

    return true;
}

void debug_utils::print_bps() {
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
bool debug_utils::set_bp_cc(breakpoint &bp) {
    struct iovec local[1];
    struct iovec remote[1];
    uint8_t *cc_buf;
    ssize_t nr;
    bool result = false;

    if (!bp.setup_buf(1)) {
        cout << "Failed to allocate memory for original memory\n";
        goto cleanup;
    }

    if (read_mem(bp.addr, bp.orig, bp.orig_len) != bp.orig_len) {
        goto cleanup;
    }

    // TODO: determine what is more expensive, looping with a single byte buffer or 
    // allocating a potentially massive buffer and filling it with 0xCC
    cc_buf = (uint8_t *) malloc(bp.orig_len);
    if (!cc_buf) {
        cout << "Failed to allocate memory for 0xCC buffer\n";
        goto cleanup;
    }

    memset(cc_buf, 0xCC, bp.orig_len);

    if (write_mem(bp.addr, cc_buf, bp.orig_len) != bp.orig_len) {
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

bool debug_utils::unset_bp_cc(breakpoint &bp) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nr;

    if (!bp.orig || !bp.addr || !bp.active) {
        cout << "Attempting to unset breakpoint with invalid values\n";
        return false;
    }

    return (write_mem(bp.addr, bp.orig, bp.orig_len) == bp.orig_len);

}

ssize_t read_mem(void *addr, uint8_t *dst, size_t len) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nr;

    local[0].iov_base = (void *) dst;
    local[0].iov_len = len;

    remote[0].iov_base = addr;
    remote[0].iov_len = len;

    return process_vm_readv(pid, local, 1, remote, 1, 0);
}

ssize_t write_mem(void *addr, uint8_t *src, size_t len) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nr;
    bool result = false;

    local[0].iov_base = (void *) dst;
    local[0].iov_len = len;

    remote[0].iov_base = addr;
    remote[0].iov_len = len;

    return process_vm_writev(pid, local, 1, remote, 1, 0);
}