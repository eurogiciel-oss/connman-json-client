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
#include "json_utils.h"
#include "renderers.h"
#include "keys.h"
#include "popup.h"
#include "special_win.h"

/*
 * This file is the glue between ncurses and the engine.
 * Thus every key pressed and callback end up here.
 */

/*
 * See renderers.c for details.
 */
extern int nb_items;
extern int nb_fields;

// See renderers.h for details.
extern struct context_info context;

/*
 * See popup.c for details.
 */
extern FORM *popup_form;
extern FIELD **popup_fields;
extern struct popup_actions **popup_btn_action;

static void print_services_for_tech(void);
static void connect_to_service(void);
static void get_state(void);
static void print_home_page(void);
static void exec_refresh(void);

// Refres isn't automatic, this could be problematic in high wifi density areas:
// your cursor would move around endlessly. Thus, automatic refresh is disabled
// by default in the context CONTEXT_SERVICES. The variable here force the
// refresh, it's true on special occasions like the user just managed to connect
static bool allow_refresh = false;

// This is used to store requests from the agent.
static char **agent_request_input_fields;

/*
 * Those are special windows, they are over anything until 'Esc'.
 * It's handy for error reporting and help messages.
 */
WINDOW *win_error;
WINDOW *win_help;

// This table contain every default action possible for each CONTEXT_XXX
static struct {
	void (*func)(); // What to do on action
	void (*func_free)(); // Free elements for the current_context
	void (*func_back)(); // What to do on 'Esc'
	void (*func_refresh)(); // Refresh the current_context
} context_actions[] = {
	{ print_services_for_tech, __renderers_free_home_page, print_home_page,
		print_home_page }, // CONTEXT_HOME
	{ NULL, __renderers_free_service_config, print_home_page,
		print_services_for_tech }, // CONTEXT_SERVICE_CONFIG
	{ connect_to_service, __renderers_free_services, print_home_page,
		print_services_for_tech }, // CONTEXT_SERVICES
};

/*
 * This method is mandatory, and might be of use somehow
 * e.g quite handy for debugging operations
 */
void callback_ended(void)
{
	return;
}

/*
 * Free everything possible in context.serv and context.tech.
 * Freed variables will be se to NULL.
 */
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

/*
 * Just a handy way to report a new error.
 */
static void report_error()
{
	if (win_error)
		return;

	win_error = win_error_new(win_body_lines, COLS-2, 2, 1,
			" Invalid argument/value.");
}

/*
 * Create a help window matching the context.
 */
static void get_help_window()
{
	char *msg;

	if (win_help)
		return;

	switch (context.current_context) {
		case CONTEXT_HOME:
			msg = " This view list technologies,\n"
				" * Press 'Return'/'Enter' for details on a technology\n"
				" * Press 'd' to disconnect a technology\n"
				" * Press 'p' to toggle a technology's power state\n"
				" * Press 'o' to toggle the OfflineMode (power on/off all technologies)\n"
				" * Press 'F5' to force refresh\n"
				" * Press '^C' to quit";
			break;

		case CONTEXT_SERVICE_CONFIG:
			msg = " This view list the connection/service settings. Use it wisely !\n"
				" /!\\ WARNING: Setting IPv4/6 'Method' to 'off' can block future connections to this service.\n"
				" You can modify underlined settings. Some settings require arrays of strings. Those have the following format: '[ \"string\", ... ]'.\n"
				" 'AutoConnect and 'Method' fields have a set of possible values, to change the value use the space bar or the left/right arrow keys.\n\n"
				" * 'Nameservers.Configuration', 'Timeservers.Configuration' and 'Domains.Configuration' must be an array of strings (IPs, domains...).\n"
				" * 'Proxy.Configuration':\n"
				"\t* 'URL' the url\n"
				"\t* 'Servers' and 'Excludes' are arrays of strings\n"
				" * Press 'F5' to force refresh, Press 'F7' to submit changes, Press '^C' to quit";
			break;

		case CONTEXT_SERVICES:
			msg = " This view list services the technology can connect to.\n"
				" 'f' at the start of the line means that this service has 'Favorite' = True\n"
				" * Press 'r' to remove saved information on a service\n"
				" * Press 'Return'/'Enter' to connect\n"
				" * Press 'F5' to force a refresh\n"
				" * Press 'F6' to force a rescan for the technology (some technologies don't support this)\n"
				" * Press '^C' to quit";
			break;
	}

	win_help = win_help_new(win_body_lines, COLS-2, 2, 1, msg);
}

