/* pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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
 */

#include <glib/gi18n-lib.h>

#include <talkatu.h>

#include <purple.h>

#include "gtkrequest.h"
#include "gtkblist.h"
#include "gtkutils.h"
#include "pidginaccountchooser.h"
#include "pidginaccountfilterconnected.h"
#include "pidgincore.h"

#include <gdk/gdkkeysyms.h>

typedef struct
{
	PurpleRequestType type;

	void *user_data;
	/* May be GtkWidget or GtkNativeDialog */
	gpointer dialog;

	GtkWidget *ok_button;

	size_t cb_count;
	GCallback *cbs;

	union
	{
		struct
		{
			GtkProgressBar *progress_bar;
		} wait;

		struct
		{
			GtkWidget *entry;

			gboolean multiline;
			gchar *hint;

		} input;

		struct
		{
			PurpleRequestFields *fields;

		} multifield;

		struct
		{
			gboolean savedialog;

		} file;

	} u;

} PidginRequestData;

static GHashTable *datasheet_stock = NULL;

static GtkWidget * create_account_field(PurpleRequestField *field);

static void
pidgin_widget_decorate_account(GtkWidget *cont, PurpleAccount *account)
{
	PurpleContactInfo *info = NULL;
	PurpleProtocol *protocol = NULL;
	GtkWidget *image;
	const gchar *icon_name = NULL;

	if(!PURPLE_IS_ACCOUNT(account)) {
		return;
	}

	info = PURPLE_CONTACT_INFO(account);
	protocol = purple_account_get_protocol(account);
	icon_name = purple_protocol_get_icon_name(protocol);

	image = gtk_image_new_from_icon_name(icon_name);

	gtk_widget_set_tooltip_text(image, purple_contact_info_get_username(info));

	if (GTK_IS_BOX(cont)) {
		gtk_widget_set_halign(image, GTK_ALIGN_START);
		gtk_widget_set_valign(image, GTK_ALIGN_START);
		gtk_box_append(GTK_BOX(cont), image);
	}
}

static void
generic_response_start(PidginRequestData *data)
{
	g_return_if_fail(data != NULL);

	g_object_set_data(G_OBJECT(data->dialog),
		"pidgin-window-is-closing", GINT_TO_POINTER(TRUE));
	gtk_widget_set_visible(GTK_WIDGET(data->dialog), FALSE);
}

static void
input_response_cb(G_GNUC_UNUSED GtkDialog *dialog, gint id,
                  PidginRequestData *data)
{
	const char *value;
	char *multiline_value = NULL;

	generic_response_start(data);

	if (data->u.input.multiline || purple_strequal(data->u.input.hint, "html")) {
		GtkTextBuffer *buffer =
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->u.input.entry));

		if (purple_strequal(data->u.input.hint, "html")) {
			multiline_value = talkatu_markup_get_html(buffer, NULL);
		} else {
			GtkTextIter start_iter, end_iter;

			gtk_text_buffer_get_start_iter(buffer, &start_iter);
			gtk_text_buffer_get_end_iter(buffer, &end_iter);

			multiline_value = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter,
										 FALSE);
		}

		value = multiline_value;
	}
	else {
		value = gtk_editable_get_text(GTK_EDITABLE(data->u.input.entry));
	}

	if (id >= 0 && (gsize)id < data->cb_count && data->cbs[id] != NULL)
		((PurpleRequestInputCb)data->cbs[id])(data->user_data, value);
	else if (data->cbs[1] != NULL)
		((PurpleRequestInputCb)data->cbs[1])(data->user_data, value);

	if (data->u.input.multiline) {
		g_free(multiline_value);
	}

	purple_request_close(PURPLE_REQUEST_INPUT, data);
}

static void
action_response_cb(G_GNUC_UNUSED GtkDialog *dialog, gint id,
                   PidginRequestData *data)
{
	generic_response_start(data);

	if (id >= 0 && (gsize)id < data->cb_count && data->cbs[id] != NULL)
		((PurpleRequestActionCb)data->cbs[id])(data->user_data, id);

	purple_request_close(PURPLE_REQUEST_INPUT, data);
}


static void
choice_response_cb(GtkDialog *dialog, gint id, PidginRequestData *data) {
	GtkDropDown *dropdown = g_object_get_data(G_OBJECT(dialog), "dropdown");

	generic_response_start(data);

	if(0 <= id && (gsize)id < data->cb_count && data->cbs[id] != NULL) {
		GObject *item = gtk_drop_down_get_selected_item(dropdown);
		if(G_IS_OBJECT(item)) {
			gpointer value = g_object_get_data(item, "choice_value");
			((PurpleRequestChoiceCb)data->cbs[id])(data->user_data, value);
		}
	}

	purple_request_close(PURPLE_REQUEST_INPUT, data);
}

static gboolean
field_string_focus_out_cb(GtkEventControllerFocus *controller,
                          PurpleRequestField *field)
{
	GtkWidget *entry = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
	const char *value;

	if (purple_request_field_string_is_multiline(field))
	{
		GtkTextBuffer *buffer;
		GtkTextIter start_iter, end_iter;

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry));

		gtk_text_buffer_get_start_iter(buffer, &start_iter);
		gtk_text_buffer_get_end_iter(buffer, &end_iter);

		value = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
	}
	else
		value = gtk_editable_get_text(GTK_EDITABLE(entry));

	purple_request_field_string_set_value(field,
			(*value == '\0' ? NULL : value));

	return FALSE;
}

static void
req_field_changed_common(G_GNUC_UNUSED GtkWidget *widget,
                         PurpleRequestField *field)
{
	PurpleRequestFieldGroup *group;
	PurpleRequestFields *fields;
	PidginRequestData *req_data;

	group = purple_request_field_get_group(field);
	fields = purple_request_field_group_get_fields_list(group);
	req_data = purple_request_fields_get_ui_data(fields);

	gtk_widget_set_sensitive(req_data->ok_button,
		purple_request_fields_all_required_filled(fields) &&
		purple_request_fields_all_valid(fields));
}

static void
field_bool_cb(GtkCheckButton *button, PurpleRequestField *field)
{
	purple_request_field_bool_set_value(field,
			gtk_check_button_get_active(button));

	req_field_changed_common(GTK_WIDGET(button), field);
}

static void
field_choice_option_cb(GObject *obj, G_GNUC_UNUSED GParamSpec *pspec,
                       gpointer data)
{
	PurpleRequestField *field = data;
	GtkDropDown *dropdown = GTK_DROP_DOWN(obj);
	GObject *item = gtk_drop_down_get_selected_item(dropdown);

	if(G_IS_OBJECT(item)) {
		gpointer value = g_object_get_data(item, "choice_value");
		purple_request_field_choice_set_value(field, value);
	}
}

static void
field_account_cb(GObject *obj, G_GNUC_UNUSED GParamSpec *pspec, gpointer data)
{
	PurpleRequestField *field = data;
	PidginAccountChooser *chooser = PIDGIN_ACCOUNT_CHOOSER(obj);

	purple_request_field_account_set_value(
	        field, pidgin_account_chooser_get_selected(chooser));

	req_field_changed_common(GTK_WIDGET(obj), field);
}

static void
multifield_ok_cb(GtkWidget *button, PidginRequestData *data)
{
	generic_response_start(data);

	if (!gtk_widget_has_focus(button))
		gtk_widget_grab_focus(button);

	if (data->cbs[0] != NULL)
		((PurpleRequestFieldsCb)data->cbs[0])(data->user_data,
											data->u.multifield.fields);

	purple_request_close(PURPLE_REQUEST_FIELDS, data);
}

static void
multifield_cancel_cb(G_GNUC_UNUSED GtkWidget *button, PidginRequestData *data)
{
	generic_response_start(data);

	if (data->cbs[1] != NULL)
		((PurpleRequestFieldsCb)data->cbs[1])(data->user_data,
											data->u.multifield.fields);

	purple_request_close(PURPLE_REQUEST_FIELDS, data);
}

static void
multifield_extra_cb(GtkWidget *button, PidginRequestData *data)
{
	PurpleRequestFieldsCb cb;

	generic_response_start(data);

	cb = g_object_get_data(G_OBJECT(button), "extra-cb");

	if (cb != NULL)
		cb(data->user_data, data->u.multifield.fields);

	purple_request_close(PURPLE_REQUEST_FIELDS, data);
}

static gboolean
destroy_multifield_cb(G_GNUC_UNUSED GtkWidget *self, PidginRequestData *data)
{
	multifield_cancel_cb(NULL, data);
	return FALSE;
}

static gchar *
pidgin_request_escape(PurpleRequestCommonParameters *cpar, const gchar *text)
{
	if (text == NULL)
		return NULL;

	if (purple_request_cpar_is_html(cpar)) {
		gboolean valid;

		valid = pango_parse_markup(text, -1, 0, NULL, NULL, NULL, NULL);

		if (valid)
			return g_strdup(text);
		else {
			purple_debug_error("pidgin", "Passed label text is not "
				"a valid markup. Falling back to plain text.");
		}
	}

	return g_markup_escape_text(text, -1);
}

