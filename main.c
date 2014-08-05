/*
 *  connman-json-client
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <signal.h>
#include <assert.h>

#include "engine.h"
#include "loop.h"
#include "ncurses_utils.h"
#include "string_utils.h"
#include "renderers.h"
#include "keys.h"
#include "popup.h"

/*

 +-----------------------------------+
 |  win_header                       | <-- state area
 +-----------------------------------+
 |                                   |
 |                                   |
 |                                   |
 |  win_body                         |
 |                                   |
 |                                   |
 |                                   |
 +-----------------------------------+
 |  win_footer                       | <-- infos/errors area
 +-----------------------------------+

 */
extern int nb_items;
extern int nb_fields;
extern struct context_info context;

extern FORM *popup_form;
extern FIELD **popup_fields;
extern struct popup_actions **popup_btn_action;

static void print_services_for_tech();
static void connect_to_service();

static void print_home_page(void);
static void exec_refresh(void);

static bool allow_refresh = false;
static char **agent_request_input_fields;

static struct {
	void (*func)(); // What to do on action
	void (*func_free)(); // Free elements for the current_context
	void (*func_back)(); // What to do on 'Esc'
	void (*func_refresh)(); // Refresh the current_context
	void (*func_refresh_msg)(); // Refresh the footer message
} context_actions[] = {
	{ print_services_for_tech, __renderers_free_home_page, print_home_page,
		print_home_page, refresh_home_msg }, // CONTEXT_HOME
	{ NULL, __renderers_free_service_config, print_home_page,
		print_services_for_tech, refresh_service_config_msg }, // CONTEXT_SERVICE_CONFIG
	{ connect_to_service, __renderers_free_services, print_home_page,
		print_services_for_tech, refresh_services_msg }, // CONTEXT_SERVICES
};

void callback_ended(void)
{
	// This method is mandatory, and might be of use somehow
	// e.g quite handy for debugging operations
	return;
}

static void context_free_userptr_data()
{
	if (context.serv) {
		if (context.serv->dbus_name) {
			free(context.serv->dbus_name);
			context.serv->dbus_name = NULL;
		}

		if (context.serv->pretty_name) {
			free(context.serv->pretty_name);
			context.serv->pretty_name = NULL;
		}
	}

	if (context.tech) {
		if (context.tech->dbus_name) {
			free(context.tech->dbus_name);
			context.tech->dbus_name = NULL;
		}

		if (context.tech->pretty_name) {
			free(context.tech->pretty_name);
			context.tech->pretty_name = NULL;
		}
	}

}

static void popup_free(void)
{
	int i = 0;

	assert(popup_btn_action != NULL);

	while (popup_btn_action[i]) {
		free(popup_btn_action[i]->key);
		free(popup_btn_action[i]);
		popup_btn_action[i] = NULL;
		i++;
	}

	free(popup_btn_action);
	popup_delete();
	exec_refresh();
}

static void stop_loop(int signum)
{
	loop_quit();

	if (context_actions[context.current_context].func_free)
		context_actions[context.current_context].func_free();

	if (popup_exists())
		popup_free();

	engine_terminate();
	context_free_userptr_data();

	if (context.serv)
		free(context.serv);

	if (context.tech)
		free(context.tech);

	context.serv = NULL;
	context.tech = NULL;
}

static void exec_refresh()
{
	ITEM *item;
	FIELD *tmp_field;
	struct userptr_data *tmp;

	if (nb_fields != 0) {
		tmp_field = current_field(my_form);
		assert(tmp_field != NULL);
		tmp = field_userptr(tmp_field);
		context.cursor_id = strdup(tmp->dbus_name);
	} else if (nb_items != 0) {
		// We use the dbus_name as a unique key
		item = current_item(my_menu);
		assert(item != NULL);
		tmp = item_userptr(item);
		context.cursor_id = strdup(tmp->dbus_name);
	}

	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_refresh();

	if (popup_exists())
		popup_refresh();
}

/*
 * Reset cursor position before refresh.
 * We use a value put in userptr by the renderers, this value is the "true"
 * dbus_name for menus and a "key" for forms.
 * The key on form make it work but it isn't a great solution.
 */
