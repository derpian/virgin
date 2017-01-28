/*
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif

/* Allow the code below to see deprecated functions, so we can continue to
 * export them via DBus. */
#undef PURPLE_DISABLE_DEPRECATED

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "account.h"
#include "buddylist.h"
#include "conversation.h"
#include "dbus-purple.h"
#include "dbus-server.h"
#include "dbus-useful.h"
#include "dbus-bindings.h"
#include "debug.h"
#include "core.h"
#include "savedstatuses.h"
#include "smiley.h"
#include "smiley-list.h"
#include "util.h"
#include "xmlnode.h"


/**************************************************************************/
/* Purple DBUS pointer registration mechanism                             */
/**************************************************************************/

/*
 * Here we include the list of #PURPLE_DBUS_DEFINE_TYPE statements for
 * all structs defined in purple.  This file has been generated by the
 * #dbus-analyze-types.py script.
 */

#include "dbus-types.c"

/*
 * The following three hashtables map are used to translate between
 * pointers (nodes) and the corresponding handles (ids).
 */

static GHashTable *map_node_id;
static GHashTable *map_id_node;
static GHashTable *map_id_type;

static gchar *init_error;
static int dbus_request_name_reply = DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;

gboolean purple_dbus_is_owner(void)
{
	return(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == dbus_request_name_reply);
}

/*
 * This function initializes the pointer-id traslation system.  It
 * creates the three above hashtables and defines parents of some types.
 */
void
purple_dbus_init_ids(void)
{
	map_id_node = g_hash_table_new(g_direct_hash, g_direct_equal);
	map_id_type = g_hash_table_new(g_direct_hash, g_direct_equal);
	map_node_id = g_hash_table_new(g_direct_hash, g_direct_equal);

	PURPLE_DBUS_TYPE(PurpleBuddy)->parent   = PURPLE_DBUS_TYPE(PurpleBlistNode);
	PURPLE_DBUS_TYPE(PurpleContact)->parent = PURPLE_DBUS_TYPE(PurpleBlistNode);
	PURPLE_DBUS_TYPE(PurpleChat)->parent    = PURPLE_DBUS_TYPE(PurpleBlistNode);
	PURPLE_DBUS_TYPE(PurpleGroup)->parent   = PURPLE_DBUS_TYPE(PurpleBlistNode);
}

void
purple_dbus_register_pointer(gpointer node, PurpleDBusType *type)
{
	static gint last_id = 0;

	g_return_if_fail(map_node_id);
	g_return_if_fail(g_hash_table_lookup(map_node_id, node) == NULL);

	last_id++;
	g_hash_table_insert(map_node_id, node, GINT_TO_POINTER(last_id));
	g_hash_table_insert(map_id_node, GINT_TO_POINTER(last_id), node);
	g_hash_table_insert(map_id_type, GINT_TO_POINTER(last_id), type);
}

void
purple_dbus_unregister_pointer(gpointer node)
{
	gpointer id = g_hash_table_lookup(map_node_id, node);

	g_hash_table_remove(map_node_id, node);
	g_hash_table_remove(map_id_node, GINT_TO_POINTER(id));
	g_hash_table_remove(map_id_type, GINT_TO_POINTER(id));
}

gint
purple_dbus_pointer_to_id(gconstpointer node)
{
	gint id = GPOINTER_TO_INT(g_hash_table_lookup(map_node_id, node));
	if ((id == 0) && (node != NULL))
	{
		if (purple_debug_is_verbose())
			purple_debug_warning("dbus",
				"Need to register an object with the dbus subsystem."
				" (If you are not a developer, please ignore this message.)\n");
		return 0;
	}
	return id;
}

gpointer
purple_dbus_id_to_pointer(gint id, PurpleDBusType *type)
{
	PurpleDBusType *objtype;

	objtype = (PurpleDBusType*)g_hash_table_lookup(map_id_type,
			GINT_TO_POINTER(id));

	while (objtype != type && objtype != NULL)
		objtype = objtype->parent;

	if (objtype == type)
		return g_hash_table_lookup(map_id_node, GINT_TO_POINTER(id));
	else
		return NULL;
}

