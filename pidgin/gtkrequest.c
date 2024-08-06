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

#include <purple.h>

#include "gtkrequest.h"
#include "pidginaccountdisplay.h"
#include "pidginaccountfilterconnected.h"
#include "pidginaccountrow.h"
#include "pidgincore.h"
#include "pidgintextbuffer.h"

#include <gdk/gdkkeysyms.h>

typedef struct
{
	PurpleRequestType type;

	void *user_data;
	/* May be GtkWidget or GtkFileDialog. */
	gpointer dialog;
	GCancellable *cancellable;

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
			PurpleRequestPage *page;

		} multifield;

		struct
		{
			gboolean savedialog;

		} file;

	} u;

} PidginRequestData;

static void
pidgin_widget_decorate_account(GtkWidget *cont, PurpleAccount *account)
{
	GtkWidget *display = NULL;

	if(!PURPLE_IS_ACCOUNT(account)) {
		return;
	}

	if(!GTK_IS_BOX(cont)) {
		return;
	}

	display = pidgin_account_display_new(account);
	gtk_widget_set_halign(display, GTK_ALIGN_CENTER);
	gtk_box_append(GTK_BOX(cont), display);
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
field_choice_option_cb(GObject *obj, G_GNUC_UNUSED GParamSpec *pspec,
                       gpointer data)
{
	PurpleRequestField *field = data;
	GtkDropDown *dropdown = GTK_DROP_DOWN(obj);
	GObject *item = gtk_drop_down_get_selected_item(dropdown);

	if(G_IS_OBJECT(item)) {
		gpointer value = g_object_get_data(item, "choice_value");
		purple_request_field_choice_set_value(PURPLE_REQUEST_FIELD_CHOICE(field),
		                                      value);
	}
}

static void
field_account_cb(GObject *obj, G_GNUC_UNUSED GParamSpec *pspec, gpointer data)
{
	PurpleRequestField *field = data;
	PidginAccountRow *row = PIDGIN_ACCOUNT_ROW(obj);

	purple_request_field_account_set_value(
	        PURPLE_REQUEST_FIELD_ACCOUNT(field),
	        pidgin_account_row_get_account(row));
}

static void
multifield_response_cb(G_GNUC_UNUSED GtkDialog *dialog, gint response,
                       gpointer data)
{
	PidginRequestData *req_data = data;
	PurpleRequestFieldsCb cb = NULL;

	generic_response_start(req_data);

	if(response == GTK_RESPONSE_OK) {
		cb = (PurpleRequestFieldsCb)req_data->cbs[0];
	} else if(response == GTK_RESPONSE_CANCEL ||
	          response == GTK_RESPONSE_DELETE_EVENT)
	{
		cb = (PurpleRequestFieldsCb)req_data->cbs[1];
	} else if(2 <= response && (gsize)response < req_data->cb_count) {
		cb = (PurpleRequestFieldsCb)req_data->cbs[response];
	}

	if(cb != NULL) {
		cb(req_data->user_data, req_data->u.multifield.page);
	}

	purple_request_close(PURPLE_REQUEST_FIELDS, data);
}

static GtkWidget *
pidgin_request_dialog_icon(PurpleRequestType dialog_type,
	PurpleRequestCommonParameters *cpar)
{
	GtkWidget *img = NULL;
	PurpleRequestIconType icon_type;
	gconstpointer icon_data;
	gsize icon_size;
	const char *icon_name = NULL;

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

static GtkWidget *
create_label_field(void) {
	GtkWidget *row = NULL;
	GtkWidget *label = NULL;

	row = adw_preferences_row_new();
	gtk_widget_set_focusable(row, FALSE);
	gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);

	label = gtk_label_new(NULL);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_bottom(label, 12);
	gtk_widget_set_margin_top(label, 12);
	g_object_bind_property(row, "title", label, "label", G_BINDING_DEFAULT);
	g_object_bind_property(row, "use-markup", label, "use-markup",
	                       G_BINDING_DEFAULT);
	g_object_bind_property(row, "use-underline", label, "use-underline",
	                       G_BINDING_DEFAULT);
	gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

	return row;
}

static void
multiline_state_flags_changed_cb(GtkWidget *self, GtkStateFlags flags,
                                 gpointer data)
{
	GtkWidget *row = data;
	gboolean before = FALSE, after = FALSE;

	before = !!(flags & GTK_STATE_FLAG_FOCUS_WITHIN);
	after = !!(gtk_widget_get_state_flags(self) & GTK_STATE_FLAG_FOCUS_WITHIN);
	if(before != after) {
		if(after) {
			gtk_widget_add_css_class(row, "focused");
		} else {
			gtk_widget_remove_css_class(row, "focused");
		}
	}
}

static GtkWidget *
create_string_field(PurpleRequestField *field,
                    G_GNUC_UNUSED PurpleKeyValuePair **hinted_widget)
{
	PurpleRequestFieldString *strfield = PURPLE_REQUEST_FIELD_STRING(field);
	const char *value;
	GtkWidget *row;

	value = purple_request_field_string_get_default_value(strfield);

	if(purple_request_field_string_is_multiline(strfield)) {
		GtkWidget *vbox = NULL;
		GtkWidget *title = NULL;
		GtkWidget *textview = NULL;
		GtkWidget *sw = NULL;
		GtkTextBuffer *buffer = NULL;

		vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
		gtk_widget_set_margin_top(vbox, 6);

		title = gtk_label_new(NULL);
		gtk_widget_set_halign(title, GTK_ALIGN_START);
		gtk_widget_set_margin_start(title, 12);
		gtk_widget_set_margin_end(title, 12);
		gtk_widget_add_css_class(title, "subtitle");
		gtk_box_append(GTK_BOX(vbox), title);

		textview = gtk_text_view_new();
		gtk_widget_set_margin_start(textview, 12);
		gtk_widget_set_margin_end(textview, 12);
		gtk_widget_remove_css_class(textview, "view");
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview),
									GTK_WRAP_WORD_CHAR);

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		if(value != NULL) {
			gtk_text_buffer_set_text(buffer, value, -1);
		}
		g_object_bind_property(field, "value", buffer, "text",
		                       G_BINDING_BIDIRECTIONAL);

		sw = gtk_scrolled_window_new();
		gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), textview);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
		                               GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
		gtk_widget_set_size_request(sw, -1, 75);
		gtk_widget_set_hexpand(sw, TRUE);
		gtk_widget_set_vexpand(sw, TRUE);
		gtk_box_append(GTK_BOX(vbox), sw);

		row = adw_preferences_row_new();
		g_object_bind_property(row, "title", title, "label",
		                       G_BINDING_DEFAULT);
		g_object_bind_property(row, "use-markup", title, "use-markup",
		                       G_BINDING_DEFAULT);
		g_object_bind_property(row, "use-underline", title, "use-underline",
		                       G_BINDING_DEFAULT);
		gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), vbox);
		gtk_widget_set_focusable(row, FALSE);
		gtk_widget_add_css_class(row, "entry");
		g_signal_connect(textview, "state-flags-changed",
		                 G_CALLBACK(multiline_state_flags_changed_cb), row);

	} else {
		if(purple_request_field_string_is_masked(strfield)) {
			row = adw_password_entry_row_new();
		} else {
			row = adw_entry_row_new();
		}

		if(value != NULL) {
			gtk_editable_set_text(GTK_EDITABLE(row), value);
		}

		g_object_bind_property(field, "value", row, "text",
		                       G_BINDING_BIDIRECTIONAL);
	}

	return row;
}

