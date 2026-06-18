#include "sys/io.h"
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char lowercase[128] = {
    0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ',
};

char uppercase[128] = {
    0,   0,   '!',  '"',  '?',  '$', '%', '^',  '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R',  'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S',  'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '\'', '`', 0,   '\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*',  0,   ' ',
};

bool shift_on;
bool caps_lock_on;

void init_keyboard() {
    shift_on = false;
    caps_lock_on = false;
}

void keyboard_handler() {
    uint8_t raw_scan_code = inb(0x60);

    // scan code without press/release bit
    uint8_t key = raw_scan_code & 0x7F;

    // 0 if key press
    char press = raw_scan_code & 0x80;

    switch (key) {
    case 1:
    case 29:
        break;
    // right shift
    case 54:
        if (press == 0) {
            shift_on = true;
        } else {
            shift_on = false;
        }
        break;
    case 56:
        break;
    case 58:
        if (press == 0) {
            caps_lock_on = !caps_lock_on;
        }
        break;
    case 59:
        if (press == 0)
            printf("\nticks: %d\n", get_current_ticks());
        break;
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 87:
    case 88:
        break;
    // left shift
    case 42:
        if (press == 0) {
            shift_on = true;
        } else {
            shift_on = false;
        }
        break;
    default:
        if (press == 0) {
            if (caps_lock_on != shift_on) {
                printf("%c", uppercase[key]);
            } else {
                printf("%c", lowercase[key]);
            }
        }
    }
}