static GtkWidget *
pidgin_request_dialog_icon(PurpleRequestType dialog_type,
	PurpleRequestCommonParameters *cpar)
{
	GtkWidget *img = NULL;
	PurpleRequestIconType icon_type;
	gconstpointer icon_data;
	gsize icon_size;
	const gchar *icon_name = "dialog-question";

	/* Dialog icon. */
	icon_data = purple_request_cpar_get_custom_icon(cpar, &icon_size);
	if (icon_data) {
		GdkPixbuf *pixbuf;

		pixbuf = purple_gdk_pixbuf_from_data(icon_data, icon_size);
		if (pixbuf) {
			/* scale the image if it is too large */
			int width = gdk_pixbuf_get_width(pixbuf);
			int height = gdk_pixbuf_get_height(pixbuf);
			if (width > 128 || height > 128) {
				int scaled_width = width > height ?
					128 : (128 * width) / height;
				int scaled_height = height > width ?
					128 : (128 * height) / width;
				GdkPixbuf *scaled;

				purple_debug_info("pidgin", "dialog icon was "
					"too large, scaling it down");

				scaled = gdk_pixbuf_scale_simple(pixbuf,
					scaled_width, scaled_height,
					GDK_INTERP_BILINEAR);
				if (scaled) {
					g_object_unref(pixbuf);
					pixbuf = scaled;
				}
			}
			img = gtk_image_new_from_pixbuf(pixbuf);
			g_object_unref(pixbuf);
		} else {
			purple_debug_info("pidgin",
				"failed to parse dialog icon");
		}
	}

	if (img)
		return img;

	icon_type = purple_request_cpar_get_icon(cpar);
	switch (icon_type)
	{
		case PURPLE_REQUEST_ICON_DEFAULT:
			icon_name = NULL;
			break;
		case PURPLE_REQUEST_ICON_REQUEST:
			icon_name = "dialog-question";
			break;
		case PURPLE_REQUEST_ICON_DIALOG:
		case PURPLE_REQUEST_ICON_INFO:
		case PURPLE_REQUEST_ICON_WAIT: /* TODO: we need another icon */
			icon_name = "dialog-information";
			break;
		case PURPLE_REQUEST_ICON_WARNING:
			icon_name = "dialog-warning";
			break;
		case PURPLE_REQUEST_ICON_ERROR:
			icon_name = "dialog-error";
			break;
		/* intentionally no default value */
	}

	if (icon_name == NULL) {
		switch (dialog_type) {
			case PURPLE_REQUEST_INPUT:
			case PURPLE_REQUEST_CHOICE:
			case PURPLE_REQUEST_ACTION:
			case PURPLE_REQUEST_FIELDS:
			case PURPLE_REQUEST_FILE:
			case PURPLE_REQUEST_FOLDER:
				icon_name = "dialog-question";
				break;
			case PURPLE_REQUEST_WAIT:
				icon_name = "dialog-information";
				break;
			/* intentionally no default value */
		}
	}

	if(icon_name == NULL) {
		icon_name = "dialog-question";
	}

	img = gtk_image_new_from_icon_name(icon_name);
	gtk_image_set_icon_size(GTK_IMAGE(img), GTK_ICON_SIZE_LARGE);

	return img;
}

static void
pidgin_request_help_clicked(GtkButton *button, G_GNUC_UNUSED gpointer _unused)
{
	PurpleRequestHelpCb cb;
	gpointer data;

	cb = g_object_get_data(G_OBJECT(button), "pidgin-help-cb");
	data = g_object_get_data(G_OBJECT(button), "pidgin-help-data");

	g_return_if_fail(cb != NULL);
	cb(data);
}

static void
pidgin_request_add_help(GtkDialog *dialog, PurpleRequestCommonParameters *cpar)
{
	GtkWidget *button;
	PurpleRequestHelpCb help_cb;
	gpointer help_data;

	help_cb = purple_request_cpar_get_help_cb(cpar, &help_data);
	if (help_cb == NULL)
		return;

	button = gtk_dialog_add_button(dialog, _("_Help"), GTK_RESPONSE_HELP);

	g_object_set_data(G_OBJECT(button), "pidgin-help-cb", help_cb);
	g_object_set_data(G_OBJECT(button), "pidgin-help-data", help_data);

	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(pidgin_request_help_clicked), NULL);
}

static void *
pidgin_request_input(const char *title, const char *primary,
					   const char *secondary, const char *default_value,
					   gboolean multiline, gboolean masked, gchar *hint,
					   const char *ok_text, GCallback ok_cb,
					   const char *cancel_text, GCallback cancel_cb,
					   PurpleRequestCommonParameters *cpar,
					   void *user_data)
{
	PidginRequestData *data;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkLabel *label;
	GtkWidget *img;
	GtkWidget *content;
	char *label_text;
	char *primary_esc, *secondary_esc;

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_INPUT;
	data->user_data = user_data;

	data->cb_count = 2;
	data->cbs = g_new0(GCallback, 2);

	data->cbs[0] = ok_cb;
	data->cbs[1] = cancel_cb;

	/* Create the dialog. */
	dialog = gtk_dialog_new_with_buttons(title ? title : PIDGIN_ALERT_TITLE,
					     NULL, 0,
					     cancel_text, 1,
					     ok_text,     0,
					     NULL);
	data->dialog = dialog;

	g_signal_connect(G_OBJECT(dialog), "response",
					 G_CALLBACK(input_response_cb), data);

	/* Setup the dialog */
	gtk_widget_set_margin_top(dialog, 6);
	gtk_widget_set_margin_bottom(dialog, 6);
	gtk_widget_set_margin_start(dialog, 6);
	gtk_widget_set_margin_end(dialog, 6);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_widget_set_margin_top(content, 6);
	gtk_widget_set_margin_bottom(content, 6);
	gtk_widget_set_margin_start(content, 6);
	gtk_widget_set_margin_end(content, 6);

	if (!multiline)
		gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), 0);
	gtk_box_set_spacing(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	                    12);

	/* Setup the main horizontal box */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(content), hbox);

	/* Dialog icon. */
	img = pidgin_request_dialog_icon(PURPLE_REQUEST_INPUT, cpar);
	gtk_widget_set_halign(img, GTK_ALIGN_START);
	gtk_widget_set_valign(img, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(hbox), img);

	pidgin_request_add_help(GTK_DIALOG(dialog), cpar);

	/* Vertical box */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_hexpand(vbox, TRUE);
	gtk_box_append(GTK_BOX(hbox), vbox);

	pidgin_widget_decorate_account(hbox, purple_request_cpar_get_account(cpar));

	/* Descriptive label */
	primary_esc = pidgin_request_escape(cpar, primary);
	secondary_esc = pidgin_request_escape(cpar, secondary);
	label_text = g_strdup_printf((primary ? "<span weight=\"bold\" size=\"larger\">"
								 "%s</span>%s%s" : "%s%s%s"),
								 (primary ? primary_esc : ""),
								 ((primary && secondary) ? "\n\n" : ""),
								 (secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = GTK_LABEL(gtk_label_new(NULL));

	gtk_label_set_markup(label, label_text);
	gtk_label_set_wrap(label, TRUE);
	gtk_label_set_xalign(label, 0);
	gtk_label_set_yalign(label, 0);
	gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(label));

	g_free(label_text);

	/* Entry field. */
	data->u.input.multiline = multiline;
	data->u.input.hint = g_strdup(hint);

	if(multiline || purple_strequal(data->u.input.hint, "html")) {
		GtkWidget *editor = talkatu_editor_new();
		GtkWidget *input = talkatu_editor_get_input(TALKATU_EDITOR(editor));
		GtkTextBuffer *buffer = NULL;

		gtk_widget_set_size_request(input, 320, 130);
		gtk_widget_set_name(input, "pidgin_request_input");
		gtk_widget_set_vexpand(editor, TRUE);
		gtk_box_append(GTK_BOX(vbox), editor);

		if (purple_strequal(data->u.input.hint, "html")) {
			GSimpleActionGroup *ag = NULL;

			ag = talkatu_action_group_new(TALKATU_FORMAT_HTML);
			buffer = talkatu_buffer_new(ag);
			talkatu_action_group_set_buffer(TALKATU_ACTION_GROUP(ag), buffer);
			g_clear_object(&ag);

			if(default_value != NULL) {
				talkatu_markup_set_html(TALKATU_BUFFER(buffer), default_value, -1);
			}
		} else {
			buffer = gtk_text_buffer_new(NULL);

			if(default_value != NULL) {
				gtk_text_buffer_set_text(buffer, default_value, -1);
			}
		}

		gtk_text_view_set_buffer(GTK_TEXT_VIEW(input), buffer);

		data->u.input.entry = input;
	} else {
		GtkWidget *entry = NULL;

		if(masked) {
			entry = gtk_password_entry_new();
			g_object_set(entry, "activates-default", TRUE,
			             "show-peek-icon", TRUE, NULL);
		} else {
			entry = gtk_entry_new();
			gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
		}

		gtk_box_append(GTK_BOX(vbox), entry);

		if(default_value != NULL) {
			gtk_editable_set_text(GTK_EDITABLE(entry), default_value);
		}

		data->u.input.entry = entry;
	}

	pidgin_set_accessible_label(data->u.input.entry, label);

	pidgin_auto_parent_window(dialog);

	/* Show everything. */
	gtk_widget_show(dialog);

	return data;
}

static void *
pidgin_request_choice(const char *title, const char *primary,
	const char *secondary, gpointer default_value, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data, va_list args)
{
	PidginRequestData *data;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *img;
	GtkWidget *dropdown;
	GListModel *model;
	GtkWidget *content;
	char *label_text;
	const char *radio_text;
	char *primary_esc, *secondary_esc;
	guint index, selected;

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_ACTION;
	data->user_data = user_data;

	data->cb_count = 2;
	data->cbs = g_new0(GCallback, 2);
	data->cbs[0] = cancel_cb;
	data->cbs[1] = ok_cb;

	/* Create the dialog. */
	data->dialog = dialog = gtk_dialog_new();

	if (title != NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), title);
