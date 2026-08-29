# NanOS Application ABI

This document describes the first NanOS graphical application ABI. The goal is
to keep the contract small, direct, and close to the MenuetOS application model:
an app starts, defines a window, waits for OS events, redraws when asked, and
uses explicit syscalls for OS services.

This is ABI version 0. It is intentionally small, but app code should treat the
items in `abi/include/nanos/app.h` and `abi/include/nanos/syscall.h` as the
shared source of truth.

## NX Image Format

NanOS applications use the `.nx` flat binary format.

The first 32 bytes are the image header:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | magic, `NANOS_APP_MAGIC` |
| 4 | 4 | header size, `NANOS_APP_HEADER_SIZE` |
| 8 | 4 | entry offset from image start |
| 12 | 4 | file image size in bytes |
| 16 | 4 | requested mapped image memory size in bytes |
| 20 | 4 | requested stack mapping size in bytes |
| 24 | 4 | initial stack top virtual address |
| 28 | 4 | optional icon offset, or `NANOS_APP_ICON_NONE` |

All fields are little-endian 32-bit values.

The image is loaded at `NANOS_APP_LOAD_BASE` and zero-filled up to the requested
mapped image memory size. That mapped image memory is writable, so app globals
and simple BSS-like storage can live after the file image. The first instruction
executed is:

```text
NANOS_APP_LOAD_BASE + entry_offset
```

The userspace stack is mapped at `NANOS_APP_STACK_BASE` with the requested stack
size. The initial stack pointer is the `stack_top` header field. A normal app
uses:

```text
NANOS_APP_STACK_BASE + NANOS_APP_STACK_SIZE
```

The kernel validates user pointers only inside the mapped image memory range and
the mapped stack range.

## Entry State

Apps enter in 32-bit ring 3 mode. The stack contains one optional launch
parameter string:

| Stack value | Meaning |
| --- | --- |
| `[esp + 0]` | pointer to a null-terminated parameter string |
| `[esp + 4]` | parameter length in bytes, excluding the null byte |

The parameter string is copied by the kernel into the app stack and is limited
to `NANOS_APP_PARAM_MAX` bytes including the null byte. Empty parameters use an
empty string and length `0`.

## Syscall Calling Convention

NanOS syscalls use `int 0x80`.

| Register | Meaning |
| --- | --- |
| `eax` | syscall number |
| `ebx` | argument 0 |
| `ecx` | argument 1 |
| `edx` | argument 2 |
| `esi` | argument 3 |
| `eax` after return | result |

Pointers passed to syscalls must point into the app image or app stack.

## Graphical App Loop

A normal graphical app should:

1. Set its event mask with `NANOS_SYSCALL_SET_EVENT_MASK`.
2. Wait with `NANOS_SYSCALL_WAIT_EVENT`.
3. On `NANOS_EVENT_REDRAW`, call `NANOS_SYSCALL_WINDOW_DRAW` with
   `NANOS_WINDOW_DRAW_BEGIN`, call `NANOS_SYSCALL_DRAW_WINDOW`, redraw the
   client area, then call `NANOS_SYSCALL_WINDOW_DRAW` with
   `NANOS_WINDOW_DRAW_END`.
4. On key, button, or mouse events, fetch event data and update only the app
   state that changed.
5. On `NANOS_EVENT_EXIT`, call `NANOS_SYSCALL_EXIT`.

Window position is desktop-owned after the first `DRAW_WINDOW` call. Apps may
pass their preferred initial geometry, but redraws must not be used to move an
existing window.

`NANOS_SYSCALL_DEFINE_BUTTON` uses the same packed position/size arguments as
rectangle drawing: `ebx` is packed x/width, `ecx` is packed y/height, `edx` is
the app-defined nonzero button id, and `esi` is the fill color. The kernel draws
a simple button frame and stores the button hit area until the next
`DRAW_WINDOW` for that process. On click, the app receives `NANOS_EVENT_BUTTON`
and `NANOS_SYSCALL_GET_BUTTON` returns the id.

## Stable Menuet-Style Syscalls

These calls are the first app-facing GUI/event/file ABI:

