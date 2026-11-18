// tools/preview/stubs/SPI.h — SPI ist in der Vorschau ein No-op.
#ifndef PREVIEW_SPI_H
#define PREVIEW_SPI_H

struct PreviewSPI {
  void begin(int = -1, int = -1, int = -1, int = -1) {}
  void end() {}
};
static PreviewSPI SPI;

#endif // PREVIEW_SPI_H
