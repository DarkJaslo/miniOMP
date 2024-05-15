// Type declaration for single work sharing descriptor
typedef struct {
    int value;
} miniomp_single_t;

// Declaration of global variable for single work descriptor
extern miniomp_single_t miniomp_single;

// Functions implemented in this module
bool GOMP_single_start (void);