#ifdef _WIN32
		gtk_window_set_title(GTK_WINDOW(dialog), PIDGIN_ALERT_TITLE);
#endif

	gtk_dialog_add_button(GTK_DIALOG(dialog), cancel_text, 0);
	gtk_dialog_add_button(GTK_DIALOG(dialog), ok_text, 1);

	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(choice_response_cb), data);

	/* Setup the dialog */
	gtk_widget_set_margin_top(dialog, 6);
	gtk_widget_set_margin_bottom(dialog, 6);
	gtk_widget_set_margin_start(dialog, 6);
	gtk_widget_set_margin_end(dialog, 6);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_widget_set_margin_top(content, 6);
	gtk_widget_set_margin_bottom(content, 6);
	gtk_widget_set_margin_start(content, 6);
	gtk_widget_set_margin_end(content, 6);
	gtk_box_set_spacing(GTK_BOX(content), 12);

	/* Setup the main horizontal box */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(content), hbox);

	/* Dialog icon. */
	img = pidgin_request_dialog_icon(PURPLE_REQUEST_CHOICE, cpar);
	gtk_widget_set_halign(img, GTK_ALIGN_START);
	gtk_widget_set_valign(img, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(hbox), img);

	pidgin_widget_decorate_account(hbox, purple_request_cpar_get_account(cpar));

	pidgin_request_add_help(GTK_DIALOG(dialog), cpar);

	/* Vertical box */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_box_append(GTK_BOX(hbox), vbox);

	/* Descriptive label */
	primary_esc = pidgin_request_escape(cpar, primary);
	secondary_esc = pidgin_request_escape(cpar, secondary);
	label_text = g_strdup_printf((primary ? "<span weight=\"bold\" size=\"larger\">"
				      "%s</span>%s%s" : "%s%s%s"),
				     (primary ? primary_esc : ""),
				     ((primary && secondary) ? "\n\n" : ""),
				     (secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), label_text);
	gtk_label_set_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0);
	gtk_widget_set_vexpand(label, TRUE);
	gtk_box_append(GTK_BOX(vbox), label);

	g_free(label_text);

	dropdown = gtk_drop_down_new_from_strings(NULL);
	gtk_box_append(GTK_BOX(vbox), dropdown);
	g_object_set_data(G_OBJECT(dialog), "dropdown", dropdown);

	index = 0;
	selected = GTK_INVALID_LIST_POSITION;
	model = gtk_drop_down_get_model(GTK_DROP_DOWN(dropdown));
	while((radio_text = va_arg(args, const char *))) {
		GObject *item = NULL;
		gpointer resp = va_arg(args, gpointer);

		gtk_string_list_append(GTK_STRING_LIST(model), radio_text);
		item = g_list_model_get_item(model, index);
		g_object_set_data(item, "choice_value", resp);
		if (resp == default_value) {
			selected = index;
		}

		g_clear_object(&item);
		index++;
	}

	if(selected != GTK_INVALID_LIST_POSITION) {
		gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), selected);
	}

	/* Show everything. */
	pidgin_auto_parent_window(dialog);

	gtk_widget_show(dialog);

	return data;
}

static void *
pidgin_request_action(const char *title, const char *primary,
	const char *secondary, int default_action,
	PurpleRequestCommonParameters *cpar, void *user_data,
	size_t action_count, va_list actions)
{
	PidginRequestData *data;
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *img = NULL;
	GtkWidget *content;
	void **buttons;
	char *label_text;
	char *primary_esc, *secondary_esc;
	gsize i;

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_ACTION;
	data->user_data = user_data;

	data->cb_count = action_count;
	data->cbs = g_new0(GCallback, action_count);

	/* Reverse the buttons */
	buttons = g_new0(void *, action_count * 2);

	for (i = 0; i < action_count * 2; i += 2) {
		buttons[(action_count * 2) - i - 2] = va_arg(actions, char *);
		buttons[(action_count * 2) - i - 1] = va_arg(actions, GCallback);
	}

	/* Create the dialog. */
	data->dialog = dialog = gtk_dialog_new();

	gtk_window_set_deletable(GTK_WINDOW(data->dialog), FALSE);

	if (title != NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), title);
#ifdef _WIN32
	else
		gtk_window_set_title(GTK_WINDOW(dialog), PIDGIN_ALERT_TITLE);
#endif

	for (i = 0; i < action_count; i++) {
		gtk_dialog_add_button(GTK_DIALOG(dialog), buttons[2 * i], i);

		data->cbs[i] = buttons[2 * i + 1];
	}

	g_free(buttons);

	g_signal_connect(G_OBJECT(dialog), "response",
					 G_CALLBACK(action_response_cb), data);

	/* Setup the dialog */
	gtk_widget_set_margin_top(dialog, 6);
	gtk_widget_set_margin_bottom(dialog, 6);
	gtk_widget_set_margin_start(dialog, 6);
	gtk_widget_set_margin_end(dialog, 6);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_widget_set_margin_top(content, 6);
	gtk_widget_set_margin_bottom(content, 6);
	gtk_widget_set_margin_start(content, 6);
	gtk_widget_set_margin_end(content, 6);
	gtk_box_set_spacing(GTK_BOX(content), 12);

	/* Setup the main horizontal box */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(content), hbox);

	img = pidgin_request_dialog_icon(PURPLE_REQUEST_ACTION, cpar);
	gtk_widget_set_halign(img, GTK_ALIGN_START);
	gtk_widget_set_valign(img, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(hbox), img);

	/* Vertical box */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_box_append(GTK_BOX(hbox), vbox);

	pidgin_widget_decorate_account(hbox,
		purple_request_cpar_get_account(cpar));

	pidgin_request_add_help(GTK_DIALOG(dialog), cpar);

	/* Descriptive label */
	primary_esc = pidgin_request_escape(cpar, primary);
	secondary_esc = pidgin_request_escape(cpar, secondary);
	label_text = g_strdup_printf((primary ? "<span weight=\"bold\" size=\"larger\">"
								 "%s</span>%s%s" : "%s%s%s"),
								 (primary ? primary_esc : ""),
								 ((primary && secondary) ? "\n\n" : ""),
								 (secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), label_text);
	gtk_label_set_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_widget_set_vexpand(label, TRUE);
	gtk_box_append(GTK_BOX(vbox), label);

	g_free(label_text);


	if (default_action != PURPLE_DEFAULT_ACTION_NONE) {
		/*
		 * Need to invert the default_action number because the
		 * buttons are added to the dialog in reverse order.
		 */
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), action_count - 1 - default_action);
	}

	/* Show everything. */
	pidgin_auto_parent_window(dialog);

	gtk_widget_show(dialog);

	return data;
}

static void
wait_response_cb(G_GNUC_UNUSED GtkDialog *dialog, G_GNUC_UNUSED gint id,
                 PidginRequestData *data)
{
	generic_response_start(data);

	if (data->cbs[0] != NULL)
		((PurpleRequestCancelCb)data->cbs[0])(data->user_data);

	purple_request_close(PURPLE_REQUEST_FIELDS, data);
}

static void *
pidgin_request_wait(const char *title, const char *primary,
	const char *secondary, gboolean with_progress,
	PurpleRequestCancelCb cancel_cb, PurpleRequestCommonParameters *cpar,
	void *user_data)
{
	PidginRequestData *data;
	GtkWidget *dialog, *content;
	GtkWidget *hbox, *vbox, *img, *label;
	gchar *primary_esc, *secondary_esc, *label_text;

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_WAIT;
	data->user_data = user_data;

	data->cb_count = 1;
	data->cbs = g_new0(GCallback, 1);
	data->cbs[0] = (GCallback)cancel_cb;

	data->dialog = dialog = gtk_dialog_new();

	g_signal_connect(G_OBJECT(dialog), "response",
	                 G_CALLBACK(wait_response_cb), data);

	gtk_window_set_deletable(GTK_WINDOW(data->dialog), cancel_cb != NULL);

	if (title != NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), title);
	else
		gtk_window_set_title(GTK_WINDOW(dialog), _("Please wait"));

	/* Setup the dialog */
	gtk_widget_set_margin_top(dialog, 6);
	gtk_widget_set_margin_bottom(dialog, 6);
	gtk_widget_set_margin_start(dialog, 6);
	gtk_widget_set_margin_end(dialog, 6);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_widget_set_margin_top(content, 6);
	gtk_widget_set_margin_bottom(content, 6);
	gtk_widget_set_margin_start(content, 6);
	gtk_widget_set_margin_end(content, 6);
	gtk_box_set_spacing(GTK_BOX(content), 12);

	/* Setup the main horizontal box */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(content), hbox);

	img = pidgin_request_dialog_icon(PURPLE_REQUEST_WAIT, cpar);
	gtk_widget_set_halign(img, GTK_ALIGN_START);
	gtk_widget_set_valign(img, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(hbox), img);

	/* Cancel button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);

	/* Vertical box */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_box_append(GTK_BOX(hbox), vbox);

	pidgin_widget_decorate_account(hbox,
		purple_request_cpar_get_account(cpar));

	pidgin_request_add_help(GTK_DIALOG(dialog), cpar);

	/* Descriptive label */
	primary_esc = pidgin_request_escape(cpar, primary);
	secondary_esc = pidgin_request_escape(cpar, secondary);
	label_text = g_strdup_printf((primary ? "<span weight=\"bold\" "
		"size=\"larger\">%s</span>%s%s" : "%s%s%s"),
		(primary ? primary_esc : ""),
		((primary && secondary) ? "\n\n" : ""),
		(secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), label_text);
	gtk_label_set_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_yalign(GTK_LABEL(label), 0);
	gtk_label_set_selectable(GTK_LABEL(label), FALSE);
	gtk_widget_set_vexpand(label, TRUE);
	gtk_box_append(GTK_BOX(vbox), label);

	g_free(label_text);

	if (with_progress) {
		GtkProgressBar *bar;

		bar = data->u.wait.progress_bar =
			GTK_PROGRESS_BAR(gtk_progress_bar_new());
		gtk_progress_bar_set_fraction(bar, 0);
		gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(bar));
	}

	/* Show everything. */
	pidgin_auto_parent_window(dialog);

	gtk_widget_show(dialog);

	return data;
}

