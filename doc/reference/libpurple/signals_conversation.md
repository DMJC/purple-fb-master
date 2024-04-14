Title: Conversation Signals
Slug: conversation-signals

## Conversation Signals

### Signal List

* [writing-im-msg](#writing-im-msg)
* [wrote-im-msg](#wrote-im-msg)
* [sending-im-msg](#sending-im-msg)
* [sent-im-msg](#sent-im-msg)
* [receiving-im-msg](#receiving-im-msg)
* [received-im-msg](#received-im-msg)
* [blocked-im-msg](#blocked-im-msg)
* [conversation-created](#conversation-created)
* [conversation-updated](#conversation-updated)
* [deleting-conversation](#deleting-conversation)
* [buddy-typing](#buddy-typing)
* [buddy-typing-stopped](#buddy-typing-stopped)
* [cleared-message-history](#cleared-message-history)
* [conversation-extended-menu](#conversation-extended-menu)

### Signal Details

#### writing-im-msg

```c
gboolean user_function(PurpleAccount *account,
                       const gchar *who,
                       gchar **message,
                       PurpleIMConversation *im,
                       PurpleMessageFlags flags,
                       gpointer user_data);
```

Emitted before a message is written in an IM conversation. If the message is changed, then the changed message is displayed and logged instead of the original message.

> Make sure to free `*message` before you replace it.

**Parameters:**

**account**
: The account.

**who**
: The name of the user.

**message**
: A pointer to the message.

**im**
: The IM conversation.

**flags**
: Flags for this message.

**user_data**
: user data set when the signal handler was connected.

**Returns:**
`TRUE` if the message should be canceled, or `FALSE` otherwise.

----

#### wrote-im-msg

```c
void user_function(PurpleAccount *account,
                   const gchar *who,
                   gchar *message,
                   PurpleIMConversation *im,
                   PurpleMessageFlags flags,
                   gpointer user_data);
```

Emitted after a message is written and possibly displayed in a conversation.

**Parameters:**

**account**
: The account.

**who**
: The name of the user.

**message**
: The message.

**im**
: The IM conversation.

**flags**
: Flags for this message.

**user_data**
: user data set when the signal handler was connected.

----

#### sending-im-msg

```c
void user_function(PurpleAccount *account,
                   const gchar *receiver,
                   gchar **message,
                   gpointer user_data);
```

Emitted before sending an IM to a user. `message` is a pointer to the message
string, so the plugin can replace the message before being sent.

> Make sure to free `*message` before you replace it!

**Parameters:**

**account**
: The account the message is being sent on.

**receiver**
: The username of the receiver.

**message**
: A pointer to the outgoing message. This can be modified.

**user_data**
: user data set when the signal handler was connected.

----

#### sent-im-msg

```c
void user_function(PurpleAccount *account,
                   const gchar *receiver,
                   const gchar *message,
                   gpointer user_data);
```

Emitted after sending an IM to a user.

**Parameters:**

**account**
: The account the message was sent on.

**receiver**
: The username of the receiver.

**message**
: The message that was sent.

**user_data**
: user data set when the signal handler was connected.

----

#### receiving-im-msg

```c
gboolean user_function(PurpleAccount *account,
                       gchar **sender,
                       gchar **message,
                       PurpleIMConversation *im,
                       PurpleMessageFlags *flags,
                       gpointer user_data)
```

Emitted when an IM is received. The callback can replace the name of the sender, the message, or the flags by modifying the pointer to the strings and integer. This can also be used to cancel a message by returning `TRUE`.

> Make sure to free `*sender` and `*message` before you replace them!

**Parameters:**

**account**
: The account the message was received on.

**sender**
: A pointer to the username of the sender.

**message**
: A pointer to the message that was sent.

**im**
: The IM conversation.

**flags**
: A pointer to the IM message flags.

**user_data**
: user data set when the signal handler was connected.

**Returns:**

`TRUE` if the message should be canceled, or `FALSE` otherwise.

----

#### received-im-msg

```c
void user_function(PurpleAccount *account,
                   gchar *sender,
                   gchar *message,
                   PurpleIMConversation *im,
                   PurpleMessageFlags flags,
                   gpointer user_data);
```

Emitted after an IM is received.

**Parameters:**

**account**
: The account the message was received on.

**sender**
: The username of the sender.

**message**
: The message that was sent.

**im**
: The IM conversation.

**flags**
: The IM message flags.

**user_data**
: user data set when the signal handler was connected.

----

#### blocked-im-msg

```c
void user_function(PurpleAccount *account,
                   const gchar *sender,
                   const gchar *message,
                   PurpleMessageFlags flags,
                   time_t when,
                   gpointer user_data);
```

Emitted after an IM is blocked due to privacy settings.

**Parameters:**

**account**
: The account the message was received on.

**sender**
: The username of the sender.

**message**
: The message that was blocked.

**flags**
: The IM message flags.

**when**
: The time the message was sent.

**user_data**
: user data set when the signal handler was connected.

----

#### conversation-created

```c
void user_function(PurpleConversation *conv, gpointer user_data);
```

Emitted when a new conversation is created.

**Parameters:**

**conv**
: The new conversation.

**user_data**
: user data set when the signal handler was connected.

----

#### conversation-updated

```c
void user_function(PurpleConversation *conv,
                   PurpleConvUpdateType type,
                   gpointer user_data);
```

Emitted when a conversation is updated.

**Parameters:**

**conv**
: The conversation that was updated.

**type**
: The type of update that was made.

**user_data**
: user data set when the signal handler was connected.

----

#### deleting-conversation

```c
void user_function(PurpleConversation *conv, gpointer user_data);
```

Emitted just before a conversation is to be destroyed.

**Parameters:**

**conv**
: The conversation that's about to be destroyed.

**user_data**
: user data set when the signal handler was connected.

----

#### buddy-typing

```c
void user_function(PurpleAccount *account,
                   const gchar *name,
                   gpointer user_data);
```

Emitted when a buddy starts typing in a conversation window.

**Parameters:**

**account**
: The account of the user which is typing.

**name**
: The name of the user which is typing.

**user_data**
: user data set when the signal handler was connected.

----

#### buddy-typing-stopped

```c
void user_function(PurpleAccount *account,
                   const gchar *name,
                   gpointer user_data);
```

Emitted when a buddy stops typing in a conversation window.

**Parameters:**

**account**
: The account of the user which stopped typing.

**name**
: The name of the user which stopped typing.

**user_data**
: user data set when the signal handler was connected.

----

#### conversation-extended-menu

```c
void user_function(PurpleConversation *conv,
                   GList **list,
                   gpointer user_data);
```

Emitted when the UI requests a list of plugin actions for a conversation.

**Parameters:**

**conv**
: The conversation.

**list**
: A pointer to the list of actions.

**user_data**
: user data set when the signal handler was connected.

----

#### cleared-message-history

```c
void user_function(PurpleConversation *conv, gpointer user_data);
```

Emitted when the conversation history is cleared.

**Parameters:**

**conv**
: The conversation.

**user_data**
: user data set when the signal handler was connected.
