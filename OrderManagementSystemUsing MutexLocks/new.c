#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

// Define structures for Product and Order
typedef struct {
    char name[256];
    float price;
    int quantity;
} Product;

typedef struct {
    int order_id;
    char customer_name[256];
    struct {
        char product_name[256];
        int quantity;
    } selected_products[10];
} Order;

// Define the OrderManagementSystem class
typedef struct {
    int next_order_id;
    struct {
        int order_id;
        Order order;
    } orders[100];
    struct {
        char name[256];
        Product product;
        int quantity; // Add quantity to inventory
    } inventory[100];
    pthread_mutex_t mtx;
} OrderManagementSystem;

// Add a product to the inventory
int addProductToInventory(OrderManagementSystem* system, const char* name, float price, int quantity) {
    pthread_mutex_lock(&system->mtx);
    Product product;
    strcpy(product.name, name);
    product.price = price;
    product.quantity = quantity;
    int added = 0;
    for (int i = 0; i < 100; i++) {
        if (system->inventory[i].quantity == 0) {
            strcpy(system->inventory[i].name, name);
            system->inventory[i].product = product;
            system->inventory[i].quantity = quantity; // Set inventory quantity
            added = 1;
            break;
        }
    }
    pthread_mutex_unlock(&system->mtx);
    return added ? 0 : -1; // Return 0 on success, -1 on failure
}

// Display the current inventory
void displayInventory(OrderManagementSystem* system) {
    pthread_mutex_lock(&system->mtx);
    printf("Current Inventory:\n");
    for (int i = 0; i < 100; i++) {
        if (system->inventory[i].quantity > 0) {
            printf("Name: %s, Price: %.2f, Quantity: %d\n", system->inventory[i].name, system->inventory[i].product.price, system->inventory[i].quantity);
        }
    }
    pthread_mutex_unlock(&system->mtx);
}

// Process an order
void processOrder(OrderManagementSystem* system, Order* order) {
    pthread_mutex_lock(&system->mtx);
    printf("Processing Order ID %d for Customer: %s\n", order->order_id, order->customer_name);
    for (int i = 0; i < 10; i++) {
        if (order->selected_products[i].quantity > 0) {
            char product_name[256];
            strcpy(product_name, order->selected_products[i].product_name);
            int quantity = order->selected_products[i].quantity;
            int found = 0;
            for (int j = 0; j < 100; j++) {
                if (strcmp(system->inventory[j].name, product_name) == 0) {
                    if (system->inventory[j].quantity >= quantity) {
                        system->inventory[j].quantity -= quantity;
                        printf("Product %s successfully deducted from inventory.\n", product_name);
                    } else {
                        printf("Insufficient quantity of product %s in inventory.\n", product_name);
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                printf("Product %s not found in inventory.\n", product_name);
            }
        }
    }
    pthread_mutex_unlock(&system->mtx);
}

// Place an order
void placeOrder(OrderManagementSystem* system, Order* order) {
    pthread_mutex_lock(&system->mtx);
    order->order_id = system->next_order_id++;
    system->orders[order->order_id].order = *order; // Store order in orders array
    processOrder(system, order);
    pthread_mutex_unlock(&system->mtx);
}

// Simulate a user
void* simulateUser(void* arg) {
    OrderManagementSystem* system = (OrderManagementSystem*)arg;
    char customer_name[256];
    sprintf(customer_name, "User%d", system->next_order_id);
    Order order;
    order.order_id = 0;
    strcpy(order.customer_name, customer_name);
    for (int i = 0; i < 10; i++) {
        sprintf(order.selected_products[i].product_name, "Product%d", i + 1);
        order.selected_products[i].quantity = rand() % 5 + 1;
    }
    placeOrder(system, &order);
    return NULL;
}

// Start the simulation
void startSimulation(OrderManagementSystem* system) {
    pthread_t user_threads[10]; // Increase the number of user threads
    for (int i = 0; i < 10; i++) { // Create 10 user threads
        if (pthread_create(&user_threads[i], NULL, simulateUser, system) != 0) {
            fprintf(stderr, "Error: Failed to create user thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < 10; i++) {
        if (pthread_join(user_threads[i], NULL) != 0) {
            fprintf(stderr, "Error: Failed to join user thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    OrderManagementSystem* system = (OrderManagementSystem*)malloc(sizeof(OrderManagementSystem));
    if (system == NULL) {
        perror("Error: Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    system->next_order_id = 1;
    if (pthread_mutex_init(&system->mtx, NULL) != 0) {
        perror("Error: Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    // Add products to inventory
    if (addProductToInventory(system, "Phone", 500.0, 10) != 0 ||
        addProductToInventory(system, "Laptop", 1200.0, 5) != 0 ||
        addProductToInventory(system, "Tablet", 300.0, 15) != 0) {
        fprintf(stderr, "Error: Failed to add products to inventory\n");
        exit(EXIT_FAILURE);
    }

    // Display current inventory
    displayInventory(system);

    // Start the simulation
    startSimulation(system);

    // Display updated inventory after orders
    displayInventory(system);

    free(system); // Free allocated memory
    return 0;
}