gint
purple_dbus_pointer_to_id_error(gconstpointer ptr, DBusError *error)
{
	gint id = purple_dbus_pointer_to_id(ptr);

	if (ptr != NULL && id == 0)
		dbus_set_error(error, "im.pidgin.purple.ObjectNotFound",
				"The return object is not mapped (this is a Purple error)");

	return id;
}

gpointer
purple_dbus_id_to_pointer_error(gint id, PurpleDBusType *type,
		const char *typename, DBusError *error)
{
	gpointer ptr = purple_dbus_id_to_pointer(id, type);

	if (ptr == NULL && id != 0)
		dbus_set_error(error, "im.pidgin.purple.InvalidHandle",
				"%s object with ID = %i not found", typename, id);

	return ptr;
}


/**************************************************************************/
/* Modified versions of some DBus functions                               */
/**************************************************************************/

dbus_bool_t
purple_dbus_message_get_args(DBusMessage *message,
		DBusError *error, int first_arg_type, ...)
{
	dbus_bool_t retval;
	va_list var_args;

	va_start(var_args, first_arg_type);
	retval = purple_dbus_message_get_args_valist(message, error, first_arg_type, var_args);
	va_end(var_args);

	return retval;
}

dbus_bool_t
purple_dbus_message_get_args_valist(DBusMessage *message,
		DBusError *error, int first_arg_type, va_list var_args)
{
	DBusMessageIter iter;

	dbus_message_iter_init(message, &iter);
	return purple_dbus_message_iter_get_args_valist(&iter, error, first_arg_type, var_args);
}

dbus_bool_t
purple_dbus_message_iter_get_args(DBusMessageIter *iter,
		DBusError *error, int first_arg_type, ...)
{
	dbus_bool_t retval;
	va_list var_args;

	va_start(var_args, first_arg_type);
	retval = purple_dbus_message_iter_get_args_valist(iter, error, first_arg_type, var_args);
	va_end(var_args);

	return retval;
}

#define TYPE_IS_CONTAINER(typecode)        \
	((typecode) == DBUS_TYPE_STRUCT ||     \
	 (typecode) == DBUS_TYPE_DICT_ENTRY || \
	 (typecode) == DBUS_TYPE_VARIANT ||    \
	 (typecode) == DBUS_TYPE_ARRAY)


dbus_bool_t
purple_dbus_message_iter_get_args_valist(DBusMessageIter *iter,
		DBusError *error, int first_arg_type, va_list var_args)
{
	int spec_type, msg_type, i;

	spec_type = first_arg_type;

	for (i = 0; spec_type != DBUS_TYPE_INVALID; i++)
	{
		msg_type = dbus_message_iter_get_arg_type(iter);

		if (msg_type != spec_type)
		{
			dbus_set_error(error, DBUS_ERROR_INVALID_ARGS,
					"Argument %d is specified to be of type \"%i\", but "
					"is actually of type \"%i\"\n", i,
					spec_type, msg_type);
			return FALSE;
		}

		if (!TYPE_IS_CONTAINER(spec_type))
		{
			gpointer ptr;
			ptr = va_arg (var_args, gpointer);
			dbus_message_iter_get_basic(iter, ptr);
		}
		else
		{
			DBusMessageIter *sub;
			sub = va_arg (var_args, DBusMessageIter*);
			dbus_message_iter_recurse(iter, sub);
			purple_debug_info("dbus", "subiter %p:%p\n", sub, * (gpointer*) sub);
			break; /* for testing only! */
		}

		spec_type = va_arg(var_args, int);
		if (!dbus_message_iter_next(iter) && spec_type != DBUS_TYPE_INVALID)
		{
			dbus_set_error (error, DBUS_ERROR_INVALID_ARGS,
					"Message has only %d arguments, but more were expected", i);
			return FALSE;
		}
	}

	return TRUE;
}



