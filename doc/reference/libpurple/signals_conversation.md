Title: Conversation Signals
Slug: conversation-signals

## Conversation Signals

### Signal List

* [conversation-created](#conversation-created)
* [conversation-updated](#conversation-updated)
* [deleting-conversation](#deleting-conversation)
* [cleared-message-history](#cleared-message-history)
* [conversation-extended-menu](#conversation-extended-menu)

### Signal Details

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