| Number | Name | Purpose |
| ---: | --- | --- |
| 0 | `NANOS_SYSCALL_DRAW_WINDOW` | create or redraw app window frame |
| 1 | `NANOS_SYSCALL_PUT_PIXEL` | draw one pixel in the app client area |
| 2 | `NANOS_SYSCALL_GET_KEY` | read latest queued key event |
| 4 | `NANOS_SYSCALL_WRITE_TEXT` | draw text in app client area |
| 7 | `NANOS_SYSCALL_PUT_IMAGE` | blit a 32-bit image in the app client area |
| 8 | `NANOS_SYSCALL_DEFINE_BUTTON` | draw/register a client-area button |
| 9 | `NANOS_SYSCALL_PROCESS_INFO` | get process/thread information |
| 10 | `NANOS_SYSCALL_WAIT_EVENT` | wait for next enabled event |
| 11 | `NANOS_SYSCALL_POLL_EVENT` | poll next enabled event |
| 12 | `NANOS_SYSCALL_WINDOW_DRAW` | begin/end a redraw transaction |
| 13 | `NANOS_SYSCALL_DRAW_RECT` | fill rectangle in app client area |
| 14 | `NANOS_SYSCALL_GET_SCREEN_SIZE` | return packed max screen x/y |
| 17 | `NANOS_SYSCALL_GET_BUTTON` | read latest app button id |
| 18 | `NANOS_SYSCALL_APP_CONTROL` | control another app by pid |
| 19 | `NANOS_SYSCALL_START_APP` | start another app by path |
| 23 | `NANOS_SYSCALL_WAIT_EVENT_TIMEOUT` | wait with timeout |
| 37 | `NANOS_SYSCALL_GET_MOUSE` | copy latest mouse event data |
| 38 | `NANOS_SYSCALL_DRAW_LINE` | draw a line in the app client area |
| 40 | `NANOS_SYSCALL_SET_EVENT_MASK` | choose wanted events |
| 47 | `NANOS_SYSCALL_WRITE_NUMBER` | draw an unsigned number in the app client area |
| 51 | `NANOS_SYSCALL_THREAD` | create a thread in the current app |
| 58 | `NANOS_SYSCALL_FILE` | path-oriented file request |
| 60 | `NANOS_SYSCALL_IPC` | fixed-size process message IPC |
| `0xffffffff` | `NANOS_SYSCALL_EXIT` | exit current app |

The earlier fd/cwd/process-control syscalls have been removed from the app ABI.
New graphical apps should use the Menuet-style calls above.

`NANOS_SYSCALL_PUT_PIXEL` takes client-area coordinates in `ebx` and `ecx`,
and a `0x00RRGGBB` color in `edx`.

`NANOS_SYSCALL_DRAW_LINE` takes packed client-area coordinates: `ebx` is
`x1/y1`, `ecx` is `x2/y2`, and `edx` is the line color. The high 16 bits hold
`x`; the low 16 bits hold `y`.

`NANOS_SYSCALL_PUT_IMAGE` takes packed client-area position/size arguments:
`ebx` is packed x/width, `ecx` is packed y/height, and `edx` points to
row-major 32-bit pixels in `0x00RRGGBB` form. The pixel buffer must live in the
app image or stack.

`NANOS_SYSCALL_GET_SCREEN_SIZE` returns `((width - 1) << 16) | (height - 1)`,
matching the MenuetOS convention of reporting the largest valid screen
coordinate.

`NANOS_SYSCALL_GET_MOUSE` copies the latest mouse event into
`struct nanos_mouse_event`. `buttons` is a bitmask using
`NANOS_MOUSE_BUTTON_LEFT`, `NANOS_MOUSE_BUTTON_RIGHT`, and
`NANOS_MOUSE_BUTTON_MIDDLE`. `type` is move, down, up, or wheel. Wheel events
set `type` to `NANOS_MOUSE_TYPE_WHEEL` and place a signed delta in `wheel`;
positive and negative values represent opposite scroll directions.

`NANOS_SYSCALL_GET_KEY` returns a packed key event. The low byte remains the
ASCII character for normal text input, so older character-only code can keep
masking with `0xff`. Bits 8-15 hold the raw x86 set-1 scancode. Special keys
set `NANOS_KEY_FLAG_SPECIAL` and place one of the `NANOS_KEY_SPECIAL_*` values
in bits 24-31. Extended scancodes, such as arrow keys, set
`NANOS_KEY_FLAG_EXTENDED`. The release flag is reserved for key-up delivery;
current keyboard events are key-down only. `NANOS_KEY_FLAG_SHIFT`,
`NANOS_KEY_FLAG_CTRL`, and `NANOS_KEY_FLAG_ALT` report the modifier state at
the time the key was pressed.

`NANOS_SYSCALL_WRITE_NUMBER` takes a simple format in `ebx`, the unsigned value
in `ecx`, packed client-area x/y in `edx`, and color in `esi`. Format
`NANOS_NUMBER_FORMAT_DECIMAL` writes decimal and `NANOS_NUMBER_FORMAT_HEX`
writes hexadecimal. The low 8 bits of the format may request minimum
zero-padding.

