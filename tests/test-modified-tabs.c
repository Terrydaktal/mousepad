/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mousepad/mousepad-application.h"
#include "mousepad/mousepad-document.h"
#include "mousepad/mousepad-settings.h"


static gchar *
cache_older_installed_schema (const gchar *schema_xml)
{
  const gchar *custom_keys[] = {
    "highlight-modified-tabs", "ctrl-d-detach-tab", "middle-click-paste"
  };
  GError *error = NULL;
  GSettingsSchema *schema;
  gchar *directory, *filename, *contents, *stderr_text = NULL;
  gint status;

  directory = g_dir_make_tmp ("mousepad-modified-tabs-XXXXXX", &error);
  g_assert_no_error (error);
  g_assert_true (g_file_get_contents (schema_xml, &contents, NULL, &error));
  g_assert_no_error (error);

  for (guint i = 0; i < G_N_ELEMENTS (custom_keys); i++)
    {
      gchar *pattern, *without_key;
      GRegex *regex;

      pattern = g_strdup_printf ("<key name=\"%s\"[^>]*>.*?</key>\\s*", custom_keys[i]);
      regex = g_regex_new (pattern, G_REGEX_DOTALL, 0, &error);
      g_assert_no_error (error);
      g_assert_true (g_regex_match (regex, contents, 0, NULL));
      without_key = g_regex_replace (regex, contents, -1, 0, "", 0, &error);
      g_assert_no_error (error);
      g_assert_false (g_regex_match (regex, without_key, 0, NULL));
      g_free (contents);
      contents = without_key;
      g_regex_unref (regex);
      g_free (pattern);
    }

  filename = g_build_filename (directory, "org.xfce.mousepad.gschema.xml", NULL);
  g_assert_true (g_file_set_contents (filename, contents, -1, &error));
  g_assert_no_error (error);
  g_free (contents);
  g_free (filename);

  {
    gchar *command[] = { "glib-compile-schemas", "--strict", directory, NULL };
    g_assert_true (g_spawn_sync (NULL, command, NULL, G_SPAWN_SEARCH_PATH,
                                NULL, NULL, NULL, &stderr_text, &status, &error));
    g_assert_no_error (error);
  }
  if (!g_spawn_check_exit_status (status, &error))
    g_error ("Cannot compile older schema fixture: %s: %s", error->message, stderr_text);
  g_free (stderr_text);

  g_assert_true (g_setenv ("GSETTINGS_SCHEMA_DIR", directory, TRUE));
  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                             "org.xfce.mousepad.preferences.window", TRUE);
  g_assert_nonnull (schema);
  g_assert_false (g_settings_schema_has_key (schema, "highlight-modified-tabs"));
  g_settings_schema_unref (schema);

  return directory;
}


static void
remove_test_directory (const gchar *directory)
{
  GDir *dir = g_dir_open (directory, 0, NULL);
  const gchar *name;

  g_assert_nonnull (dir);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      gchar *filename = g_build_filename (directory, name, NULL);
      if (!g_file_test (filename, G_FILE_TEST_IS_SYMLINK)
          && g_file_test (filename, G_FILE_TEST_IS_DIR))
        remove_test_directory (filename);
      else
        g_assert_cmpint (g_unlink (filename), ==, 0);
      g_free (filename);
    }
  g_dir_close (dir);
  g_assert_cmpint (g_rmdir (directory), ==, 0);
}


static GtkWidget *
find_tab_label (GtkWidget *widget)
{
  GList *children;
  GtkWidget *label = NULL;

  if (GTK_IS_LABEL (widget))
    return widget;
  if (!GTK_IS_CONTAINER (widget))
    return NULL;

  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (GList *item = children; item != NULL && label == NULL; item = item->next)
    label = find_tab_label (GTK_WIDGET (item->data));
  g_list_free (children);
  return label;
}


