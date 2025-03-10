/* purple
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
 */

#ifndef PURPLE_PROTOCOL_H
#define PURPLE_PROTOCOL_H
/**
 * SECTION:protocol
 * @section_id: libpurple-protocol
 * @short_description: <filename>protocol.h</filename>
 * @title: Protocol Object and Interfaces
 */

#define PURPLE_TYPE_PROTOCOL            (purple_protocol_get_type())
#define PURPLE_PROTOCOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PURPLE_TYPE_PROTOCOL, PurpleProtocol))
#define PURPLE_PROTOCOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PURPLE_TYPE_PROTOCOL, PurpleProtocolClass))
#define PURPLE_IS_PROTOCOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL))
#define PURPLE_IS_PROTOCOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PURPLE_TYPE_PROTOCOL))
#define PURPLE_PROTOCOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PURPLE_TYPE_PROTOCOL, PurpleProtocolClass))

typedef struct _PurpleProtocol PurpleProtocol;
typedef struct _PurpleProtocolClass PurpleProtocolClass;

#include "account.h"
#include "buddyicon.h"
#include "buddylist.h"
#include "connection.h"
#include "conversations.h"
#include "debug.h"
#include "xfer.h"
#include "image.h"
#include "message.h"
#include "notify.h"
#include "plugins.h"
#include "purpleaccountoption.h"
#include "purpleaccountusersplit.h"
#include "roomlist.h"
#include "status.h"
#include "whiteboard.h"

/**
 * PurpleProtocol:
 * @id:              Protocol ID
 * @name:            Translated name of the protocol
 * @options:         Protocol options
 * @user_splits:     A GList of PurpleAccountUserSplit
 * @account_options: A GList of PurpleAccountOption
 * @icon_spec:       The icon spec.
 * @whiteboard_ops:  Whiteboard operations
 *
 * Represents an instance of a protocol registered with the protocols
 * subsystem. Protocols must initialize the members to appropriate values.
 */
struct _PurpleProtocol
{
	GObject gparent;

	/*< public >*/
	const char *id;
	const char *name;

	PurpleProtocolOptions options;

	GList *user_splits;
	GList *account_options;

	PurpleBuddyIconSpec *icon_spec;
	PurpleWhiteboardOps *whiteboard_ops;

	/*< private >*/
	void (*_purple_reserved1)(void);
	void (*_purple_reserved2)(void);
	void (*_purple_reserved3)(void);
	void (*_purple_reserved4)(void);
};

/**
 * PurpleProtocolClass:
 * @login:        Log in to the server.
 * @close:        Close connection with the server.
 * @status_types: Returns a list of #PurpleStatusType which exist for this
 *                account; and must add at least the offline and online states.
 * @list_icon:    Returns the base icon name for the given buddy and account. If
 *                buddy is %NULL and the account is non-%NULL, it will return
 *                the name to use for the account's icon. If both are %NULL, it
 *                will return the name to use for the protocol's icon.
 *
 * The base class for all protocols.
 *
 * All protocol types must implement the methods in this class.
 */
/* If adding new methods to this class, ensure you add checks for them in
   purple_protocols_add().
*/
struct _PurpleProtocolClass
{
	GObjectClass parent_class;

	void (*login)(PurpleAccount *account);

	void (*close)(PurpleConnection *connection);

	GList *(*status_types)(PurpleAccount *account);

	const char *(*list_icon)(PurpleAccount *account, PurpleBuddy *buddy);

	/*< private >*/
	void (*_purple_reserved1)(void);
	void (*_purple_reserved2)(void);
	void (*_purple_reserved3)(void);
	void (*_purple_reserved4)(void);
};

#define PURPLE_TYPE_PROTOCOL_CLIENT (purple_protocol_client_iface_get_type())

typedef struct _PurpleProtocolClientInterface PurpleProtocolClientInterface;

