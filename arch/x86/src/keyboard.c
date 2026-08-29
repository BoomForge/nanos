#include <arch/x86/isr.h>
#include <arch/x86/keyboard.h>
#include <arch/x86/pic.h>
#include <arch/x86/ports.h>
#include <kernel/input.h>
#include <kernel/types.h>
#include <nanos/syscall.h>

#define KEYBOARD_DATA 0x60u

static int shift_down;
static int ctrl_down;
static int alt_down;
static int extended_pending;

static char scancode_to_ascii(uint8_t code)
{
    static const char normal[128] = {
        0, 27, '1', '2', '3', '4', '5', '6',
        '7', '8', '9', '0', '-', '=', '\b', '\t',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', '\n', 0, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' '
    };
    static const char shifted[128] = {
        0, 27, '!', '@', '#', '$', '%', '^',
        '&', '*', '(', ')', '_', '+', '\b', '\t',
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}', '\n', 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0, '*',
        0, ' '
    };

    if (code >= 128u) {
        return 0;
    }

    if (shift_down) {
        return shifted[code];
    }
    return normal[code];
}

static uint32_t scancode_to_special(uint8_t code, int extended)
{
    if (extended) {
        if (code == 0x47u) {
            return NANOS_KEY_SPECIAL_HOME;
        }
        if (code == 0x48u) {
            return NANOS_KEY_SPECIAL_UP;
        }
        if (code == 0x49u) {
            return NANOS_KEY_SPECIAL_PAGE_UP;
        }
        if (code == 0x4bu) {
            return NANOS_KEY_SPECIAL_LEFT;
        }
        if (code == 0x4du) {
            return NANOS_KEY_SPECIAL_RIGHT;
        }
        if (code == 0x4fu) {
            return NANOS_KEY_SPECIAL_END;
        }
        if (code == 0x50u) {
            return NANOS_KEY_SPECIAL_DOWN;
        }
        if (code == 0x51u) {
            return NANOS_KEY_SPECIAL_PAGE_DOWN;
        }
        if (code == 0x52u) {
            return NANOS_KEY_SPECIAL_INSERT;
        }
        if (code == 0x53u) {
            return NANOS_KEY_SPECIAL_DELETE;
        }
    }

    if (code == 0x01u) {
        return NANOS_KEY_SPECIAL_ESCAPE;
    }
    if (code == 0x0eu) {
        return NANOS_KEY_SPECIAL_BACKSPACE;
    }
    if (code == 0x0fu) {
        return NANOS_KEY_SPECIAL_TAB;
    }
    if (code == 0x1cu) {
        return NANOS_KEY_SPECIAL_ENTER;
    }
    return 0u;
}

static uint32_t modifier_flags(void)
{
    uint32_t flags;

    flags = 0u;
    if (shift_down) {
        flags |= NANOS_KEY_FLAG_SHIFT;
    }
    if (ctrl_down) {
        flags |= NANOS_KEY_FLAG_CTRL;
    }
    if (alt_down) {
        flags |= NANOS_KEY_FLAG_ALT;
    }
    return flags;
}

static uint32_t pack_key_event(uint8_t code, char ch, uint32_t special,
    int extended)
{
    uint32_t key;

    key = (uint32_t)(uint8_t)ch;
    key |= modifier_flags();
    key |= ((uint32_t)code << NANOS_KEY_SCANCODE_SHIFT) &
        NANOS_KEY_SCANCODE_MASK;
    if (extended) {
        key |= NANOS_KEY_FLAG_EXTENDED;
    }
    if (special != 0u) {
        key |= NANOS_KEY_FLAG_SPECIAL;
        key |= special << NANOS_KEY_SPECIAL_SHIFT;
    }
    return key;
}

static void keyboard_irq(struct interrupt_frame *frame)
{
    uint8_t code;
    uint8_t scancode;
    char ch;
    uint32_t special;
    uint32_t key;
    int extended;

    (void)frame;

    code = x86_inb(KEYBOARD_DATA);
    if (code == 0xe0u) {
        extended_pending = 1;
        return;
    }

    extended = extended_pending;
    extended_pending = 0;
    scancode = code & 0x7fu;

    if (scancode == 0x2au || scancode == 0x36u) {
        shift_down = ((code & 0x80u) == 0u);
        return;
    }
    if (scancode == 0x1du) {
        ctrl_down = ((code & 0x80u) == 0u);
        return;
    }
    if (scancode == 0x38u) {
        alt_down = ((code & 0x80u) == 0u);
        return;
    }

    if ((code & 0x80u) != 0u) {
        return;
    }

    ch = scancode_to_ascii(scancode);
    special = scancode_to_special(scancode, extended);
    if (ch != 0 || special != 0u) {
        key = pack_key_event(scancode, ch, special, extended);
        kernel_input_on_key_event(key);
    }
}

void keyboard_init(void)
{
    isr_set_handler(PIC_IRQ_BASE + 1u, keyboard_irq);
    pic_clear_mask(1u);
}