static void
pidgin_request_wait_update(void *ui_handle, gboolean pulse, gfloat fraction)
{
	GtkProgressBar *bar;
	PidginRequestData *data = ui_handle;

	g_return_if_fail(data->type == PURPLE_REQUEST_WAIT);

	bar = data->u.wait.progress_bar;
	if (pulse)
		gtk_progress_bar_pulse(bar);
	else
		gtk_progress_bar_set_fraction(bar, fraction);
}

static void
req_entry_field_changed_cb(GtkWidget *entry, PurpleRequestField *field)
{
	if (purple_request_field_get_field_type(field) == PURPLE_REQUEST_FIELD_INTEGER) {
		int value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry));
		purple_request_field_int_set_value(field, value);

	} else if (purple_request_field_string_is_multiline(field)) {
		char *text;
		GtkTextIter start_iter, end_iter;

		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(entry), &start_iter);
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(entry), &end_iter);

		text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(entry), &start_iter, &end_iter, FALSE);
		purple_request_field_string_set_value(field, (!text || !*text) ? NULL : text);
		g_free(text);

	} else {
		const char *text = NULL;
		text = gtk_editable_get_text(GTK_EDITABLE(entry));
		purple_request_field_string_set_value(field, (*text == '\0') ? NULL : text);
	}

	req_field_changed_common(entry, field);
}

static void
setup_entry_field(GtkWidget *entry, PurpleRequestField *field)
{
	const char *type_hint;

	g_signal_connect(entry, "changed", G_CALLBACK(req_entry_field_changed_cb),
	                 field);

	if ((type_hint = purple_request_field_get_type_hint(field)) != NULL)
	{
		if (g_str_has_prefix(type_hint, "screenname")) {
			GtkWidget *optmenu = NULL;
			PurpleRequestFieldGroup *group = purple_request_field_get_group(field);
			GList *fields = purple_request_field_group_get_fields(group);

			/* Ensure the account option menu is created (if the widget hasn't
			 * been initialized already) for username auto-completion. */
			while (fields)
			{
				PurpleRequestField *fld = fields->data;
				fields = fields->next;

				if (purple_request_field_get_field_type(fld) == PURPLE_REQUEST_FIELD_ACCOUNT &&
						purple_request_field_is_visible(fld))
				{
					const char *type_hint = purple_request_field_get_type_hint(fld);
					if (purple_strequal(type_hint, "account"))
					{
						optmenu = GTK_WIDGET(purple_request_field_get_ui_data(fld));
						if (optmenu == NULL) {
							optmenu = GTK_WIDGET(create_account_field(fld));
							purple_request_field_set_ui_data(fld, optmenu);
						}
						break;
					}
				}
			}
			pidgin_setup_screenname_autocomplete(entry, optmenu, pidgin_screenname_autocomplete_default_filter, GINT_TO_POINTER(purple_strequal(type_hint, "screenname-all")));
		}
	}
}

static GtkWidget *
create_string_field(PurpleRequestField *field)
{
	const char *value;
	GtkWidget *widget;
	gboolean is_editable;

	value = purple_request_field_string_get_default_value(field);
	is_editable = purple_request_field_is_sensitive(field);

	if (purple_request_field_string_is_multiline(field))
	{
		GtkWidget *textview;
		GtkEventController *controller;

		textview = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),
								   TRUE);
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview),
									GTK_WRAP_WORD_CHAR);

		if (value != NULL)
		{
			GtkTextBuffer *buffer;

			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

			gtk_text_buffer_set_text(buffer, value, -1);
		}

		gtk_widget_set_tooltip_text(textview, purple_request_field_get_tooltip(field));

		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), is_editable);

		controller = gtk_event_controller_focus_new();
		gtk_widget_add_controller(textview, controller);
		g_signal_connect(controller, "leave",
						 G_CALLBACK(field_string_focus_out_cb), field);

	    if (purple_request_field_is_required(field))
	    {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
			g_signal_connect(buffer, "changed",
			                 G_CALLBACK(req_entry_field_changed_cb), field);
	    }

		widget = gtk_scrolled_window_new();
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
		                               GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
		gtk_widget_set_size_request(widget, -1, 75);
		gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widget), textview);
	}
	else
	{
		GtkEventController *controller = NULL;

		if (purple_request_field_string_is_masked(field)) {
			widget = gtk_password_entry_new();
			g_object_set(widget, "activates-default", TRUE,
			             "show-peek-icon", TRUE, NULL);
		} else {
			widget = gtk_entry_new();
			gtk_entry_set_activates_default(GTK_ENTRY(widget), TRUE);
		}

		setup_entry_field(widget, field);

		if(value != NULL) {
			gtk_editable_set_text(GTK_EDITABLE(widget), value);
		}

		gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

		gtk_editable_set_editable(GTK_EDITABLE(widget), is_editable);

		controller = gtk_event_controller_focus_new();
		gtk_widget_add_controller(widget, controller);
		g_signal_connect(controller, "leave",
						 G_CALLBACK(field_string_focus_out_cb), field);
	}

	return widget;
}

static GtkWidget *
create_int_field(PurpleRequestField *field)
{
	int value;
	GtkWidget *widget;

	widget = gtk_spin_button_new_with_range(
		purple_request_field_int_get_lower_bound(field),
		purple_request_field_int_get_upper_bound(field), 1);

	setup_entry_field(widget, field);

	value = purple_request_field_int_get_default_value(field);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);

	gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

	return widget;
}

static GtkWidget *
create_bool_field(PurpleRequestField *field,
	PurpleRequestCommonParameters *cpar)
{
	GtkWidget *widget;
	gchar *label;

	label = pidgin_request_escape(cpar,
		purple_request_field_get_label(field));
	widget = gtk_check_button_new_with_label(label);
	g_free(label);

	gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

	gtk_check_button_set_active(GTK_CHECK_BUTTON(widget),
		purple_request_field_bool_get_default_value(field));

	g_signal_connect(widget, "toggled", G_CALLBACK(field_bool_cb), field);

	return widget;
}

static GtkWidget *
create_choice_field(PurpleRequestField *field) {
	GtkWidget *widget;
	GListModel *model = NULL;
	GList *elements = NULL;
	guint default_index = GTK_INVALID_LIST_POSITION;
	gpointer default_value;
	guint index;

	default_value = purple_request_field_choice_get_value(field);
	widget = gtk_drop_down_new_from_strings(NULL);
	model = gtk_drop_down_get_model(GTK_DROP_DOWN(widget));

	index = 0;
	elements = purple_request_field_choice_get_elements(field);
	for(GList *l = elements; l != NULL; l = g_list_next(l)) {
		PurpleKeyValuePair *choice = l->data;
		GObject *item = NULL;

		gtk_string_list_append(GTK_STRING_LIST(model), choice->key);
		item = g_list_model_get_item(model, index);
		g_object_set_data(item, "choice_value", choice->value);
		if(choice->value == default_value) {
			default_index = index;
		}

		g_clear_object(&item);
		index++;
	}

	gtk_drop_down_set_selected(GTK_DROP_DOWN(widget), default_index);

	gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

	g_signal_connect(G_OBJECT(widget), "notify::selected",
	                 G_CALLBACK(field_choice_option_cb), field);

	if(default_index == GTK_INVALID_LIST_POSITION && index > 0) {
		GObject *item = g_list_model_get_item(model, 0);
		gpointer value = g_object_get_data(item, "choice_value");
		purple_request_field_choice_set_value(field, value);
		g_clear_object(&item);
	}

	return widget;
}

static GtkWidget *
create_image_field(PurpleRequestField *field)
{
	GtkWidget *widget;
	GdkPixbuf *buf, *scale;

	buf = purple_gdk_pixbuf_from_data(
			(const guchar *)purple_request_field_image_get_buffer(field),
			purple_request_field_image_get_size(field));

	scale = gdk_pixbuf_scale_simple(buf,
			purple_request_field_image_get_scale_x(field) * gdk_pixbuf_get_width(buf),
			purple_request_field_image_get_scale_y(field) * gdk_pixbuf_get_height(buf),
			GDK_INTERP_BILINEAR);
	widget = gtk_image_new_from_pixbuf(scale);
	g_object_unref(G_OBJECT(buf));
	g_object_unref(G_OBJECT(scale));

	gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

	return widget;
}