static GtkWidget *
create_int_field(PurpleRequestField *field,
                 G_GNUC_UNUSED PurpleKeyValuePair **hinted_widget)
{
	PurpleRequestFieldInt *intfield = PURPLE_REQUEST_FIELD_INT(field);
	GtkWidget *widget = NULL;
	GtkWidget *spin = NULL;
	int value;

	spin = gtk_spin_button_new_with_range(
		purple_request_field_int_get_lower_bound(intfield),
		purple_request_field_int_get_upper_bound(intfield), 1);

	value = purple_request_field_int_get_default_value(intfield);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);
	g_object_bind_property(field, "value", spin, "value",
	                       G_BINDING_BIDIRECTIONAL);

	gtk_widget_set_valign(spin, GTK_ALIGN_CENTER);

	widget = adw_action_row_new();
	gtk_widget_set_focusable(widget, FALSE);
	adw_action_row_add_suffix(ADW_ACTION_ROW(widget), spin);
	adw_action_row_set_activatable_widget(ADW_ACTION_ROW(widget), spin);

	return widget;
}

static GtkWidget *
create_bool_field(PurpleRequestField *field) {
	PurpleRequestFieldBool *boolfield = PURPLE_REQUEST_FIELD_BOOL(field);
	GtkWidget *row = NULL;
	GtkWidget *sw = NULL;

	sw = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH(sw),
	                      purple_request_field_bool_get_default_value(boolfield));
	gtk_widget_set_focusable(sw, TRUE);
	gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
	g_object_bind_property(field, "value", sw, "active",
	                       G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

	row = adw_action_row_new();
	gtk_widget_set_focusable(row, FALSE);
	adw_action_row_add_suffix(ADW_ACTION_ROW(row), sw);
	adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), sw);

	return row;
}

