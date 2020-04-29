#include <stdio.h>
#include <stdlib.h>
#include <libimagequant.h>
#include <lcdfgif/gif.h>
#include <gifsicle.h>
#include <emscripten.h>
#include <string.h>

typedef struct encoder
{
  liq_attr *attr;
  Gif_Stream *stream;
  int width;
  int height;
} encoder;

EMSCRIPTEN_KEEPALIVE
encoder *encoder_new(int width, int height)
{
  Gif_Stream *stream = Gif_NewStream();
  stream->screen_width = width;
  stream->screen_height = height;
  stream->loopcount = 0;

  encoder *result = malloc(sizeof(encoder));
  result->attr = liq_attr_create();
  result->stream = stream;
  result->width = width;
  result->height = height;
  return result;
}

inline Gif_Colormap *create_colormap_from_palette(const liq_palette *palette)
{
  Gif_Colormap *colormap = Gif_NewFullColormap(palette->count, palette->count);

  for (int i = 0; i < palette->count; i++)
  {
    liq_color color = palette->entries[i];
    colormap->col[i].pixel = 256;
    colormap->col[i].gfc_red = color.r;
    colormap->col[i].gfc_green = color.g;
    colormap->col[i].gfc_blue = color.b;
    colormap->col[i].haspixel = 1;
  }

  return colormap;
}

EMSCRIPTEN_KEEPALIVE
void encoder_add_frame(encoder *enc, void *image_data, int delay)
{
  liq_image *raw_image = liq_image_create_rgba(enc->attr, image_data, enc->width, enc->height, 0);
  liq_result *res = liq_quantize_image(enc->attr, raw_image);
  const liq_palette *palette = liq_get_palette(res);

  Gif_Image *image = Gif_NewImage();
  image->width = enc->width;
  image->height = enc->height;
  image->delay = delay;
  image->local = create_colormap_from_palette(palette);

  Gif_CreateUncompressedImage(image, 0);
  liq_write_remapped_image(res, raw_image, image->image_data, enc->width * enc->height);
  Gif_AddImage(enc->stream, image);

  liq_result_destroy(res);
  liq_image_destroy(raw_image);
}

EMSCRIPTEN_KEEPALIVE
void encoder_encode(encoder *enc)
{
  liq_attr_destroy(enc->attr);

  Gif_CompressInfo info;
  Gif_InitCompressInfo(&info);
  info.loss = 20;

  FILE *file = fopen("/output.gif", "wb");
  Gif_FullWriteFile(enc->stream, &info, file);
  fclose(file);

  Gif_DeleteStream(enc->stream);
}

EMSCRIPTEN_KEEPALIVE
void encoder_free(encoder *enc)
{
  Gif_DeleteStream(enc->stream);
  free(enc);
}