void repos_cursor(void)
{
	int i, j, nb_active_fields;
	struct userptr_data *tmp;
	ITEM *item;

	if (!context.cursor_id)
		return;

	if (nb_fields != 0) {
		nb_active_fields = 0;

		for (i = 0; i < nb_fields; i++) {
			if (!(field_opts(field[i]) & O_ACTIVE))
				continue;

			nb_active_fields++;
			tmp = field_userptr(field[i]);

			if (strncmp(tmp->dbus_name, context.cursor_id, 256) == 0) {
				for (j = 1; j < nb_active_fields; j++)
					form_driver(my_form, REQ_NEXT_FIELD);

				wrefresh(my_form->sub);
				break;
			}
		}
	} else if (nb_items != 0) {
		for (i = 0; i < nb_items; i++) {
			item = my_items[i];
			tmp = item_userptr(item);

			if (strncmp(context.cursor_id, tmp->dbus_name, 256) == 0) {
				for (j = 0; j < i; j++)
					menu_driver(my_menu, REQ_NEXT_ITEM);

				wrefresh(my_menu->usersub);
				break;
			}
		}
	}

	free(context.cursor_id);
	context.cursor_id = NULL;
	wrefresh(win_body);
}

static void exec_action(struct userptr_data *data)
{
	switch (context.current_context) {
		case CONTEXT_SERVICES:
			context.serv->dbus_name = strdup(data->dbus_name);

			if (data->pretty_name)
				context.serv->pretty_name = strdup(data->pretty_name);
			break;

		case CONTEXT_HOME:
			context.tech->dbus_name = strdup(data->dbus_name);
			context.tech->pretty_name = strdup(data->pretty_name);
			break;

		default:
			break;
	}

	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func();
}

static void exec_back(void)
{
	context_free_userptr_data();
	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_back();

	if (popup_exists())
		popup_refresh();
}

static void action_on_cmd_callback(struct json_object *jobj)
{
	struct json_object *data, *cmd_name_tmp;
	const char *cmd_name;

	json_object_object_get_ex(jobj, key_command, &cmd_name_tmp);
	json_object_object_get_ex(jobj, key_command_data, &data);
	cmd_name = json_object_get_string(cmd_name_tmp);

	/* dispatch according to the command name */
	if (strcmp(key_engine_get_home_page, cmd_name) == 0)
		__renderers_home_page(data);

	else if (strcmp(key_engine_get_services_from_tech, cmd_name) == 0)
		__renderers_services(data);

	else
		print_info_in_footer(true, "Unknown command called");
}

static void action_on_signal(struct json_object *jobj)
{
	struct json_object *sig_path, *sig_data;
	json_bool tmp_bool;
	bool is_tech_removed, is_current_tech, got_connected;
	const char *tmp_str;

	json_object_object_get_ex(jobj, key_command_path, &sig_path);
	json_object_object_get_ex(jobj, key_command_data, &sig_data);

	is_tech_removed = strncmp("TechnologyRemoved", json_object_get_string(sig_path), 50) == 0;

	if (context.tech->dbus_name == NULL)
		is_current_tech = false;
	else
		is_current_tech = strncmp(context.tech->dbus_name, json_object_get_string(sig_data), 256) == 0;

	if (is_tech_removed && is_current_tech)
		exec_back();
	else {
		got_connected = false;

		if (sig_data && json_object_get_type(sig_data) == json_type_array) {
			tmp_str = json_object_get_string(json_object_array_get_idx(sig_data, 0));
			tmp_bool = json_object_get_boolean(json_object_array_get_idx(sig_data, 1));
			
			if (tmp_str && strcmp("Connected", tmp_str) == 0 && tmp_bool)
				got_connected = true;
		}

		// This is to prevent always changing wifi services in areas
		// with a load of networks
		if (context.current_context != CONTEXT_SERVICES ||
				(allow_refresh || got_connected)) {
			exec_refresh();
			allow_refresh = false;
		}
	}
}

static void popup_btn_action_ok(void)
{
	struct json_object *cmd, *cmd_data;
	int i;
	char *label_str, *field_str;
	char *label_str_clean, *field_str_clean;

	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_agent_response));
	cmd_data = json_object_new_object();

	for (i = 0; i < popup_form->maxfield; i++) {
		label_str = field_buffer(popup_fields[i], 0);
		label_str_clean = strtok(label_str, " ");

		if (!label_str_clean)
			label_str_clean = NULL;

		i++;
		field_str = field_buffer(popup_fields[i], 0);
		field_str_clean = strtok(field_str, " ");

		if (!field_str_clean)
			field_str_clean = "";

		json_object_object_add(cmd_data, label_str_clean,
				json_object_new_string(field_str_clean));
	}

	json_object_object_add(cmd, key_command_data, cmd_data);
	engine_query(cmd);
	popup_free();
}

