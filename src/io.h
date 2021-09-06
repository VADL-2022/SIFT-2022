#ifdef __cplusplus
extern "C" {
#endif
  
int forEachInDir(const char* dir, void(*process)(const char* filename));

#ifdef __cplusplus
}
#endif