/**************************************************************************/
/* Useful functions                                                       */
/**************************************************************************/

const char *purple_emptystr_to_null(const char *str)
{
	if (str == NULL || str[0] == 0)
		return NULL;
	else
		return str;
}

const char *
purple_null_to_emptystr(const char *s)
{
	if (s)
		return s;
	else
		return "";
}

dbus_int32_t *
purple_dbusify_GList(GList *list, dbus_int32_t *len)
{
	dbus_int32_t *array;
	int i;
	GList *elem;

	*len = g_list_length(list);
	array = g_new0(dbus_int32_t, *len);
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = purple_dbus_pointer_to_id(elem->data);

	return array;
}

dbus_int32_t *
purple_dbusify_GSList(GSList *list, dbus_int32_t *len)
{
	dbus_int32_t *array;
	int i;
	GSList *elem;

	*len = g_slist_length(list);
	array = g_new0(dbus_int32_t, *len);
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = purple_dbus_pointer_to_id(elem->data);

	return array;
}

gpointer *
purple_GList_to_array(GList *list, dbus_int32_t *len)
{
	gpointer *array;
	int i;
	GList *elem;

	*len = g_list_length(list);
	array = g_new0(gpointer, *len);
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = elem->data;

	return array;
}

gpointer *
purple_GSList_to_array(GSList *list, dbus_int32_t *len)
{
	gpointer *array;
	int i;
	GSList *elem;

	*len = g_slist_length(list);
	array = g_new0(gpointer, *len);
	for (i = 0, elem = list; elem != NULL; elem = elem->next, i++)
		array[i] = elem->data;

	return array;
}

GHashTable *
purple_dbus_iter_hash_table(DBusMessageIter *iter, DBusError *error)
{
	GHashTable *hash;

	/* we do not need to destroy strings because they are part of the message */
	hash = g_hash_table_new(g_str_hash, g_str_equal);

	do {
		char *key, *value;
		DBusMessageIter subiter;

		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_DICT_ENTRY)
			goto error;
			/* With all due respect to Dijkstra,
			 * this goto is for exception
			 * handling, and it is ok because it
			 * avoids duplication of the code
			 * responsible for destroying the hash
			 * table.  Exceptional instructions
			 * for exceptional situations.
			 */

		dbus_message_iter_recurse(iter, &subiter);
		if (!purple_dbus_message_iter_get_args(&subiter, error,
				DBUS_TYPE_STRING, &key,
				DBUS_TYPE_STRING, &value,
				DBUS_TYPE_INVALID))
			goto error; /* same here */

		g_hash_table_insert(hash, key, value);
	} while (dbus_message_iter_next(iter));

	return hash;

error:
	g_hash_table_destroy(hash);
	return NULL;
}

/**************************************************************/
/* DBus bindings ...                                          */
/**************************************************************/

static DBusConnection *purple_dbus_connection;

DBusConnection *
purple_dbus_get_connection(void)
{
	return purple_dbus_connection;
}

#include "dbus-bindings.c"
#include "dbus-signals.c"

static gboolean
purple_dbus_dispatch_cb(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	const char *name;
	PurpleDBusBinding *bindings;
	int i;

	bindings = (PurpleDBusBinding*) user_data;

	if (!dbus_message_has_path(message, PURPLE_DBUS_PATH))
		return FALSE;

	name = dbus_message_get_member(message);

	if (name == NULL)
		return FALSE;

	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
		return FALSE;

	for (i = 0; bindings[i].name; i++)
		if (!strcmp(name, bindings[i].name))
		{
			DBusMessage *reply;
			DBusError error;

			dbus_error_init(&error);

			reply = bindings[i].handler(message, &error);

			if (reply == NULL && dbus_error_is_set(&error))
				reply = dbus_message_new_error (message,
						error.name, error.message);

			if (reply != NULL)
			{
				dbus_connection_send(connection, reply, NULL);
				dbus_message_unref(reply);
			}

			return TRUE; /* return reply! */
		}

	return FALSE;
}


