Title: Buddy List Signals
Slug: buddy-list-signals

## Buddy List Signals

### Signal List

* [update-idle](#update-idle)
* [blist-node-extended-menu](#blist-node-extended-menu)
* [blist-node-aliased](#blist-node-aliased)
* [ui-caps-changed](#ui-caps-changed)

### Signal Details

#### update-idle

```c
void user_function(gpointer user_data);
```

Emitted when the buddy list is refreshed and the idle times are updated.

**Parameters:**

**user_data**
: user data set when the signal handler was connected.

----

#### blist-node-extended-menu

```c
void user_function(PurpleBlistNode *node, GList **menu, gpointer user_data);
```

Emitted when a buddylist menu is being constructed `menu` is a pointer to a
GList of PurpleMenuAction's allowing a plugin to add menu items.

----

#### blist-node-added

```c
void user_function(PurpleBlistNode *node, gpointer user_data);
```

Emitted when a new blist node is added to the buddy list.

----

#### blist-node-removed

```c
void user_function(PurpleBlistNode *node, gpointer user_data);
```

Emitted when a blist node is removed from the buddy list.

----

#### blist-node-aliased

```c
void user_function(PurpleBlistNode *node,
                   const gchar *old_alias,
                   gpointer user_data);
```

Emitted when a blist node (buddy, chat, or contact) is aliased.

----

#### ui-caps-changed

```c
void user_function(PurpleMediaCaps newcaps,
                   PurpleMediaCaps oldcaps,
                   gpointer user_data);
```

Emitted when updating the media capabilities of the UI.

**Parameters:**

**newcaps**
: .

**oldcaps**
: .

**user_data**
: user data set when the signal handler was connected.