/*
 * json_type_bool -> int
 */
static void popup_btn_action_retry(int retry)
{
	struct json_object *cmd;

	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_agent_retry));
	json_object_object_add(cmd, key_command_data,
			json_object_new_boolean(retry));

	engine_query(cmd);
	popup_free();
}

static void popup_btn_action_retry_ok(void)
{
	popup_btn_action_retry(TRUE);
}

static void popup_btn_action_retry_no(void)
{
	popup_btn_action_retry(FALSE);
}

static void agent_input_popup(const char *serv_name, struct json_object *data)
{
	int i, len = 0;
	char req_char;
	char buf[60];
	// Nom du champ, type, alternates, requirement
	char *fmt = "%s (%s, %s): %c";
	const char *value_str;
	// Arguments as in connman/doc/agent-api.txt
	struct json_object *type, *req, *alt, *value;
	struct popup_actions *popup_btn_ok;

	json_object_object_foreach(data, _key, _val) {
		assert(_key && _val);
		len++;
	}

	agent_request_input_fields = malloc(sizeof(char *) * (len * 2 + 1));
	popup_btn_ok = malloc(sizeof(struct popup_actions));
	popup_btn_ok->key = strdup("OK");
	popup_btn_ok->func = popup_btn_action_ok;
	popup_btn_action = malloc(sizeof(struct popup_actions *) * 2);
	popup_btn_action[0] = popup_btn_ok;
	popup_btn_action[1] = NULL;
	i = 0;

	json_object_object_foreach(data, key, val) {
		json_object_object_get_ex(val, "Type", &type);
		json_object_object_get_ex(val, "Requirement", &req);
		json_object_object_get_ex(val, "Alternates", &alt);
		json_object_object_get_ex(val, "Value", &value);

		if (strncmp(json_object_get_string(req), "mandatory", 10) == 0)
			req_char = '*';
		else if (strncmp(json_object_get_string(req), "alternate", 10) == 0)
			req_char = '~';
		else
			req_char = ' ';

		snprintf(buf, 60, fmt, key, json_object_get_string(type),
				alt == NULL ? "no alt" : json_object_get_string(alt),
				req_char);
		buf[59] = '\0';
		agent_request_input_fields[i] = strndup(buf, 60);
		i++;
		value_str = " ";

		if (value)
			value_str = json_object_get_string(value);

		agent_request_input_fields[i] = strndup(value_str, 60);
		i++;
	}

	agent_request_input_fields[len*2] = NULL;
}

static void action_on_agent_msg(struct json_object *jobj)
{
	struct json_object *request, *service, *data;
	const char *request_str, *service_str, *fmt = "The network %s request"
		" credentials";
	char buf[150];
	int i;

	assert(!popup_exists());
	json_object_object_get_ex(jobj, key_dbus_json_agent_msg_key, &request);
	json_object_object_get_ex(jobj, "data", &data);
	json_object_object_get_ex(jobj, key_service, &service);
	request_str = json_object_get_string(request);
	service_str = json_object_get_string(service);

	if (strncmp("Input Requested", request_str, 15) == 0)
		agent_input_popup(service_str, data);

	else if (strncmp("Agent RequestBrowser", request_str, 21) == 0)
		print_info_in_footer(false, "The agent request you to"
				" open the web page: %s",
				json_object_get_string(jobj));

	else {
		print_info_in_footer(true,
				"Not yet handled agent request:");
		print_info_in_footer2(true,
				json_object_get_string(jobj));
	}

	snprintf(buf, 150, fmt, context.serv->pretty_name);
	buf[149] = '\0';
	popup_new(18, 76, (LINES-17)/2, (COLS-75)/2,
			agent_request_input_fields, buf);
	assert(popup_exists());
	popup_refresh();

	for (i = 0; agent_request_input_fields[i]; i++)
		free(agent_request_input_fields[i]);

	free(agent_request_input_fields);
	agent_request_input_fields = NULL;
}

/*
	{
		"ERROR Agent": "invalid-key",
		"service": "wifi_XXXXXXXXXXXX_YYYYYYYYYYYYYY_managed_psk",
		"agent_error_message": "Retry ?",
		"callback": "report_error_return"
	}
 */