/**
 * PurpleProtocolClientInterface:
 * @get_actions:     Returns the actions the protocol can perform. These will
 *                   show up in the Accounts menu, under a submenu with the name
 *                   of the account.
 * @list_emblem:     Fills the four <type>char**</type>'s with string
 *                   identifiers for "emblems" that the UI will interpret and
 *                   display as relevant
 * @status_text:     Gets a short string representing this buddy's status. This
 *                   will be shown on the buddy list.
 * @tooltip_text:    Allows the protocol to add text to a buddy's tooltip.
 * @blist_node_menu: Returns a list of #PurpleActionMenu structs, which
 *                   represent extra actions to be shown in (for example) the
 *                   right-click menu for @node.
 * @normalize: Convert the username @who to its canonical form. Also checks for
 *             validity.
 *             <sbr/>For example, AIM treats "fOo BaR" and "foobar" as the same
 *             user; this function should return the same normalized string for
 *             both of those. On the other hand, both of these are invalid for
 *             protocols with number-based usernames, so function should return
 *             %NULL in such case.
 *             <sbr/>@account: The account the username is related to. Can be
 *                             %NULL.
 *             <sbr/>@who:     The username to convert.
 *             <sbr/>Returns:  Normalized username, or %NULL, if it's invalid.
 * @offline_message: Checks whether offline messages to @buddy are supported.
 *                   <sbr/>Returns: %TRUE if @buddy can be sent messages while
 *                                  they are offline, or %FALSE if not.
 * @get_account_text_table: This allows protocols to specify additional strings
 *                          to be used for various purposes. The idea is to
 *                          stuff a bunch of strings in this hash table instead
 *                          of expanding the struct for every addition. This
 *                          hash table is allocated every call and
 *                          <emphasis>MUST</emphasis> be unrefed by the caller.
 *                          <sbr/>@account: The account to specify.  This can be
 *                                          %NULL.
 *                          <sbr/>Returns:  The protocol's string hash table.
 *                                          The hash table should be destroyed
 *                                          by the caller when it's no longer
 *                                          needed.
 * @get_moods: Returns an array of #PurpleMood's, with the last one having
 *             "mood" set to %NULL.
 * @get_max_message_size: Gets the maximum message size in bytes for the
 *                        conversation.
 *                        <sbr/>It may depend on connection-specific or
 *                        conversation-specific variables, like channel or
 *                        buddy's name length.
 *                        <sbr/>This value is intended for plaintext message,
 *                              the exact value may be lower because of:
 *                        <sbr/> - used newlines (some protocols count them as
 *                                 more than one byte),
 *                        <sbr/> - formatting,
 *                        <sbr/> - used special characters.
 *                        <sbr/>@conv:   The conversation to query, or NULL to
 *                                       get safe minimum for the protocol.
 *                        <sbr/>Returns: Maximum message size, 0 if unspecified,
 *                                       -1 for infinite.
 *
 * The protocol client interface.
 *
 * This interface provides a gateway between purple and the protocol.
 */
struct _PurpleProtocolClientInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	GList *(*get_actions)(PurpleConnection *connection);

	const char *(*list_emblem)(PurpleBuddy *buddy);

	char *(*status_text)(PurpleBuddy *buddy);

	void (*tooltip_text)(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info,
						 gboolean full);

	GList *(*blist_node_menu)(PurpleBlistNode *node);

	void (*buddy_free)(PurpleBuddy *buddy);

	void (*convo_closed)(PurpleConnection *connection, const char *who);

	const char *(*normalize)(const PurpleAccount *account, const char *who);

	PurpleChat *(*find_blist_chat)(PurpleAccount *account, const char *name);

	gboolean (*offline_message)(const PurpleBuddy *buddy);

	GHashTable *(*get_account_text_table)(PurpleAccount *account);

	PurpleMood *(*get_moods)(PurpleAccount *account);

	gssize (*get_max_message_size)(PurpleConversation *conv);
};

#define PURPLE_IS_PROTOCOL_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_CLIENT))
#define PURPLE_PROTOCOL_CLIENT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_CLIENT, \
                                               PurpleProtocolClientInterface))

#define PURPLE_TYPE_PROTOCOL_SERVER (purple_protocol_server_iface_get_type())

typedef struct _PurpleProtocolServerInterface PurpleProtocolServerInterface;

/**
 * PurpleProtocolServerInterface:
 * @register_user:   New user registration
 * @unregister_user: Remove the user from the server. The account can either be
 *                   connected or disconnected. After the removal is finished,
 *                   the connection will stay open and has to be closed!
 * @get_info:        Should arrange for purple_notify_userinfo() to be called
 *                   with @who's user info.
 * @add_buddy:       Add a buddy to a group on the server.
 *                   <sbr/>This protocol function may be called in situations in
 *                   which the buddy is already in the specified group. If the
 *                   protocol supports authorization and the user is not already
 *                   authorized to see the status of @buddy, @add_buddy should
 *                   request authorization.
 *                   <sbr/>If authorization is required, then use the supplied
 *                   invite message.
 * @keepalive:       If implemented, this will be called regularly for this
 *                   protocol's active connections. You'd want to do this if you
 *                   need to repeatedly send some kind of keepalive packet to
 *                   the server to avoid being disconnected. ("Regularly" is
 *                   defined to be 30 unless @get_keepalive_interval is
 *                   implemented to override it).
 *                   <filename>libpurple/connection.c</filename>.)
 * @get_keepalive_interval: If implemented, this will override the default
 *                          keepalive interval.
 * @alias_buddy:     Save/store buddy's alias on server list/roster
 * @group_buddy:     Change a buddy's group on a server list/roster
 * @rename_group:    Rename a group on a server list/roster
 * @set_buddy_icon:  Set the buddy icon for the given connection to @img. The
 *                   protocol does <emphasis>NOT</emphasis> own a reference to
 *                   @img; if it needs one, it must #g_object_ref(@img)
 *                   itself.
 * @send_raw:        For use in plugins that may understand the underlying
 *                   protocol
 * @set_public_alias: Set the user's "friendly name" (or alias or nickname or
 *                    whatever term you want to call it) on the server. The
 *                    protocol should call @success_cb or @failure_cb
 *                    <emphasis>asynchronously</emphasis> (if it knows
 *                    immediately that the set will fail, call one of the
 *                    callbacks from an idle/0-second timeout) depending on if
 *                    the nickname is set successfully. See
 *                    purple_account_set_public_alias().
 *                    <sbr/>@gc:    The connection for which to set an alias
 *                    <sbr/>@alias: The new server-side alias/nickname for this
 *                                  account, or %NULL to unset the
 *                                  alias/nickname (or return it to a
 *                                  protocol-specific "default").
 *                    <sbr/>@success_cb: Callback to be called if the public
 *                                       alias is set
 *                    <sbr/>@failure_cb: Callback to be called if setting the
 *                                       public alias fails
 * @get_public_alias: Retrieve the user's "friendly name" as set on the server.
 *                    The protocol should call @success_cb or @failure_cb
 *                    <emphasis>asynchronously</emphasis> (even if it knows
 *                    immediately that the get will fail, call one of the
 *                    callbacks from an idle/0-second timeout) depending on if
 *                    the nickname is retrieved. See
 *                    purple_account_get_public_alias().
 *                    <sbr/>@gc:         The connection for which to retireve
 *                                       the alias
 *                    <sbr/>@success_cb: Callback to be called with the
 *                                       retrieved alias
 *                    <sbr/>@failure_cb: Callback to be called if the protocol
 *                                       is unable to retrieve the alias
 *
 * The protocol server interface.
 *
 * This interface provides a gateway between purple and the protocol's server.
 */
