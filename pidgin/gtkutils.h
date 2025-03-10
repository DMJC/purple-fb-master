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

#ifndef _PIDGINUTILS_H_
#define _PIDGINUTILS_H_
/**
 * SECTION:gtkutils
 * @section_id: pidgin-gtkutils
 * @short_description: <filename>gtkutils.h</filename>
 * @title: Utility functions
 */

#include "gtkconv.h"
#include "pidgin.h"

#include <purple.h>

typedef enum
{
	PIDGIN_BUTTON_HORIZONTAL,
	PIDGIN_BUTTON_VERTICAL

} PidginButtonOrientation;

typedef enum
{
	PIDGIN_BUTTON_NONE = 0,
	PIDGIN_BUTTON_TEXT,
	PIDGIN_BUTTON_IMAGE,
	PIDGIN_BUTTON_TEXT_IMAGE

} PidginButtonStyle;

typedef enum
{
	PIDGIN_PROTOCOL_ICON_SMALL,
	PIDGIN_PROTOCOL_ICON_MEDIUM,
	PIDGIN_PROTOCOL_ICON_LARGE
} PidginProtocolIconSize;

#ifndef _WIN32
typedef enum
{
	PIDGIN_BROWSER_DEFAULT = 0,
	/* value '1' was used by PIDGIN_BROWSER_CURRENT, which no longer exists */
	PIDGIN_BROWSER_NEW_WINDOW = 2,
	PIDGIN_BROWSER_NEW_TAB

} PidginBrowserPlace;
#endif /* _WIN32 */

typedef struct {
	gboolean is_buddy;
	union {
		PurpleBuddy *buddy;
		PurpleLogSet *logged_buddy;
	} entry;
} PidginBuddyCompletionEntry;

typedef gboolean (*PidginFilterBuddyCompletionEntryFunc) (const PidginBuddyCompletionEntry *completion_entry, gpointer user_data);


G_BEGIN_DECLS

/**
 * pidgin_create_small_button:
 * @image:   A button image.
 *
 * Creates a small button
 *
 * Returns: (transfer full): A GtkButton created from the image.
 */
GtkWidget *pidgin_create_small_button(GtkWidget *image);

/**
 * pidgin_create_window:
 * @title:        The window title, or %NULL
 * @border_width: The window's desired border width
 * @role:         A string indicating what the window is responsible for doing, or %NULL
 * @resizable:    Whether the window should be resizable (%TRUE) or not (%FALSE)
 *
 * Creates a new window
 *
 * Returns: (transfer full): A new window.
 */
GtkWidget *pidgin_create_window(const char *title, guint border_width, const char *role, gboolean resizable);

/**
 * pidgin_create_dialog:
 * @title:        The window title, or %NULL
 * @border_width: The window's desired border width
 * @role:         A string indicating what the window is responsible for doing, or %NULL
 * @resizable:    Whether the window should be resizable (%TRUE) or not (%FALSE)
 *
 * Creates a new dialog window
 *
 * Returns: (transfer full): A new dialog window.
 */
GtkWidget *pidgin_create_dialog(const char *title, guint border_width, const char *role, gboolean resizable);

/**
 * pidgin_create_video_widget:
 *
 * Creates a new drawing area suitable for displaying a video
 *
 * Returns: (transfer full): A new drawing area for displaying video.
 */
GtkWidget *pidgin_create_video_widget(void);

/**
 * pidgin_dialog_get_vbox_with_properties:
 * @dialog:       The dialog window
 * @homogeneous:  TRUE if all children are to be given equal space allotments.
 * @spacing:      the number of pixels to place by default between children
 *
 * Retrieves the main content box (vbox) from a pidgin dialog window
 *
 * Returns: (transfer none): The main vbox from @dialog.
 */
GtkWidget *pidgin_dialog_get_vbox_with_properties(GtkDialog *dialog, gboolean homogeneous, gint spacing);