/*
 * Free the popup allocated memory.
 * Note: this is different from popup.c popup_delete().
 */
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

/*
 * Stop the main loop and free the allocated memory.
 */
static void stop_loop(int signum, siginfo_t *si, void *useless)
{
	loop_quit();

	if (context_actions[context.current_context].func_free)
		context_actions[context.current_context].func_free();

	if (popup_exists())
		popup_free();

	if (win_exists(win_error))
		win_driver(&win_error, 27);

	if (win_exists(win_help))
		win_driver(&win_help, 27);

	engine_terminate();
	context_free_userptr_data();

	if (context.serv)
		free(context.serv);

	if (context.tech)
		free(context.tech);

	context.serv = NULL;
	context.tech = NULL;
}

/*
 * Create windows at the right dimensions.
 * If LINES and COLS are too small, we force the 24*80 screen.
 */
static void create_win(void)
{
	if (LINES < 24)
		LINES = 24;

	if (COLS < 80)
		COLS = 80;

	win_body_lines = LINES - 5;
	win_body = newwin(win_body_lines + 2, COLS, 1, 0);
	assert(win_body != NULL);
	box(win_body, 0, 0);

	win_header = newwin(1, COLS, 0, 0);
	win_footer = newwin(2, COLS, LINES-2, 0);
	assert(win_footer != NULL && win_header != NULL);

	redrawwin(win_header);
	redrawwin(win_body);
	redrawwin(win_footer);

	// If we don't do that, ncurses keep sending 'Esc' key on every key
	// pressed
	keypad(win_body, TRUE);
}

/*
 * Delete windows, as in delwin().
 */
static void delete_win(void)
{
	delwin(win_header);
	delwin(win_body);
	delwin(win_footer);
}

/*
 * Triggered on SIGWINCH signal.
 * This function have a resize effect (but we really delete and create windows).
 */
static void resize(int signum, siginfo_t *si, void *useless)
{
	sigset_t mask, orig_mask;

	// Mask the SIGWINCH signal, this will allow quite fast resize but won't
	// resist if you go nuclear on resize...
	sigemptyset(&mask);
	sigaddset(&mask, SIGWINCH);
	assert(sigprocmask(SIG_BLOCK, &mask, &orig_mask) == 0);

	loop_quit();
	endwin();
	clear();
	refresh();
	delete_win();
	create_win();
	get_state();
	exec_refresh();
	loop_run(true);

	if (popup_exists())
		popup_move((LINES-17)/2, (COLS-75)/2);

	if (win_error)
		win_resize(win_error, win_body_lines, COLS-2);

	if (win_help)
		win_resize(win_help, win_body_lines, COLS-2);

	// Restore the normal behaviour of SIGWINCH
	assert(sigprocmask(SIG_SETMASK, &orig_mask, NULL) == 0);
}

/*
 * Refresh context.cursor_id, free memory for the current context and execute
 * func_refresh() of the current context. The popup will be refreshed as well
 * (if it exists).
 */
static void exec_refresh(void)
{
	ITEM *item;
	FIELD *tmp_field;
	struct userptr_data *tmp;

	if (nb_fields != 0) {
		tmp_field = current_field(main_form);
		assert(tmp_field != NULL);
		tmp = field_userptr(tmp_field);
		context.cursor_id = strdup(tmp->dbus_name);
	} else if (nb_items != 0) {
		// We use the dbus_name as a unique key
		item = current_item(main_menu);
		assert(item != NULL);
		tmp = item_userptr(item);
		context.cursor_id = strdup(tmp->dbus_name);
	}

	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_refresh();
}

/*
 * Reset cursor position before refresh.
 * We use a value put in userptr by the renderers, this value is the "true"
 * dbus_name for menus and a "key" for forms.
 * The key on form make it work but it isn't a great solution.
 */