struct _PurpleProtocolServerInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	void (*register_user)(PurpleAccount *account);

	void (*unregister_user)(PurpleAccount *account, PurpleAccountUnregistrationCb cb,
							void *user_data);

	void (*set_info)(PurpleConnection *connection, const char *info);

	void (*get_info)(PurpleConnection *connection, const char *who);

	void (*set_status)(PurpleAccount *account, PurpleStatus *status);

	void (*set_idle)(PurpleConnection *connection, int idletime);

	void (*change_passwd)(PurpleConnection *connection, const char *old_pass,
						  const char *new_pass);

	void (*add_buddy)(PurpleConnection *pc, PurpleBuddy *buddy,
					  PurpleGroup *group, const char *message);

	void (*add_buddies)(PurpleConnection *pc, GList *buddies, GList *groups,
						const char *message);

	void (*remove_buddy)(PurpleConnection *connection, PurpleBuddy *buddy,
						 PurpleGroup *group);

	void (*remove_buddies)(PurpleConnection *connection, GList *buddies, GList *groups);

	void (*keepalive)(PurpleConnection *connection);

	int (*get_keepalive_interval)(void);

	void (*alias_buddy)(PurpleConnection *connection, const char *who,
						const char *alias);

	void (*group_buddy)(PurpleConnection *connection, const char *who,
					const char *old_group, const char *new_group);

	void (*rename_group)(PurpleConnection *connection, const char *old_name,
					 PurpleGroup *group, GList *moved_buddies);

	void (*set_buddy_icon)(PurpleConnection *connection, PurpleImage *img);

	void (*remove_group)(PurpleConnection *gc, PurpleGroup *group);

	int (*send_raw)(PurpleConnection *gc, const char *buf, int len);

	void (*set_public_alias)(PurpleConnection *gc, const char *alias,
	                         PurpleSetPublicAliasSuccessCallback success_cb,
	                         PurpleSetPublicAliasFailureCallback failure_cb);

	void (*get_public_alias)(PurpleConnection *gc,
	                         PurpleGetPublicAliasSuccessCallback success_cb,
	                         PurpleGetPublicAliasFailureCallback failure_cb);
};

#define PURPLE_IS_PROTOCOL_SERVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_SERVER))
#define PURPLE_PROTOCOL_SERVER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_SERVER, \
                                               PurpleProtocolServerInterface))

#define PURPLE_TYPE_PROTOCOL_IM (purple_protocol_im_iface_get_type())

typedef struct _PurpleProtocolIMInterface PurpleProtocolIMInterface;

/**
 * PurpleProtocolIMInterface:
 * @send:        This protocol function should return a positive value on
 *               success. If the message is too big to be sent, return
 *               <literal>-E2BIG</literal>. If the account is not connected,
 *               return <literal>-ENOTCONN</literal>. If the protocol is unable
 *               to send the message for another reason, return some other
 *               negative value. You can use one of the valid #errno values, or
 *               just big something. If the message should not be echoed to the
 *               conversation window, return 0.
 * @send_typing: If this protocol requires the #PURPLE_IM_TYPING message to be
 *               sent repeatedly to signify that the user is still typing, then
 *               the protocol should return the number of seconds to wait before
 *               sending a subsequent notification. Otherwise the protocol
 *               should return 0.
 *
 * The protocol IM interface.
 *
 * This interface provides callbacks needed by protocols that implement IMs.
 */
struct _PurpleProtocolIMInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	int  (*send)(PurpleConnection *connection, PurpleMessage *msg);

	unsigned int (*send_typing)(PurpleConnection *connection, const char *name,
							PurpleIMTypingState state);
};

#define PURPLE_IS_PROTOCOL_IM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_IM))
#define PURPLE_PROTOCOL_IM_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_IM, \
                                           PurpleProtocolIMInterface))

#define PURPLE_TYPE_PROTOCOL_CHAT (purple_protocol_chat_iface_get_type())

typedef struct _PurpleProtocolChatInterface PurpleProtocolChatInterface;