/**
 * pidgin_dialog_get_vbox:
 * @dialog:       The dialog window
 *
 * Retrieves the main content box (vbox) from a pidgin dialog window
 *
 * Returns: (transfer none): the main vbox from @dialog.
 */
GtkWidget *pidgin_dialog_get_vbox(GtkDialog *dialog);

/**
 * pidgin_dialog_add_button:
 * @dialog: The dialog window
 * @label: The stock-id or the label for the button
 * @callback: (scope call): The callback function for the button
 * @callbackdata: The user data for the callback function
 *
 * Add a button to a dialog created by #pidgin_create_dialog.
 *
 * Returns: (transfer full): The created button.
 */
GtkWidget *pidgin_dialog_add_button(GtkDialog *dialog, const char *label,
		GCallback callback, gpointer callbackdata);

/**
 * pidgin_dialog_get_action_area:
 * @dialog:       The dialog window
 *
 * Retrieves the action area (button box) from a pidgin dialog window
 *
 * Returns: (transfer none): The action area (button box) from @dialog.
 */
GtkWidget *pidgin_dialog_get_action_area(GtkDialog *dialog);

/**
 * pidgin_set_sensitive_if_input:
 * @entry:  The text entry widget.
 * @dialog: The dialog containing the text entry widget.
 *
 * Checks if text has been entered into a GtkTextEntry widget.  If
 * so, the GTK_RESPONSE_OK on the given dialog is set to TRUE.
 * Otherwise GTK_RESPONSE_OK is set to FALSE.
 */
void pidgin_set_sensitive_if_input(GtkWidget *entry, GtkWidget *dialog);

/**
 * pidgin_separator:
 * @menu: The menu to add a separator to.
 *
 * Adds a separator to a menu.
 *
 * Returns: (transfer full): The separator.
 */
GtkWidget *pidgin_separator(GtkWidget *menu);

/**
 * pidgin_new_check_item:
 * @menu: The menu to which to append the check menu item.
 * @str: The title to use for the newly created menu item.
 * @cb: (scope call): A function to call when the menu item is activated.
 * @data: Data to pass to the signal function.
 * @checked: The initial state of the check item
 *
 * Creates a check menu item.
 *
 * Returns: (transfer full): The newly created menu item.
 */
GtkWidget *pidgin_new_check_item(GtkWidget *menu, const char *str,
		GCallback cb, gpointer data, gboolean checked);

/**
 * pidgin_new_menu_item:
 * @menu:       The menu to which to append the menu item.
 * @mnemonic:   The title for the menu item.
 * @icon:       An icon to place to the left of the menu item,
 *                   or %NULL for no icon.
 * @cb: (scope call): A function to call when the menu item is activated.
 * @data:       Data to pass to the signal function.
 *
 * Creates a menu item.
 *
 * Returns: (transfer full): The newly created menu item.
 */
GtkWidget *pidgin_new_menu_item(GtkWidget *menu, const char *mnemonic,
                const char *icon, GCallback cb, gpointer data);

/**
 * pidgin_pixbuf_button_from_stock:
 * @text:  The text for the button.
 * @icon:  The stock icon name.
 * @style: The orientation of the button.
 *
 * Creates a button with the specified text and stock icon.
 *
 * Returns: (transfer full): The button.
 */
GtkWidget *pidgin_pixbuf_button_from_stock(const char *text, const char *icon,
										 PidginButtonOrientation style);

/**
 * pidgin_pixbuf_toolbar_button_from_stock:
 * @stock: The stock icon name.
 *
 * Creates a toolbar button with the stock icon.
 *
 * Returns: (transfer full): The button.
 */
GtkWidget *pidgin_pixbuf_toolbar_button_from_stock(const char *stock);

/**
 * pidgin_make_frame:
 * @parent: The widget to put the frame into.
 * @title:  The title for the frame.
 *
 * Creates a HIG preferences frame.
 *
 * Returns: (transfer full): The vbox to put things into.
 */
GtkWidget *pidgin_make_frame(GtkWidget *parent, const char *title);

