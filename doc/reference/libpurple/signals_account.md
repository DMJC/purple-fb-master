Title: Account Signals
Slug: account-signals

## Account Signals

### Signal List

* [account-signed-on](#account-signed-on)
* [account-signed-off](#account-signed-off)

### Signal Details

#### account-created

```c
void user_function(PurpleAccount *account, gpointer user_data);
```

Emitted when an account is created by calling purple_account_new.

**Parameters:**

**account**
: The account.

**user_data**
: User data set when the signal handler was connected.

----

#### account-destroying

```c
void user_function(PurpleAccount *account, gpointer user_data);
```

Emitted when an account is about to be destroyed.

**Parameters:**

**account**
: The account.

**user_data**
: User data set when the signal handler was connected.

----

#### account-signed-on

```c
void user_function(PurpleAccount *account, gpointer user_data);
```

Emitted when an account has signed on.

**Parameters:**

**account**
: The account that has signed on.

**user_data**
: User data set when the signal handler was connected.

----

#### account-signed-off

```c
void user_function(PurpleAccount *account, gpointer user_data);
```

Emitted when an account has signed off.

**Parameters:**

**account**
: The account that has signed off.

**user_data**
: User data set when the signal handler was connected.