`NANOS_SYSCALL_THREAD` currently supports `NANOS_THREAD_CREATE` in `ebx`.
`ecx` is the absolute userspace entry address and `edx` is the initial stack top
inside the app stack range. The new thread shares the app image memory and uses
its own physical stack mapped at the same stack virtual range. The return value
is the new thread pid.

`NANOS_SYSCALL_PROCESS_INFO` uses `ebx` as a pointer to
`struct nanos_process_info` and `ecx` as the selector. Selector
`NANOS_PROCESS_INFO_COUNT` returns the number of live process slots and does not
copy a struct. Selector `NANOS_PROCESS_INFO_CURRENT` copies the caller's
process/thread info. Any other selector is a zero-based live-process index and
copies that process/thread info. On a copied struct, the return value is the
reported pid. The `service_id` field is `0` unless the app has bound a numeric
IPC service id.

`NANOS_SYSCALL_APP_CONTROL` uses `ebx` as the operation and `ecx` as a target
pid. `NANOS_APP_CONTROL_KILL` terminates the target application. If `ecx` names
a worker thread, NanOS resolves it to the owning application and terminates all
threads in that app. Killing the current app does not return to userspace.

`NANOS_SYSCALL_WINDOW_DRAW` uses `ebx` as the phase. Use
`NANOS_WINDOW_DRAW_BEGIN` before a full window redraw and
`NANOS_WINDOW_DRAW_END` after the final drawing syscall. NanOS hides the cursor
during the transaction and restores it at the end, so applications do not paint
through the software cursor.

`NANOS_SYSCALL_IPC` uses `ebx` as the operation, `ecx` as the target PID for
send, and `edx` as a pointer to `struct nanos_ipc_message`.
`NANOS_IPC_SEND` queues `size` bytes from `data` to the target application and
sets `source_pid` to the sender's application id. If `ecx` names a worker
thread, NanOS resolves it to that thread's app owner before queuing the message.
`NANOS_IPC_RECV` copies the next queued message into the struct and returns the
message size, or `0` if no IPC message is queued. A process receives
`NANOS_EVENT_IPC` when an IPC message is waiting and its event mask includes
`NANOS_EVENT_MASK_IPC`.

`NANOS_IPC_PENDING` returns the caller's queued IPC message count.
`NANOS_IPC_PEEK` copies the next queued message into the struct and returns its
size without removing it from the queue, or `0` if no IPC message is queued.

`NANOS_IPC_BIND_SERVICE` uses `ecx` as a numeric service id and binds it to the
calling app. `NANOS_IPC_RESOLVE_SERVICE` uses `ecx` as a numeric service id and
returns the owning app pid, or `0` if no live app owns it.
`NANOS_IPC_UNBIND_SERVICE` removes the caller's binding for that service id.
Each app owns at most one service id for now. Service ids are simple app
discovery names; IPC messages still use app pids as their delivery target.

## Redraw Contract

The desktop owns:

- window position
- window size after user move/resize
- focus
- title buttons
- event routing
- cursor position and shape policy

The app owns:

- its window title string
- its client-area drawing
- its local UI state

App drawing syscalls are clipped by the kernel to the app's client area and to
the currently visible parts of that window. If another window is above an app,
the covered pixels are not writable by the covered app's drawing calls.

When the desktop posts `NANOS_EVENT_REDRAW`, the app must redraw its full client
area. The compositor may clear or repaint window regions at any time before
sending redraw. Desktop-side redraw requests are coalesced: an app should treat
`NANOS_EVENT_REDRAW` as a level-triggered repaint demand, not as a count of
every expose operation and not as a hint that previous client pixels are still
present.

## Reference App

`userspace/guidemo.S` is the canonical ABI example for now. It demonstrates:

- `.nx` header constants from `nanos/app.h`
- direct `int 0x80` syscalls
- event mask setup
- redraw transactions through syscall `12`
- redraw, key, mouse, button, and exit handling
- client-area button definition through syscall `8`
- process info query through syscall `9`
- client-area pixel drawing through syscall `1`
- client-area image blitting through syscall `7`
- client-area line drawing through syscall `38`
- screen-size query through syscall `14`
- client-area number drawing through syscall `47`
- thread creation through syscall `51`
- launching another `.nx` app through syscall `19`
- binding a numeric IPC service id through syscall `60`
- checking pending IPC messages through syscall `60`
- sending an IPC message through syscall `60`
- app-side client drawing inside a desktop-owned window
- app-side widget drawing helpers from `userspace/include/nanos/gui_widgets.inc`
- simple app-side dialog drawing with the same helper include

`userspace/ipcchild.S` is a companion example. It receives
`NANOS_EVENT_IPC`, reads a `struct nanos_ipc_message`, and updates its own
window content from userspace.