/**
 * pidgin_protocol_option_menu_get_selected:
 * @optmenu: The drop-down option menu created by
 *           pidgin_protocol_option_menu_new().
 *
 * Gets the currently selected protocol from a protocol drop down box.
 *
 * Returns: Returns the protocol ID that is currently selected.
 */
const char *pidgin_protocol_option_menu_get_selected(GtkWidget *optmenu);

/**
 * pidgin_setup_screenname_autocomplete:
 * @entry:       The GtkEntry on which to setup autocomplete.
 * @chooser: A menu for accounts, returned by pidgin_account_chooser_new(). If
 *           @chooser is not %NULL, it'll be updated when a username is chosen
 *           from the autocomplete list.
 * @filter_func: (scope call): A function for checking if an autocomplete entry
 *                    should be shown. This can be %NULL.
 * @user_data:  The data to be passed to the filter_func function.
 *
 * Add autocompletion of screenames to an entry, supporting a filtering
 * function.
 */
void pidgin_setup_screenname_autocomplete(
        GtkWidget *entry, GtkWidget *chooser,
        PidginFilterBuddyCompletionEntryFunc filter_func, gpointer user_data);

/**
 * pidgin_screenname_autocomplete_default_filter:
 * @completion_entry: The completion entry to filter.
 * @all_accounts:  If this is %FALSE, only the autocompletion entries
 *                         which belong to an online account will be filtered.
 *
 * The default filter function for username autocomplete.
 *
 * Returns: Returns %TRUE if the autocompletion entry is filtered.
 */
gboolean pidgin_screenname_autocomplete_default_filter(const PidginBuddyCompletionEntry *completion_entry, gpointer all_accounts);

/**
 * pidgin_save_accels_cb:
 *
 * Save menu accelerators callback
 */
void pidgin_save_accels_cb(GtkAccelGroup *accel_group, guint arg1,
							 GdkModifierType arg2, GClosure *arg3,
							 gpointer data);

/**
 * pidgin_save_accels:
 *
 * Save menu accelerators
 */
gboolean pidgin_save_accels(gpointer data);

/**
 * pidgin_load_accels:
 *
 * Load menu accelerators
 */
void pidgin_load_accels(void);

/**
 * pidgin_retrieve_user_info:
 * @conn:   The connection to get information from.
 * @name:   The user to get information about.
 *
 * Get information about a user. Show immediate feedback.
 */
void pidgin_retrieve_user_info(PurpleConnection *conn, const char *name);

/**
 * pidgin_retrieve_user_info_in_chat:
 * @conn:   The connection to get information from.
 * @name:   The user to get information about.
 * @chatid: The chat id.
 *
 * Get information about a user in a chat. Show immediate feedback.
 */
void pidgin_retrieve_user_info_in_chat(PurpleConnection *conn, const char *name, int chatid);

/**
 * pidgin_parse_x_im_contact:
 * @msg:          The MIME message.
 * @all_accounts: If TRUE, check all compatible accounts, online or
 *                     offline. If FALSE, check only online accounts.
 * @ret_account:  The best guess at a compatible protocol,
 *                     based on ret_protocol. If NULL, no account was found.
 * @ret_protocol: The returned protocol type.
 * @ret_username: The returned username.
 * @ret_alias:    The returned alias.
 *
 * Parses an application/x-im-contact MIME message and returns the
 * data inside.
 *
 * Returns: TRUE if the message was parsed for the minimum necessary data.
 *         FALSE otherwise.
 */
gboolean pidgin_parse_x_im_contact(const char *msg, gboolean all_accounts,
									 PurpleAccount **ret_account,
									 char **ret_protocol, char **ret_username,
									 char **ret_alias);

/**
 * pidgin_set_accessible_label:
 * @w: The widget that we want to name.
 * @l: A GtkLabel that we want to use as the ATK name for the widget.
 *
 * Sets an ATK name for a given widget.  Also sets the labelled-by
 * and label-for ATK relationships.
 */
