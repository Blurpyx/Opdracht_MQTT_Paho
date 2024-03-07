#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

// Memory-mapped I/O access
#define BCM2711_PERI_BASE 0xFE000000
#define GPIO_BASE          (BCM2711_PERI_BASE + 0x200000)
#define PAGE_SIZE         (4*1024)
#define BLOCK_SIZE        (4*1024)

int mem_fd;
void *gpio_map;
volatile unsigned *gpio;

// Set up a memory region to access GPIO
void setup_io() {
    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("Can't open /dev/mem: %s\n", strerror(errno));
        exit(-1);
    }

    /* mmap GPIO */
    gpio_map = mmap(
        NULL,                 // Any address in our space will do
        BLOCK_SIZE,           // Map length
        PROT_READ|PROT_WRITE,// Enable reading & writing to mapped memory
        MAP_SHARED,           // Shared with other processes
        mem_fd,               // File to map
        GPIO_BASE             // Offset to GPIO peripheral
    );

    close(mem_fd); // No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED) {
        printf("mmap error %d\n", (int)gpio_map);
        exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
}

// GPIO read function
int gpio_read(int pin) {
    return (*(gpio + 13) & (1 << pin)) ? 1 : 0;
}

// Callback function for button click
void on_button_clicked(GtkButton *button, gpointer user_data) {
    // Get the entry widget
    GtkWidget *entry = GTK_WIDGET(user_data);

    // Get the text from the entry widget
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

    // Convert the text to an integer (GPIO pin number)
    int pin = atoi(text);

    // Read the value from GPIO pin
    int value = gpio_read(pin);
    g_print("Value read from GPIO pin %d: %d\n", pin, value);
}

// Callback function for activating the application
static void on_activate(GtkApplication *app, gpointer user_data);

int main(int argc, char *argv[]) {
    // Setup GPIO
    setup_io();

    // Initialize GTK
    GtkApplication *app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

// Implementation of the callback function for activating the application
static void on_activate(GtkApplication *app, gpointer user_data) {
    // Create a window
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "GPIO Reader");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 150);

    // Create a vertical box layout container
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    // Create a label
    GtkWidget *label = gtk_label_new("Enter GPIO pin number:");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    // Create a text entry field
    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    // Create a button
    GtkWidget *button = gtk_button_new_with_label("Read GPIO");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), entry);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    // Show all widgets
    gtk_widget_show_all(window);
}