static const char *
dbus_gettext(const char **ptr)
{
	const char *text = *ptr;
	*ptr += strlen(text) + 1;
	return text;
}

static void
purple_dbus_introspect_cb(GList **bindings_list, void *bindings)
{
	*bindings_list = g_list_prepend(*bindings_list, bindings);
}

static DBusMessage *purple_dbus_introspect(DBusMessage *message)
{
	DBusMessage *reply;
	GString *str;
	GList *bindings_list, *node;
	const char *signals;
	const char *type;
	const char *pointer_type;

	str = g_string_sized_new(0x1000); /* TODO: why this size? */

	g_string_append(str, "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>\n");
	g_string_append_printf(str, "<node name='%s'>\n", PURPLE_DBUS_PATH);
	g_string_append(str, "  <interface name='org.freedesktop.DBus.Introspectable'>\n    <method name='Introspect'>\n      <arg name='data' direction='out' type='s'/>\n    </method>\n  </interface>\n\n");

	g_string_append_printf(str, "  <interface name='%s'>\n", PURPLE_DBUS_INTERFACE);

	bindings_list = NULL;
	purple_signal_emit(purple_dbus_get_handle(), "dbus-introspect", &bindings_list);

	for (node = bindings_list; node; node = node->next)
	{
		PurpleDBusBinding *bindings;
		int i;

		bindings = (PurpleDBusBinding*)node->data;

		for (i = 0; bindings[i].name; i++)
		{
			const char *text;

			g_string_append_printf(str, "    <method name='%s'>\n", bindings[i].name);

			text = bindings[i].parameters;
			while (*text)
			{
				const char *name, *direction, *type;

				direction = dbus_gettext(&text);
				type = dbus_gettext(&text);
				name = dbus_gettext(&text);

				g_string_append_printf(str,
						"      <arg name='%s' type='%s' direction='%s'/>\n",
						name, type, direction);
			}
			g_string_append(str, "    </method>\n");
		}
	}

	if (sizeof(int) == sizeof(dbus_int32_t))
		pointer_type = "type='i'";
	else
		pointer_type = "type='x'";

	signals = dbus_signals;
	while ((type = strstr(signals, "type='p'")) != NULL) {
		g_string_append_len(str, signals, type - signals);
		g_string_append(str, pointer_type);
		signals = type + sizeof("type='p'") - 1;
	}
	g_string_append(str, signals);

	g_string_append(str, "  </interface>\n</node>\n");

	reply = dbus_message_new_method_return(message);
	dbus_message_append_args(reply, DBUS_TYPE_STRING, &(str->str),
			DBUS_TYPE_INVALID);
	g_string_free(str, TRUE);
	g_list_free(bindings_list);

	return reply;
}