void pidgin_set_accessible_label(GtkWidget *w, GtkLabel *l);

/**
 * pidgin_set_accessible_relations:
 * @w: The widget that we want to label.
 * @l: A GtkLabel that we want to use as the label for the widget.
 *
 * Sets the labelled-by and label-for ATK relationships.
 */
void pidgin_set_accessible_relations(GtkWidget *w, GtkLabel *l);

/**
 * pidgin_menu_position_func_helper:
 * @menu: The menu we are positioning.
 * @x: Address of the gint representing the horizontal position
 *        where the menu shall be drawn. This is an output parameter.
 * @y: Address of the gint representing the vertical position
 *        where the menu shall be drawn. This is an output parameter.
 * @push_in: This is an output parameter?
 * @data: Not used by this particular position function.
 *
 * A helper function for GtkMenuPositionFuncs. This ensures the menu will
 * be kept on screen if possible.
 */
void pidgin_menu_position_func_helper(GtkMenu *menu, gint *x, gint *y,
										gboolean *push_in, gpointer data);

/**
 * pidgin_menu_popup_at_treeview_selection:
 * @menu: The menu to show.
 * @treeview: The treeview to use for positioning.
 *
 * Open a menu popup at the position determined by the selection of a given
 * treeview. This function is similar to @gtk_menu_popup_at_pointer, but should
 * be used when the menu is activated via a keyboard shortcut.
 */
void pidgin_menu_popup_at_treeview_selection(GtkWidget *menu, GtkWidget *treeview);

/**
 * pidgin_dnd_file_manage:
 * @sd: GtkSelectionData for managing drag'n'drop
 * @account: Account to be used (may be NULL if conv is not NULL)
 * @who: Buddy name (may be NULL if conv is not NULL)
 *
 * Manages drag'n'drop of files.
 */
void pidgin_dnd_file_manage(GtkSelectionData *sd, PurpleAccount *account, const char *who);

/**
 * pidgin_buddy_icon_get_scale_size:
 *
 * Convenience wrapper for purple_buddy_icon_spec_get_scaled_size
 */
void pidgin_buddy_icon_get_scale_size(GdkPixbuf *buf, PurpleBuddyIconSpec *spec, PurpleBuddyIconScaleFlags rules, int *width, int *height);

/**
 * pidgin_create_protocol_icon:
 * @account:      The account.
 * @size:         The size of the icon to return.
 *
 * Returns the base image to represent the account, based on
 * the currently selected theme.
 *
 * Returns: (transfer full): A newly-created pixbuf with a reference count of 1,
 *         or NULL if any of several error conditions occurred:
 *         the file could not be opened, there was no loader
 *         for the file's format, there was not enough memory
 *         to allocate the image buffer, or the image file
 *         contained invalid data.
 */
GdkPixbuf *pidgin_create_protocol_icon(PurpleAccount *account, PidginProtocolIconSize size);

/**
 * pidgin_create_icon_from_protocol:
 * @protocol: The #PurpleProtocol instance.
 * @size: The size of the icon to return.
 * @account: (nullable): An optional #PurpleAccount to use.
 *
 * Returns the base image to represent @protocol based on the currently
 * selected theme.  If @account is not %NULL then the returned icon will
 * represent the account.
 *
 * Returns: (transfer full): A newly-created pixbuf with a reference count of 1,
 *         or NULL if any of several error conditions occurred:
 *         the file could not be opened, there was no loader
 *         for the file's format, there was not enough memory
 *         to allocate the image buffer, or the image file
 *         contained invalid data.
 *
 * Since: 3.0.0
 */
GdkPixbuf *pidgin_create_icon_from_protocol(PurpleProtocol *protocol, PidginProtocolIconSize size, PurpleAccount *account);

/**
 * pidgin_create_status_icon:
 * @primitive:  The status primitive
 * @w:          The widget to render this
 * @size:       The icon size to render at
 *
 * Creates a status icon for a given primitve
 *
 * Returns: (transfer full): A GdkPixbuf, created from stock
 */
