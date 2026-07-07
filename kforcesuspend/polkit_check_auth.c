/*
 * Note: Claude kindly churned this out for me.
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <sys/auxv.h>
#include <dbus/dbus.h>

#define POLKIT_BUS_NAME       "org.freedesktop.PolicyKit1"
#define POLKIT_OBJECT_PATH    "/org/freedesktop/PolicyKit1/Authority"
#define POLKIT_INTERFACE      "org.freedesktop.PolicyKit1.Authority"
#define ACTION_ID             "org.freedesktop.login1.suspend-ignore-inhibit"

static dbus_bool_t append_unix_process_subject(DBusMessageIter *parent_iter, uint32_t pid, uint64_t start_time)
{
    DBusMessageIter struct_iter, dict_iter, entry_iter, variant_iter;
    const char *kind = "unix-process";

    if (!dbus_message_iter_open_container(parent_iter, DBUS_TYPE_STRUCT,
                                           NULL, &struct_iter))
        return FALSE;

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &kind))
        return FALSE;

    if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY,
                                           "{sv}", &dict_iter))
        return FALSE;

    /* "pid" -> variant(uint32) */
    {
        const char *key = "pid";
        if (!dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                               NULL, &entry_iter))
            return FALSE;
        if (!dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key))
            return FALSE;
        if (!dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT,
                                               DBUS_TYPE_UINT32_AS_STRING,
                                               &variant_iter))
            return FALSE;
        if (!dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT32, &pid))
            return FALSE;
        if (!dbus_message_iter_close_container(&entry_iter, &variant_iter))
            return FALSE;
        if (!dbus_message_iter_close_container(&dict_iter, &entry_iter))
            return FALSE;
    }

    /* "start-time" -> variant(uint64) */
    {
        const char *key = "start-time";
        if (!dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY,
                                               NULL, &entry_iter))
            return FALSE;
        if (!dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key))
            return FALSE;
        if (!dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT,
                                               DBUS_TYPE_UINT64_AS_STRING,
                                               &variant_iter))
            return FALSE;
        if (!dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT64, &start_time))
            return FALSE;
        if (!dbus_message_iter_close_container(&entry_iter, &variant_iter))
            return FALSE;
        if (!dbus_message_iter_close_container(&dict_iter, &entry_iter))
            return FALSE;
    }

    if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
        return FALSE;

    if (!dbus_message_iter_close_container(parent_iter, &struct_iter))
        return FALSE;

    return TRUE;
}

dbus_bool_t polkit_check_suspend_ignore_inhibit(int timeout_ms)
{
    dbus_bool_t ret = FALSE;

    DBusConnection *conn = NULL;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusMessageIter args_iter, empty_dict_iter, result_iter;
    const char *action_id = ACTION_ID;
    uint32_t flags = 0; /* CheckAuthorizationFlags: no AllowUserInteraction */
    const char *cancellation_id = "";

    conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, NULL);
    if (!conn)
        goto Cleanup;
    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    msg = dbus_message_new_method_call(POLKIT_BUS_NAME, POLKIT_OBJECT_PATH,
                                        POLKIT_INTERFACE, "CheckAuthorization");
    if (!msg)
        goto Cleanup;

    dbus_message_iter_init_append(msg, &args_iter);

    /* arg0: Subject subject */
    if (!append_unix_process_subject(&args_iter, (uint32_t) getpid(), 0))
        goto Cleanup;

    /* arg1: String action_id */
    if (!dbus_message_iter_append_basic(&args_iter, DBUS_TYPE_STRING, &action_id))
        goto Cleanup;

    /* arg2: Dict<String,String> details (empty) */
    if (!dbus_message_iter_open_container(&args_iter, DBUS_TYPE_ARRAY,
                                           "{ss}", &empty_dict_iter))
        goto Cleanup;
    if (!dbus_message_iter_close_container(&args_iter, &empty_dict_iter))
        goto Cleanup;

    /* arg3: uint32 flags */
    if (!dbus_message_iter_append_basic(&args_iter, DBUS_TYPE_UINT32, &flags))
        goto Cleanup;

    /* arg4: String cancellation_id */
    if (!dbus_message_iter_append_basic(&args_iter, DBUS_TYPE_STRING, &cancellation_id))
        goto Cleanup;

    reply = dbus_connection_send_with_reply_and_block(conn, msg, timeout_ms, NULL);
    if (!reply)
        goto Cleanup;

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
        goto Cleanup;

    /* Reply: AuthorizationResult (bba{ss}) */
    if (!dbus_message_iter_init(reply, &result_iter))
        goto Cleanup;

    if (dbus_message_iter_get_arg_type(&result_iter) != DBUS_TYPE_STRUCT)
        goto Cleanup;

    {
        DBusMessageIter struct_iter;
        dbus_message_iter_recurse(&result_iter, &struct_iter);

        if (dbus_message_iter_get_arg_type(&struct_iter) != DBUS_TYPE_BOOLEAN)
            goto Cleanup;
        dbus_message_iter_get_basic(&struct_iter, &ret);

        /*
         dbus_message_iter_next(&struct_iter);
         if (dbus_message_iter_get_arg_type(&struct_iter) != DBUS_TYPE_BOOLEAN)
            goto Cleanup;
         dbus_message_iter_get_basic(&struct_iter, &is_challenge);
         */
        /* remaining details a{ss} ignored */
    }

Cleanup:
    if (reply)
        dbus_message_unref(reply);
    if (msg)
        dbus_message_unref(msg);
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }
    return ret;
}

const char* try_get_procname()
{
    const char *execfn = (const char *) getauxval(AT_EXECFN);
    return execfn ? basename(execfn) : program_invocation_short_name;
}
