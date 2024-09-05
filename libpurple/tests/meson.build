PROGS = [
    'account_option',
    'account_manager',
    'authorization_request',
    'badge',
    'badges',
    'channel_join_details',
    'circular_buffer',
    'create_conversation_details',
    'contact',
    'contact_info',
    'contact_manager',
    'conversation',
    'conversation_manager',
    'conversation_member',
    'conversation_members',
    'credential_manager',
    'credential_provider',
    'file_transfer',
    'file_transfer_manager',
    'history_adapter',
    'history_manager',
    'idle_manager',
    'image',
    'keyvaluepair',
    'markup',
    'menu',
    'message',
    'messages',
    'notification',
    'notification_add_contact',
    'notification_authorization_request',
    'notification_manager',
    'person',
    'presence',
    'presence_manager',
    'protocol',
    'protocol_contacts',
    'protocol_conversation',
    'protocol_file_transfer',
    'protocol_roster',
    'purplepath',
    'request_field',
    'request_group',
    'request_page',
    'saved_presence',
    'str',
    'tags',
    'util',
    'whiteboard_manager',
    'xmlnode',
]

test_ui = static_library(
    'test-ui',
    'test_ui.c',
    'test_ui.h',
    c_args: [
        '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
        '-DG_LOG_USE_STRUCTURED',
        '-DG_LOG_DOMAIN="Purple-TestUI"',
    ],
    dependencies: [libpurple_dep, glib]
)

testenv.set('XDG_CONFIG_HOME', meson.current_build_dir() / 'config')
testenv.set('G_ENABLE_DIAGNOSTIC', '1')

foreach prog : PROGS
    e = executable(f'test_@prog@', f'test_@prog@.c',
                   c_args : [
                       '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
                       '-DTEST_CACHE_DIR="@0@/cache"'.format(meson.current_build_dir()),
                   ],
                   dependencies : [libpurple_dep, glib],
                   link_with: test_ui,
    )
    test(prog, e,
        env: testenv,
    )
endforeach

subdir('avatar')
subdir('sqlite3')