static GtkWidget *
create_choice_field(PurpleRequestField *field) {
	PurpleRequestFieldChoice *choicefield = PURPLE_REQUEST_FIELD_CHOICE(field);
	GtkWidget *widget;
	GListModel *model = NULL;
	GList *elements = NULL;
	guint default_index = GTK_INVALID_LIST_POSITION;
	gpointer default_value;
	guint index;

	default_value = purple_request_field_choice_get_value(choicefield);
	widget = gtk_drop_down_new_from_strings(NULL);
	model = gtk_drop_down_get_model(GTK_DROP_DOWN(widget));

	index = 0;
	elements = purple_request_field_choice_get_elements(choicefield);
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

	g_signal_connect(G_OBJECT(widget), "notify::selected",
	                 G_CALLBACK(field_choice_option_cb), field);

	if(default_index == GTK_INVALID_LIST_POSITION && index > 0) {
		GObject *item = g_list_model_get_item(model, 0);
		gpointer value = g_object_get_data(item, "choice_value");
		purple_request_field_choice_set_value(choicefield, value);
		g_clear_object(&item);
	}

	return widget;
}

static GtkWidget *
create_image_field(PurpleRequestField *field)
{
	PurpleRequestFieldImage *ifield = PURPLE_REQUEST_FIELD_IMAGE(field);
	GtkWidget *widget;
	GdkPixbuf *buf, *scale;

	buf = purple_gdk_pixbuf_from_data(
			(const guchar *)purple_request_field_image_get_buffer(ifield),
			purple_request_field_image_get_size(ifield));

	scale = gdk_pixbuf_scale_simple(buf,
			purple_request_field_image_get_scale_x(ifield) * gdk_pixbuf_get_width(buf),
			purple_request_field_image_get_scale_y(ifield) * gdk_pixbuf_get_height(buf),
			GDK_INTERP_BILINEAR);
	widget = gtk_image_new_from_pixbuf(scale);
	g_object_unref(buf);
	g_object_unref(scale);

	return widget;
}

static gboolean
field_custom_account_filter_cb(gpointer item, gpointer data) {
	PurpleRequestFieldAccount *field = data;
	gboolean ret = FALSE;

	if(PURPLE_IS_ACCOUNT(item)) {
		ret = purple_request_field_account_match(field, PURPLE_ACCOUNT(item));
	}

	return ret;
}

