

### Debugging (lowest priority, only affects working on SIFT but not in field)

1. --skip-image-indices doesn't work in --main-mission, only --main-mission-interactive
2. --skip-image-indices may not actually skip the last index (i.e. if given  --skip-image-indices 6 10 then it may not skip image index 9)
