#include <gtk/gtk.h>  


/* 
**  Test code to destroy comboboxtext widget - seems to be an issue at ubuntu 18.04
**
**  To compile: cc -o test_cbox_destroy test_cbox_destroy.c `pkg-config --cflags gtk+-3.0 --libs gtk+-3.0`
*/




/* Functions */

static void print_hello ()
{
    g_print ("Hello World -- test destroy cbox\n");
}

static void OnClearCbox (GtkWidget *widget, gpointer data)
{
    g_print ("Clearing cbox\n");
    GtkWidget *cbox = (GtkWidget *) data;
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (cbox));
}

static void OnDestroyCbox (GtkWidget *widget, gpointer data)
{
    g_print ("Destroy cbox\n");
    GtkWidget *cbox = (GtkWidget *) data;
    gtk_widget_destroy (cbox);
}

static void OnReShow (GtkWidget *widget, gpointer data)
{
    g_print ("ReShow\n");
    GtkWidget *window = (GtkWidget *) data;
    gtk_widget_show_all (window);
}



/* Main */

int main (int   argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *clr_btn, *dest_btn, *show_btn, *quit_btn;
    GtkWidget *cbox;

    /* This is called in all GTK applications. */
    gtk_init (&argc, &argv);

    print_hello();

    /* Create a new window, and set its title */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Cbox destroy");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new ();

    /* Pack the container in the window */
    gtk_container_add (GTK_CONTAINER (window), grid);

    /* Add cbox */
    cbox = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cbox), "1", "test 1");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cbox), "2", "test 2");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT (cbox), "3", "test 3");
    gtk_grid_attach (GTK_GRID (grid), cbox, 0, 4, 4, 1);

    /* Clear button */
    clr_btn = gtk_button_new_with_label ("Empty Cbox");
    g_signal_connect (clr_btn, "clicked", G_CALLBACK (OnClearCbox), cbox);
    gtk_grid_attach (GTK_GRID (grid), clr_btn, 0, 0, 1, 1);

    /* Destroy button */
    dest_btn = gtk_button_new_with_label ("Destroy Cbox");
    g_signal_connect (dest_btn, "clicked", G_CALLBACK (OnDestroyCbox), cbox);
    gtk_grid_attach (GTK_GRID (grid), dest_btn, 1, 0, 1, 1);

    /* Show button */
    show_btn = gtk_button_new_with_label ("Re-show Window");
    g_signal_connect (show_btn, "clicked", G_CALLBACK (OnReShow), window);
    gtk_grid_attach (GTK_GRID (grid), show_btn, 2, 0, 1, 1);

    /* Quit */
    quit_btn = gtk_button_new_with_label ("Quit");
    g_signal_connect (quit_btn, "clicked", G_CALLBACK (gtk_main_quit), NULL);
    gtk_grid_attach (GTK_GRID (grid), quit_btn, 3, 0, 1, 1);

    /* Show and start */
    gtk_widget_show_all (window);
    gtk_main ();

    return 0;
}