static void action_on_agent_error(struct json_object *jobj)
{
	char *fmt = "Agent error: %s on \"%s\", %s", buf[150];
	const char *error_msg_str, *service_str, *msg_str;
	struct json_object *tmp;
	struct popup_actions *popup_btn_no, *popup_btn_yes;

	assert(!popup_exists());
	json_object_object_get_ex(jobj, key_dbus_json_agent_error_key, &tmp);
	error_msg_str = json_object_get_string(tmp);
	json_object_object_get_ex(jobj, key_service, &tmp);
	service_str = json_object_get_string(tmp);
	json_object_object_get_ex(jobj, key_agent_error_message, &tmp);
	msg_str = json_object_get_string(tmp);

	snprintf(buf, 150, fmt, error_msg_str, service_str, msg_str);
	buf[149] = '\0';

	popup_btn_action = malloc(sizeof(struct popup_actions *) * 3);
	popup_btn_yes = malloc(sizeof(struct popup_actions));
	popup_btn_yes->key = strdup("YES");
	popup_btn_yes->func = popup_btn_action_retry_ok;
	popup_btn_action[0] = popup_btn_yes;
	popup_btn_no = malloc(sizeof(struct popup_actions));
	popup_btn_no->key = strdup("NO");
	popup_btn_no->func = popup_btn_action_retry_no;
	popup_btn_action[1] = popup_btn_no;
	popup_btn_action[2] = NULL;

	popup_new(16, 76, (LINES-15)/2, (COLS-75)/2, NULL, buf);
	assert(popup_exists());
	popup_refresh();
}

static void main_callback(int status, struct json_object *jobj)
{
	struct json_object *cmd_tmp, *signal, *agent_msg, *agent_error,
			*return_user_data, *error;
	const char *error_str;

	json_object_object_get_ex(jobj, key_dbus_json_error_key, &error);
	error_str = json_object_get_string(error);

	if (status < 0) {
		print_info_in_footer2(true, "Error (code %d) %s", -status,
				error_str ? error_str : "[ no error message ]");
	}

	/* get the main object items */
	json_object_object_get_ex(jobj, key_command, &cmd_tmp);
	json_object_object_get_ex(jobj, key_dbus_json_signal_key, &signal);
	json_object_object_get_ex(jobj, key_dbus_json_agent_msg_key, &agent_msg);
	json_object_object_get_ex(jobj, key_dbus_json_agent_error_key, &agent_error);
	json_object_object_get_ex(jobj, key_return_force_refresh, &return_user_data);

	if (cmd_tmp)
		action_on_cmd_callback(jobj);

	else if (signal)
		action_on_signal(jobj);

	else if (agent_msg)
		action_on_agent_msg(jobj);

	else if (agent_error)
		action_on_agent_error(jobj);

	else if (return_user_data) {
		allow_refresh = true;
		exec_refresh();

	} else
		print_info_in_footer(true, "Unidentified call back: "
				"status: %d, jobj: %s\n", status,
				json_object_get_string(jobj));
	
	context_actions[context.current_context].func_refresh_msg();
	// Release the memory of the json object now
	json_object_put(jobj);
}

static void print_home_page(void)
{
	struct json_object *cmd;

	werase(win_footer);
	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_get_home_page));
	engine_query(cmd);
}

static void print_services_for_tech()
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_get_services_from_tech));
	json_object_object_add(tmp, key_technology,
			json_object_new_string(context.tech->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		print_info_in_footer(true, "@print_services_for_tech:"
				"invalid argument/value");
}

static void connect_to_service()
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_connect));
	json_object_object_add(tmp, key_service,
			json_object_new_string(context.serv->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		print_info_in_footer(true, "@connect_to_service:"
				" invalid argument/value");

	werase(win_body);
	mvwprintw(win_body, 1, 2, "Connecting...");
	box(win_body, 0, 0);
	wrefresh(win_body);
}

static void disconnect_of_service(struct userptr_data *data)
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_disconnect));
	json_object_object_add(tmp, key_technology,
			json_object_new_string(data->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		print_info_in_footer(true, "@disconnect_of_service:"
				" invalid argument/value");
}

static void scan_tech(const char *tech_dbus_name)
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();
	json_object_object_add(tmp, key_technology,
			json_object_new_string(tech_dbus_name));
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_scan_tech));
	json_object_object_add(cmd, key_command_data, tmp);
	engine_query(cmd);
}

/*
 * Return the position of the previous label suffixed with ".Configuration".
 */