static gboolean
field_custom_account_filter_cb(gpointer item, G_GNUC_UNUSED gpointer data) {
	PurpleFilterAccountFunc func = data;
	gboolean ret = FALSE;

	if(PURPLE_IS_ACCOUNT(item)) {
		ret = func(PURPLE_ACCOUNT(item));
	}

	return ret;
}

static GtkWidget *
create_account_field(PurpleRequestField *field)
{
	GtkWidget *widget = NULL;
	PurpleAccount *account = NULL;
	PurpleFilterAccountFunc account_filter = NULL;
	GtkFilter *filter = NULL;

	widget = pidgin_account_chooser_new();
	account  = purple_request_field_account_get_default_value(field);

	account_filter = purple_request_field_account_get_filter(field);
	if(account_filter != NULL) {
		GtkCustomFilter *custom_filter = NULL;

		custom_filter = gtk_custom_filter_new(field_custom_account_filter_cb,
		                                      account_filter, NULL);

		filter = GTK_FILTER(custom_filter);
	}

	if(!purple_request_field_account_get_show_all(field)) {
		GtkEveryFilter *every = NULL;

		every = gtk_every_filter_new();

		if(GTK_IS_FILTER(filter)) {
			gtk_multi_filter_append(GTK_MULTI_FILTER(every), filter);
		}

		filter = pidgin_account_filter_connected_new();
		gtk_multi_filter_append(GTK_MULTI_FILTER(every), filter);

		filter = GTK_FILTER(every);
	}

	pidgin_account_chooser_set_selected(PIDGIN_ACCOUNT_CHOOSER(widget),
	                                    account);

	if(GTK_IS_FILTER(filter)) {
		pidgin_account_chooser_set_filter(PIDGIN_ACCOUNT_CHOOSER(widget), filter);
		g_object_unref(filter);
	}

	g_signal_connect(widget, "notify::account", G_CALLBACK(field_account_cb),
	                 field);

	gtk_widget_set_tooltip_text(widget, purple_request_field_get_tooltip(field));

	return widget;
}

static void
setup_list_field_listitem_cb(G_GNUC_UNUSED GtkSignalListItemFactory *self,
                             GtkListItem *item, gpointer data)
{
	PurpleRequestField *field = data;
	GtkWidget *box = NULL;
	GtkWidget *widget = NULL;

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_list_item_set_child(item, box);

	widget = gtk_label_new(NULL);
	gtk_box_append(GTK_BOX(box), widget);

	if(purple_request_field_list_has_icons(field)) {
		widget = gtk_image_new();
		gtk_box_append(GTK_BOX(box), widget);
	}
}

static void
bind_list_field_listitem_cb(G_GNUC_UNUSED GtkSignalListItemFactory *self,
                            GtkListItem *item, gpointer data)
{
	PurpleRequestField *field = data;
	GtkWidget *box = NULL;
	GtkWidget *label = NULL;
	GObject *wrapper = NULL;

	box = gtk_list_item_get_child(item);
	wrapper = gtk_list_item_get_item(item);

	label = gtk_widget_get_first_child(box);
	gtk_label_set_text(GTK_LABEL(label), g_object_get_data(wrapper, "text"));

	if(purple_request_field_list_has_icons(field)) {
		GtkWidget *image = NULL;

		image = gtk_widget_get_last_child(box);
		gtk_image_set_from_pixbuf(GTK_IMAGE(image),
		                          g_object_get_data(wrapper, "pixbuf"));
	}
}

static void
list_field_select_changed_cb(GtkSelectionModel *self,
                             G_GNUC_UNUSED guint position,
                             G_GNUC_UNUSED guint n_items, gpointer data)
{
	PurpleRequestField *field = data;
	GtkBitset *bitset = NULL;

	purple_request_field_list_clear_selected(field);

	bitset = gtk_selection_model_get_selection(self);
	n_items = gtk_bitset_get_size(bitset);

	for(guint index = 0; index < n_items; index++) {
		GObject *wrapper = NULL;
		const char *text = NULL;

		wrapper = g_list_model_get_item(G_LIST_MODEL(self),
		                                gtk_bitset_get_nth(bitset, index));

		text = g_object_get_data(wrapper, "text");
		purple_request_field_list_add_selected(field, text);

		g_object_unref(wrapper);
	}

	gtk_bitset_unref(bitset);
}

static GtkWidget *
create_list_field(PurpleRequestField *field)
{
	GtkWidget *sw;
	GtkWidget *listview = NULL;
	GtkSelectionModel *sel = NULL;
	GtkListItemFactory *factory = NULL;
	GListStore *store = NULL;
	guint index = 0;
	GList *l;
	gboolean has_icons;

	has_icons = purple_request_field_list_has_icons(field);

	/* Create the list store */
	store = g_list_store_new(G_TYPE_OBJECT);
	if(purple_request_field_list_get_multi_select(field)) {
		sel = GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(store)));
	} else {
		sel = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(store)));
	}

	/* Create the row factory. */
	factory = gtk_signal_list_item_factory_new();
	g_signal_connect(factory, "setup", G_CALLBACK(setup_list_field_listitem_cb),
	                 field);
	g_signal_connect(factory, "bind", G_CALLBACK(bind_list_field_listitem_cb),
	                 field);

	/* Create the list view */
	listview = gtk_list_view_new(sel, factory);

	if (has_icons) {
		gtk_widget_set_size_request(listview, 200, 400);
	}

	for(index = 0, l = purple_request_field_list_get_items(field);
	    l != NULL;
	    index++, l = l->next)
	{
		PurpleKeyValuePair *item = l->data;
		const char *text = (const char *)item->key;
		GObject *wrapper = NULL;

		wrapper = g_object_new(G_TYPE_OBJECT, NULL);
		g_list_store_append(store, wrapper);

		g_object_set_data(wrapper, "data",
		                  purple_request_field_list_get_data(field, text));
		g_object_set_data_full(wrapper, "text", g_strdup(text), g_free);

		if(has_icons) {
			const char *icon_path = (const char *)item->value;
			GdkPixbuf* pixbuf = NULL;

			if(icon_path) {
				pixbuf = purple_gdk_pixbuf_new_from_file(icon_path);
			}

			g_object_set_data_full(wrapper, "pixbuf", pixbuf, g_object_unref);
		}

		if(purple_request_field_list_is_selected(field, text)) {
			gtk_selection_model_select_item(sel, index, FALSE);
		}

		g_object_unref(wrapper);
	}

	/*
	 * We only want to catch changes made by the user, so it's important
	 * that we wait until after the list is created to connect this
	 * handler.  If we connect the handler before the loop above and
	 * there are multiple items selected, then selecting the first item
	 * in the view causes list_field_select_changed_cb to be triggered
	 * which clears out the rest of the list of selected items.
	 */
	g_signal_connect(G_OBJECT(sel), "selection-changed",
	                 G_CALLBACK(list_field_select_changed_cb), field);

	sw = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), listview);
	return sw;
}

static GdkPixbuf*
_pidgin_datasheet_stock_icon_get(const gchar *stock_name)
{
	GdkPixbuf *image = NULL;

	if (stock_name == NULL)
		return NULL;

	/* core is quitting */
	if (datasheet_stock == NULL)
		return NULL;

	if (g_hash_table_lookup_extended(datasheet_stock, stock_name,
		NULL, (gpointer*)&image))
	{
		return image;
	}

	purple_debug_error("gtkrequest", "Unknown icon: %s", stock_name);

	return NULL;
}

static PurpleRequestDatasheetRecord*
datasheet_get_selected_row(GtkWidget *sheet_widget)
{
	PurpleRequestDatasheet *sheet;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *sel_list;
	gpointer key;

	g_return_val_if_fail(sheet_widget != NULL, NULL);

	view = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(sheet_widget), "view"));
	sheet = g_object_get_data(G_OBJECT(sheet_widget), "sheet");

	g_return_val_if_fail(view != NULL, NULL);
	g_return_val_if_fail(sheet != NULL, NULL);

	selection = gtk_tree_view_get_selection(view);
	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return NULL;

	sel_list = gtk_tree_selection_get_selected_rows(selection, &model);
	gtk_tree_model_get_iter(model, &iter, sel_list->data);
	g_list_free_full(sel_list, (GDestroyNotify)gtk_tree_path_free);

	gtk_tree_model_get(model, &iter, 0, &key, -1);

	return purple_request_datasheet_record_find(sheet, key);
}

#if 0
static void
datasheet_button_check_sens(GtkWidget *button, gpointer _sheet_widget)
{
	PurpleRequestDatasheetAction *act;
	GtkWidget *sheet_widget = GTK_WIDGET(_sheet_widget);

	g_return_if_fail(sheet_widget != NULL);

	act = g_object_get_data(G_OBJECT(button), "action");

	g_return_if_fail(act != NULL);

	gtk_widget_set_sensitive(button,
		purple_request_datasheet_action_is_sensitive(act,
			datasheet_get_selected_row(sheet_widget)));
}
#endif

static void
datasheet_selection_changed(G_GNUC_UNUSED GtkWidget *sheet_widget)
{
#if 0
	gpointer buttons_box;

	g_return_if_fail(sheet_widget != NULL);

	buttons_box = g_object_get_data(G_OBJECT(sheet_widget), "buttons");
	gtk_container_foreach(GTK_CONTAINER(buttons_box),
		datasheet_button_check_sens, sheet_widget);
#endif
}

