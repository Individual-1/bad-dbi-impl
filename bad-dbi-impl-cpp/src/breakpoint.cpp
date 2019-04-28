#include "breakpoint.hpp"

ostream & breakpoint::operator<<(ostream &os, const breakpoint &bp) {
    out << "addr: " << bp.addr << " | active: " << bp.active;

    return out;
}

bool breakpoint::setup_buf(size_t len) {
    if (orig) {
        free(orig);
        orig = NULL;
    }

    orig_len = len;
    orig = (uint8_t *) calloc(sizeof(uint8_t), orig_len);

    if (!orig) {
        orig_len = 0;
        return false;
    }

    return true;
}

void breakpoint::teardown_buf() {
    orig_len = 0;
    free(orig);
    orig = NULL;
}