static int search_previous_config_label(int pos)
{
	FIELD *f;
	char *key_str, *tmp_str;
	bool field_is_editable;

	for (; pos > 0 && field[pos] != NULL; pos--) {
		f = field[pos];
		field_is_editable = (unsigned)field_opts(f) & O_EDIT;

		if (field_is_editable)
			continue;

		key_str = field_buffer(f, 0);
		tmp_str = trim_whitespaces(key_str);

		if (string_ends_with_configuration(tmp_str))
			break;
	}
		
	return pos;
}

static void build_json_config(struct json_object *jobj, int min, int max)
{
	int i, pos_label;
	const char *pos_buffer, *buffer;
	char *pos_buffer_clean, *buffer_clean;
	json_bool autoconnect;
	struct json_object *tmp;
	bool buffer_is_array;

	for (i = max; i > min; i--) {
		if (!((unsigned)field_opts(field[i]) & O_EDIT))
			continue;

		pos_label = i - 1;
		assert(pos_label >= 0 && pos_label >= min);

		buffer = field_buffer(field[i], 0);
		pos_buffer = field_buffer(field[pos_label], 0);
		assert(buffer != NULL && pos_buffer != NULL);
		buffer_clean = trim_whitespaces((char *)buffer);
		pos_buffer_clean = trim_whitespaces((char *)pos_buffer);

		buffer_is_array = strcmp("Nameservers.Configuration",
				pos_buffer_clean) == 0;
		buffer_is_array |= strcmp("Timeservers.Configuration",
				pos_buffer_clean) == 0;
		buffer_is_array |= strcmp("Domains.Configuration",
				pos_buffer_clean) == 0;

		if (strcmp("AutoConnect", pos_buffer_clean) == 0) {
			autoconnect = strcmp("true", buffer_clean);
			json_object_object_add(jobj, pos_buffer_clean,
					json_object_new_boolean(autoconnect == 0 ? TRUE : FALSE));

		} else if (buffer_is_array) {
			tmp = json_tokener_parse(buffer_clean);

			if (tmp && json_object_get_type(tmp) != json_type_array) {
				json_object_put(tmp);
				tmp = json_object_new_array();

			} else if (!tmp)
				tmp = json_object_new_array();

			if (json_object_array_length(tmp) == 0)
				json_object_array_add(tmp, json_object_new_string(""));

			json_object_object_add(jobj, pos_buffer_clean, tmp);

		} else
			json_object_object_add(jobj, pos_buffer_clean,
					json_object_new_string(buffer_clean));
	}
}

static void modify_service_config()
{
	int i, pos_label;
	struct json_object *cmd = json_object_new_object();
	struct json_object *cmd_data = json_object_new_object();
	struct json_object *options = json_object_new_object();
	struct json_object *tmp;
	char *key_str;

	for (i = 0; field[i]; i++) {
		if (!((unsigned)field_opts(field[i]) & O_EDIT))
			continue;

		pos_label = search_previous_config_label(i);
		assert(pos_label >= 0);

		if (pos_label == 0) {
			// 11 = strlen("AutoConnect"), the rest of the
			// buffer is filled with spaces
			if (strncmp("AutoConnect", field_buffer(field[i-1], 0), 11) != 0)
				continue;

			build_json_config(options, i-1, i);

		} else if (pos_label == i-1) {
			build_json_config(options, pos_label, i);

		} else { // object representation
			tmp = json_object_new_object();
			build_json_config(tmp, pos_label, i);
			key_str = field_buffer(field[pos_label], 0);
			json_object_object_add(options, trim_whitespaces(key_str), tmp);
		}
	}

	json_object_object_add(cmd_data, key_service,
			json_object_new_string(context.serv->dbus_name));
	json_object_object_add(cmd_data, "options", options);
	json_object_object_add(cmd, key_command_data, cmd_data);
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_config_service));

	if (engine_query(cmd) == -EINVAL)
		print_info_in_footer(true, "@modify_service_config: "
				"invalid argument/value");
}

static void exec_action_context_home(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;

		case 100: // 'd'
			item = current_item(my_menu);
			disconnect_of_service(item_userptr(item));
			print_info_in_footer(false, "Disconnecting...");
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(my_menu);
			exec_action(item_userptr(item));
			break;
	}
}