void repos_cursor(void)
{
	int i, j;
	struct userptr_data *tmp;
	ITEM *item;

	if (!context.cursor_id)
		return;

	if (nb_fields != 0) { // On forms

		for (i = 0; i < nb_fields; i++) {
			if (!(field_opts(main_fields[i]) & O_ACTIVE))
				continue;

			tmp = field_userptr(main_fields[i]);

			if (strncmp(tmp->dbus_name, context.cursor_id, 256) == 0) {
				// This trick print the page of the field and
				// set the cursor on the correct field.
				unpost_form(main_form);
				main_form->curpage = main_fields[i]->page;
				main_form->current = main_fields[i];
				post_form(main_form);
				break;
			}
		}

	} else if (nb_items != 0) { // On menus
		for (i = 0; i < nb_items; i++) {
			item = main_items[i];
			tmp = item_userptr(item);

			if (strncmp(context.cursor_id, tmp->dbus_name, 256) == 0) {
				for (j = 0; j < i; j++)
					menu_driver(main_menu, REQ_NEXT_ITEM);

				wrefresh(main_menu->usersub);
				break;
			}
		}
	}

	free(context.cursor_id);
	context.cursor_id = NULL;
	wrefresh(win_body);
}

/*
 * Free the current context and execute the appropriate action.
 * context.serv or context.tech are also updated.
 */
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

/*
 * Free the current context and execute func_back(). Also refresh the popup (if
 * it exists)
 */
static void exec_back(void)
{
	context_free_userptr_data();
	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_back();
}

/*
 * Execute a renderer action based on the jobj command name.
 * @param jobj See engine.c for more details.
 */
static void action_on_cmd_callback(struct json_object *jobj)
{
	struct json_object *data;
	const char *cmd_name;

	json_object_object_get_ex(jobj, key_command_data, &data);
	cmd_name = __json_get_command_str(jobj);

	/* dispatch according to the command name */
	if (strcmp(key_engine_get_home_page, cmd_name) == 0)
		__renderers_home_page(data);

	else if (strcmp(key_engine_get_services_from_tech, cmd_name) == 0)
		__renderers_services(data);

	else if (strcmp(key_engine_get_state, cmd_name) == 0)
		__renderers_state(data);

	else
		print_info_in_footer(true, "Unknown command called");
}

/*
 * Execute a refresh or back action depending on the signal data.
 * If the technology the user is currently looking at is removed, exec_back() is
 * executed. Else, exec_refresh() is executed, see comments on allow_refresh for
 * more details.
 * @param jobj See the format in commands.c monitor_changed().
 */
static void action_on_signal(struct json_object *jobj)
{
	struct json_object *sig_path, *sig_data, *sig_name;
	json_bool tmp_bool;
	bool is_tech_removed, is_current_tech, got_connected, got_removed;
	const char *tmp_str;
	char *dbus_short_name;

	json_object_object_get_ex(jobj, key_command_path, &sig_path);
	json_object_object_get_ex(jobj, key_command_data, &sig_data);
	json_object_object_get_ex(jobj, key_signal, &sig_name);

	is_tech_removed = strncmp(key_sig_tech_removed, json_object_get_string(sig_name), 50) == 0;

	if (context.tech->dbus_name == NULL)
		is_current_tech = false;
	else
		is_current_tech = strncmp(context.tech->dbus_name, json_object_to_json_string(sig_data), 256) == 0;

	if (is_tech_removed && is_current_tech) {
		exec_back();
		return;
	}

	got_connected = false;
	got_removed = false;

	if (sig_data && json_object_get_type(sig_data) == json_type_array) {
		tmp_str = json_object_get_string(json_object_array_get_idx(sig_data, 0));
		tmp_bool = json_object_get_boolean(json_object_array_get_idx(sig_data, 1));

		if (tmp_str && strcmp("Connected", tmp_str) == 0 && tmp_bool)
			got_connected = true;

		if (tmp_str && strcmp("Favorite", tmp_str) == 0 && !tmp_bool)
			got_removed = true;
	}

	if (context.current_context == CONTEXT_SERVICE_CONFIG) {
		if (context.serv->dbus_name == NULL)
			return;

		dbus_short_name = extract_dbus_short_name(context.serv->dbus_name);

		if (strcmp(dbus_short_name, json_object_get_string(sig_path)) != 0) {
			free(dbus_short_name);
			return;
		}

		free(dbus_short_name);
	}

	// This is to prevent always changing wifi services in areas
	// with a load of networks
	if (context.current_context != CONTEXT_SERVICES ||
			(allow_refresh || got_connected || got_removed)) {
		exec_refresh();
		allow_refresh = false;
	}
}

