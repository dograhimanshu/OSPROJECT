#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
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
    sem_t sem;
} OrderManagementSystem;

// Add a product to the inventory
int addProductToInventory(OrderManagementSystem* system, const char* name, float price, int quantity) {
    pthread_mutex_lock(&system->mtx);
    printf("Adding product to inventory: %s, price: %.2f, quantity: %d\n", name, price, quantity);
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
                        printf("Product %s successfully deducted from inventory. Remaining quantity: %d\n", product_name, system->inventory[j].quantity);
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
}

// Place an order
void placeOrder(OrderManagementSystem* system, Order* order) {
    sem_wait(&system->sem);
    printf("Placing order ID %d for Customer: %s\n", system->next_order_id, order->customer_name);
    pthread_mutex_lock(&system->mtx);
    order->order_id = system->next_order_id++;
    system->orders[order->order_id].order = *order; // Store order in orders array
    pthread_mutex_unlock(&system->mtx);
    processOrder(system, order);
    sem_post(&system->sem);
    printf("Order ID %d placed successfully.\n", order->order_id);
}

// User interface to place an order
void userPlaceOrder(OrderManagementSystem* system) {
    Order order;
    char customer_name[256];
    printf("Enter customer name: ");
    scanf("%s", customer_name);
    strcpy(order.customer_name, customer_name);

    printf("Enter the number of products to order (max 10): ");
    int num_products;
    scanf("%d", &num_products);
    if (num_products > 10) num_products = 10;

    for (int i = 0; i < num_products; i++) {
        char product_name[256];
        int quantity;
        printf("Enter product name %d: ", i + 1);
        scanf("%s", product_name);
        printf("Enter quantity for %s: ", product_name);
        scanf("%d", &quantity);

        strcpy(order.selected_products[i].product_name, product_name);
        order.selected_products[i].quantity = quantity;
    }

    // Fill the remaining products with zero quantities
    for (int i = num_products; i < 10; i++) {
        order.selected_products[i].quantity = 0;
    }

    placeOrder(system, &order);
}

// Main function
int main() {
    printf("Initializing order management system...\n");
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
    if (sem_init(&system->sem, 0, 1) != 0) {
        perror("Error: Semaphore initialization failed");
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

    // Loop to allow multiple users to place orders
    int num_users;
    printf("Enter the number of users: ");
    scanf("%d", &num_users);

    for (int i = 0; i < num_users; i++) {
        printf("\nUser %d\n", i + 1);
        userPlaceOrder(system);
    }

    // Display updated inventory after orders
    displayInventory(system);

    // Clean up
    pthread_mutex_destroy(&system->mtx);
    sem_destroy(&system->sem);
    free(system);

    printf("Order management system terminated.\n");
    return 0;
}
