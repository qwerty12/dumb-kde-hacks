#define _GNU_SOURCE
#include <elf.h>
#include <link.h>
#include <dlfcn.h>
#include <xcb/xproto.h>

static typeof(xcb_get_window_attributes_reply) *xcb_get_window_attributes_reply_orig;

static uintptr_t exe_begin = 0;
static uintptr_t exe_end = UINTPTR_MAX;

static int call_originates_from_exe(const void *return_addr)
{
    const uintptr_t addr = (const uintptr_t) return_addr;
    return addr >= exe_begin && addr < exe_end;
}

static int find_exe_range_cb(struct dl_phdr_info *info, size_t size, void *data)
{
    if (info->dlpi_name && info->dlpi_name[0] != '\0')
        return 0;

    uintptr_t start = UINTPTR_MAX;
    uintptr_t end = 0;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[i];

        if (ph->p_type != PT_LOAD)
            continue;

        const uintptr_t seg_start = info->dlpi_addr + ph->p_vaddr;
        const uintptr_t seg_end = seg_start + ph->p_memsz;

        if (seg_start < start)
            start = seg_start;

        if (seg_end > end)
            end = seg_end;
    }

    if (start != UINTPTR_MAX && end != 0) {
        exe_begin = start;
        exe_end = end;
    }

    return 1;
}

__attribute__((visibility("default")))
xcb_get_window_attributes_reply_t *
xcb_get_window_attributes_reply (xcb_connection_t                    *c,
                                 xcb_get_window_attributes_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e)
{
    xcb_get_window_attributes_reply_t *ret = xcb_get_window_attributes_reply_orig(c, cookie, e);

    if (ret && !e && call_originates_from_exe(__builtin_return_address(0))) {
        if (!(ret->all_event_masks & XCB_EVENT_MASK_BUTTON_PRESS))
            ret->do_not_propagate_mask |= XCB_EVENT_MASK_BUTTON_PRESS; // make `checkWindowOrDescendantWantButtonEvents()` return false
    }

    return ret;
}

__attribute__((constructor))
void init(void)
{
    xcb_get_window_attributes_reply_orig = dlsym(RTLD_NEXT, "xcb_get_window_attributes_reply");
    dl_iterate_phdr(find_exe_range_cb, NULL); // not-so-important safeguard
}