static void exec_action_context_service_config(int ch)
{
	int cur_page = form_page(my_form);
	struct userptr_data *data;

	switch (ch) {
		case KEY_DOWN:
			form_driver(my_form, REQ_NEXT_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;

		case KEY_UP:
			form_driver(my_form, REQ_PREV_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;

		case KEY_NPAGE:
			form_driver(my_form, REQ_NEXT_PAGE);
			set_form_page(my_form, ++cur_page);
			__renderers_services_config_paging();
			break;

		case KEY_PPAGE:
			form_driver(my_form, REQ_PREV_PAGE);
			set_form_page(my_form, --cur_page);
			__renderers_services_config_paging();
			break;

		// Delete the char before cursor
		case KEY_BACKSPACE:
		case 127:
			form_driver(my_form, REQ_DEL_PREV);
			break;

		// Delete the char under the cursor
		case KEY_DC:
			form_driver(my_form, REQ_DEL_CHAR);
			break;

		case KEY_LEFT:
			form_driver(my_form, REQ_PREV_CHAR);
			break;

		case KEY_RIGHT:
			form_driver(my_form, REQ_NEXT_CHAR);
			break;

		case KEY_F(7):
			// The dbus name of the service is stored in the first
			// item (it's a non active one). The active items have a
			// tagging system to keep the pointer's position on the
			// same field. We can't use exec_action for this because
			// we need to extract the values in fields *before*
			// freeing them (exec_action does the opposite).
			data = field_userptr(field[0]);
			context.serv->dbus_name = strdup(data->dbus_name);
			modify_service_config();
			print_info_in_footer(false, "Configuring...");
			break;

		default:
			if ((unsigned) field_opts(current_field(my_form)) & O_EDIT)
				form_driver(my_form, ch);
			break;
	}
}

static void exec_action_context_services(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;

		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(my_menu);
			exec_action(item_userptr(item));
			print_info_in_footer(false, "Connecting...");
			break;

		case KEY_F(6):
			assert(context.tech != NULL &&
					context.tech->dbus_name != NULL &&
					context.tech->pretty_name != NULL);
			scan_tech(context.tech->dbus_name);
			print_info_in_footer(false, "Scanning %s...",
					context.tech->pretty_name);
			break;
	}
}

void ncurses_action(void)
{
	int ch = wgetch(win_body);

	if (ch == 27) {

		if (context.current_context == CONTEXT_HOME)
			print_info_in_footer2(false, "^C to quit");
		else
			exec_back();

		return;
	}

	if (ch == KEY_F(5)) {
		exec_refresh();
		return;
	}

	if (popup_exists()) {
		popup_driver(ch);
		popup_refresh();
		return;
	}

	switch (context.current_context) {
		case CONTEXT_HOME:
			exec_action_context_home(ch);
			break;

		case CONTEXT_SERVICE_CONFIG:
			exec_action_context_service_config(ch);
			break;

		case CONTEXT_SERVICES:
			exec_action_context_services(ch);
			break;

		default:
			print_info_in_footer(true, "No actions for"
					" context %d", context.current_context);
			break;
	}

	wrefresh(win_body);
}

int main(void)
{
	struct json_object *cmd;
	engine_callback = main_callback;

	if (engine_init() < 0)
		exit(1);

	signal(SIGINT, stop_loop);
	loop_init();

	initscr();
	assert(LINES >= 24 && COLS >= 80);
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	win_body_lines = LINES - 5;

	// If you don't do this, the service config form won't be displayed
	while (win_body_lines % 4 != 3)
		win_body_lines--;

	win_body = newwin(win_body_lines + 2, COLS, 1, 0);
	box(win_body, 0 ,0);
	keypad(win_body, TRUE);

	win_header = newwin(2, COLS, 0, 0);
	win_footer = newwin(2, COLS, LINES-2, 0);

	// Print all windows, according to the man it's more efficient than 3
	// wrefresh
	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();

	context.current_context = CONTEXT_HOME;
	context.serv = malloc(sizeof(struct userptr_data));
	assert(context.serv != NULL);
	context.tech = malloc(sizeof(struct userptr_data));
	assert(context.tech != NULL);
	context.serv->dbus_name = NULL;
	context.serv->pretty_name = NULL;
	context.tech->dbus_name = NULL;
	context.tech->pretty_name = NULL;

	// get_home_page (and render it)
	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_get_home_page));
	engine_query(cmd);
	refresh_home_msg();

	loop_run(true);
	loop_terminate();

	delwin(win_header);
	delwin(win_body);
	delwin(win_footer);
	endwin();

	return 0;
}