static DBusHandlerResult
purple_dbus_dispatch(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	if (purple_signal_emit_return_1(purple_dbus_get_handle(),
			"dbus-method-called", connection, message))
		return DBUS_HANDLER_RESULT_HANDLED;

	if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect") &&
			dbus_message_has_path(message, PURPLE_DBUS_PATH))
	{
		DBusMessage *reply;
		reply = purple_dbus_introspect(message);
		dbus_connection_send (connection, reply, NULL);
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
purple_dbus_register_bindings(void *handle, PurpleDBusBinding *bindings)
{
	purple_signal_connect(purple_dbus_get_handle(), "dbus-method-called",
			handle,
			PURPLE_CALLBACK(purple_dbus_dispatch_cb),
			bindings);
	purple_signal_connect(purple_dbus_get_handle(), "dbus-introspect",
			handle,
			PURPLE_CALLBACK(purple_dbus_introspect_cb),
			bindings);
}

static void
purple_dbus_dispatch_init(void)
{
	static DBusObjectPathVTable vtable = {NULL, &purple_dbus_dispatch, NULL, NULL, NULL, NULL};
	DBusError error;

	dbus_error_init(&error);
	purple_dbus_connection = dbus_bus_get(DBUS_BUS_STARTER, &error);

	if (purple_dbus_connection == NULL)
	{
		init_error = g_strdup_printf(N_("Failed to get connection: %s"), error.message);
		dbus_error_free(&error);
		return;
	}

	/* Do not allow libdbus to exit on connection failure (This may
	   work around random exit(1) on SIGPIPE errors) */
	dbus_connection_set_exit_on_disconnect (purple_dbus_connection, FALSE);

	if (!dbus_connection_register_object_path(purple_dbus_connection,
			PURPLE_DBUS_PATH, &vtable, NULL))
	{
		init_error = g_strdup_printf(N_("Failed to get name: %s"), error.name);
		dbus_error_free(&error);
		return;
	}

	dbus_request_name_reply = dbus_bus_request_name(purple_dbus_connection,
			PURPLE_DBUS_SERVICE, 0, &error);

	if (dbus_error_is_set(&error))
	{
		dbus_connection_unref(purple_dbus_connection);
		purple_dbus_connection = NULL;
		init_error = g_strdup_printf(N_("Failed to get serv name: %s"), error.name);
		dbus_error_free(&error);
		return;
	}

	dbus_connection_setup_with_g_main(purple_dbus_connection, NULL);

	purple_signal_register(purple_dbus_get_handle(), "dbus-method-called",
			 purple_marshal_BOOLEAN__POINTER_POINTER,
			 G_TYPE_BOOLEAN, 2, G_TYPE_POINTER, G_TYPE_POINTER);

	purple_signal_register(purple_dbus_get_handle(), "dbus-introspect",
			 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
			 G_TYPE_POINTER); /* pointer to a pointer */

	PURPLE_DBUS_REGISTER_BINDINGS(purple_dbus_get_handle());

	if (purple_debug_is_verbose())
		purple_debug_misc("dbus", "initialized");
}



/**************************************************************************/
/* Signals                                                                */
/**************************************************************************/



static char *
purple_dbus_convert_signal_name(const char *purple_name)
{
	int purple_index, g_index;
	char *g_name = g_new(char, strlen(purple_name) + 1);
	gboolean capitalize_next = TRUE;

	for (purple_index = g_index = 0; purple_name[purple_index]; purple_index++)
		if (purple_name[purple_index] != '-' && purple_name[purple_index] != '_')
		{
			if (capitalize_next)
				g_name[g_index++] = g_ascii_toupper(purple_name[purple_index]);
			else
				g_name[g_index++] = purple_name[purple_index];
			capitalize_next = FALSE;
		} else
			capitalize_next = TRUE;

	g_name[g_index] = 0;

	return g_name;
}

#define my_arg(type) (ptr != NULL ? * ((type *)ptr) : va_arg(data, type))

static gboolean
purple_dbus_message_append_values(DBusMessageIter *iter,
		int number, GType *types, va_list data)
{
	int i;
	gboolean error = FALSE;

	for (i = 0; i < number; i++)
	{
		const char *str;
		int id;
		gint xint;
		guint xuint;
		gint64 xint64;
		guint64 xuint64;
		gboolean xboolean;
		gpointer ptr = NULL;
		gpointer val;
#if 0
		if (purple_value_is_outgoing(purple_values[i]))
		{
			ptr = my_arg(gpointer);
			g_return_val_if_fail(ptr, TRUE);
		}
#endif
		switch (types[i])
		{
		case G_TYPE_INT:
			xint = my_arg(gint);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &xint);
			break;
		case G_TYPE_UINT:
			xuint = my_arg(guint);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &xuint);
			break;
		case G_TYPE_INT64:
			xint64 = my_arg(gint64);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &xint64);
			break;
		case G_TYPE_UINT64:
			xuint64 = my_arg(guint64);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &xuint64);
			break;
		case G_TYPE_BOOLEAN:
			xboolean = my_arg(gboolean);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &xboolean);
			break;
		case G_TYPE_STRING:
			str = purple_null_to_emptystr(my_arg(char*));
			if (!g_utf8_validate(str, -1, NULL)) {
				gchar *tmp;
				purple_debug_error("dbus", "Invalid UTF-8 string passed to signal, emitting salvaged string!\n");
				tmp = purple_utf8_salvage(str);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &tmp);
				g_free(tmp);
			} else {
				dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &str);
			}
			break;
		default:
			if (G_TYPE_IS_OBJECT(types[i])  ||
			    G_TYPE_IS_BOXED(types[i])   ||
			    types[i] == G_TYPE_POINTER   )
			{
				val = my_arg(gpointer);
				id = purple_dbus_pointer_to_id(val);
				if (id == 0 && val != NULL)
					error = TRUE;      /* Some error happened. */
				dbus_message_iter_append_basic(iter,
						(sizeof(id) == sizeof(dbus_int32_t)) ? DBUS_TYPE_INT32 : DBUS_TYPE_INT64, &id);
			}
			else if (G_TYPE_IS_ENUM(types[i]))
			{
				xint = my_arg(gint);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &xint);
			}
			else  /* no conversion implemented */
			{
				g_return_val_if_reached(TRUE);
			}
		}
	}
	return error;
}

