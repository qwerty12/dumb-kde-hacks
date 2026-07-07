#include <stdint.h>
#include <string.h>
#include <dbus/dbus.h>

#include "plthook.h"

#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)

#ifdef POLKIT_CHECK_AUTH
const char* try_get_procname();
dbus_bool_t polkit_check_suspend_ignore_inhibit(int);

static dbus_bool_t check_auth = FALSE;
#endif

static dbus_bool_t message_is_suspend_true(DBusMessage *message)
{
    const char *sig = dbus_message_get_signature(message);
    if (UNLIKELY(!sig || strcmp(sig, "b") != 0))
        return FALSE;

    DBusMessageIter iter;
    dbus_message_iter_init(message, &iter);

    dbus_bool_t value = FALSE;
    dbus_message_iter_get_basic(&iter, &value);

    return value == TRUE;
}

dbus_bool_t
dbus_connection_send_with_reply_hook (DBusConnection     *connection,
                                      DBusMessage        *message,
                                      DBusPendingCall   **pending_return,
                                      int                 timeout_milliseconds)
{
    if (!pending_return)
        return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);

    {
        const char *msg_interface = dbus_message_get_interface(message);
        if (!msg_interface || strcmp(msg_interface, "org.freedesktop.login1.Manager") != 0)
            return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);

        const char *msg_member = dbus_message_get_member(message);
        if (UNLIKELY(!msg_member) || strcmp(msg_member, "Suspend") != 0 || UNLIKELY(!message_is_suspend_true(message)))
            return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);
    }

#ifdef POLKIT_CHECK_AUTH
    if (check_auth && !polkit_check_suspend_ignore_inhibit(500))
        return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);
#endif

    DBusMessage *replacement = dbus_message_new_method_call(
        dbus_message_get_destination(message),
        dbus_message_get_path(message),
        "org.freedesktop.login1.Manager",
        "SuspendWithFlags");
    if (UNLIKELY(!replacement))
        return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);

    if (UNLIKELY(!dbus_message_append_args(replacement, DBUS_TYPE_UINT64, (const dbus_uint64_t[]) { UINT64_C(1) << 4 }, DBUS_TYPE_INVALID))) { // SD_LOGIND_SKIP_INHIBITORS
        dbus_message_unref(replacement);
        return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);
    }
    dbus_message_set_allow_interactive_authorization(replacement, TRUE);

    const dbus_bool_t ok = dbus_connection_send_with_reply(connection, replacement, pending_return, timeout_milliseconds);
    dbus_message_unref(replacement);
    return ok;
}

__attribute__((constructor))
void install_hook_function()
{
    plthook_t *plthook;

    if (plthook_open(&plthook, QT_DBUS_SONAME) != 0) {
        //printf("plthook_open error: %s\n", plthook_error());
        return;
    }

#ifdef POLKIT_CHECK_AUTH
    const char *in = try_get_procname();
    if (in && strcmp(in, "ksmserver-logout-greeter") == 0)
        check_auth = TRUE;
#endif

    if (UNLIKELY(plthook_replace(plthook, "dbus_connection_send_with_reply", (void*)dbus_connection_send_with_reply_hook, NULL) != 0)) {
        //printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return;
    }

    plthook_close(plthook);
}