static void
datasheet_update_rec(PurpleRequestDatasheetRecord *rec, GtkListStore *model,
	GtkTreeIter *iter)
{
	guint i, col_count;
	PurpleRequestDatasheet *sheet;

	g_return_if_fail(rec != NULL);
	g_return_if_fail(model != NULL);
	g_return_if_fail(iter != NULL);

	sheet = purple_request_datasheet_record_get_datasheet(rec);

	g_return_if_fail(sheet != NULL);

	col_count = purple_request_datasheet_get_column_count(sheet);

	for (i = 0; i < col_count; i++) {
		PurpleRequestDatasheetColumnType type;

		type = purple_request_datasheet_get_column_type(
			sheet, i);
		if (type == PURPLE_REQUEST_DATASHEET_COLUMN_STRING) {
			GValue val;

			val.g_type = 0;
			g_value_init(&val, G_TYPE_STRING);
			g_value_set_string(&val,
				purple_request_datasheet_record_get_string_data(
					rec, i));
			gtk_list_store_set_value(model, iter,
				i + 1, &val);
		} else if (type ==
			PURPLE_REQUEST_DATASHEET_COLUMN_IMAGE)
		{
			GdkPixbuf *pixbuf;

			pixbuf = _pidgin_datasheet_stock_icon_get(
				purple_request_datasheet_record_get_image_data(
					rec, i));
			gtk_list_store_set(model, iter, i + 1,
				pixbuf, -1);
		} else
			g_warn_if_reached();
	}
}

static void
datasheet_fill(PurpleRequestDatasheet *sheet, GtkListStore *model)
{
	const GList *it;

	gtk_list_store_clear(model);

	it = purple_request_datasheet_get_records(sheet);
	for (; it != NULL; it = g_list_next(it)) {
		PurpleRequestDatasheetRecord *rec = it->data;
		GtkTreeIter iter;

		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter, 0,
			purple_request_datasheet_record_get_key(rec), -1);

		datasheet_update_rec(rec, model, &iter);
	}

	datasheet_selection_changed(GTK_WIDGET(g_object_get_data(
		G_OBJECT(model), "sheet-widget")));
}

static void
datasheet_update(PurpleRequestDatasheet *sheet, gpointer key,
	GtkListStore *model)
{
	PurpleRequestDatasheetRecord *rec;
	GtkTreeIter iter;
	GtkTreeModel *tmodel = GTK_TREE_MODEL(model);
	gboolean found = FALSE;

	g_return_if_fail(tmodel != NULL);

	if (key == NULL) {
		datasheet_fill(sheet, model);
		return;
	}

	rec = purple_request_datasheet_record_find(sheet, key);

	if (gtk_tree_model_get_iter_first(tmodel, &iter)) {
		do {
			gpointer ikey;

			gtk_tree_model_get(tmodel, &iter, 0, &ikey, -1);

			if (key == ikey) {
				found = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next(tmodel, &iter));
	}

	if (rec == NULL && !found)
		return;

	if (rec == NULL) {
		gtk_list_store_remove(model, &iter);
		return;
	}

	if (!found) {
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter, 0, key, -1);
	}

	datasheet_update_rec(rec, model, &iter);

	datasheet_selection_changed(GTK_WIDGET(g_object_get_data(
		G_OBJECT(model), "sheet-widget")));
}


static void
datasheet_selection_changed_cb(G_GNUC_UNUSED GtkTreeSelection *sel,
                               gpointer sheet_widget)
{
	datasheet_selection_changed(GTK_WIDGET(sheet_widget));
}

static void
datasheet_action_clicked(GtkButton *btn, PurpleRequestDatasheetAction *act)
{
	GtkWidget *sheet_widget;

	sheet_widget = g_object_get_data(G_OBJECT(btn), "sheet-widget");

	g_return_if_fail(sheet_widget != NULL);

	purple_request_datasheet_action_call(act, datasheet_get_selected_row(
		sheet_widget));
}

static GtkWidget *
create_datasheet_field(PurpleRequestField *field, GtkSizeGroup *buttons_sg)
{
	PurpleRequestDatasheet *sheet;
	guint i, col_count;
	GType *col_types;
	GtkListStore *model;
	GtkTreeView *view;
	GtkTreeSelection *sel;
	GtkWidget *scrollable;
	GtkCellRenderer *renderer_image = NULL, *renderer_text = NULL;
	GtkTreeViewColumn *id_column;
	GtkWidget *main_box;
	GtkWidget *buttons_box;
	const GList *it;

	sheet = purple_request_field_datasheet_get_sheet(field);
	main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	col_count = purple_request_datasheet_get_column_count(sheet);

	col_types = g_new0(GType, col_count + 1);
	col_types[0] = G_TYPE_POINTER;
	for (i = 0; i < col_count; i++) {
		PurpleRequestDatasheetColumnType type;
		type = purple_request_datasheet_get_column_type(sheet, i);
		if (type == PURPLE_REQUEST_DATASHEET_COLUMN_STRING)
			col_types[i + 1] = G_TYPE_STRING;
		else if (type == PURPLE_REQUEST_DATASHEET_COLUMN_IMAGE)
			col_types[i + 1] = GDK_TYPE_PIXBUF;
		else
			g_warn_if_reached();
	}
	model = gtk_list_store_newv(col_count + 1, col_types);
	g_free(col_types);

	view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(
		GTK_TREE_MODEL(model)));
	g_object_set_data(G_OBJECT(model), "sheet-widget", main_box);
	g_object_unref(G_OBJECT(model));

	id_column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_visible(id_column, FALSE);
	gtk_tree_view_append_column(view, id_column);

	for (i = 0; i < col_count; i++) {
		PurpleRequestDatasheetColumnType type;
		const gchar *title;
		GtkCellRenderer *renderer = NULL;
		const gchar *type_str = "";

		type = purple_request_datasheet_get_column_type(sheet, i);
		title = purple_request_datasheet_get_column_title(sheet, i);

		if (type == PURPLE_REQUEST_DATASHEET_COLUMN_STRING) {
			type_str = "text";
			if (!renderer_text)
				renderer_text = gtk_cell_renderer_text_new();
			renderer = renderer_text;
		}
		else if (type == PURPLE_REQUEST_DATASHEET_COLUMN_IMAGE) {
			type_str = "pixbuf";
			if (!renderer_image)
				renderer_image = gtk_cell_renderer_pixbuf_new();
			renderer = renderer_image;
		} else
			g_warn_if_reached();

		if (title == NULL)
			title = "";
		gtk_tree_view_insert_column_with_attributes(
			view, -1, title, renderer, type_str,
			i + 1, NULL);
	}

	gtk_widget_set_size_request(GTK_WIDGET(view), 400, 250);

	scrollable = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable),
	                              GTK_WIDGET(view));
	gtk_widget_set_hexpand(scrollable, TRUE);
	gtk_box_append(GTK_BOX(main_box), scrollable);

	buttons_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_size_group_add_widget(buttons_sg, buttons_box);
	gtk_box_append(GTK_BOX(main_box), buttons_box);

	it = purple_request_datasheet_get_actions(sheet);
	for (; it != NULL; it = g_list_next(it)) {
		PurpleRequestDatasheetAction *act = it->data;
		GtkButton *btn;
		const gchar *label;

		label = purple_request_datasheet_action_get_label(act);

		btn = GTK_BUTTON(gtk_button_new_with_label(label ? label : ""));

		g_object_set_data(G_OBJECT(btn), "action", act);
		g_object_set_data(G_OBJECT(btn), "sheet-widget", main_box);
		g_signal_connect(G_OBJECT(btn), "clicked",
			G_CALLBACK(datasheet_action_clicked), act);

		gtk_box_append(GTK_BOX(buttons_box), GTK_WIDGET(btn));
	}

	g_object_set_data(G_OBJECT(main_box), "view", view);
	g_object_set_data(G_OBJECT(main_box), "buttons", buttons_box);
	g_object_set_data(G_OBJECT(main_box), "sheet", sheet);

	datasheet_fill(sheet, model);
	purple_signal_connect(sheet, "record-changed",
		pidgin_request_get_handle(),
		G_CALLBACK(datasheet_update), model);

	sel = gtk_tree_view_get_selection(view);
	g_signal_connect(G_OBJECT(sel), "changed",
		G_CALLBACK(datasheet_selection_changed_cb), main_box);

	return main_box;
}

