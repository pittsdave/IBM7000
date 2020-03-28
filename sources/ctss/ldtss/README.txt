To Build the loader tape:

1. Load the loadts.img into CTSS:

$ setupctss ../sources/ctss/ldtss/loadts.img 

2. Load the pusav.img into CTSS:

$ setupctss ../sources/ctss/util/pusav.img 

2. Login into CTSS and run the PUSAV program to generate the
   ABS format file:

pusav loader

3. Extract the ABS format image as a tape:

$ extractctss B loader abs m1416 pitts loader.tap
