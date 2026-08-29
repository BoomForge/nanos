#include <arch/x86/isr.h>
#include <arch/x86/mouse.h>
#include <arch/x86/pic.h>
#include <arch/x86/ports.h>
#include <kernel/input.h>
#include <kernel/print.h>
#include <kernel/types.h>

#define PS2_DATA 0x60u
#define PS2_STATUS 0x64u
#define PS2_COMMAND 0x64u

#define PS2_STATUS_OUTPUT_FULL 0x01u
#define PS2_STATUS_INPUT_FULL 0x02u

static uint8_t packet[4];
static uint32_t packet_index;
static uint32_t packet_size;
static int mouse_ready;
static int mouse_has_wheel;
static uint32_t packet_count;

static void ps2_wait_input_clear(void)
{
    uint32_t timeout;

    timeout = 100000u;
    while ((x86_inb(PS2_STATUS) & PS2_STATUS_INPUT_FULL) != 0u && timeout > 0u) {
        --timeout;
    }
}

static void ps2_wait_output_full(void)
{
    uint32_t timeout;

    timeout = 100000u;
    while ((x86_inb(PS2_STATUS) & PS2_STATUS_OUTPUT_FULL) == 0u && timeout > 0u) {
        --timeout;
    }
}

static void ps2_write_command(uint8_t command)
{
    ps2_wait_input_clear();
    x86_outb(PS2_COMMAND, command);
}

static void ps2_write_data(uint8_t value)
{
    ps2_wait_input_clear();
    x86_outb(PS2_DATA, value);
}

static uint8_t ps2_read_data(void)
{
    ps2_wait_output_full();
    return x86_inb(PS2_DATA);
}

static uint8_t mouse_write(uint8_t value)
{
    ps2_write_command(0xd4u);
    ps2_write_data(value);
    return ps2_read_data();
}

static void mouse_set_sample_rate(uint8_t rate)
{
    (void)mouse_write(0xf3u);
    (void)mouse_write(rate);
}

static uint8_t mouse_get_device_id(void)
{
    (void)mouse_write(0xf2u);
    return ps2_read_data();
}

static void mouse_try_enable_wheel(void)
{
    mouse_set_sample_rate(200u);
    mouse_set_sample_rate(100u);
    mouse_set_sample_rate(80u);
    if (mouse_get_device_id() == 3u) {
        mouse_has_wheel = 1;
        packet_size = 4u;
    }
}

static int32_t mouse_decode_wheel(void)
{
    int32_t wheel;

    if (!mouse_has_wheel) {
        return 0;
    }

    wheel = (int32_t)(packet[3] & 0x0fu);
    if ((wheel & 0x08) != 0) {
        wheel -= 16;
    }
    return wheel;
}

static void mouse_handle_packet(void)
{
    int32_t dx;
    int32_t dy;
    int32_t wheel;
    uint8_t buttons;

    if ((packet[0] & 0x08u) == 0u) {
        return;
    }

    dx = (int32_t)packet[1];
    dy = (int32_t)packet[2];
    if ((packet[0] & 0x10u) != 0u) {
        dx -= 256;
    }
    if ((packet[0] & 0x20u) != 0u) {
        dy -= 256;
    }

    buttons = (uint8_t)(packet[0] & 0x07u);
    wheel = mouse_decode_wheel();
    kernel_input_on_mouse(dx, -dy, buttons, wheel);

    ++packet_count;
    if (packet_count == 1u) {
        print_writeln("mouse packet received");
    }
}

static void mouse_irq(struct interrupt_frame *frame)
{
    uint8_t data;

    (void)frame;

    data = x86_inb(PS2_DATA);
    if (!mouse_ready) {
        return;
    }

    if (packet_index == 0u && (data & 0x08u) == 0u) {
        return;
    }

    packet[packet_index] = data;
    ++packet_index;
    if (packet_index == packet_size) {
        packet_index = 0u;
        mouse_handle_packet();
    }
}

void mouse_init(void)
{
    uint8_t status;

    ps2_write_command(0xa8u);

    ps2_write_command(0x20u);
    ps2_wait_output_full();
    status = x86_inb(PS2_DATA);
    status = (uint8_t)(status | 0x02u);
    status = (uint8_t)(status & 0xdfu);

    ps2_write_command(0x60u);
    ps2_write_data(status);

    (void)mouse_write(0xf6u);

    packet_index = 0u;
    packet_size = 3u;
    mouse_has_wheel = 0;
    packet_count = 0u;
    mouse_try_enable_wheel();
    (void)mouse_write(0xf4u);
    mouse_ready = 1;

    isr_set_handler(PIC_IRQ_BASE + 12u, mouse_irq);
    pic_clear_mask(2u);
    pic_clear_mask(12u);

    print_writeln("mouse initialized");
}