static void *
pidgin_request_fields(const char *title, const char *primary,
	const char *secondary, PurpleRequestFields *fields, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data)
{
	PidginRequestData *data;
	GtkWidget *win;
	GtkWidget *hbox, *vbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *grid;
	GtkWidget *button;
	GtkWidget *img;
	GtkWidget *content;
	GtkSizeGroup *sg, *datasheet_buttons_sg;
	GList *gl, *fl;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;
	char *label_text;
	char *primary_esc, *secondary_esc;
	const gboolean compact = purple_request_cpar_is_compact(cpar);
	GSList *extra_actions;
	size_t i;
	gboolean ok_btn = (ok_text != NULL);

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_FIELDS;
	data->user_data = user_data;
	data->u.multifield.fields = fields;

	purple_request_fields_set_ui_data(fields, data);

	extra_actions = purple_request_cpar_get_extra_actions(cpar);

	data->cb_count = 2;
	data->cbs = g_new0(GCallback, 2);

	data->cbs[0] = ok_cb;
	data->cbs[1] = cancel_cb;

	data->dialog = win = gtk_dialog_new();
	if(title != NULL) {
		gtk_window_set_title(GTK_WINDOW(win), title);
	} else {
		gtk_window_set_title(GTK_WINDOW(win), PIDGIN_ALERT_TITLE);
	}
	gtk_window_set_resizable(GTK_WINDOW(win), TRUE);

	g_signal_connect(G_OBJECT(win), "close-request",
					 G_CALLBACK(destroy_multifield_cb), data);

	/* Setup the main horizontal box */
	content = gtk_dialog_get_content_area(GTK_DIALOG(win));
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_box_append(GTK_BOX(content), hbox);

	/* Dialog icon. */
	img = pidgin_request_dialog_icon(PURPLE_REQUEST_FIELDS, cpar);
	gtk_widget_set_halign(img, GTK_ALIGN_START);
	gtk_widget_set_valign(img, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(hbox), img);

	pidgin_request_add_help(GTK_DIALOG(win), cpar);

	i = 0;
	for (GSList *it = extra_actions; it != NULL; it = it->next) {
		PurpleKeyValuePair *extra_action = it->data;

		button = gtk_dialog_add_button(GTK_DIALOG(win), extra_action->key, i++);
		g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(multifield_extra_cb), data);
		g_object_set_data(G_OBJECT(button), "extra-cb", extra_action->value);
	}

	/* Cancel button */
	button = gtk_dialog_add_button(GTK_DIALOG(win), cancel_text, GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(multifield_cancel_cb), data);

	/* OK button */
	if (!ok_btn) {
		gtk_window_set_default_widget(GTK_WINDOW(win), button);
	} else {
		button = gtk_dialog_add_button(GTK_DIALOG(win), ok_text, GTK_RESPONSE_OK);
		g_signal_connect(G_OBJECT(button), "clicked",
				G_CALLBACK(multifield_ok_cb), data);
		data->ok_button = button;
		gtk_window_set_default_widget(GTK_WINDOW(win), button);
	}

	pidgin_widget_decorate_account(hbox,
		purple_request_cpar_get_account(cpar));

	/* Setup the vbox */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_hexpand(vbox, TRUE);
	gtk_box_append(GTK_BOX(hbox), vbox);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	datasheet_buttons_sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	if(primary) {
		primary_esc = pidgin_request_escape(cpar, primary);
		label_text = g_strdup_printf(
				"<span weight=\"bold\" size=\"larger\">%s</span>", primary_esc);
		g_free(primary_esc);
		label = gtk_label_new(NULL);

		gtk_label_set_markup(GTK_LABEL(label), label_text);
		gtk_label_set_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_xalign(GTK_LABEL(label), 0);
		gtk_label_set_yalign(GTK_LABEL(label), 0);
		gtk_box_append(GTK_BOX(vbox), label);
		g_free(label_text);
	}

	if (secondary) {
		secondary_esc = pidgin_request_escape(cpar, secondary);
		label = gtk_label_new(NULL);

		gtk_label_set_markup(GTK_LABEL(label), secondary_esc);
		g_free(secondary_esc);
		gtk_label_set_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_xalign(GTK_LABEL(label), 0);
		gtk_label_set_yalign(GTK_LABEL(label), 0);
		gtk_box_append(GTK_BOX(vbox), label);
	}

	for (gl = purple_request_fields_get_groups(fields);
		 gl != NULL;
		 gl = gl->next)
	{
		GList *field_list;
		size_t field_count = 0;
		size_t cols = 1;
		size_t rows;
		size_t row_num = 0;
		gboolean contains_resizable = FALSE;

		group      = gl->data;
		field_list = purple_request_field_group_get_fields(group);

		if(purple_request_field_group_get_title(group) != NULL) {
			frame = pidgin_make_frame(vbox,
				purple_request_field_group_get_title(group));
		} else {
			frame = vbox;
		}

		field_count = g_list_length(field_list);
		rows = field_count;

		for (fl = field_list; fl != NULL; fl = fl->next)
		{
			PurpleRequestFieldType type;

			field = (PurpleRequestField *)fl->data;

			type = purple_request_field_get_field_type(field);

			if (type == PURPLE_REQUEST_FIELD_DATASHEET)
				contains_resizable = TRUE;

			if (type == PURPLE_REQUEST_FIELD_LABEL)
			{
				rows++;
			}
			else if ((type == PURPLE_REQUEST_FIELD_LIST) ||
				 (type == PURPLE_REQUEST_FIELD_STRING &&
				  purple_request_field_string_is_multiline(field)))
			{
				rows += 2;
			} else if (compact && type != PURPLE_REQUEST_FIELD_BOOLEAN)
				rows++;
		}

		grid = gtk_grid_new();
		gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
		gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

		/* This box could be setup in a number of ways, so just set all of the
		 * fill and expand properties instead of only setting the
		 * minimums.
		 */
		g_object_set(G_OBJECT(grid),
		             "hexpand", contains_resizable,
		             "vexpand", contains_resizable,
		             NULL);

		gtk_box_append(GTK_BOX(frame), grid);

		for (row_num = 0, fl = field_list;
			 row_num < rows && fl != NULL;
			 row_num++)
		{
			gboolean dummy_counter = TRUE;
			/* it's the same as loop above */
			for (; dummy_counter && fl != NULL; dummy_counter = FALSE, fl = fl->next)
			{
				size_t col_offset = 0;
				PurpleRequestFieldType type;
				GtkWidget *widget = NULL;
				gchar *field_label;

				label = NULL;
				field = fl->data;

				if (!purple_request_field_is_visible(field)) {
					continue;
				}

				type = purple_request_field_get_field_type(field);
				field_label = pidgin_request_escape(cpar,
					purple_request_field_get_label(field));

				if (type != PURPLE_REQUEST_FIELD_BOOLEAN && field_label)
				{
					char *text = NULL;

					if (field_label[strlen(field_label) - 1] != ':' &&
						field_label[strlen(field_label) - 1] != '?' &&
						type != PURPLE_REQUEST_FIELD_LABEL)
					{
						text = g_strdup_printf("%s:", field_label);
					}

					label = gtk_label_new(NULL);
					gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), text ? text : field_label);
					g_free(text);

					gtk_widget_set_hexpand(label, TRUE);
					gtk_widget_set_vexpand(label, TRUE);
					gtk_label_set_xalign(GTK_LABEL(label), 0);

					gtk_size_group_add_widget(sg, label);

					if (type == PURPLE_REQUEST_FIELD_LABEL ||
					    type == PURPLE_REQUEST_FIELD_LIST ||
						(type == PURPLE_REQUEST_FIELD_STRING &&
						 purple_request_field_string_is_multiline(field)))
					{
						gtk_grid_attach(GTK_GRID(grid), label,
							0, row_num, 2 * cols, 1);

						row_num++;
					}
					else
					{
						gtk_grid_attach(GTK_GRID(grid), label,
							col_offset, row_num, 1, 1);
					}

				}
				g_clear_pointer(&field_label, g_free);

				widget = GTK_WIDGET(purple_request_field_get_ui_data(field));
				if (widget == NULL)
				{
					if (type == PURPLE_REQUEST_FIELD_STRING)
						widget = create_string_field(field);
					else if (type == PURPLE_REQUEST_FIELD_INTEGER)
						widget = create_int_field(field);
					else if (type == PURPLE_REQUEST_FIELD_BOOLEAN)
						widget = create_bool_field(field, cpar);
					else if (type == PURPLE_REQUEST_FIELD_CHOICE)
						widget = create_choice_field(field);
					else if (type == PURPLE_REQUEST_FIELD_LIST)
						widget = create_list_field(field);
					else if (type == PURPLE_REQUEST_FIELD_IMAGE)
						widget = create_image_field(field);
					else if (type == PURPLE_REQUEST_FIELD_ACCOUNT)
						widget = create_account_field(field);
					else if (type == PURPLE_REQUEST_FIELD_DATASHEET)
						widget = create_datasheet_field(field, datasheet_buttons_sg);
					else
						continue;
				}

				gtk_widget_set_sensitive(widget,
					purple_request_field_is_sensitive(field));

				if (label)
					gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);

				gtk_widget_set_hexpand(widget, TRUE);
				gtk_widget_set_vexpand(widget, TRUE);
				gtk_widget_set_margin_start(widget, 5);
				gtk_widget_set_margin_end(widget, 5);

				if (type == PURPLE_REQUEST_FIELD_STRING &&
					purple_request_field_string_is_multiline(field))
				{
					gtk_grid_attach(GTK_GRID(grid), widget,
						0, row_num, 2 * cols, 1);
				}
				else if (type == PURPLE_REQUEST_FIELD_LIST)
				{
					gtk_grid_attach(GTK_GRID(grid), widget,
						0, row_num, 2 * cols, 1);
				}
				else if (type == PURPLE_REQUEST_FIELD_BOOLEAN)
				{
					gtk_grid_attach(GTK_GRID(grid), widget,
						col_offset, row_num, 1, 1);
				}
				else if (compact) {
					row_num++;
					gtk_grid_attach(GTK_GRID(grid), widget,
						0, row_num, 2 * cols, 1);
				} else {
					gtk_grid_attach(GTK_GRID(grid), widget,
						1, row_num, 2 * cols - 1, 1);
				}

				purple_request_field_set_ui_data(field, widget);
			}
		}
	}

	g_object_unref(sg);
	g_object_unref(datasheet_buttons_sg);

	if (!purple_request_fields_all_required_filled(fields))
		gtk_widget_set_sensitive(data->ok_button, FALSE);

	if (!purple_request_fields_all_valid(fields))
		gtk_widget_set_sensitive(data->ok_button, FALSE);

	pidgin_auto_parent_window(win);

	gtk_widget_show(win);

	return data;
}