/*
 * The user pressed "OK" button on an agent request, all fields of the popup
 * form are extracted and the agent response is sent. The popup will also be
 * removed.
 */
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

	if (engine_query(cmd) == -EINVAL)
		report_error();

	popup_free();
}

/*
 * Response to the "Retry" agent error.
 * @param retry (json_type_bool -> int) 0 or 1
 */
static void popup_btn_action_retry(int retry)
{
	struct json_object *cmd;

	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_agent_retry));
	json_object_object_add(cmd, key_command_data,
			json_object_new_boolean(retry));

	if (engine_query(cmd) == -EINVAL)
		report_error();

	popup_free();
}

/*
 * Respond TRUE to the "Retry" agent error.
 */
static void popup_btn_action_retry_ok(void)
{
	popup_btn_action_retry(TRUE);
}

/*
 * Respond FALSE to the "Retry" agent error.
 */
static void popup_btn_action_retry_no(void)
{
	popup_btn_action_retry(FALSE);
}

/*
 * Create the fields for the popup.
 * @param serv_name the service name the agen request is about
 * @param data the agent requested values, see connman/doc/agent.api for details
 */
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

/*
 * Create the popup from an agent request.
 * Note: only "Input Requested" is fully supported for now.
 * @param jobj the agent request
 */
static void action_on_agent_msg(struct json_object *jobj)
{
	struct json_object *request, *service, *data;
	const char *request_str, *service_str, *fmt = "The network %s request"
		" credentials";
	char buf[150];
	int i;

	assert(!popup_exists());
	json_object_object_get_ex(jobj, key_agent_msg, &request);
	json_object_object_get_ex(jobj, key_agent_msg_data, &data);
	json_object_object_get_ex(jobj, key_service, &service);
	request_str = json_object_get_string(request);
	service_str = json_object_get_string(service);

	if (strncmp(key_agent_request_input, request_str, 15) == 0)
		agent_input_popup(service_str, data);

	else if (strncmp(key_agent_request_browser, request_str, 21) == 0)
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
 * Create a popup from an agent error message. Often asking to retry.
 * @param jobj the agent error e.g.:
 {
	key_agent_error: "invalid-key",
	key_service: "wifi_XXXXXXXXXXXX_YYYYYYYYYYYYYY_managed_psk",
	key_agent_error_message: "Retry ?",
	key_agent_error_callback: "report_error_return"
 }
 */
static void action_on_agent_error(struct json_object *jobj)
{
	char *fmt = "Agent error: %s on \"%s\", %s", buf[150];
	const char *error_msg_str, *service_str, *msg_str;
	struct json_object *tmp;
	struct popup_actions *popup_btn_no, *popup_btn_yes;

	assert(!popup_exists());
	json_object_object_get_ex(jobj, key_agent_error, &tmp);
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

/*
 * Dispatch the callback data to the appropriate functions based on the presence
 * of the attributes in jobj:
 *	- key_command
 *	- key_signal
 *	- key_agent_msg
 *	- key_agent_error
 *	- key_return_force_refresh
 * @param status the status code of this callback, status < 0 is an error
 * @param jobj see above
 */
static void main_callback(int status, struct json_object *jobj)
{
	struct json_object *cmd_tmp, *signal, *agent_msg, *agent_error,
			*return_user_data, *error;
	const char *error_str;

	json_object_object_get_ex(jobj, key_error, &error);
	error_str = json_object_get_string(error);

	if (status < 0) {
		print_info_in_footer2(true, "Error (code %d) %s", -status,
				error_str ? error_str : "[ no error message ]");
	}

	/* get the main object items */
	json_object_object_get_ex(jobj, key_command, &cmd_tmp);
	json_object_object_get_ex(jobj, key_signal, &signal);
	json_object_object_get_ex(jobj, key_agent_msg, &agent_msg);
	json_object_object_get_ex(jobj, key_agent_error, &agent_error);
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

	} else {
		int i = 0;

		json_object_object_foreach(jobj, key, val) {
			assert(key && val);
			i++;
		}

		if (i != 0)
			print_info_in_footer(true, "Unidentified call back: "
					"status: %d, jobj: %s\n", status,
					json_object_get_string(jobj));
	}
	
	// Release the memory of the json object now
	json_object_put(jobj);

	if (win_exists(win_error))
		win_driver(&win_error, 0);

	if (win_exists(win_help))
		win_driver(&win_help, 0);
}

/*
 * Asks for the home page.
 */
static void print_home_page(void)
{
	struct json_object *cmd;

	werase(win_footer);
	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_get_home_page));

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Asks for the state of connman. Note that the state is a part of the home
 * page. This particular function is useful to refresh win_header on resize.
 */