static GtkWidget *
create_account_field(PurpleRequestField *field, GtkWidget **account_hint)
{
	PurpleRequestFieldAccount *afield = NULL;
	GtkWidget *widget = NULL;
	PurpleAccount *account = NULL;
	GtkCustomFilter *custom_filter = NULL;
	GtkFilter *filter = NULL;
	const char *type_hint = NULL;

	widget = pidgin_account_row_new();
	afield = PURPLE_REQUEST_FIELD_ACCOUNT(field);
	account = purple_request_field_account_get_default_value(afield);

	custom_filter = gtk_custom_filter_new(field_custom_account_filter_cb,
	                                      afield, NULL);
	filter = GTK_FILTER(custom_filter);

	if(!purple_request_field_account_get_show_all(afield)) {
		GtkEveryFilter *every = NULL;

		every = gtk_every_filter_new();

		if(GTK_IS_FILTER(filter)) {
			gtk_multi_filter_append(GTK_MULTI_FILTER(every), filter);
		}

		filter = pidgin_account_filter_connected_new();
		gtk_multi_filter_append(GTK_MULTI_FILTER(every), filter);

		filter = GTK_FILTER(every);
	}

	pidgin_account_row_set_account(PIDGIN_ACCOUNT_ROW(widget), account);
	g_signal_connect(widget, "notify::account", G_CALLBACK(field_account_cb),
	                 field);

	if(GTK_IS_FILTER(filter)) {
		pidgin_account_row_set_filter(PIDGIN_ACCOUNT_ROW(widget), filter);
		g_object_unref(filter);
	}

	type_hint = purple_request_field_get_type_hint(field);
	if(purple_strequal(type_hint, "account")) {
		*account_hint = widget;
	}

	return widget;
}