/**
 * PurpleProtocolChatInterface:
 * @info: Returns a list of #PurpleProtocolChatEntry structs, which represent
 *        information required by the protocol to join a chat. libpurple will
 *        call join_chat along with the information filled by the user.
 *        <sbr/>Returns: A list of #PurpleProtocolChatEntry's
 * @info_defaults: Returns a hashtable which maps #PurpleProtocolChatEntry
 *                 struct identifiers to default options as strings based on
 *                 @chat_name. The resulting hashtable should be created with
 *                 #g_hash_table_new_full(#g_str_hash, #g_str_equal, %NULL,
 *                 #g_free). Use @get_name if you instead need to extract a chat
 *                 name from a hashtable.
 *                 <sbr/>@chat_name: The chat name to be turned into components
 *                 <sbr/>Returns: Hashtable containing the information extracted
 *                                from @chat_name
 * @join: Called when the user requests joining a chat. Should arrange for
 *        purple_serv_got_joined_chat() to be called.
 *        <sbr/>@components: A hashtable containing information required to join
 *                           the chat as described by the entries returned by
 *                           @info. It may also be called when accepting an
 *                           invitation, in which case this matches the data
 *                           parameter passed to purple_serv_got_chat_invite().
 * @reject: Called when the user refuses a chat invitation.
 *          <sbr/>@components: A hashtable containing information required to
 *                            join the chat as passed to purple_serv_got_chat_invite().
 * @get_name: Returns a chat name based on the information in components. Use
 *            @info_defaults if you instead need to generate a hashtable from a
 *            chat name.
 *            <sbr/>@components: A hashtable containing information about the
 *                               chat.
 * @invite: Invite a user to join a chat.
 *          <sbr/>@id:      The id of the chat to invite the user to.
 *          <sbr/>@message: A message displayed to the user when the invitation
 *                          is received.
 *          <sbr/>@who:     The name of the user to send the invation to.
 * @leave: Called when the user requests leaving a chat.
 *         <sbr/>@id: The id of the chat to leave
 * @send: Send a message to a chat.
 *              <sbr/>This protocol function should return a positive value on
 *              success. If the message is too big to be sent, return
 *              <literal>-E2BIG</literal>. If the account is not connected,
 *              return <literal>-ENOTCONN</literal>. If the protocol is unable
 *              to send the message for another reason, return some other
 *              negative value. You can use one of the valid #errno values, or
 *              just big something.
 *              <sbr/>@id:      The id of the chat to send the message to.
 *              <sbr/>@msg:     The message to send to the chat.
 *              <sbr/>Returns:  A positive number or 0 in case of success, a
 *                              negative error number in case of failure.
 * @get_user_real_name: Gets the real name of a participant in a chat. For
 *                      example, on XMPP this turns a chat room nick
 *                      <literal>foo</literal> into
 *                      <literal>room\@server/foo</literal>.
 *                      <sbr/>@gc:  the connection on which the room is.
 *                      <sbr/>@id:  the ID of the chat room.
 *                      <sbr/>@who: the nickname of the chat participant.
 *                      <sbr/>Returns: the real name of the participant. This
 *                                     string must be freed by the caller.
 *
 * The protocol chat interface.
 *
 * This interface provides callbacks needed by protocols that implement chats.
 */
struct _PurpleProtocolChatInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	GList *(*info)(PurpleConnection *connection);

	GHashTable *(*info_defaults)(PurpleConnection *connection, const char *chat_name);

	void (*join)(PurpleConnection *connection, GHashTable *components);

	void (*reject)(PurpleConnection *connection, GHashTable *components);

	char *(*get_name)(GHashTable *components);

	void (*invite)(PurpleConnection *connection, int id,
					const char *message, const char *who);

	void (*leave)(PurpleConnection *connection, int id);

	int  (*send)(PurpleConnection *connection, int id, PurpleMessage *msg);

	char *(*get_user_real_name)(PurpleConnection *gc, int id, const char *who);

	void (*set_topic)(PurpleConnection *gc, int id, const char *topic);
};

#define PURPLE_IS_PROTOCOL_CHAT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_CHAT))
#define PURPLE_PROTOCOL_CHAT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_CHAT, \
                                             PurpleProtocolChatInterface))

#define PURPLE_TYPE_PROTOCOL_PRIVACY (purple_protocol_privacy_iface_get_type())

typedef struct _PurpleProtocolPrivacyInterface PurpleProtocolPrivacyInterface;

/**
 * PurpleProtocolPrivacyInterface:
 * @add_permit:		Add the buddy on the required authorized list.
 * @add_deny:		Add the buddy on the required blocked list.
 * @rem_permit:		Remove the buddy from the requried authorized list.
 * @rem_deny:		Remove the buddy from the required blocked list.
 * @set_permit_deny:Update the server with the privacy information on the permit and deny lists.
 *
 * The protocol privacy interface.
 *
 * This interface provides privacy callbacks such as to permit/deny users.
 */
struct _PurpleProtocolPrivacyInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	void (*add_permit)(PurpleConnection *gc, const char *name);

	void (*add_deny)(PurpleConnection *gc, const char *name);

	void (*rem_permit)(PurpleConnection *gc, const char *name);

	void (*rem_deny)(PurpleConnection *gc, const char *name);

	void (*set_permit_deny)(PurpleConnection *gc);
};

#define PURPLE_IS_PROTOCOL_PRIVACY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_PRIVACY))
#define PURPLE_PROTOCOL_PRIVACY_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_PRIVACY, \
                                                PurpleProtocolPrivacyInterface))

