// Type declaration for single work sharing descriptor
typedef struct {
    // complete the definition of single descriptor
    int value;
} miniomp_single_t;

// Declaration of global variable for single work descriptor
extern miniomp_single_t miniomp_single;

// Stores per-thread how many singles we have entered (for sync purposes)
extern pthread_key_t miniomp_single_key;

// Functions implemented in this module
bool GOMP_single_start (void);
