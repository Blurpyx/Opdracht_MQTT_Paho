#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

// Define GPIO pins
#define GPIO_IN_PIN 4
#define GPIO_OUT_PIN 17

// Memory-mapped I/O access
#define BCM2711_PERI_BASE 0xFE000000
#define GPIO_BASE          (BCM2711_PERI_BASE + 0x200000)
#define PAGE_SIZE         (4*1024)
#define BLOCK_SIZE        (4*1024)

int mem_fd;
void *gpio_map;
volatile unsigned *gpio;

// Set up a memory regions to access GPIO
void setup_io() {
    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("Can't open /dev/mem: %s\n", strerror(errno));
        exit(-1);
    }

    /* mmap GPIO */
    gpio_map = mmap(
        NULL,                 // Any adddress in our space will do
        BLOCK_SIZE,           // Map length
        PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
        MAP_SHARED,           // Shared with other processes
        mem_fd,               // File to map
        GPIO_BASE             // Offset to GPIO peripheral
    );

    close(mem_fd); // No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED) {
        printf("mmap error %d\n", (uintptr_t)gpio_map);
        exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
}

// GPIO read function
int gpio_read(int pin) {
    return (*(gpio + 13) & (1 << pin)) ? 1 : 0;
}

// GPIO write function
void gpio_write(int pin, int value) {
    if (value)
        *(gpio + 7) = 1 << pin;
    else
        *(gpio + 10) = 1 << pin;
}

// GTK callback function for button click
void on_button_clicked(GtkButton *button, gpointer user_data) {
    int pin = GPOINTER_TO_INT(user_data);
    int value = gpio_read(pin);
    g_print("Value read from GPIO pin %d: %d\n", pin, value);
}

// Callback function for activating the application
static void on_activate(GtkApplication *app, gpointer user_data);

int main(int argc, char *argv[]) {
    // Setup GPIO
    setup_io();

    // Initialize GTK
    GtkApplication *app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

// Implementation of the callback function for activating the application
static void on_activate(GtkApplication *app, gpointer user_data) {
    // Create a window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GPIO Reader");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 100);

    // Create a button
    GtkWidget *button = gtk_button_new_with_label("Read GPIO");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), GINT_TO_POINTER(GPIO_IN_PIN));

    // Add button to window
    gtk_window_set_child(GTK_WINDOW(window), button);

    // Show window
    gtk_widget_show(window);
}
