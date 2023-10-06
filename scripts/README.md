This directory contains a number of files to help with the build.

## homebrew.ini

This file is meant to simplify the process of building on macOS against
Homebrew. To use it, you must first have all of the dependencies installed.
Note that this list may be out of date, if you discover something wrong, please
let us know!

```
brew install cmark gettext glib glib-networking gstreamer gtk4 gumbo-parser \
     help2man json-glib libadwaita libcanberra libidn libsoup@3 libxml2 \
     meson ninja pkg-config python@3
```

Once you have all of the dependencies, you can use this file when running
`meson setup` to deal with all the oddities that Homebrew adds. From the top
of the source directory you need to run the following command:

```
meson setup --native-file scripts/homebrew.ini build
```

You can replace build with whatever directory you want to use. After that runs
successfully, you can follow the normal build process.