static void
pidgin_request_file_folder_response_cb(G_GNUC_UNUSED GtkWidget *widget,
                                       gint response, PidginRequestData *data)
{
	GFile *current_path;

	if (response != GTK_RESPONSE_ACCEPT) {
		if (data->cbs[0] != NULL)
			((PurpleRequestFileCb)data->cbs[0])(data->user_data, NULL);
		purple_request_close(data->type, data);
		return;
	}

	current_path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(data->dialog));
	if (current_path != NULL) {
		gchar *current_folder = g_file_get_path(current_path);
		if (data->u.file.savedialog) {
			purple_prefs_set_path(PIDGIN_PREFS_ROOT "/filelocations/last_save_folder", current_folder);
		} else {
			purple_prefs_set_path(PIDGIN_PREFS_ROOT "/filelocations/last_open_folder", current_folder);
		}
		g_free(current_folder);
	}
	if (data->cbs[1] != NULL) {
		GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(data->dialog));
		char *filename = g_file_get_path(file);
		((PurpleRequestFileCb)data->cbs[1])(data->user_data, filename);
		g_free(filename);
		g_object_unref(file);
	}
	purple_request_close(data->type, data);

	g_clear_object(&current_path);
}

static void *
pidgin_request_file(const char *title, const char *filename,
                    gboolean savedialog, GCallback ok_cb, GCallback cancel_cb,
                    G_GNUC_UNUSED PurpleRequestCommonParameters *cpar,
                    gpointer user_data)
{
	PidginRequestData *data;
	GtkFileChooserNative *filesel;
#ifdef _WIN32
	GFile *file = NULL;
	const gchar *current_folder;
	gboolean folder_set = FALSE;
#endif

	data = g_new0(PidginRequestData, 1);
	data->type = PURPLE_REQUEST_FILE;
	data->user_data = user_data;
	data->cb_count = 2;
	data->cbs = g_new0(GCallback, 2);
	data->cbs[0] = cancel_cb;
	data->cbs[1] = ok_cb;
	data->u.file.savedialog = savedialog;

	filesel = gtk_file_chooser_native_new(
	        title ? title
	              : (savedialog ? _("Save File...") : _("Open File...")),
	        NULL,
	        savedialog ? GTK_FILE_CHOOSER_ACTION_SAVE
	                   : GTK_FILE_CHOOSER_ACTION_OPEN,
	        savedialog ? _("_Save") : _("_Open"), _("_Cancel"));

	if ((filename != NULL) && (*filename != '\0')) {
		GFile *path = g_file_new_for_path(filename);

		if(savedialog) {
			gtk_file_chooser_set_file(GTK_FILE_CHOOSER(filesel), path, NULL);
		} else if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			gtk_file_chooser_set_file(GTK_FILE_CHOOSER(filesel), path, NULL);
		}

		g_object_unref(path);
	}

#ifdef _WIN32
	if (savedialog) {
		current_folder = purple_prefs_get_path(PIDGIN_PREFS_ROOT "/filelocations/last_save_folder");
	} else {
		current_folder = purple_prefs_get_path(PIDGIN_PREFS_ROOT "/filelocations/last_open_folder");
	}

	if ((filename == NULL || *filename == '\0' || !g_file_test(filename, G_FILE_TEST_EXISTS)) &&
		(current_folder != NULL) && (*current_folder != '\0'))
	{
		file = g_file_new_for_path(current_folder);
		folder_set = gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filesel), file, NULL);
	}

	if (!folder_set && (filename == NULL || *filename == '\0' || !g_file_test(filename, G_FILE_TEST_EXISTS))) {
		char *my_documents = wpurple_get_special_folder(CSIDL_PERSONAL);

		g_clear_object(&file);

		if (my_documents != NULL) {
			file = g_file_new_for_path(my_documents);
			gtk_file_chooser_set_current_folder(
					GTK_FILE_CHOOSER(filesel), file, NULL);

			g_free(my_documents);
		}
	}

	g_clear_object(&file);
#endif

	g_signal_connect(G_OBJECT(filesel), "response",
	                 G_CALLBACK(pidgin_request_file_folder_response_cb), data);

#if 0
	/* FIXME: Not implemented for native dialogs. */
	pidgin_auto_parent_window(filesel);
#endif

	data->dialog = filesel;
	gtk_native_dialog_show(GTK_NATIVE_DIALOG(filesel));

	return (void *)data;
}

static void *
pidgin_request_folder(const char *title, const char *dirname, GCallback ok_cb,
                      GCallback cancel_cb,
                      G_GNUC_UNUSED PurpleRequestCommonParameters *cpar,
                      gpointer user_data)
{
	PidginRequestData *data;
	GtkFileChooserNative *dirsel;

	data = g_new0(PidginRequestData, 1);
	data->type = PURPLE_REQUEST_FOLDER;
	data->user_data = user_data;
	data->cb_count = 2;
	data->cbs = g_new0(GCallback, 2);
	data->cbs[0] = cancel_cb;
	data->cbs[1] = ok_cb;
	data->u.file.savedialog = FALSE;

	dirsel = gtk_file_chooser_native_new(
	        title ? title : _("Select Folder..."), NULL,
	        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_OK"), _("_Cancel"));

	if ((dirname != NULL) && (*dirname != '\0')) {
		GFile *path = g_file_new_for_path(dirname);

		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dirsel), path,
		                                    NULL);

		g_object_unref(path);
	}

	g_signal_connect(G_OBJECT(dirsel), "response",
	                 G_CALLBACK(pidgin_request_file_folder_response_cb), data);

	data->dialog = dirsel;
#if 0
	/* FIXME: Not implemented for native dialogs. */
	pidgin_auto_parent_window(dirsel);
#endif

	gtk_native_dialog_show(GTK_NATIVE_DIALOG(dirsel));

	return (void *)data;
}

/* if request callback issues another request, it should be attached to the
 * primary request parent */
static void
pidgin_window_detach_children(GtkWindow* win)
{
	GList *it;
	GtkWindow *par;

	g_return_if_fail(win != NULL);

	par = gtk_window_get_transient_for(win);
	it = gtk_window_list_toplevels();
	for (it = g_list_first(it); it != NULL; it = g_list_delete_link(it, it)) {
		GtkWindow *child = GTK_WINDOW(it->data);
		if (gtk_window_get_transient_for(child) != win)
			continue;
		if (gtk_window_get_destroy_with_parent(child)) {
#ifdef _WIN32
			/* XXX test/verify it: Win32 gtk ignores
			 * gtk_window_set_destroy_with_parent(..., FALSE). */
			gtk_window_set_transient_for(child, NULL);
#endif
			continue;
		}
		gtk_window_set_transient_for(child, par);
	}
}

static void
pidgin_close_request(PurpleRequestType type, void *ui_handle)
{
	PidginRequestData *data = (PidginRequestData *)ui_handle;

	g_free(data->cbs);

	if (type == PURPLE_REQUEST_FILE || type == PURPLE_REQUEST_FOLDER) {
		/* Will be a GtkNativeDialog, not GtkDialog. */
		g_object_unref(data->dialog);
	} else {
		pidgin_window_detach_children(GTK_WINDOW(data->dialog));

		gtk_window_destroy(GTK_WINDOW(data->dialog));
	}

	if (type == PURPLE_REQUEST_FIELDS)
		purple_request_fields_destroy(data->u.multifield.fields);

	g_free(data);
}

GtkWindow *
pidgin_request_get_dialog_window(void *ui_handle)
{
	PidginRequestData *data = ui_handle;

	g_return_val_if_fail(
		purple_request_is_valid_ui_handle(data, NULL), NULL);

	if (data->type == PURPLE_REQUEST_FILE ||
	    data->type == PURPLE_REQUEST_FOLDER) {
		/* Not a GtkWidget, but a GtkFileChooserNative. Eventually this function
		 * should not be needed, once we don't need to auto-parent. */
		return NULL;
	}

	return GTK_WINDOW(data->dialog);
}

static PurpleRequestUiOps ops = {
	.features = PURPLE_REQUEST_FEATURE_HTML,
	.request_input = pidgin_request_input,
	.request_choice = pidgin_request_choice,
	.request_action = pidgin_request_action,
	.request_wait = pidgin_request_wait,
	.request_wait_update = pidgin_request_wait_update,
	.request_fields = pidgin_request_fields,
	.request_file = pidgin_request_file,
	.request_folder = pidgin_request_folder,
	.close_request = pidgin_close_request,
};

PurpleRequestUiOps *
pidgin_request_get_ui_ops(void)
{
	return &ops;
}

void *
pidgin_request_get_handle(void)
{
	static int handle;

	return &handle;
}

static void
pidgin_request_datasheet_stock_remove(gpointer obj)
{
	if (obj == NULL)
		return;
	g_object_unref(obj);
}

void
pidgin_request_init(void)
{
	datasheet_stock = g_hash_table_new_full(g_str_hash, g_str_equal,
		g_free, pidgin_request_datasheet_stock_remove);
}

void
pidgin_request_uninit(void)
{
	purple_signals_disconnect_by_handle(pidgin_request_get_handle());
	g_hash_table_destroy(datasheet_stock);
	datasheet_stock = NULL;
}