static void
setup_list_field_listitem_cb(G_GNUC_UNUSED GtkSignalListItemFactory *self,
                             GtkListItem *item, gpointer data)
{
	PurpleRequestFieldList *field = data;
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
	PurpleRequestFieldList *field = data;
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
	PurpleRequestFieldList *field = data;
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
create_list_field(PurpleRequestField *field) {
	PurpleRequestFieldList *listfield = PURPLE_REQUEST_FIELD_LIST(field);
	GtkWidget *sw;
	GtkWidget *listview = NULL;
	GtkSelectionModel *sel = NULL;
	GtkListItemFactory *factory = NULL;
	GListStore *store = NULL;
	guint index = 0;
	GList *l;
	gboolean has_icons;

	has_icons = purple_request_field_list_has_icons(listfield);

	/* Create the list store */
	store = g_list_store_new(G_TYPE_OBJECT);
	if(purple_request_field_list_get_multi_select(listfield)) {
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

	for(index = 0, l = purple_request_field_list_get_items(listfield);
	    l != NULL;
	    index++, l = l->next)
	{
		PurpleKeyValuePair *item = l->data;
		const char *text = (const char *)item->key;
		GObject *wrapper = NULL;

		wrapper = g_object_new(G_TYPE_OBJECT, NULL);
		g_list_store_append(store, wrapper);

		g_object_set_data(wrapper, "data",
		                  purple_request_field_list_get_data(listfield, text));
		g_object_set_data_full(wrapper, "text", g_strdup(text), g_free);

		if(has_icons) {
			const char *icon_path = (const char *)item->value;
			GdkPixbuf* pixbuf = NULL;

			if(icon_path) {
				pixbuf = purple_gdk_pixbuf_new_from_file(icon_path);
			}

			g_object_set_data_full(wrapper, "pixbuf", pixbuf, g_object_unref);
		}

		if(purple_request_field_list_is_selected(listfield, text)) {
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

static void *
pidgin_request_fields(const char *title, const char *primary,
	const char *secondary, PurpleRequestPage *page, const char *ok_text,
	GCallback ok_cb, const char *cancel_text, GCallback cancel_cb,
	PurpleRequestCommonParameters *cpar, void *user_data)
{
	PidginRequestData *data;
	GtkWidget *win;
	GtkWidget *page_widget = NULL;
	AdwPreferencesGroup *group_widget = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *img;
	GtkWidget *content;
	GtkSizeGroup *datasheet_buttons_sg;
	GSList *extra_actions;
	gint response;
	guint n_groups;

	data            = g_new0(PidginRequestData, 1);
	data->type      = PURPLE_REQUEST_FIELDS;
	data->user_data = user_data;
	data->u.multifield.page = page;

	data->dialog = win = gtk_dialog_new();
	if(title != NULL) {
		gtk_window_set_title(GTK_WINDOW(win), title);
	} else {
		gtk_window_set_title(GTK_WINDOW(win), PIDGIN_ALERT_TITLE);
	}
	gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(win), 480, -1);

	content = gtk_dialog_get_content_area(GTK_DIALOG(win));
	page_widget = adw_preferences_page_new();
	gtk_widget_set_vexpand(page_widget, TRUE);
	gtk_box_append(GTK_BOX(content), page_widget);

	/* Setup the general info box. */
	group_widget = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
	adw_preferences_page_add(ADW_PREFERENCES_PAGE(page_widget), group_widget);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin_bottom(vbox, 12);
	adw_preferences_group_add(group_widget, vbox);

	/* Dialog icon. */
	img = pidgin_request_dialog_icon(PURPLE_REQUEST_FIELDS, cpar);
	if(gtk_image_get_storage_type(GTK_IMAGE(img)) == GTK_IMAGE_ICON_NAME) {
		gtk_image_set_pixel_size(GTK_IMAGE(img), 64);
	}
	gtk_box_append(GTK_BOX(vbox), img);

	pidgin_widget_decorate_account(vbox,
	                               purple_request_cpar_get_account(cpar));

	pidgin_request_add_help(GTK_DIALOG(win), cpar);

	/* Add responses and callbacks. */
	g_signal_connect(data->dialog, "response",
	                 G_CALLBACK(multifield_response_cb), data);
	extra_actions = purple_request_cpar_get_extra_actions(cpar);
	data->cb_count = 2 + g_slist_length(extra_actions);
	data->cbs = g_new0(GCallback, data->cb_count);

	data->cbs[0] = ok_cb;
	data->cbs[1] = cancel_cb;

	response = 2; /* So that response == data->cbs index. */
	for(; extra_actions != NULL; extra_actions = extra_actions->next) {
		PurpleKeyValuePair *extra_action = extra_actions->data;

		gtk_dialog_add_button(GTK_DIALOG(win), extra_action->key, response);
		data->cbs[response] = extra_action->value;
		response++;
	}

	/* Cancel button */
	gtk_dialog_add_button(GTK_DIALOG(win), cancel_text, GTK_RESPONSE_CANCEL);
	response = GTK_RESPONSE_CANCEL;

	/* OK button */
	if(ok_text != NULL) {
		data->ok_button = gtk_dialog_add_button(GTK_DIALOG(win), ok_text,
		                                        GTK_RESPONSE_OK);
		response = GTK_RESPONSE_OK;
	}
	gtk_dialog_set_default_response(GTK_DIALOG(win), response);

	datasheet_buttons_sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	if(primary != NULL) {
		GtkWidget *label = gtk_label_new(primary);
		gtk_label_set_wrap(GTK_LABEL(label), TRUE);
		gtk_widget_add_css_class(label, "title-1");
		adw_preferences_group_add(group_widget, label);
	}

	if(secondary != NULL) {
		GtkWidget *label = gtk_label_new(secondary);
		gtk_label_set_wrap(GTK_LABEL(label), TRUE);
		adw_preferences_group_add(group_widget, label);
	}

	n_groups = g_list_model_get_n_items(G_LIST_MODEL(page));
	for(guint group_index = 0; group_index < n_groups; group_index++) {
		PurpleRequestGroup *group = NULL;
		guint n_fields = 0;
		GSList *username_widgets = NULL;
		GtkWidget *account_hint = NULL;

		group = g_list_model_get_item(G_LIST_MODEL(page), group_index);
		group_widget = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
		adw_preferences_group_set_title(group_widget,
		                                purple_request_group_get_title(group));
		adw_preferences_page_add(ADW_PREFERENCES_PAGE(page_widget),
		                         group_widget);

		n_fields = g_list_model_get_n_items(G_LIST_MODEL(group));
		for(guint field_index = 0; field_index < n_fields; field_index++) {
			PurpleRequestField *field = NULL;
			GtkWidget *widget = NULL;
			gboolean was_handled_by_create = FALSE;

			field = g_list_model_get_item(G_LIST_MODEL(group), field_index);
			if(!purple_request_field_is_visible(field)) {
				g_object_unref(field);
				continue;
			}

			if(PURPLE_IS_REQUEST_FIELD_LABEL(field)) {
				widget = create_label_field();
				was_handled_by_create = TRUE;
			} else if(PURPLE_IS_REQUEST_FIELD_STRING(field)) {
				PurpleKeyValuePair *username_hint = NULL;

				widget = create_string_field(field, &username_hint);
				was_handled_by_create = TRUE;

				if(username_hint != NULL) {
					username_widgets = g_slist_prepend(username_widgets,
					                                   username_hint);
				}
			} else if(PURPLE_IS_REQUEST_FIELD_INT(field)) {
				PurpleKeyValuePair *username_hint = NULL;

				widget = create_int_field(field, &username_hint);
				was_handled_by_create = TRUE;

				if(username_hint != NULL) {
					username_widgets = g_slist_prepend(username_widgets,
					                                   username_hint);
				}
			} else if(PURPLE_IS_REQUEST_FIELD_BOOL(field)) {
				widget = create_bool_field(field);
				was_handled_by_create = TRUE;
			} else if(PURPLE_IS_REQUEST_FIELD_CHOICE(field)) {
				widget = create_choice_field(field);
			} else if(PURPLE_IS_REQUEST_FIELD_LIST(field)) {
				widget = create_list_field(field);
			} else if(PURPLE_IS_REQUEST_FIELD_IMAGE(field)) {
				widget = create_image_field(field);
			} else if(PURPLE_IS_REQUEST_FIELD_ACCOUNT(field)) {
				widget = create_account_field(field, &account_hint);
				was_handled_by_create = TRUE;
			} else {
				g_warning("Unhandled field type: %s",
				          G_OBJECT_TYPE_NAME(field));
				g_object_unref(field);
				continue;
			}

			g_object_bind_property(field, "tooltip", widget, "tooltip-text",
			                       G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
			g_object_bind_property(field, "sensitive", widget, "sensitive",
			                       G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

			if(!was_handled_by_create) {
				GtkWidget *row = NULL;

				gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);

				row = adw_action_row_new();
				adw_action_row_add_suffix(ADW_ACTION_ROW(row), widget);
				widget = row;
			}

			g_object_bind_property(field, "label", widget, "title",
			                       G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
			adw_preferences_row_set_use_underline(ADW_PREFERENCES_ROW(widget),
			                                      TRUE);

			adw_preferences_group_add(group_widget, widget);

			g_object_unref(field);
		}

		g_slist_free_full(username_widgets,
		                  (GDestroyNotify)purple_key_value_pair_free);

		g_object_unref(group);
	}

	g_object_unref(datasheet_buttons_sg);

	g_object_bind_property(page, "valid", data->ok_button, "sensitive", 0);

	gtk_widget_set_visible(win, TRUE);

	return data;
}

GtkWindow *
pidgin_request_get_dialog_window(void *ui_handle)
{
	PidginRequestData *data = ui_handle;

	g_return_val_if_fail(
		purple_request_is_valid_ui_handle(data, NULL), NULL);

	if (data->type == PURPLE_REQUEST_FILE ||
	    data->type == PURPLE_REQUEST_FOLDER) {
		/* Not a GtkWidget, but a GtkFileDialog. Eventually this function
		 * should not be needed, once we don't need to auto-parent. */
		return NULL;
	}

	return GTK_WINDOW(data->dialog);
}

static PurpleRequestUiOps ops = {
	.request_fields = pidgin_request_fields,
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

void
pidgin_request_init(void)
{
}

void
pidgin_request_uninit(void)
{
	purple_signals_disconnect_by_handle(pidgin_request_get_handle());
}
