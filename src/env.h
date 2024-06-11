// Type declaration for Internal Control Variables (ICV) structure
typedef struct {
  int nthreads_var;
  int parallel_threads; //Current threads in *the* parallel region
} miniomp_icv_t;

// Global variable storing the ICV (internal control variables) supported in our implementation
extern miniomp_icv_t miniomp_icv;

// Functions implemented in this module
void parse_env(void);