#define PURPLE_TYPE_PROTOCOL_MEDIA (purple_protocol_media_iface_get_type())

typedef struct _PurpleProtocolMediaInterface PurpleProtocolMediaInterface;

/**
 * PurpleProtocolMediaInterface:
 * @initiate_session: Initiate a media session with the given contact.
 *                    <sbr/>@account: The account to initiate the media session
 *                                    on.
 *                    <sbr/>@who: The remote user to initiate the session with.
 *                    <sbr/>@type: The type of media session to initiate.
 *                    <sbr/>Returns: %TRUE if the call succeeded else %FALSE.
 *                                   (Doesn't imply the media session or stream
 *                                   will be successfully created)
 * @get_caps: Checks to see if the given contact supports the given type of
 *            media session.
 *            <sbr/>@account: The account the contact is on.
 *            <sbr/>@who: The remote user to check for media capability with.
 *            <sbr/>Returns: The media caps the contact supports.
 * @send_dtmf: Sends DTMF codes out-of-band in a protocol-specific way if the
 *             protocol supports it, or failing that in-band if the media backend
 *             can do so. See purple_media_send_dtmf().
 *
 * The protocol media interface.
 *
 * This interface provides callbacks for media sessions on the protocol.
 */
struct _PurpleProtocolMediaInterface
{
	/*< private >*/
	GTypeInterface parent_iface;

	/*< public >*/
	gboolean (*initiate_session)(PurpleAccount *account, const char *who,
					PurpleMediaSessionType type);

	PurpleMediaCaps (*get_caps)(PurpleAccount *account,
					  const char *who);

	gboolean (*send_dtmf)(PurpleMedia *media, gchar dtmf,
				    guint8 volume, guint8 duration);
};

#define PURPLE_IS_PROTOCOL_MEDIA(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_PROTOCOL_MEDIA))
#define PURPLE_PROTOCOL_MEDIA_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), PURPLE_TYPE_PROTOCOL_MEDIA, \
                                              PurpleProtocolMediaInterface))

/**
 * PURPLE_PROTOCOL_IMPLEMENTS:
 * @protocol: The protocol in which to check
 * @IFACE:    The interface name in caps. e.g. <literal>CLIENT</literal>
 * @func:     The function to check
 *
 * Returns: %TRUE if a protocol implements a function in an interface,
 *          %FALSE otherwise.
 */