#undef my_arg

void
purple_dbus_signal_emit_purple(const char *name, int num_values,
		GType *types, va_list vargs)
{
	DBusMessage *signal;
	DBusMessageIter iter;
	char *newname;

#if 0 /* this is noisy with no dbus connection */
	g_return_if_fail(purple_dbus_connection);
#else
	if (purple_dbus_connection == NULL)
		return;
#endif


	/*
	 * The test below is a hack that prevents our "dbus-method-called"
	 * signal from being propagated to dbus.  What we really need is a
	 * flag for each signal that states whether this signal is to be
	 * dbus-propagated or not.
	 */
	if (!strcmp(name, "dbus-method-called"))
		return;

	newname = purple_dbus_convert_signal_name(name);
	signal = dbus_message_new_signal(PURPLE_DBUS_PATH, PURPLE_DBUS_INTERFACE, newname);
	dbus_message_iter_init_append(signal, &iter);

	if (purple_dbus_message_append_values(&iter, num_values, types, vargs))
		if (purple_debug_is_verbose())
			purple_debug_warning("dbus",
				"The signal \"%s\" caused some dbus error."
				" (If you are not a developer, please ignore this message.)\n",
				name);

	dbus_connection_send(purple_dbus_connection, signal, NULL);

	g_free(newname);
	dbus_message_unref(signal);
}

const char *
purple_dbus_get_init_error(void)
{
	return init_error;
}

void *
purple_dbus_get_handle(void)
{
	static int handle;

	return &handle;
}

void
purple_dbus_init(void)
{
	if (g_thread_supported())
		dbus_g_thread_init();

	purple_dbus_init_ids();

	g_free(init_error);
	init_error = NULL;
	purple_dbus_dispatch_init();
	if (init_error != NULL)
		purple_debug_error("dbus", "%s\n", init_error);
}

void
purple_dbus_uninit(void)
{
	DBusError error;
	if (!purple_dbus_connection)
		return;

	dbus_error_init(&error);
	dbus_connection_unregister_object_path(purple_dbus_connection, PURPLE_DBUS_PATH);
	dbus_bus_release_name(purple_dbus_connection, PURPLE_DBUS_SERVICE, &error);
	dbus_error_free(&error);
	dbus_connection_unref(purple_dbus_connection);
	purple_dbus_connection = NULL;
	purple_signals_disconnect_by_handle(purple_dbus_get_handle());
	g_free(init_error);
	init_error = NULL;
}
