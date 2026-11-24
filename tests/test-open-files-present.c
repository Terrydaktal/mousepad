/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mousepad/mousepad-application.h"
#include "mousepad/mousepad-document.h"
#include "mousepad/mousepad-encoding.h"
#include "mousepad/mousepad-window.h"


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


static GtkNotebook *
find_notebook (GtkWidget *widget)
{
  GList *children;
  GtkNotebook *notebook = NULL;

  if (GTK_IS_NOTEBOOK (widget))
    return GTK_NOTEBOOK (widget);
  if (!GTK_IS_CONTAINER (widget))
    return NULL;

  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (GList *item = children; item != NULL && notebook == NULL; item = item->next)
    notebook = find_notebook (GTK_WIDGET (item->data));
  g_list_free (children);
  return notebook;
}


int
main (int argc,
      char **argv)
{
  GApplication *application;
  GtkWidget *window, *empty_window;
  GtkNotebook *notebook;
  MousepadDocument *untitled;
  GError *error = NULL;
  GFile *files[1];
  gchar *tmpdir, *filepath;
  gint opened;

  g_assert_cmpstr (g_getenv ("GSETTINGS_BACKEND"), ==, "memory");
  tmpdir = g_dir_make_tmp ("mousepad-open-files-XXXXXX", &error);
  g_assert_no_error (error);
  g_assert_true (g_setenv ("XDG_CONFIG_HOME", tmpdir, TRUE));
  g_assert_true (g_setenv ("XDG_DATA_HOME", tmpdir, TRUE));

  filepath = g_build_filename (tmpdir, "sample.txt", NULL);
  g_assert_true (g_file_set_contents (filepath, "hello world\n", -1, &error));
  g_assert_no_error (error);
  files[0] = g_file_new_for_path (filepath);

  gtk_init (&argc, &argv);
  application = g_object_new (MOUSEPAD_TYPE_APPLICATION,
                              "application-id", "org.xfce.mousepad",
                              "flags", G_APPLICATION_NON_UNIQUE, NULL);
  g_assert_true (g_application_register (application, NULL, &error));
  g_assert_no_error (error);

  /* Create a window containing an unmodified untitled active tab. */
  window = mousepad_window_new (MOUSEPAD_APPLICATION (application));
  untitled = mousepad_document_new ();
  mousepad_window_add (MOUSEPAD_WINDOW (window), untitled);
  notebook = find_notebook (window);
  g_assert_nonnull (notebook);
  g_assert_cmpint (gtk_notebook_get_n_pages (notebook), ==, 1);

  /* Opening a new file replaces the unmodified untitled tab (tab count stays 1),
   * but mousepad_window_open_files() must still report 1 opened document so
   * mousepad_application_open() calls gtk_window_present(). */
  opened = mousepad_window_open_files (MOUSEPAD_WINDOW (window), files, 1,
                                       MOUSEPAD_ENCODING_UTF_8, 0, 0, FALSE);
  g_assert_cmpint (gtk_notebook_get_n_pages (notebook), ==, 1);
  g_assert_cmpint (opened, ==, 1);

  /* Re-opening an already open file presents its existing window directly and
   * must return 0 so callers do not focus a different window. */
  opened = mousepad_window_open_files (MOUSEPAD_WINDOW (window), files, 1,
                                       MOUSEPAD_ENCODING_UTF_8, 0, 0, FALSE);
  g_assert_cmpint (opened, ==, 0);

  /* Attempting to open an already open file into a brand-new empty window must
   * still return -1 so mousepad_application_open() destroys the empty window. */
  empty_window = mousepad_window_new (MOUSEPAD_APPLICATION (application));
  opened = mousepad_window_open_files (MOUSEPAD_WINDOW (empty_window), files, 1,
                                       MOUSEPAD_ENCODING_UTF_8, 0, 0, FALSE);
  g_assert_cmpint (opened, ==, -1);

  gtk_widget_destroy (empty_window);
  gtk_widget_destroy (window);
  g_object_unref (files[0]);
  g_object_unref (application);
  g_free (filepath);
  remove_test_directory (tmpdir);
  g_free (tmpdir);
  return 0;
}