GdkPixbuf * pidgin_create_status_icon(PurpleStatusPrimitive primitive, GtkWidget *w, const char *size);

/**
 * pidgin_stock_id_from_status_primitive:
 * @prim:   The status primitive
 *
 * Returns an appropriate stock-id for a status primitive.
 *
 * Returns: The stock-id
 */
const char *pidgin_stock_id_from_status_primitive(PurpleStatusPrimitive prim);

/**
 * pidgin_stock_id_from_presence:
 * @presence:   The presence.
 *
 * Returns an appropriate stock-id for a PurplePresence.
 *
 * Returns: The stock-id
 */
const char *pidgin_stock_id_from_presence(PurplePresence *presence);

/**
 * pidgin_append_menu_action:
 * @menu:    The menu to append to.
 * @act:     The PurpleActionMenu to append.
 * @gobject: The object to be passed to the action callback.
 *
 * Append a PurpleActionMenu to a menu.
 *
 * Returns: (transfer full): The menuitem added.
 */
GtkWidget *pidgin_append_menu_action(GtkWidget *menu, PurpleActionMenu *act,
                                 gpointer gobject);

/**
 * pidgin_set_cursor:
 * @widget:      The widget for which to set the mouse pointer
 * @cursor_type: The type of cursor to set
 *
 * Sets the mouse pointer for a GtkWidget.
 *
 * After setting the cursor, the display is flushed, so the change will
 * take effect immediately.
 *
 * If the window for @widget is %NULL, this function simply returns.
 */
void pidgin_set_cursor(GtkWidget *widget, GdkCursorType cursor_type);

/**
 * pidgin_clear_cursor:
 *
 * Sets the mouse point for a GtkWidget back to that of its parent window.
 *
 * If @widget is %NULL, this function simply returns.
 *
 * If the window for @widget is %NULL, this function simply returns.
 *
 * Note: The display is not flushed from this function.
 */
void pidgin_clear_cursor(GtkWidget *widget);

/**
 * pidgin_buddy_icon_chooser_new:
 * @parent:      The parent window
 * @callback:    The callback to call when the window is closed. If the user chose an icon, the char* argument will point to its path
 * @data:        Data to pass to @callback
 *
 * Creates a File Selection widget for choosing a buddy icon
 *
 * Returns: (transfer full): The file dialog
 */
GtkFileChooserNative *pidgin_buddy_icon_chooser_new(
        GtkWindow *parent, void (*callback)(const char *, gpointer),
        gpointer data);

/**
 * pidgin_convert_buddy_icon:
 * @protocol:   The protocol to convert the icon
 * @path:       The path of a file to convert
 * @len:        If not %NULL, the length of the returned data will be set here.
 *
 * Converts a buddy icon to the required size and format
 *
 * Returns:           The converted image data, or %NULL if an error occurred.
 */
gpointer pidgin_convert_buddy_icon(PurpleProtocol *protocol, const char *path, size_t *len);

/**
 * PidginUtilMiniDialogCallback:
 *
 * The type of callbacks passed to pidgin_make_mini_dialog().
 */
typedef void (*PidginUtilMiniDialogCallback)(gpointer user_data, GtkButton *button);

/**
 * pidgin_make_mini_dialog:
 * @handle:         The #PurpleConnection to which this mini-dialog
 *                       refers, or %NULL if it does not refer to a
 *                       connection.  If @handle is supplied, the mini-dialog
 *                       will be automatically removed and destroyed when the
 *                       connection signs off.
 * @stock_id:       The ID of a stock image to use in the mini dialog.
 * @primary:        The primary text
 * @secondary:      The secondary text, or %NULL for no description.
 * @user_data:      Data to pass to the callbacks
 * @...:            a %NULL-terminated list of button labels
 *                       (<type>char *</type>) and callbacks
 *                       (#PidginUtilMiniDialogCallback).  @user_data will be
 *                       passed as the first argument.  (Callbacks may lack a
 *                       second argument, or be %NULL to take no action when
 *                       the corresponding button is pressed.) When a button is
 *                       pressed, the callback (if any) will be called; when
 *                       the callback returns the dialog will be destroyed.
 *
 * Creates a #PidginMiniDialog, tied to a #PurpleConnection, suitable for
 * embedding in the buddy list scrollbook with pidgin_blist_add_alert().
 *
 * See <link linkend="pidgin-pidginstock">Stock Resources</link>.
 *
 * Returns: (transfer full): A #PidginMiniDialog, suitable for passing to
 *                       pidgin_blist_add_alert().
 */
