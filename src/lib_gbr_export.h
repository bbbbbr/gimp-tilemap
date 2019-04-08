//
// lib_gbr_export.h
//

#include "lib_gbr.h"


int32_t gbr_object_producer_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_tile_data_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_tile_settings_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_tile_export_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_tile_import_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_palettes_encode(gbr_record * p_gbr, pascal_file_object * p_obj);
int32_t gbr_object_tile_pal_encode(gbr_record * p_gbr, pascal_file_object * p_obj);

int32_t gbr_convert_image_to_tileset(gbr_record * p_gbr, image_data * p_image, color_data * p_colors);
int32_t gbr_export_tileset_palette(color_data * p_colors, gbr_record * p_gbr);

int32_t gbr_export_set_defaults(gbr_record * p_gbr);