static void
assert_label_color (GtkWidget *label,
                    GtkStateFlags state,
                    gboolean red)
{
  GtkStyleContext *context = gtk_widget_get_style_context (label);
  GdkRGBA color;
  gboolean is_red;

  while (g_main_context_iteration (NULL, FALSE))
    ;
  gtk_style_context_get_color (context, state, &color);
  is_red = color.red > 0.99 && color.green < 0.01 && color.blue < 0.01;
  g_print ("state=%u color=(%.3f,%.3f,%.3f) modified-class=%d expected-red=%d\n",
           (guint) state, color.red, color.green, color.blue,
           gtk_style_context_has_class (context, "modified"), red);
  g_assert_cmpint (gtk_style_context_has_class (context, "modified"), ==, red);
  g_assert_cmpint (is_red, ==, red);
}


int
main (int argc,
      char **argv)
{
  GApplication *application;
  MousepadDocument *document;
  GtkWidget *window, *notebook, *tab, *label;
  gchar *fixture;

  g_assert_cmpint (argc, ==, 2);
  g_assert_cmpstr (g_getenv ("GSETTINGS_BACKEND"), ==, "memory");
  fixture = cache_older_installed_schema (argv[1]);
  g_assert_true (g_setenv ("XDG_CONFIG_HOME", fixture, TRUE));

  gtk_init (&argc, &argv);
  g_object_set (gtk_settings_get_default (), "gtk-theme-name", "Adwaita",
                 "gtk-enable-animations", FALSE, NULL);
  application = g_object_new (MOUSEPAD_TYPE_APPLICATION,
                              "application-id", "org.test.MousepadModifiedTabs",
                              "flags", G_APPLICATION_NON_UNIQUE, NULL);
  g_application_set_default (application);

  /* The build's schema must work even when GTK has cached an older schema source. */
  g_assert_true (mousepad_setting_get_boolean (MOUSEPAD_SETTING_HIGHLIGHT_MODIFIED_TABS));
  g_assert_cmpuint (mousepad_setting_get_uint (MOUSEPAD_SETTING_TAB_WIDTH), >, 0);
  g_assert_false (mousepad_setting_get_boolean (MOUSEPAD_SETTING_CTRL_D_DETACH_TAB));
  g_assert_false (mousepad_setting_get_boolean (MOUSEPAD_SETTING_MIDDLE_CLICK_PASTE));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (window), notebook);
  document = mousepad_document_new ();
  tab = mousepad_document_get_tab_label (document);
  label = find_tab_label (tab);
  g_assert_nonnull (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (document), tab);
  gtk_widget_show_all (window);

  assert_label_color (label, GTK_STATE_FLAG_NORMAL, FALSE);
  gtk_text_buffer_set_text (document->buffer, "Synthetic unsaved test text", -1);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, TRUE);
  assert_label_color (label, GTK_STATE_FLAG_SELECTED, TRUE);
  assert_label_color (label, GTK_STATE_FLAG_BACKDROP, TRUE);
  gtk_text_buffer_set_modified (document->buffer, FALSE);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, FALSE);
  gtk_text_buffer_set_modified (document->buffer, TRUE);

  mousepad_setting_set_boolean (MOUSEPAD_SETTING_HIGHLIGHT_MODIFIED_TABS, FALSE);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, FALSE);
  mousepad_setting_set_boolean (MOUSEPAD_SETTING_HIGHLIGHT_MODIFIED_TABS, TRUE);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, TRUE);

  /* Moving a document recreates its label while retaining the document itself. */
  g_object_ref (document);
  gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), GTK_WIDGET (document));
  tab = mousepad_document_get_tab_label (document);
  label = find_tab_label (tab);
  g_assert_nonnull (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (document), tab);
  g_object_unref (document);
  gtk_widget_show_all (window);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, TRUE);
  assert_label_color (label, GTK_STATE_FLAG_SELECTED | GTK_STATE_FLAG_BACKDROP, TRUE);
  mousepad_setting_set_boolean (MOUSEPAD_SETTING_HIGHLIGHT_MODIFIED_TABS, FALSE);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, FALSE);
  mousepad_setting_set_boolean (MOUSEPAD_SETTING_HIGHLIGHT_MODIFIED_TABS, TRUE);
  assert_label_color (label, GTK_STATE_FLAG_NORMAL, TRUE);

  gtk_widget_destroy (window);
  g_object_unref (application);

  remove_test_directory (fixture);
  g_free (fixture);
  return 0;
}