GtkWidget *pidgin_make_mini_dialog(PurpleConnection *handle,
	const char* stock_id, const char *primary, const char *secondary,
	void *user_data, ...) G_GNUC_NULL_TERMINATED;

/**
 * pidgin_make_mini_dialog_with_custom_icon:
 * @custom_icon: A custom GdkPixbuf to use.
 * @primary: The primary text
 * @secondary: The secondary text, or %NULL for no description.
 * @user_data: Data to pass to the callbacks
 * @...: a %NULL-terminated list of button labels
 *       (<type>char *</type>) and callbacks
 *       (#PidginUtilMiniDialogCallback).  @user_data will be
 *       passed as the first argument.  (Callbacks may lack a
 *       second argument, or be %NULL to take no action when
 *       the corresponding button is pressed.) When a button is
 *       pressed, the callback (if any) will be called; when
 *       the callback returns the dialog will be destroyed.
 *
 * Does exactly what pidgin_make_mini_dialog() does, except you can specify
 * a custom icon for the dialog.
 *
 * Returns: (transfer full): A #PidginMiniDialog, suitable for passing to
 *          pidgin_blist_add_alert().
 */
GtkWidget *pidgin_make_mini_dialog_with_custom_icon(PurpleConnection *gc,
	GdkPixbuf *custom_icon,
	const char *primary,
	const char *secondary,
	void *user_data,
	...) G_GNUC_NULL_TERMINATED;

/**
 * pidgin_tree_view_search_equal_func:
 *
 * This is a callback function to be used for Ctrl+F searching in treeviews.
 * Sample Use:
 * 		gtk_tree_view_set_search_equal_func(treeview,
 * 				pidgin_tree_view_search_equal_func,
 * 				search_data, search_data_destroy_cb);
 *
 */
gboolean pidgin_tree_view_search_equal_func(GtkTreeModel *model, gint column,
			const gchar *key, GtkTreeIter *iter, gpointer data);

/**
 * pidgin_set_urgent:
 * @window:  The window to draw attention to
 * @urgent:  Whether to set the urgent hint or not
 *
 * Sets or resets a window to 'urgent,' by setting the URGENT hint in X
 * or blinking in the win32 taskbar
 */
void pidgin_set_urgent(GtkWindow *window, gboolean urgent);

/**
 * pidgin_get_dim_grey_string:
 * @widget:  The widget to return dim grey for
 *
 * Returns an HTML-style color string for use as a dim grey
 * string
 *
 * Returns: The dim grey string
 */
const char *pidgin_get_dim_grey_string(GtkWidget *widget);

/**
 * pidgin_text_combo_box_entry_new:
 * @default_item: Initial contents of GtkEntry
 * @items: (element-type utf8): GList containing strings to add to GtkComboBox
 *
 * Create a simple text GtkComboBoxEntry equivalent
 *
 * Returns: (transfer full): A newly created text GtkComboBox containing a GtkEntry
 *          child.
 */
GtkWidget *pidgin_text_combo_box_entry_new(const char *default_item, GList *items);

/**
 * pidgin_text_combo_box_entry_get_text:
 * @widget:         The simple text GtkComboBoxEntry equivalent widget
 *
 * Retrieve the text from the entry of the simple text GtkComboBoxEntry equivalent
 *
 * Returns:               The text in the widget's entry. It must not be freed
 */