#define PURPLE_PROTOCOL_IMPLEMENTS(protocol, IFACE, func) \
	(PURPLE_IS_PROTOCOL_##IFACE(protocol) && \
	 PURPLE_PROTOCOL_##IFACE##_GET_IFACE(protocol)->func != NULL)

G_BEGIN_DECLS

/**************************************************************************/
/* Protocol Object API                                                    */
/**************************************************************************/

/**
 * purple_protocol_get_type:
 *
 * Returns: The #GType for #PurpleProtocol.
 */
GType purple_protocol_get_type(void);

/**
 * purple_protocol_get_id:
 * @protocol: The protocol.
 *
 * Returns the ID of a protocol.
 *
 * Returns: The ID of the protocol.
 */
const char *purple_protocol_get_id(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_name:
 * @protocol: The protocol.
 *
 * Returns the translated name of a protocol.
 *
 * Returns: The translated name of the protocol.
 */
const char *purple_protocol_get_name(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_options:
 * @protocol: The protocol.
 *
 * Returns the options of a protocol.
 *
 * Returns: The options of the protocol.
 */
PurpleProtocolOptions purple_protocol_get_options(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_user_splits:
 * @protocol: The protocol.
 *
 * Returns the user splits of a protocol.
 *
 * Returns: (element-type PurpleAccountUserSplit) (transfer none): The user
 *          splits of the protocol.
 */
GList *purple_protocol_get_user_splits(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_account_options:
 * @protocol: The protocol.
 *
 * Returns the account options for a protocol.
 *
 * Returns: (element-type PurpleAccountOption) (transfer none): The account
 *          options for the protocol.
 */
GList *purple_protocol_get_account_options(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_icon_spec:
 * @protocol: The protocol.
 *
 * Returns the icon spec of a protocol.
 *
 * Returns: The icon spec of the protocol.
 */
PurpleBuddyIconSpec *purple_protocol_get_icon_spec(const PurpleProtocol *protocol);

/**
 * purple_protocol_get_whiteboard_ops:
 * @protocol: The protocol.
 *
 * Returns the whiteboard ops of a protocol.
 *
 * Returns: (transfer none): The whiteboard ops of the protocol.
 */
PurpleWhiteboardOps *purple_protocol_get_whiteboard_ops(const PurpleProtocol *protocol);

/**************************************************************************/
/* Protocol Class API                                                     */
/**************************************************************************/

void purple_protocol_class_login(PurpleProtocol *protocol, PurpleAccount *account);

void purple_protocol_class_close(PurpleProtocol *protocol, PurpleConnection *connection);

GList *purple_protocol_class_status_types(PurpleProtocol *protocol,
		PurpleAccount *account);

const char *purple_protocol_class_list_icon(PurpleProtocol *protocol,
		PurpleAccount *account, PurpleBuddy *buddy);

/**************************************************************************/
/* Protocol Client Interface API                                          */
/**************************************************************************/

/**
 * purple_protocol_client_iface_get_type:
 *
 * Returns: The #GType for the protocol client interface.
 *
 * Since: 3.0.0
 */
GType purple_protocol_client_iface_get_type(void);

/**
 * purple_protocol_client_iface_get_actions:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 *
 * Gets a list of actions for @connection.
 *
 * Returns: (transfer full) (element-type PurpleProtocolAction): The list of actions for @connection.
 *
 * Since: 3.0.0
 */
GList *purple_protocol_client_iface_get_actions(PurpleProtocol *protocol,
		PurpleConnection *connection);

/**
 * purple_protocol_client_iface_list_emblem:
 * @protocol: The #PurpleProtocol instance.
 * @buddy: The #PurpleBuddy instance.
 *
 * Gets the icon name of the emblem that should be used for @buddy.
 *
 * Returns: The icon name of the emblem or NULL.
 *
 * Since: 3.0.0
 */
const char *purple_protocol_client_iface_list_emblem(PurpleProtocol *protocol,
		PurpleBuddy *buddy);

/**
 * purple_protocol_client_iface_status_text:
 * @protocol: The #PurpleProtocol instance.
 * @buddy: The #PurpleBuddy instance.
 *
 * Gets the status text for @buddy.
 *
 * Returns: (transfer full): The status text for @buddy or NULL.
 *
 * Since: 3.0.0
 */
char *purple_protocol_client_iface_status_text(PurpleProtocol *protocol,
		PurpleBuddy *buddy);

/**
 * purple_protocol_client_iface_tooltip_text:
 * @protocol: The #PurpleProtocol instance.
 * @buddy: The #PurpleBuddy instance.
 * @user_info: The #PurpleNotifyUserInfo instance.
 * @full: Whether or not additional info should be added.
 *
 * Asks @protocol to update @user_info for @buddy.  If @full is %TRUE then
 * more detailed information will added.
 *
 * Since: 3.0.0
 */
void purple_protocol_client_iface_tooltip_text(PurpleProtocol *protocol,
		PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full);

GList *purple_protocol_client_iface_blist_node_menu(PurpleProtocol *protocol,
		PurpleBlistNode *node);

void purple_protocol_client_iface_buddy_free(PurpleProtocol *protocol, PurpleBuddy *buddy);

void purple_protocol_client_iface_convo_closed(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *who);

const char *purple_protocol_client_iface_normalize(PurpleProtocol *protocol,
		const PurpleAccount *account, const char *who);

PurpleChat *purple_protocol_client_iface_find_blist_chat(PurpleProtocol *protocol,
		PurpleAccount *account, const char *name);

gboolean purple_protocol_client_iface_offline_message(PurpleProtocol *protocol,
		const PurpleBuddy *buddy);

GHashTable *purple_protocol_client_iface_get_account_text_table(PurpleProtocol *protocol,
		PurpleAccount *account);

PurpleMood *purple_protocol_client_iface_get_moods(PurpleProtocol *protocol,
		PurpleAccount *account);

gssize purple_protocol_client_iface_get_max_message_size(PurpleProtocol *protocol,
		PurpleConversation *conv);

/**************************************************************************/
/* Protocol Server Interface API                                          */
/**************************************************************************/

/**
 * purple_protocol_server_iface_get_type:
 *
 * Returns: The #GType for the protocol server interface.
 */
GType purple_protocol_server_iface_get_type(void);

void purple_protocol_server_iface_register_user(PurpleProtocol *protocol,
		PurpleAccount *account);

/**
 * purple_protocol_server_iface_unregister_user:
 * @cb: (scope call):
 */
void purple_protocol_server_iface_unregister_user(PurpleProtocol *protocol,
		PurpleAccount *account, PurpleAccountUnregistrationCb cb, void *user_data);

void purple_protocol_server_iface_set_info(PurpleProtocol *protocol, PurpleConnection *connection,
		const char *info);

void purple_protocol_server_iface_get_info(PurpleProtocol *protocol, PurpleConnection *connection,
		const char *who);

void purple_protocol_server_iface_set_status(PurpleProtocol *protocol,
		PurpleAccount *account, PurpleStatus *status);

void purple_protocol_server_iface_set_idle(PurpleProtocol *protocol, PurpleConnection *connection,
		int idletime);

void purple_protocol_server_iface_change_passwd(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *old_pass, const char *new_pass);

void purple_protocol_server_iface_add_buddy(PurpleProtocol *protocol,
		PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group,
		const char *message);

void purple_protocol_server_iface_add_buddies(PurpleProtocol *protocol,
		PurpleConnection *pc, GList *buddies, GList *groups,
		const char *message);

void purple_protocol_server_iface_remove_buddy(PurpleProtocol *protocol,
		PurpleConnection *connection, PurpleBuddy *buddy, PurpleGroup *group);

void purple_protocol_server_iface_remove_buddies(PurpleProtocol *protocol,
		PurpleConnection *connection, GList *buddies, GList *groups);

void purple_protocol_server_iface_keepalive(PurpleProtocol *protocol,
		PurpleConnection *connection);

int purple_protocol_server_iface_get_keepalive_interval(PurpleProtocol *protocol);

void purple_protocol_server_iface_alias_buddy(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *who, const char *alias);

void purple_protocol_server_iface_group_buddy(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *who, const char *old_group,
		const char *new_group);

void purple_protocol_server_iface_rename_group(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *old_name, PurpleGroup *group,
		GList *moved_buddies);

void purple_protocol_server_iface_set_buddy_icon(PurpleProtocol *protocol,
		PurpleConnection *connection, PurpleImage *img);

void purple_protocol_server_iface_remove_group(PurpleProtocol *protocol,
		PurpleConnection *gc, PurpleGroup *group);

int purple_protocol_server_iface_send_raw(PurpleProtocol *protocol,
		PurpleConnection *gc, const char *buf, int len);

/**
 * purple_protocol_server_iface_set_public_alias:
 * @success_cb: (scope call):
 * @failure_cb: (scope call):
 */
void purple_protocol_server_iface_set_public_alias(PurpleProtocol *protocol,
		PurpleConnection *gc, const char *alias,
		PurpleSetPublicAliasSuccessCallback success_cb,
		PurpleSetPublicAliasFailureCallback failure_cb);

/**
 * purple_protocol_server_iface_get_public_alias:
 * @success_cb: (scope call):
 * @failure_cb: (scope call):
 */
void purple_protocol_server_iface_get_public_alias(PurpleProtocol *protocol,
		PurpleConnection *gc, PurpleGetPublicAliasSuccessCallback success_cb,
		PurpleGetPublicAliasFailureCallback failure_cb);

/**************************************************************************/
/* Protocol IM Interface API                                              */
/**************************************************************************/

/**
 * purple_protocol_im_iface_get_type:
 *
 * Returns: The #GType for the protocol IM interface.
 */
GType purple_protocol_im_iface_get_type(void);

int purple_protocol_im_iface_send(PurpleProtocol *protocol, PurpleConnection *connection,
		 PurpleMessage *msg);

unsigned int purple_protocol_im_iface_send_typing(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *name, PurpleIMTypingState state);

/**************************************************************************/
/* Protocol Chat Interface API                                            */
/**************************************************************************/

/**
 * purple_protocol_chat_iface_get_type:
 *
 * Returns: The #GType for the protocol chat interface.
 *
 * Since: 3.0.0
 */
GType purple_protocol_chat_iface_get_type(void);

/**
 * purple_protocol_chat_iface_info:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 *
 * Gets the list of #PurpleProtocolChatEntry's that are required to join a
 * multi user chat.
 *
 * Returns: (transfer full) (element-type PurpleProtocolChatEntry): The list
 *          of #PurpleProtocolChatEntry's that are used to join a chat.
 *
 * Since: 3.0.0
 */
GList *purple_protocol_chat_iface_info(PurpleProtocol *protocol,
		PurpleConnection *connection);

/**
 * purple_protocol_chat_iface_info_defaults:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @chat_name: The name of the chat
 *
 * Returns a #GHashTable of the default protocol dependent components that will
 * be passed to #purple_protocol_chat_iface_join.
 *
 * Returns: (transfer full) (element-type utf8 utf8): The values that will be
 *          used to join the chat.
 *
 * Since: 3.0.0
 */
GHashTable *purple_protocol_chat_iface_info_defaults(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *chat_name);

/**
 * purple_protocol_chat_iface_join:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @components: (element-type utf8 utf8): The protocol dependent join
 *              components
 *
 * Joins the chat described in @components.
 *
 * Since: 3.0.0
 */
void purple_protocol_chat_iface_join(PurpleProtocol *protocol, PurpleConnection *connection,
		GHashTable *components);

/**
 * purple_protocol_chat_iface_reject:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @components: (element-type utf8 utf8): The protocol dependent join
 *              components
 *
 * Not quite sure exactly what this does or where it's used.  Please fill in
 * the details if you know.
 *
 * Since: 3.0.0
 */
void purple_protocol_chat_iface_reject(PurpleProtocol *protocol,
		PurpleConnection *connection, GHashTable *components);

/**
 * purple_protocol_chat_iface_get_name:
 * @protocol: The #PurpleProtocol instance
 * @components: (element-type utf8 utf8): The protocol dependent join
 *              components
 *
 * Gets the name from @components.
 *
 * Returns: (transfer full): The chat name from @components.
 *
 * Since: 3.0.0
 */
char *purple_protocol_chat_iface_get_name(PurpleProtocol *protocol,
		GHashTable *components);

/**
 * purple_protocol_chat_iface_invite:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @id: The id of the chat
 * @message: The invite message
 * @who: The target of the invite
 *
 * Sends an invite to @who with @message.
 *
 * Since: 3.0.0
 */
void purple_protocol_chat_iface_invite(PurpleProtocol *protocol,
		PurpleConnection *connection, int id, const char *message, const char *who);

/**
 * purple_protocol_chat_iface_leave:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @id: The id of the chat
 *
 * Leaves the chat identified by @id.
 *
 * Since: 3.0.0
 */
void purple_protocol_chat_iface_leave(PurpleProtocol *protocol, PurpleConnection *connection,
		int id);

/**
 * purple_protocol_chat_iface_send:
 * @protocol: The #PurpleProtocol instance
 * @connection: The #PurpleConnection instance
 * @id: The id of the chat
 * @msg: The message to send
 *
 * Sends @msg to the chat identified by @id.
 *
 * Returns: 0 on success, non-zero on failure.
 *
 * Since: 3.0.0
 */
int  purple_protocol_chat_iface_send(PurpleProtocol *protocol, PurpleConnection *connection,
		int id, PurpleMessage *msg);

/**
 * purple_protocol_chat_iface_get_user_real_name:
 * @protocol: The #PurpleProtocol instance
 * @gc: The #PurpleConnection instance
 * @id: The id of the chat
 * @who: The username
 *
 * Gets the real name of @who.
 *
 * Returns: (transfer full): The realname of @who.
 *
 * Since: 3.0.0
 */
char *purple_protocol_chat_iface_get_user_real_name(PurpleProtocol *protocol,
		PurpleConnection *gc, int id, const char *who);

/**
 * purple_protocol_chat_iface_set_topic:
 * @protocol: The #PurpleProtocol instance
 * @gc: The #PurpleConnection instance
 * @id: The id of the chat
 * @topic: The new topic
 *
 * Sets the topic for the chat with id @id to @topic.
 *
 * Since: 3.0.0
 */
void purple_protocol_chat_iface_set_topic(PurpleProtocol *protocol,
		PurpleConnection *gc, int id, const char *topic);

/**************************************************************************/
/* Protocol Privacy Interface API                                         */
/**************************************************************************/

/**
 * purple_protocol_privacy_iface_get_type:
 *
 * Returns: The #GType for the protocol privacy interface.
 *
 * Since: 3.0.0
 */
GType purple_protocol_privacy_iface_get_type(void);

/**
 * purple_protocol_privacy_iface_add_permit:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 * @name: The username to permit.
 *
 * Adds a permit to the privacy settings for @connection to allow @name to
 * contact the user.
 *
 * Since: 3.0.0
 */
void purple_protocol_privacy_iface_add_permit(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *name);

/**
 * purple_protocol_privacy_iface_add_deny:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 * @name: The username to deny.
 *
 * Adds a deny to the privacy settings for @connection to deny @name from
 * contacting the user.
 *
 * Since: 3.0.0
 */
void purple_protocol_privacy_iface_add_deny(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *name);

/**
 * purple_protocol_privacy_iface_rem_permit:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 * @name: The username to remove from the permit privacy settings.
 *
 * Removes an existing permit for @name.
 *
 * Since: 3.0.0
 */
void purple_protocol_privacy_iface_rem_permit(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *name);

/**
 * purple_protocol_privacy_iface_rem_deny:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 * @name: The username to remove from the deny privacy settings.
 *
 * Removes an existing deny for @name.
 *
 * Since: 3.0.0
 */
void purple_protocol_privacy_iface_rem_deny(PurpleProtocol *protocol,
		PurpleConnection *connection, const char *name);

/**
 * purple_protocol_privacy_iface_set_permit_deny:
 * @protocol: The #PurpleProtocol instance.
 * @connection: The #PurpleConnection instance.
 *
 * Forces a sync of the privacy settings with server.
 *
 * Since: 3.0.0
 */
void purple_protocol_privacy_iface_set_permit_deny(PurpleProtocol *protocol,
		PurpleConnection *connection);

/**************************************************************************/
/* Protocol Media Interface API                                           */
/**************************************************************************/

/**
 * purple_protocol_media_iface_get_type:
 *
 * Returns: The #GType for the protocol media interface.
 *
 * Since: 3.0.0
 */
GType purple_protocol_media_iface_get_type(void);

/**
 * purple_protocol_media_iface_initiate_session:
 * @protocol: The #PurpleProtocol instance.
 * @account: The #PurpleAccount instance.
 * @who: The user to initiate a media session with.
 * @type: The type of media session to create.
 *
 * Initiates a media connection of @type to @who.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 *
 * Since: 3.0.0
 */
gboolean purple_protocol_media_iface_initiate_session(PurpleProtocol *protocol,
		PurpleAccount *account, const char *who, PurpleMediaSessionType type);

/**
 * purple_protocol_media_iface_get_caps:
 * @protocol: The #PurpleProtocol instance.
 * @account: The #PurpleAccount instance.
 * @who: The user to get the media capabilites for.
 *
 * Gets the #PurpleMediaCaps for @who which determine what types of media are
 * available.
 *
 * Returns: the media capabilities of @who.
 *
 * Since: 3.0.0
 */
PurpleMediaCaps purple_protocol_media_iface_get_caps(PurpleProtocol *protocol,
		PurpleAccount *account, const char *who);

/**
 * purple_protocol_media_iface_send_dtmf:
 * @protocol: The #PurpleProtocol instance.
 * @media: The #PurpleMedia instance.
 * @dtmf: A DTMF to send.
 * @volume: The volume to send @dtmf at.
 * @duration: The duration to send @dtmf (in ms?)
 *
 * Sends a DTMF (dual-tone multi-frequency) signal via the established @media
 * for the given @duration at the given @volume.
 *
 * It is up to the specific implementation if DTMF is send in or out of band.
 *
 * Returns: TRUE if successful, FALSE otherwise.
 *
 * Since: 3.0.0
 */
gboolean purple_protocol_media_iface_send_dtmf(PurpleProtocol *protocol,
		PurpleMedia *media, gchar dtmf, guint8 volume, guint8 duration);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_H */
