#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h> // Include for multi-threading
#include "MQTTClient.h" // Include for MQTT client

#define BCM2711_PERI_BASE 0xFE000000
#define GPIO_BASE          (BCM2711_PERI_BASE + 0x200000)
#define PAGE_SIZE         (4*1024)
#define BLOCK_SIZE        (4*1024)

int mem_fd;
void *gpio_map;
volatile unsigned *gpio;

// Mutex for protecting access to GPIO and temperature
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Setup memory-mapped I/O access
void setup_io() {
    /* open /dev/mem */
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        printf("Can't open /dev/gpiomem: %s\n", strerror(errno));
        exit(-1);
    }

    gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
    close(mem_fd);

    if (gpio_map == MAP_FAILED) {
        printf("mmap error %d\n", (int)gpio_map);
        exit(-1);
    }

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

// Read CPU temperature
double read_cpu_temperature() {
    FILE *temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!temp_file) {
        printf("Error opening temperature file\n");
        return -1;
    }

    double temperature;
    fscanf(temp_file, "%lf", &temperature);
    fclose(temp_file);

    return temperature / 1000.0; // Convert millidegrees Celsius to degrees Celsius
}

// Callback function for reading GPIO
void on_read_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int pin = atoi(text);
    
    pthread_mutex_lock(&mutex); // Lock mutex before accessing GPIO
    int value = gpio_read(pin);
    pthread_mutex_unlock(&mutex); // Unlock mutex after accessing GPIO
    
    g_print("Value read from GPIO pin %d: %d\n", pin, value);
}

// Callback function for writing GPIO to true (high)
void on_true_radio_clicked(GtkToggleButton *button, gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int pin = atoi(text);

    if (gtk_toggle_button_get_active(button)) {
        pthread_mutex_lock(&mutex); // Lock mutex before accessing GPIO
        gpio_write(pin, 1); // Write true (high) to GPIO pin
        pthread_mutex_unlock(&mutex); // Unlock mutex after accessing GPIO
    }
    
    g_print("Value written to GPIO pin %d: %d\n", pin, 1);
}

// Callback function for writing GPIO to false (low)
void on_false_radio_clicked(GtkToggleButton *button, gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    int pin = atoi(text);

    if (gtk_toggle_button_get_active(button)) {
        pthread_mutex_lock(&mutex); // Lock mutex before accessing GPIO
        gpio_write(pin, 0); // Write false (low) to GPIO pin
        pthread_mutex_unlock(&mutex); // Unlock mutex after accessing GPIO
    }
    
    g_print("Value written to GPIO pin %d: %d\n", pin, 0);
}

// Callback function for activating the application
static void on_activate(GtkApplication *app, gpointer user_data);
void on_refresh_temperature_clicked(GtkButton *button, gpointer user_data);

int main(int argc, char *argv[]) {
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
    gtk_window_set_title(GTK_WINDOW(window), "GPIO Reader/Writer");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 300);

    // Create a vertical box layout container
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    // Create a label and entry for GPIO pin number
    GtkWidget *pin_label = gtk_label_new("Enter GPIO pin number:");
    GtkWidget *pin_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), pin_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), pin_entry, FALSE, FALSE, 0);

    // Create a button for reading GPIO
    GtkWidget *read_button = gtk_button_new_with_label("Read GPIO");
    g_signal_connect(read_button, "clicked", G_CALLBACK(on_read_button_clicked), pin_entry);
    gtk_box_pack_start(GTK_BOX(box), read_button, FALSE, FALSE, 0);

    // Create radio buttons for writing GPIO
    GtkWidget *write_label = gtk_label_new("Write GPIO:");
    GtkWidget *true_radio = gtk_radio_button_new_with_label(NULL, "True (High)");
    GtkWidget *false_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(true_radio), "False (Low)");
    gtk_box_pack_start(GTK_BOX(box), write_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), true_radio, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), false_radio, FALSE, FALSE, 0);

    // Connect radio buttons to signal handlers
    g_signal_connect(true_radio, "clicked", G_CALLBACK(on_true_radio_clicked), pin_entry);
    g_signal_connect(false_radio, "clicked", G_CALLBACK(on_false_radio_clicked), pin_entry);

    // Add a label for CPU temperature
    GtkWidget *temp_label = gtk_label_new("CPU Temperature:");
    gtk_box_pack_start(GTK_BOX(box), temp_label, FALSE, FALSE, 0);

    // Add a label to display CPU temperature
    GtkWidget *temp_value_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(box), temp_value_label, FALSE, FALSE, 0);

    // Create a button for refreshing CPU temperature
    GtkWidget *temp_button = gtk_button_new_with_label("Refresh Temperature");
    gtk_box_pack_start(GTK_BOX(box), temp_button, FALSE, FALSE, 0);

    // Connect the button to a signal handler for refreshing the temperature
    g_signal_connect(temp_button, "clicked", G_CALLBACK(on_refresh_temperature_clicked), temp_value_label);

    // Show all widgets
    gtk_widget_show_all(window);
}

// Callback function for refreshing CPU temperature
void on_refresh_temperature_clicked(GtkButton *button, gpointer user_data) {
    // Get the label for displaying the temperature
    GtkWidget *temp_label = GTK_WIDGET(user_data);
    
    // Read the CPU temperature
    double temperature = read_cpu_temperature();
    
    // Format the temperature string
    char temp_string[50];
    snprintf(temp_string, sizeof(temp_string), "CPU Temperature: %.1f°C", temperature);

    // Update the label with the new temperature
    gtk_label_set_text(GTK_LABEL(temp_label), temp_string);
}