const char *pidgin_text_combo_box_entry_get_text(GtkWidget *widget);

/**
 * pidgin_text_combo_box_entry_set_text:
 * @widget:         The simple text GtkComboBoxEntry equivalent widget
 * @text:           The text to set
 *
 * Set the text in the entry of the simple text GtkComboBoxEntry equivalent
 */
void pidgin_text_combo_box_entry_set_text(GtkWidget *widget, const char *text);

/**
 * pidgin_auto_parent_window:
 * @window:    The window to make transient.
 *
 * Automatically make a window transient to a suitable parent window.
 *
 * Returns: Whether the window was made transient or not.
 */
gboolean pidgin_auto_parent_window(GtkWidget *window);

/**
 * pidgin_add_widget_to_vbox:
 * @vbox:         The vertically-oriented GtkBox to add the widget to.
 * @widget_label: The label to give the widget, can be %NULL.
 * @sg:           The GtkSizeGroup to add the label to, can be %NULL.
 * @widget:       The GtkWidget to add.
 * @expand:       Whether to expand the widget horizontally.
 * @p_label:      Place to store a pointer to the GtkLabel, or %NULL if you don't care.
 *
 * Add a labelled widget to a GtkBox
 *
 * Returns:  (transfer full): A GtkBox already added to the GtkBox containing the GtkLabel and the GtkWidget.
 */
GtkWidget *pidgin_add_widget_to_vbox(GtkBox *vbox, const char *widget_label, GtkSizeGroup *sg, GtkWidget *widget, gboolean expand, GtkWidget **p_label);

/**
 * pidgin_pixbuf_from_data:
 * @buf: The raw binary image data.
 * @count: The length of buf in bytes.
 *
 * Create a GdkPixbuf from a chunk of image data.
 *
 * Returns: (transfer full): A GdkPixbuf created from the image data, or NULL if
 *         there was an error parsing the data.
 */
GdkPixbuf *pidgin_pixbuf_from_data(const guchar *buf, gsize count);

/**
 * pidgin_pixbuf_anim_from_data:
 * @buf: The raw binary image data.
 * @count: The length of buf in bytes.
 *
 * Create a GdkPixbufAnimation from a chunk of image data.
 *
 * Returns: (transfer full): A GdkPixbufAnimation created from the image data, or NULL if
 *         there was an error parsing the data.
 */
GdkPixbufAnimation *pidgin_pixbuf_anim_from_data(const guchar *buf, gsize count);

/**
 * pidgin_pixbuf_from_image:
 * @image: a PurpleImage.
 *
 * Create a GdkPixbuf from a PurpleImage.
 *
 * Returns: (transfer full): a GdkPixbuf created from the @image.
 */
GdkPixbuf *
pidgin_pixbuf_from_image(PurpleImage *image);

/**
 * pidgin_pixbuf_new_from_file:
 * @filename: Name of file to load, in the GLib file name encoding
 *
 * Helper function that calls gdk_pixbuf_new_from_file() and checks both
 * the return code and the GError and returns NULL if either one failed.
 *
 * The gdk-pixbuf documentation implies that it is sufficient to check
 * the return value of gdk_pixbuf_new_from_file() to determine
 * whether the image was able to be loaded.  However, this is not the case
 * with gdk-pixbuf 2.23.3 and probably many earlier versions.  In some
 * cases a GdkPixbuf object is returned that will cause some operations
 * (like gdk_pixbuf_scale_simple()) to rapidly consume memory in an
 * infinite loop.
 *
 * This function shouldn't be necessary once Pidgin requires a version of
 * gdk-pixbuf where the aforementioned bug is fixed.  However, it might be
 * nice to keep this function around for the debug message that it logs.
 *
 * Returns: (transfer full): The GdkPixbuf if successful.  Otherwise NULL is returned and
 *         a warning is logged.
 */
GdkPixbuf *pidgin_pixbuf_new_from_file(const char *filename);