static void get_state(void)
{
	struct json_object *cmd;

	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_get_state));

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Asks for the services for the technology in context.tech->dbus_name.
 */
static void print_services_for_tech(void)
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
		report_error();
}

/*
 * Asks to connect to the service in context.serv->dbus_name.
 */
static void connect_to_service(void)
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
		report_error();

	werase(win_body);
	mvwprintw(win_body, 1, 2, "Connecting...");
	box(win_body, 0, 0);
	wrefresh(win_body);
}

/*
 * Asks for deconnection of data->dbus_name.
 * @param data the user pointer of a technology item
 */
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
		report_error();
}

/*
 * Asks to toggle the power state of the technology data->dbus_name.
 * @param data the user pointer of a technology item
 */
static void toggle_power_tech(struct userptr_data *data)
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_toggle_tech_power));
	json_object_object_add(tmp, key_technology,
			json_object_new_string(data->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Asks to toggle the OfflineMode attribute of the Manager.
 */
static void toggle_offline_mode()
{
	struct json_object *cmd;

	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_toggle_offline_mode));

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Asks for a rescan of the technology. Not all technologies support this.
 * @param tech_dbus_name the technology to rescan
 */
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

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Asks to remove the service saved information. This is kind of the equivalent
 * to "Favorite = False". see doc/service-api.txt for more details.
 * @param data the user pointer of the menu, data->dbus-name must be valid
 */
static void remove_service(struct userptr_data *data)
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string(key_engine_remove_service));
	json_object_object_add(tmp, key_service,
			json_object_new_string(data->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		report_error();
}

/*
 * Return the position of the previous label suffixed with ".Configuration".
 */
static int search_previous_config_label(int pos)
{
	FIELD *f;
	char *key_str, *tmp_str;

	for (; pos > 0 && main_fields[pos] != NULL; pos--) {
		f = main_fields[pos];

		if ((unsigned)field_opts(f) & O_EDIT)
			continue;

		key_str = field_buffer(f, 0);
		tmp_str = trim_whitespaces(key_str);

		if (string_ends_with_configuration(tmp_str))
			break;
	}
		
	return pos;
}

/*
 * Extract a json object out of the fields of form between 2 indexes min and
 * max.
 * @param jobj holds the extracted json objects
 * @param min the lowest index in main_fields[]
 * @param max the highest index in main_fields[], you can set this to 0 to cover all
 *	fields
 */
static void build_json_config(struct json_object *jobj, int min, int max)
{
	int i, pos_label;
	const char *pos_buffer, *buffer;
	char *pos_buffer_clean, *buffer_clean;
	json_bool autoconnect;
	struct json_object *tmp;
	bool buffer_is_array;

	for (i = max; i > min; i--) {
		if (!((unsigned)field_opts(main_fields[i]) & O_EDIT))
			continue;

		pos_label = i - 1;
		assert(pos_label >= 0 && pos_label >= min);

		buffer = field_buffer(main_fields[i], 0);
		pos_buffer = field_buffer(main_fields[pos_label], 0);
		assert(buffer != NULL && pos_buffer != NULL);
		buffer_clean = trim_whitespaces((char *)buffer);
		pos_buffer_clean = trim_whitespaces((char *)pos_buffer);

		buffer_is_array = strcmp(key_serv_nameservers_config,
				pos_buffer_clean) == 0;
		buffer_is_array |= strcmp(key_serv_timeservers_config,
				pos_buffer_clean) == 0;
		buffer_is_array |= strcmp(key_serv_domains_config,
				pos_buffer_clean) == 0;

		if (strcmp(key_serv_autoconnect, pos_buffer_clean) == 0) {
			autoconnect = strcmp("true", buffer_clean);
			json_object_object_add(jobj, pos_buffer_clean,
					json_object_new_boolean(autoconnect == 0 ? TRUE : FALSE));

		} else if (buffer_is_array) {
			tmp = json_tokener_parse(buffer_clean);

			if (tmp && json_object_get_type(tmp) != json_type_array) {
				// We are waiting for an array
				// We create one if we didn't get an array
				json_object_put(tmp);
				tmp = json_object_new_array();

			} else if (!tmp) // in case the field buffer was empty
				tmp = json_object_new_array();

			if (json_object_array_length(tmp) == 0)
				json_object_array_add(tmp, json_object_new_string(""));

			json_object_object_add(jobj, pos_buffer_clean, tmp);

		} else
			json_object_object_add(jobj, pos_buffer_clean,
					json_object_new_string(buffer_clean));
	}
}

/*
 * Asks to modify the configuration of a context.serv->dbus_name. Configuration
 * is extracted from the fields of the form and formated into a json object.
 */
static void modify_service_config(void)
{
	int i, pos_label;
	struct json_object *cmd = json_object_new_object();
	struct json_object *cmd_data = json_object_new_object();
	struct json_object *options = json_object_new_object();
	struct json_object *tmp;
	char *key_str;

	for (i = 0; main_fields[i]; i++) {
		if (!((unsigned)field_opts(main_fields[i]) & O_EDIT))
			continue;

		pos_label = search_previous_config_label(i);
		assert(pos_label >= 0);

		if (pos_label == 0) {
			// 11 = strlen("AutoConnect"), the rest of the
			// buffer is filled with spaces
			if (strncmp(key_serv_autoconnect, field_buffer(main_fields[i-1], 0), 11) != 0)
				continue;

			build_json_config(options, i-1, i);

		} else if (pos_label == i-1) {
			build_json_config(options, pos_label, i);

		} else { // object representation
			tmp = json_object_new_object();
			build_json_config(tmp, pos_label, i);
			key_str = field_buffer(main_fields[pos_label], 0);
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
		report_error();
}

/*
 * React to key pressed within context CONTEXT_HOME.
 */
static void exec_action_context_home(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_UP:
			menu_driver(main_menu, REQ_UP_ITEM);
			break;

		case KEY_DOWN:
			menu_driver(main_menu, REQ_DOWN_ITEM);
			break;

		case 'd':
			item = current_item(main_menu);
			disconnect_of_service(item_userptr(item));
			print_info_in_footer(false, "Disconnecting...");
			break;

		case 'p':
			item = current_item(main_menu);
			toggle_power_tech(item_userptr(item));
			print_info_in_footer(false, "Toogling power...");
			break;

		case 'o':
			toggle_offline_mode();
			print_info_in_footer(false, "Toogling OfflineMode...");
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(main_menu);
			exec_action(item_userptr(item));
			break;
	}
}

/*
 * React to key pressed within context CONTEXT_SERVICE_CONFIG.
 */
static void exec_action_context_service_config(int ch)
{
	int cur_page = form_page(main_form);
	struct userptr_data *data;

	if (field_type(current_field(main_form)) == TYPE_ENUM) {
		switch (ch) {
			case KEY_LEFT:
				form_driver(main_form, REQ_PREV_CHOICE);
				return;

			case ' ':
			case KEY_RIGHT:
				form_driver(main_form, REQ_NEXT_CHOICE);
				return;

			case KEY_UP:
			case KEY_DOWN:
			case KEY_NPAGE:
			case KEY_PPAGE:
			case KEY_F(7):
				break;

			default:
				return;
		}
	}

	switch (ch) {
		case KEY_DOWN:
			form_driver(main_form, REQ_NEXT_FIELD);
			break;

		case KEY_UP:
			form_driver(main_form, REQ_PREV_FIELD);
			break;

		case KEY_NPAGE:
			form_driver(main_form, REQ_NEXT_PAGE);
			set_form_page(main_form, ++cur_page);
			__renderers_services_config_paging();
			break;

		case KEY_PPAGE:
			form_driver(main_form, REQ_PREV_PAGE);
			set_form_page(main_form, --cur_page);
			__renderers_services_config_paging();
			break;

		// Delete the char before cursor
		case KEY_BACKSPACE:
		case 127:
			form_driver(main_form, REQ_DEL_PREV);
			break;

		// Delete the char under the cursor
		case KEY_DC:
			form_driver(main_form, REQ_DEL_CHAR);
			break;

		case KEY_LEFT:
			form_driver(main_form, REQ_PREV_CHAR);
			break;

		case KEY_RIGHT:
			form_driver(main_form, REQ_NEXT_CHAR);
			break;

		case KEY_F(7):
			// Fix the current item buffer
			form_driver(main_form, REQ_VALIDATION);
			// The dbus name of the service is stored in the first
			// item (it's a non active one). The active items have a
			// tagging system to keep the pointer's position on the
			// same field. We can't use exec_action for this because
			// we need to extract the values in fields *before*
			// freeing them (exec_action does the opposite).
			data = field_userptr(main_fields[0]);
			context.serv->dbus_name = strdup(data->dbus_name);
			modify_service_config();
			print_info_in_footer(false, "Configuring...");
			break;

		default:
			if ((unsigned) field_opts(current_field(main_form)) & O_EDIT)
				form_driver(main_form, ch);
			break;
	}
}

/*
 * React to key pressed within context CONTEXT_SERVICES.
 */
static void exec_action_context_services(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_DOWN:
			menu_driver(main_menu, REQ_DOWN_ITEM);
			break;

		case KEY_UP:
			menu_driver(main_menu, REQ_UP_ITEM);
			break;

		case 'r':
			item = current_item(main_menu);
			remove_service(item_userptr(item));
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(main_menu);
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

/*
 * Called by the main loop, dispatch key pressed according to the context.
 */
void ncurses_action(void)
{
	int ch = wgetch(win_body);

	if (win_exists(win_help)) {
		win_driver(&win_help, ch);

		// If the user hit any key but 'Esc', the work is over
		if (win_exists(win_help))
			return;

		// If the user hit 'Esc', we have to force the redraw of win_body
		redrawwin(win_body);
		wrefresh(win_body);
		return;
	}

	if (win_exists(win_error)) {
		win_driver(&win_error, ch);

		// If the user hit any key but 'Esc', the work is over
		if (win_exists(win_error))
			return;

		// If the user hit 'Esc', we have to force the redraw of win_body
		redrawwin(win_body);
		wrefresh(win_body);
		return;
	}

	if (ch == 27) {
		// In case the user didn't read the whole footer...
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

	if (ch == KEY_F(1)) {
		get_help_window();
		win_refresh(win_help);
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

/*
 * Initialize everything, run the loop then terminate everything.
 */
int main(void)
{
	struct sigaction sig_int, sig_winch;

	if (engine_init() < 0)
		exit(1);

	engine_callback = main_callback;

	// Affect actions to SIGINT and SIGWINCH
	sig_int.sa_flags = SA_SIGINFO;
	sig_winch.sa_flags = SA_SIGINFO;
	sigemptyset(&sig_int.sa_mask);
	sigemptyset(&sig_winch.sa_mask);
	sig_int.sa_sigaction = stop_loop;
	sig_winch.sa_sigaction = resize;
	sigaction(SIGINT, &sig_int, NULL);
	sigaction(SIGWINCH, &sig_winch, NULL);

	loop_init();

	initscr();
	assert(LINES >= 24 && COLS >= 80);
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	create_win();

	context.current_context = CONTEXT_HOME;
	context.serv = malloc(sizeof(struct userptr_data));
	assert(context.serv != NULL);
	context.tech = malloc(sizeof(struct userptr_data));
	assert(context.tech != NULL);
	context.serv->dbus_name = NULL;
	context.serv->pretty_name = NULL;
	context.tech->dbus_name = NULL;
	context.tech->pretty_name = NULL;

	print_home_page();

	loop_run(true);
	loop_terminate();

	delete_win();
	endwin();

	return 0;
}
