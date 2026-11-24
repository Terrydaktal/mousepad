libmousepad_sources = [
  'mousepad-application.c',
  'mousepad-application.h',
  'mousepad-close-button.c',
  'mousepad-close-button.h',
  'mousepad-dialogs.c',
  'mousepad-dialogs.h',
  'mousepad-document.c',
  'mousepad-document.h',
  'mousepad-encoding-dialog.c',
  'mousepad-encoding-dialog.h',
  'mousepad-encoding.c',
  'mousepad-encoding.h',
  'mousepad-file.c',
  'mousepad-file.h',
  'mousepad-history.c',
  'mousepad-history.h',
  'mousepad-plugin-provider.c',
  'mousepad-plugin-provider.h',
  'mousepad-plugin.c',
  'mousepad-plugin.h',
  'mousepad-prefs-dialog.c',
  'mousepad-prefs-dialog.h',
  'mousepad-print.c',
  'mousepad-print.h',
  'mousepad-private.h',
  'mousepad-replace-dialog.c',
  'mousepad-replace-dialog.h',
  'mousepad-search-bar.c',
  'mousepad-search-bar.h',
  'mousepad-settings-store.c',
  'mousepad-settings-store.h',
  'mousepad-settings.c',
  'mousepad-settings.h',
  'mousepad-statusbar.c',
  'mousepad-statusbar.h',
  'mousepad-util.c',
  'mousepad-util.h',
  'mousepad-view.c',
  'mousepad-view.h',
  'mousepad-window.c',
  'mousepad-window.h',
]

libmousepad_sources += gnome.genmarshal(
  'mousepad-marshal',
  sources: 'mousepad-marshal.list',
  prefix: '_mousepad_marshal',
  internal: true,
  install_header: false,
)

libmousepad_sources += gnome.compile_resources(
  'mousepad-resources',
  'resources' / 'mousepad.gresource.xml',
  source_dir: 'resources',
)

mousepad_schemas = gnome.compile_schemas(
  depend_files: files('org.xfce.mousepad.gschema.xml'),
)

c_args = [
  '-DG_LOG_DOMAIN="@0@"'.format('Mousepad'),
  '-DMOUSEPAD_GSETTINGS_SCHEMA_DIR="@0@"'.format(gio_schemasdir),
  '-DMOUSEPAD_BUILD_GSETTINGS_SCHEMA_DIR="@0@"'.format(meson.current_build_dir()),
  '-DMOUSEPAD_PLUGIN_DIRECTORY="@0@"'.format(mousepad_plugin_directory),
]
if get_option('keyfile-settings')
  c_args += '-DMOUSEPAD_SETTINGS_KEYFILE_BACKEND=1'
endif

mousepad_so_version = '0'
mousepad_version = '@0@.0.0'.format(mousepad_so_version)

libmousepad = library(
  'mousepad',
  libmousepad_sources,
  sources: xfce_revision_h,
  version: mousepad_version,
  soversion: mousepad_so_version,
  c_args: c_args,
  link_depends: mousepad_schemas,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gmodule,
    gtk,
    gtksourceview,
    libm,
  ],
  install: true,
)

executable(
  'mousepad',
  [
    'main.c',
  ],
  sources: xfce_revision_h,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('Mousepad'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    libintl,
  ],
  link_with: [
    libmousepad,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

install_data('org.xfce.mousepad.gschema.xml', install_dir: gio_schemasdir)

xvfb_run = find_program('xvfb-run', required: false)
if xvfb_run.found()
  modified_tabs_test = executable(
    'test-modified-tabs',
    '../tests/test-modified-tabs.c',
    c_args: ['-UG_DISABLE_ASSERT', '-UG_DISABLE_CHECKS'],
    include_directories: include_directories('..'),
    dependencies: [glib, gtk, gtksourceview],
    link_with: libmousepad,
    build_by_default: false,
  )
  test(
    'modified-tabs',
    xvfb_run,
    args: ['-a', modified_tabs_test, meson.current_source_dir() / 'org.xfce.mousepad.gschema.xml'],
    env: ['GSETTINGS_BACKEND=memory', 'GDK_BACKEND=x11', 'NO_AT_BRIDGE=1'],
    is_parallel: false,
    timeout: 30,
  )
  open_files_present_test = executable(
    'test-open-files-present',
    '../tests/test-open-files-present.c',
    c_args: ['-UG_DISABLE_ASSERT', '-UG_DISABLE_CHECKS'],
    include_directories: include_directories('..'),
    dependencies: [glib, gtk, gtksourceview],
    link_with: libmousepad,
    build_by_default: false,
  )
  test(
    'open-files-present',
    xvfb_run,
    args: ['-a', open_files_present_test],
    env: ['GSETTINGS_BACKEND=memory', 'GDK_BACKEND=x11', 'NO_AT_BRIDGE=1'],
    is_parallel: false,
    timeout: 30,
  )
endif