/**
 * pidgin_pixbuf_new_from_file_at_size:
 * @filename: Name of file to load, in the GLib file name encoding
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 *
 * Helper function that calls gdk_pixbuf_new_from_file_at_size() and checks
 * both the return code and the GError and returns NULL if either one failed.
 *
 * The gdk-pixbuf documentation implies that it is sufficient to check
 * the return value of gdk_pixbuf_new_from_file_at_size() to determine
 * whether the image was able to be loaded.  However, this is not the case
 * with gdk-pixbuf 2.23.3 and probably many earlier versions.  In some
 * cases a GdkPixbuf object is returned that will cause some operations
 * (like gdk_pixbuf_scale_simple()) to rapidly consume memory in an
 * infinite loop.
 *
 * This function shouldn't be necessary once Pidgin requires a version of
 * gdk-pixbuf where the aforementioned bug is fixed.  However, it might be
 * nice to keep this function around for the debug message that it logs.
 *
 * Returns: (transfer full): The GdkPixbuf if successful.  Otherwise NULL is returned and
 *         a warning is logged.
 */
GdkPixbuf *pidgin_pixbuf_new_from_file_at_size(const char *filename, int width, int height);

/**
 * pidgin_pixbuf_new_from_file_at_scale:
 * @filename: Name of file to load, in the GLib file name encoding
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: TRUE to preserve the image's aspect ratio
 *
 * Helper function that calls gdk_pixbuf_new_from_file_at_scale() and checks
 * both the return code and the GError and returns NULL if either one failed.
 *
 * The gdk-pixbuf documentation implies that it is sufficient to check
 * the return value of gdk_pixbuf_new_from_file_at_scale() to determine
 * whether the image was able to be loaded.  However, this is not the case
 * with gdk-pixbuf 2.23.3 and probably many earlier versions.  In some
 * cases a GdkPixbuf object is returned that will cause some operations
 * (like gdk_pixbuf_scale_simple()) to rapidly consume memory in an
 * infinite loop.
 *
 * This function shouldn't be necessary once Pidgin requires a version of
 * gdk-pixbuf where the aforementioned bug is fixed.  However, it might be
 * nice to keep this function around for the debug message that it logs.
 *
 * Returns: (transfer full): The GdkPixbuf if successful.  Otherwise NULL is returned and
 *         a warning is logged.
 */
GdkPixbuf *pidgin_pixbuf_new_from_file_at_scale(const char *filename, int width, int height, gboolean preserve_aspect_ratio);

/**
 * pidgin_pixbuf_scale_down:
 * @src: The source image.
 * @max_width: Maximum width in px.
 * @max_height: Maximum height in px.
 * @interp_type: Interpolation method.
 * @preserve_ratio: %TRUE to preserve image's aspect ratio.
 *
 * Scales the image to the desired dimensions. If image is smaller, it will be
 * returned without modifications.
 *
 * If new image is created, @src reference cound will be decreased and new image
 * with a ref count of 1 will be returned.
 *
 * Returns: (transfer full): The image with proper sizing. %NULL in case of error.
 */
GdkPixbuf *
pidgin_pixbuf_scale_down(GdkPixbuf *src, guint max_width, guint max_height,
	GdkInterpType interp_type, gboolean preserve_ratio);

/**
 * pidgin_make_scrollable:
 * @child:              The child widget
 * @hscrollbar_policy:  Horizontal scrolling policy
 * @vscrollbar_policy:  Vertical scrolling policy
 * @shadow_type:        Shadow type
 * @width:              Desired widget width, or -1 for default
 * @height:             Desired widget height, or -1 for default
 *
 * Add scrollbars to a widget
 *
 * Returns: (transfer full): A scrolled window with @child packed inside of it.
 */
GtkWidget *pidgin_make_scrollable(GtkWidget *child, GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy, GtkShadowType shadow_type, int width, int height);

G_END_DECLS

#endif /* _PIDGINUTILS_H_ */

